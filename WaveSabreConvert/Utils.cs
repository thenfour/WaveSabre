using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using static WaveSabreConvert.Song;

namespace WaveSabreConvert
{
    static class Utils
    {

        const string encodingTable = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+";

        public static byte[] Dejucer(string s)
        {
            var size = -1;
            if (!int.TryParse(s.Substring(0, s.IndexOf('.')), out size))
                return null;
            
            var b64str = s.Substring(s.IndexOf('.') + 1, s.Length - s.IndexOf('.') - 1);

            var pos = 0;
            var data = new byte[size];

            for (var i = 0; i < b64str.Length; ++i) 
            {
                var c = b64str[i];
                for (var j = 0; j < 64; ++j)
                {
                    if (encodingTable[j] == c)
                    {
                        SetBitRange (size, data, pos, 6, j);
                        pos += 6;
                        break;
                    }
                }
            }
            return data;
        }

        private static void SetBitRange(int size, byte[] data, int bitRangeStart, int numBits, int bitsToSet)
        {
            var bite = bitRangeStart >> 3;
            var offsetInByte = ( bitRangeStart & 7 );
            var mask = ~(((  0xffffffff) << (32 - numBits)) >> (32 - numBits));

            while (numBits > 0 && bite < size)
            {
                var bitsThisTime = Math.Min( numBits, 8 - offsetInByte);
                var tempMask = (mask << offsetInByte) | ~(((0xffffffff) >> offsetInByte) << offsetInByte);
                var tempBits = bitsToSet << offsetInByte;

                data[bite] = (byte)((data[bite] & tempMask) | tempBits);
                ++bite;
                numBits -= bitsThisTime;
                bitsToSet >>= bitsThisTime;
                mask >>= bitsThisTime;
                offsetInByte = 0;
            }
        }

        public static string Serializer(object o)
        {
            var data = "";

            try
            {
                var x = new System.Xml.Serialization.XmlSerializer(o.GetType());
                using (StringWriter writer = new StringWriter())
                {
                    x.Serialize(writer, o);
                    data = writer.ToString();
                }
            }
            catch
            {
            }

            return data;
        }

        public static object Deserializer(string data, Type type)
        {
            object output;

            try
            {
                var x = new System.Xml.Serialization.XmlSerializer(type);
                using (StringReader reader = new StringReader(data))
                {
                    output = x.Deserialize(reader);
                }
            }
            catch
            {
                output = null;
            }

            return output;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr LoadLibrary(string dllToLoad);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);

        [DllImport("kernel32.dll")]
        private static extern bool FreeLibrary(IntPtr hModule);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int WaveSabreDeviceVSTChunkToMinifiedChunkDelegate(
            [MarshalAs(UnmanagedType.LPStr)] string deviceName,
            int inputSize,
            IntPtr inputData,
            ref int outputSize,
            out IntPtr outputData);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int WaveSabreFreeChunkDelegate(IntPtr p);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int WaveSabreTestCompressionDelegate(
            int inputSize,
            IntPtr inputData);

        public const string vstpath = @"C:\root\vstpluginlinks\WaveSabre";

        public static int WaveSabreTestCompression(byte[] data)
        {
            string dll = FindDeviceDllFullPath(Song.DeviceId.Maj7);
            IntPtr dllHandle = LoadLibrary(dll);
            if (dllHandle == IntPtr.Zero)
            {
                Console.WriteLine($"Failed to load the VST DLL from {dll}. (does it exist? is it the right bitness?)");
                return 0;
            }
            IntPtr functionHandle = GetProcAddress(dllHandle, "WaveSabreTestCompression");
            if (functionHandle == IntPtr.Zero)
            {
                FreeLibrary(dllHandle);
                return 0;
            }

            IntPtr inputBuffer = Marshal.AllocHGlobal(data.Length);
            Marshal.Copy(data, 0, inputBuffer, data.Length);

            WaveSabreTestCompressionDelegate __imp_WaveSabreTestCompression = Marshal.GetDelegateForFunctionPointer<WaveSabreTestCompressionDelegate>(functionHandle);

            int result = __imp_WaveSabreTestCompression(data.Length, inputBuffer);
            Marshal.FreeHGlobal(inputBuffer);
            FreeLibrary(dllHandle);

            return result;
        }

        public static string FindFileInDirectories(IEnumerable<string> directories, string filename)
        {
            foreach (string directory in directories)
            {
                string fullPath = Path.Combine(directory, filename);

                if (File.Exists(fullPath))
                {
                    return fullPath;
                }

                string[] subDirectories = Directory.GetDirectories(directory);

                foreach (string subDirectory in subDirectories)
                {
                    string subDirectoryPath = Path.Combine(directory, subDirectory);
                    string subDirectoryFile = FindFileInDirectories(new List<string> { subDirectoryPath }, filename);

                    if (!string.IsNullOrEmpty(subDirectoryFile))
                    {
                        return subDirectoryFile;
                    }
                }
            }

            return null;
        }

        public static string FindDeviceDllFullPath(Song.DeviceId deviceID)
        {
            // TODO: dynamic. maybe look in the DAW's config?
            string leaf = deviceID.ToString() + ".dll";
            return FindFileInDirectories(
                new List<string> {
                    vstpath
                    //Environment.ExpandEnvironmentVariables("%program files.....")

                }, leaf);
        }

        public static byte[] ConvertDeviceChunk(Song.DeviceId deviceID, byte[] inputData)
        {
            string dll = FindDeviceDllFullPath(deviceID);
            IntPtr dllHandle = LoadLibrary(dll);
            if (dllHandle == IntPtr.Zero)
            {
                Console.WriteLine($"Failed to load the VST DLL from {dll}. (does it exist? is it the right bitness?)");
                return inputData;
            }
            IntPtr functionHandle = GetProcAddress(dllHandle, "WaveSabreDeviceVSTChunkToMinifiedChunk");
            if (functionHandle == IntPtr.Zero)
            {
                FreeLibrary(dllHandle);
                return inputData;
            }
            IntPtr pfnFree = GetProcAddress(dllHandle, "WaveSabreFreeChunk");
            if (pfnFree == IntPtr.Zero)
            {
                FreeLibrary(dllHandle);
                return inputData;
            }

            IntPtr inputBuffer = Marshal.AllocHGlobal(inputData.Length);
            Marshal.Copy(inputData, 0, inputBuffer, inputData.Length);

            int outputSize = 0;
            IntPtr outputBuffer = IntPtr.Zero;

            WaveSabreDeviceVSTChunkToMinifiedChunkDelegate __imp_WaveSabreDeviceVSTChunkToMinifiedChunk = Marshal.GetDelegateForFunctionPointer<WaveSabreDeviceVSTChunkToMinifiedChunkDelegate>(functionHandle);

            WaveSabreFreeChunkDelegate __imp_WaveSabreFreeChunk = Marshal.GetDelegateForFunctionPointer<WaveSabreFreeChunkDelegate>(pfnFree);

            int result = __imp_WaveSabreDeviceVSTChunkToMinifiedChunk(deviceID.ToString(), inputData.Length, inputBuffer, ref outputSize, out outputBuffer);
            if (result <= 0 || outputBuffer == IntPtr.Zero)
            {
                Marshal.FreeHGlobal(inputBuffer);
                FreeLibrary(dllHandle);
                return inputData;
            }

            byte[] outputData = new byte[outputSize];
            Marshal.Copy(outputBuffer, outputData, 0, outputSize);
            //Marshal.FreeHGlobal(outputBuffer);
            __imp_WaveSabreFreeChunk(outputBuffer);

            Marshal.FreeHGlobal(inputBuffer);

            FreeLibrary(dllHandle);

            return outputData;
        } // ConvertDeviceChunk
    } // class Utils
}
