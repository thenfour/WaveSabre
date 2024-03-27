using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using static WaveSabreConvert.Utils;

namespace WaveSabreConvert
{
    public class Song
    {
        // NB: These enum values are used as strings to reference the VST DLLs and C++ class names (Serializer.cs)
        // NB: keep in sync with WaveSabrePlayerLib/SongRenderer.h
        public enum DeviceId
        {
            Leveller,
            Crusher,
            Echo,
            Chamber,
            Twister,
            Cathedral,
            Maj7,
            Maj7Width,
            Maj7Comp,
            Maj7Sat,
            Maj7MBC,
        }

        public class Receive
        {
            public int SendingTrackIndex;
            public int ReceivingChannelIndex;
            public float Volume;
        }

        // important that this is a class because its index is searched by reference
        public class Device
        {
            public DeviceId Id;
            public byte[] Chunk;
        }

        public enum EventType
        {
            NoteOff = 0, // 00
            NoteOn = 1, // 01
            CC = 2, // 10
            PitchBend = 3, // 10
        }

        public class Event
        {
            public int TimeStamp; // in samples
            public int? DurationSamples; // for note ons, this gets populated during delta encoding phase; for note offs this is also populated just as a marker that it's been associated with a corresponding note on
            public EventType Type;
            public byte Note;
            public byte Velocity;
        }

        public class DeltaCodedEvent
        {
            public int TimeFromLastEvent; // in samples
            public int DurationSamples;
            //public EventType Type;
            public byte Note;
            public byte Velocity;
        }

        public class Point
        {
            public int TimeStamp;
            public float Value;
        }

        public class DeltaCodedPoint
        {
            public int TimeFromLastPoint;
            public byte Value;
        }

        public class Automation
        {
            public int DeviceIndex;
            public int ParamId;
            public List<Point> Points = new List<Point>();
            /// <summary>
            /// Auto populated by converter, do not populate
            /// </summary>
            public List<DeltaCodedPoint> DeltaCodedPoints = new List<DeltaCodedPoint>();
        }

        public class Track
        {
            public string Name;
            public float Volume;
            public List<Receive> Receives = new List<Receive>();
            public List<Device> Devices = new List<Device>();
            public List<Event> Events = new List<Event>();
            public List<Automation> Automations = new List<Automation>();
            /// <summary>
            /// Auto populated by converter, do not populate
            /// </summary>
            public List<DeltaCodedEvent> DeltaCodedEvents = new List<DeltaCodedEvent>();
            /// <summary>
            /// Auto populated by converter, do not populate
            /// </summary>
            public List<int> DeviceIndices = new List<int>();
            /// <summary>
            /// Auto populated by converter, do not populate
            /// </summary>
            public int MidiLaneId;

            public override string ToString()
            {
                return this.Name;
            }
        }

        public class MidiLane
        {
            public List<DeltaCodedEvent> MidiEvents = new List<DeltaCodedEvent>();
        }

        public int Tempo;
        public int SampleRate;
        public double Length;

        // we can save hundreds of bytes by just scaling down timestamps. Normally in samples,
        // if we just work in N-sample chunks then we remove entropy by quantizing.
        public int TimestampScaleLog2;
        public int NoteDurationScaleLog2;

        public List<Track> Tracks = new List<Track>();
        /// <summary>
        /// Auto populated by converter, do not populate
        /// </summary>
        public List<Device> Devices = new List<Device>();
        /// <summary>
        /// Auto populated by converter, do not populate
        /// </summary>
        public List<MidiLane> MidiLanes = new List<MidiLane>();

        // detect things which could cause a problem within the song
        public void DetectWarnings(ILog logger)
        {
            // detect multiple side-chain useage on tracks
            foreach (var t in Tracks)
            {
                var recCount = 0;
                foreach (var r in t.Receives)
                {
                    if (r.ReceivingChannelIndex > 0)
                    {
                        recCount++;
                    }
                }

                if (recCount > 1)
                {
                    logger.WriteLine("WARNING: Track {0} has {1} side-chain receives, audio will be summed for all devices", t.Name, recCount);
                }
            }
        }


        private static int CalculateDifference(byte[] chunk1, byte[] chunk2)
        {
            int difference = 0;
            for (int i = 0; i < chunk1.Length; i++)
            {
                difference += Math.Abs(chunk1[i] - chunk2[i]);
            }
            return difference;
        }

        //private static int CalculateDifferenceCount(byte[] chunk1, byte[] chunk2)
        //{
        //    int difference = 0;
        //    for (int i = 0; i < chunk1.Length; i++)
        //    {
        //        if (chunk1[i] != chunk2[i]) difference++;
        //    }
        //    return difference;
        //}


        public static List<Device> FindOptimalDeviceOrderUsingDistance(DeviceId deviceType, int startingDeviceIndex, List<Device> devices)
        {
            int chunkSize = devices[0].Chunk.Length;
            List<Device> orderedDevices = new List<Device>();
            HashSet<int> usedIndices = new HashSet<int>();

            // Add the starting device to the ordered list.
            orderedDevices.Add(devices[startingDeviceIndex]);
            usedIndices.Add(startingDeviceIndex);

            while (orderedDevices.Count < devices.Count)
            {
                int lastIndex = orderedDevices.Count - 1;
                Device lastDevice = orderedDevices[lastIndex];
                int bestMatchIndex = -1;
                int lowestDifference = int.MaxValue;

                for (int i = 0; i < devices.Count; i++)
                {
                    if (usedIndices.Contains(i))
                        continue;

                    int difference = CalculateDifference(lastDevice.Chunk, devices[i].Chunk);

                    if (difference < lowestDifference)
                    {
                        lowestDifference = difference;
                        bestMatchIndex = i;
                    }
                }

                if (bestMatchIndex != -1)
                {
                    orderedDevices.Add(devices[bestMatchIndex]);
                    usedIndices.Add(bestMatchIndex);
                }
            }

            return orderedDevices;
        }

        public class CompressionTester : IDisposable
        {
            private IntPtr dllHandle = IntPtr.Zero;
            private WaveSabreTestCompressionDelegate compressionFunction;

            public CompressionTester()
            {
                string dll = FindDeviceDllFullPath(Song.DeviceId.Maj7);
                dllHandle = LoadLibrary(dll);
                if (dllHandle == IntPtr.Zero)
                {
                    Console.WriteLine($"Failed to load the VST DLL from {dll}. (does it exist? is it the right bitness?)");
                    throw new Exception("DLL load failed.");
                }

                IntPtr functionHandle = GetProcAddress(dllHandle, "WaveSabreTestCompression");
                if (functionHandle == IntPtr.Zero)
                {
                    FreeLibrary(dllHandle);
                    throw new Exception("Function not found in DLL.");
                }

                compressionFunction = Marshal.GetDelegateForFunctionPointer<WaveSabreTestCompressionDelegate>(functionHandle);
            }

            public int GetCompressedSize(byte[] data)
            {
                if (data.Length == 0) return 0;

                IntPtr inputBuffer = Marshal.AllocHGlobal(data.Length);
                Marshal.Copy(data, 0, inputBuffer, data.Length);

                int result = compressionFunction(data.Length, inputBuffer);

                Marshal.FreeHGlobal(inputBuffer);

                return result;
            }

            public void Dispose()
            {
                if (dllHandle != IntPtr.Zero)
                {
                    FreeLibrary(dllHandle);
                    dllHandle = IntPtr.Zero;
                }
            }

            // P/Invoke declarations
            [DllImport("kernel32", SetLastError = true)]
            private static extern IntPtr LoadLibrary(string lpFileName);

            [DllImport("kernel32", SetLastError = true)]
            private static extern bool FreeLibrary(IntPtr hModule);

            [DllImport("kernel32", SetLastError = true)]
            private static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);

            private delegate int WaveSabreTestCompressionDelegate(int dataSize, IntPtr data);
        }


        public static List<Device> FindOptimalDeviceOrderUsingCompressedSize(int startingDeviceIndex, List<Device> devices, CompressionTester compressionTester)
        {
            List<Device> orderedDevices = new List<Device>();
            HashSet<int> usedIndices = new HashSet<int>();

            // Start with the specified device.
            orderedDevices.Add(devices[startingDeviceIndex]);
            usedIndices.Add(startingDeviceIndex);

            while (orderedDevices.Count < devices.Count)
            {
                int bestNextIndex = -1;
                int smallestCompressedSize = int.MaxValue;

                for (int i = 0; i < devices.Count; i++)
                {
                    if (usedIndices.Contains(i)) continue;

                    // Create a temporary list to test adding this device.
                    var testList = new List<Device>(orderedDevices) { devices[i] };
                    byte[] testData = JoinDeviceChunksInterleavedAndDeltaEncodeS16(testList);

                    int compressedSize = compressionTester.GetCompressedSize(testData);
                    if (compressedSize < smallestCompressedSize)
                    {
                        smallestCompressedSize = compressedSize;
                        bestNextIndex = i;
                    }
                }

                if (bestNextIndex != -1)
                {
                    orderedDevices.Add(devices[bestNextIndex]);
                    usedIndices.Add(bestNextIndex);
                }
            }

            return orderedDevices;
        }




        //public static List<Device> FindWorstDeviceOrderUsingCompressedSize(int startingDeviceIndex, List<Device> devices, CompressionTester compressionTester)
        //{
        //    List<Device> orderedDevices = new List<Device>();
        //    HashSet<int> usedIndices = new HashSet<int>();

        //    // Start with the specified device.
        //    orderedDevices.Add(devices[startingDeviceIndex]);
        //    usedIndices.Add(startingDeviceIndex);

        //    while (orderedDevices.Count < devices.Count)
        //    {
        //        int bestNextIndex = -1;
        //        int biggestCompressedSize = -1;

        //        for (int i = 0; i < devices.Count; i++)
        //        {
        //            if (usedIndices.Contains(i)) continue;

        //            // Create a temporary list to test adding this device.
        //            var testList = new List<Device>(orderedDevices) { devices[i] };
        //            byte[] testData = JoinDeviceChunksAndDeltaEncodeS16(testList);

        //            int compressedSize = compressionTester.GetCompressedSize(testData);
        //            if (compressedSize > biggestCompressedSize)
        //            {
        //                biggestCompressedSize = compressedSize;
        //                bestNextIndex = i;
        //            }
        //        }

        //        if (bestNextIndex != -1)
        //        {
        //            orderedDevices.Add(devices[bestNextIndex]);
        //            usedIndices.Add(bestNextIndex);
        //        }
        //    }

        //    return orderedDevices;
        //}



        public static void DeltaEncodeDevicesBy16Bit(List<Device> devices)
        {
            for (int i = 1; i < devices.Count; i++)
            {
                byte[] previousChunk = devices[i - 1].Chunk;
                byte[] currentChunk = devices[i].Chunk;

                // Assuming chunk length is even and each device chunk represents an array of ushort.
                for (int j = 0; j < currentChunk.Length; j += 2)
                {
                    // Convert bytes to ushort for both current and previous chunks.
                    ushort previousValue = BitConverter.ToUInt16(previousChunk, j);
                    ushort currentValue = BitConverter.ToUInt16(currentChunk, j);

                    // Calculate the delta as ushort and store it back as bytes in the current chunk.
                    ushort delta = (ushort)(currentValue - previousValue);
                    byte[] deltaBytes = BitConverter.GetBytes(delta);

                    // Store the delta bytes back in the current chunk.
                    currentChunk[j] = deltaBytes[0];
                    currentChunk[j + 1] = deltaBytes[1];
                }
            }
        }

        public static void DeltaEncodeDevicesBy16BitSigned(List<Device> devices)
        {
            for (int i = 1; i < devices.Count; i++)
            {
                byte[] previousChunk = devices[i - 1].Chunk;
                byte[] currentChunk = devices[i].Chunk;

                // Assuming chunk length is even and each device chunk represents an array of Int16 (signed).
                for (int j = 0; j < currentChunk.Length; j += 2)
                {
                    // Convert bytes to Int16 for both current and previous chunks, preserving the sign.
                    short previousValue = BitConverter.ToInt16(previousChunk, j);
                    short currentValue = BitConverter.ToInt16(currentChunk, j);

                    // Calculate the delta as Int16 and store it back as bytes in the current chunk.
                    short delta = (short)(currentValue - previousValue);
                    byte[] deltaBytes = BitConverter.GetBytes(delta);

                    // Store the delta bytes back in the current chunk.
                    currentChunk[j] = deltaBytes[0];
                    currentChunk[j + 1] = deltaBytes[1];
                }
            }
        }

        //public static void DeltaEncodeDevicesByByte(List<Device> devices)
        //{
        //    for (int i = 1; i < devices.Count; i++)
        //    {
        //        byte[] previousChunk = devices[i - 1].Chunk;
        //        byte[] currentChunk = devices[i].Chunk;

        //        for (int j = 0; j < currentChunk.Length; j++)
        //        {
        //            // Calculate the delta and store it.
        //            // Note: Depending on your requirements, you might need to handle overflow differently.
        //            currentChunk[j] = (byte)(currentChunk[j] - previousChunk[j]);
        //        }
        //    }
        //}
        //public static void PerformDeltaEncoding16BitWithLowSignBit(List<Device> devices)
        //{
        //    for (int i = 1; i < devices.Count; i++)
        //    {
        //        byte[] previousChunk = devices[i - 1].Chunk;
        //        byte[] currentChunk = devices[i].Chunk;

        //        for (int j = 0; j < currentChunk.Length; j += 2)
        //        {
        //            // Convert bytes to ushort for both current and previous chunks.
        //            ushort previousValue = BitConverter.ToUInt16(previousChunk, j);
        //            ushort currentValue = BitConverter.ToUInt16(currentChunk, j);

        //            // Calculate the signed delta using custom logic.
        //            int rawDelta = currentValue - previousValue;
        //            bool isNegative = rawDelta < 0;
        //            ushort absDelta = (ushort)(Math.Abs(rawDelta));

        //            // Encode the delta with the sign in the low bit.
        //            ushort encodedDelta = Encode16BitDeltaWithSignBit(absDelta, isNegative);

        //            // Store the encoded delta back as bytes in the current chunk.
        //            byte[] deltaBytes = BitConverter.GetBytes(encodedDelta);
        //            currentChunk[j] = deltaBytes[0];
        //            currentChunk[j + 1] = deltaBytes[1];
        //        }
        //    }
        //}

        //private static ushort Encode16BitDeltaWithSignBit(ushort delta, bool isNegative)
        //{
        //    // Shift the delta left by 1 to make space for the sign bit
        //    ushort encodedDelta = (ushort)(delta << 1);
        //    if (isNegative)
        //    {
        //        // Set the low bit if negative
        //        encodedDelta |= 1;
        //    }
        //    return encodedDelta;
        //}



        //public static void PerformByteDeltaEncodingWithLowSignBit(List<Device> devices)
        //{
        //    for (int i = 1; i < devices.Count; i++)
        //    {
        //        byte[] previousChunk = devices[i - 1].Chunk;
        //        byte[] currentChunk = devices[i].Chunk;

        //        for (int j = 0; j < currentChunk.Length; j++)
        //        {
        //            // Calculate the signed delta as an int to handle potential underflow/overflow.
        //            int rawDelta = currentChunk[j] - previousChunk[j];
        //            bool isNegative = rawDelta < 0;
        //            // Ensure the delta fits in 7 bits by taking the absolute value and capping it at 127.
        //            byte absDelta = (byte)Math.Min(Math.Abs(rawDelta), 127);

        //            // Encode the delta with the sign in the low bit.
        //            byte encodedDelta = EncodeByteDeltaWithSignBit(absDelta, isNegative);

        //            // Store the encoded delta back in the current chunk.
        //            currentChunk[j] = encodedDelta;
        //        }
        //    }
        //}

        //private static byte EncodeByteDeltaWithSignBit(byte delta, bool isNegative)
        //{
        //    // Shift the delta left by 1 to make space for the sign bit
        //    byte encodedDelta = (byte)(delta << 1);
        //    if (isNegative)
        //    {
        //        // Set the low bit if negative
        //        encodedDelta |= 1;
        //    }
        //    return encodedDelta;
        //}

        public static byte[] JoinDeviceChunks(List<Device> devices)
        {
            // Calculate the total size of the combined byte array.
            int totalSize = devices.Sum(device => device.Chunk.Length);

            // Allocate the combined array.
            byte[] combinedChunks = new byte[totalSize];

            // Copy each Chunk into the combined array.
            int currentPosition = 0;
            foreach (var device in devices)
            {
                Array.Copy(device.Chunk, 0, combinedChunks, currentPosition, device.Chunk.Length);
                currentPosition += device.Chunk.Length;
            }

            return combinedChunks;
        }



        public static byte[] JoinDeviceChunksInterleavedAndDeltaEncodeS16(List<Device> devices)
        {
            if (devices == null || devices.Count == 0)
            {
                return Array.Empty<byte>();
            }

            // Assuming all chunks are of the same length and contain an even number of bytes.
            int chunkLength = devices[0].Chunk.Length; // Length in bytes
            if (chunkLength % 2 != 0)
            {
                throw new InvalidOperationException("Chunk length must be even to represent an array of 16-bit integers.");
            }
            int numInt16sPerChunk = chunkLength / 2;

            List<byte> resultBytes = new List<byte>();

            // Process each 16-bit position across all devices.
            for (int int16Index = 0; int16Index < numInt16sPerChunk; int16Index++)
            {
                short previousValue = 0; // Will be used to store the previous value for delta encoding.

                for (int deviceIndex = 0; deviceIndex < devices.Count; deviceIndex++)
                {
                    byte[] currentChunk = devices[deviceIndex].Chunk;

                    // Extract the current 16-bit value.
                    short currentValue = BitConverter.ToInt16(currentChunk, int16Index * 2);

                    if (deviceIndex == 0)
                    {
                        // For the first device, just encode the value directly.
                        previousValue = currentValue;
                    }
                    else
                    {
                        // Calculate delta encoding with respect to the previous device.
                        short delta = (short)(currentValue - previousValue);
                        currentValue = delta; // Update current value to the delta for the next iteration.
                        previousValue += delta; // Update previousValue for correct delta calculation in the next round.
                    }

                    // Convert back to bytes and add to the result.
                    byte[] valueBytes = BitConverter.GetBytes(currentValue);
                    resultBytes.AddRange(valueBytes);
                }
            }

            return resultBytes.ToArray();
        }

        //public static byte[] JoinDeviceChunksAndDeltaEncodeS16(List<Device> devices)
        //{
        //    // Initialize a list to accumulate the delta-encoded bytes.
        //    List<byte> encodedBytes = new List<byte>();

        //    // Add the first device's chunk as-is since there's no previous chunk to delta encode against.
        //    encodedBytes.AddRange(devices[0].Chunk);

        //    // Iterate over the devices starting from the second one.
        //    for (int i = 1; i < devices.Count; i++)
        //    {
        //        byte[] previousChunk = devices[i - 1].Chunk;
        //        byte[] currentChunk = devices[i].Chunk;

        //        // Check to ensure the current chunk and the previous chunk are of the same length.
        //        if (currentChunk.Length != previousChunk.Length)
        //        {
        //            throw new InvalidOperationException("Chunks must be of equal length for delta encoding.");
        //        }

        //        for (int j = 0; j < currentChunk.Length; j += 2)
        //        {
        //            // Convert bytes to Int16, assuming little-endian.
        //            short previousValue = BitConverter.ToInt16(previousChunk, j);
        //            short currentValue = BitConverter.ToInt16(currentChunk, j);

        //            // Calculate the delta and convert it back to bytes.
        //            short delta = (short)(currentValue - previousValue);
        //            byte[] deltaBytes = BitConverter.GetBytes(delta);

        //            // Add the delta-encoded bytes to the accumulating list.
        //            encodedBytes.AddRange(deltaBytes);
        //        }
        //    }

        //    // Convert the list of bytes to an array and return it.
        //    return encodedBytes.ToArray();
        //}

        //public static byte[] JoinDeviceChunksInterleaved(List<Device> devices)
        //{
        //    if (devices == null || devices.Count == 0 || devices[0].Chunk == null || devices[0].Chunk.Length == 0)
        //    {
        //        return Array.Empty<byte>();
        //    }

        //    // Assuming all chunks are of the same length and contain an even number of bytes (valid Int16 array).
        //    int chunkLength = devices[0].Chunk.Length / 2; // Length in Int16 units
        //    int totalDevices = devices.Count;
        //    int totalSize = devices.Sum(device => device.Chunk.Length); // Total bytes in the interleaved array

        //    byte[] interleavedChunks = new byte[totalSize];

        //    for (int int16Index = 0; int16Index < chunkLength; int16Index++)
        //    {
        //        for (int deviceIndex = 0; deviceIndex < totalDevices; deviceIndex++)
        //        {
        //            // Calculate the position in the source and destination arrays.
        //            int sourcePosition = int16Index * 2;
        //            int destinationPosition = (int16Index * totalDevices + deviceIndex) * 2;

        //            // Copy two bytes (one Int16) from each chunk in turn.
        //            Array.Copy(devices[deviceIndex].Chunk, sourcePosition, interleavedChunks, destinationPosition, 2);
        //        }
        //    }

        //    return interleavedChunks;
        //}
        //public static byte[] JoinDeviceChunksInterleavedByByte(List<Device> devices)
        //{
        //    if (devices == null || devices.Count == 0 || devices[0].Chunk == null || devices[0].Chunk.Length == 0)
        //    {
        //        return Array.Empty<byte>();
        //    }

        //    // Assuming all chunks are of the same length.
        //    int chunkLength = devices[0].Chunk.Length; // Length in bytes
        //    int totalDevices = devices.Count;
        //    int totalSize = devices.Sum(device => device.Chunk.Length); // Total bytes in the interleaved array

        //    byte[] interleavedChunks = new byte[totalSize];

        //    for (int byteIndex = 0; byteIndex < chunkLength; byteIndex++)
        //    {
        //        for (int deviceIndex = 0; deviceIndex < totalDevices; deviceIndex++)
        //        {
        //            // Calculate the destination position in the interleaved array.
        //            int destinationPosition = (byteIndex * totalDevices) + deviceIndex;

        //            // Copy one byte from each chunk in turn.
        //            interleavedChunks[destinationPosition] = devices[deviceIndex].Chunk[byteIndex];
        //        }
        //    }

        //    return interleavedChunks;
        //}


        public static List<Device> FindOptimalDeviceOrder(List<Device> Devices, ILog logger)
        {
            using (var compressionTester = new CompressionTester())
            {
                // search for the best order to serialize devices.
                // one device type at a time, the way we will serialize is that the first device chunk will be absolute values,
                // then subsequent are delta-encoded from the previous device chunk. The idea is that devices are often copies of each other with tweaks.
                // so to optimize this, pick a first device, then search for the most-similar next one, etc.
                // picking the first one is a bit of a shot in the dark; we can also iterate to find which starting device results in the best compression.
                var dd = new Dictionary<DeviceId, List<Device>>();

                // separate devices by type
                foreach (var d in Devices)
                {
                    if (!dd.ContainsKey(d.Id)) dd.Add(d.Id, new List<Device>());
                    dd[d.Id].Add(d);
                }

                var keys = dd.Keys.ToList();
                foreach (var deviceId in keys)
                {
                    var devices = dd[deviceId];
                    //int bestDeviceStartId = 0;
                    int worstCompressedSize = 0;
                    int uncompressedSize = 0;
                    int bestCompressedSize = 0;
                    List<Device> bestDeviceList = null;
                    for (int id = 0; id < devices.Count; ++id)
                    {
                        var c = FindOptimalDeviceOrderUsingDistance(deviceId, id, devices);
                        //var c = FindOptimalDeviceOrderUsingCompressedSize(id, devices, compressionTester);
                        //var c = FindWorstDeviceOrderUsingCompressedSize(id, devices, compressionTester);
                        //DeltaEncodeDevicesBy16Bit(c);
                        //DeltaEncodeDevicesByByte(c);
                        //PerformDeltaEncoding16BitWithLowSignBit(c);
                        //PerformByteDeltaEncodingWithLowSignBit(c);

                        //var big = JoinDeviceChunksInterleaved(c);
                        //var big = JoinDeviceChunksInterleavedAndDeltaEncodeS16(c);
                        //var big = JoinDeviceChunksInterleavedByByte(c);
                        var big = JoinDeviceChunks(c);
                        uncompressedSize = big.Length;
                        var compressedSize = compressionTester.GetCompressedSize(big);
                        if (bestDeviceList == null)
                        {
                            bestDeviceList = c;
                            bestCompressedSize = compressedSize;
                            worstCompressedSize = compressedSize;
                        }
                        else
                        {
                            if (compressedSize > worstCompressedSize) worstCompressedSize = compressedSize;
                            if (compressedSize < bestCompressedSize)
                            {
                                bestDeviceList = c;
                                bestCompressedSize = compressedSize;
                            }
                        }
                    }

                    logger.WriteLine($"device {deviceId} uncompressed:{uncompressedSize} best size: {bestCompressedSize} worst size {worstCompressedSize}");

                    dd[deviceId] = bestDeviceList;// FindOptimalDeviceOrder(deviceId, 0, devices);
                }

                // now bring back into unified list.
                var ret = new List<Device>();
                foreach (var d in dd.Values)
                {
                    ret.AddRange(d);
                }
                return ret;
            }
        }

        // restructures song elements int indexes lists
        public void Restructure(ILog logger)
        {
            // make list of all devices
            foreach (var t in Tracks)
            {
                foreach (var d in t.Devices)
                {
                    Devices.Add(d);
                }
            }

            // sort by devices type, keeping similar chunk data together, should be better for compressor
            Devices.Sort((x, y) => x.Id.CompareTo(y.Id));

            foreach (var d in Devices)
            {
                var oldLen = d.Chunk;
                d.Chunk = Utils.ConvertDeviceChunk(d.Id, d.Chunk, true);
                //logger.WriteLine($"{d.Id} chunk size: {oldLen} -> {d.Chunk.Length}");
            }

            Devices = FindOptimalDeviceOrder(Devices, logger);

            // link device id back to track
            foreach (var t in Tracks)
            {
                foreach (var d in t.Devices)
                {
                    t.DeviceIndices.Add(Devices.IndexOf(d));
                }
            }

            // create midi lanes from tracks and index them
            int midiLaneDupes = 0;
            foreach (var t in Tracks)
            {
                var midiLane = new MidiLane() { MidiEvents = t.DeltaCodedEvents };
                t.MidiLaneId = AddMidiLane(midiLane, ref midiLaneDupes);
            }

            if (midiLaneDupes > 0)
                logger.WriteLine("Found {0} duplicate midi lane(s)", midiLaneDupes);

            var dupeCount = DedupeAutomations();
            if (dupeCount > 0)
            {
                logger.WriteLine("Removed {0} automation point(s)", dupeCount);
            }
        }

        // determines if this midi lane is exactly the same as an existing one
        // otherwise add it
        private int AddMidiLane(MidiLane midiLane, ref int midiLaneDupes)
        {
            foreach (var m in MidiLanes)
            {
                var duplicate = false;
                if (m.MidiEvents.Count == midiLane.MidiEvents.Count)
                {
                    duplicate = true;
                    for (var i = 0; i < m.MidiEvents.Count; i++)
                    {
                        if (m.MidiEvents[i].Note != midiLane.MidiEvents[i].Note
                            || m.MidiEvents[i].Velocity != midiLane.MidiEvents[i].Velocity
                            || m.MidiEvents[i].DurationSamples != midiLane.MidiEvents[i].DurationSamples
                            || m.MidiEvents[i].TimeFromLastEvent != midiLane.MidiEvents[i].TimeFromLastEvent)
                        {
                            duplicate = false;
                            break;
                        }
                    }
                }

                if (duplicate)
                {
                    if (midiLane.MidiEvents.Count > 0)
                        midiLaneDupes++;

                    return MidiLanes.IndexOf(m);
                }
            }

            MidiLanes.Add(midiLane);
            return MidiLanes.IndexOf(midiLane);
        }

        // looks for automation points where three in a row are the same value
        // then removes the point on the right, adjusting the middle point's time stamp to take its place
        private int DedupeAutomations()
        {
            var dupeCount = 0;

            foreach (var track in Tracks)
            {
                if (track.Automations.Count > 0)
                {
                    foreach (var auto in track.Automations)
                    {
                        var points = auto.DeltaCodedPoints;

                        bool done = false;
                        while (!done)
                        {
                            var dupeId = GetDupeDeltaPoint(points);
                            if (dupeId == -1)
                            {
                                done = true;
                            }
                            else
                            {
                                points[dupeId].TimeFromLastPoint += points[dupeId + 1].TimeFromLastPoint;
                                points.Remove(points[dupeId + 1]);
                                dupeCount++;
                            }
                        }
                    }
                }
            }

            return dupeCount;
        }

        private int GetDupeDeltaPoint(List<DeltaCodedPoint> points)
        {
            for (var i = 1; i < points.Count - 1; i++)
            {
                if (points[i].Value == points[i - 1].Value && points[i].Value == points[i + 1].Value)
                    return i;
            }

            return -1;
        }

        // performs delta encoding on midi and automation events
        // also calculates note durations from note on / off pairs.
        public void DeltaEncodeAndOther(ILog logger)
        {
            var allDurations = new List<(int durationSamples, string description)>();
            //int? shortestDuration = null;
            //string shortestDurationDesc = "";
            int lastTime;
            foreach (var t in Tracks)
            {
                foreach (var a in t.Automations)
                {
                    lastTime = 0;
                    foreach (var p in a.Points)
                    {
                        a.DeltaCodedPoints.Add(new DeltaCodedPoint()
                        {
                            TimeFromLastPoint = p.TimeStamp - lastTime,
                            Value = FloatToByte(p.Value)
                        });

                        lastTime = p.TimeStamp;
                    }
                }

                // populate note on durations by finding each's corresponding note off.
                int currentTimestamp = 0;
                List<Event> pitchbendEvents = new List<Event>();
                List<Event> ccEvents = new List<Event>();
                for (int i = 0; i < t.Events.Count; ++ i)
                {
                    var e = t.Events[i];
                    Debug.Assert(e.TimeStamp >= currentTimestamp); // this algo assumes that events are ordered by time
                    currentTimestamp = e.TimeStamp;

                    switch (e.Type)
                    {
                        case EventType.NoteOn: // we only care about processing note ons.
                            break;
                        case EventType.NoteOff: // ignore note offs
                            continue;
                        case EventType.CC: // ignore. some CCs are to be expected so don't go crazy trying to report these.
                            ccEvents.Add(e);
                            continue;
                        case EventType.PitchBend:
                            pitchbendEvents.Add(e);
                            //logger.WriteLine($"! ERROR: Pitchbend data is not supported. track:{t.Name}, event:{i}, type:{e.Note}, value:{e.Velocity}: {Utils.MidiEventToString(e.Note, e.Velocity)}");
                            continue;
                    }

                    // find corresponding note off.
                    for (int j = i + 1; j < t.Events.Count; ++ j)
                    {
                        var off = t.Events[j];
                        if (off.TimeStamp < e.TimeStamp)
                        {
                            // this is actually an error.
                            logger.WriteLine($"! ERROR: note events out of order? {e.TimeStamp} should not be less than {off.TimeStamp}; track {t.Name}");
                            return;
                        }
                        if (off.Note != e.Note) continue;
                        if (off.Type == EventType.NoteOn)
                        {
                            // so this is quite an interesting scenario imo. Reaper thinks so too. You have 2 note ons for the same note, before the note off event.
                            // that's fine, but when a note off happens, which note does it correspond to? it could be:
                            // Scenario A:
                            // xxxxxxxxx
                            //     xxxxxxxxxx
                            // |on |on |off |off
                            //
                            // OR scenario B: the same sequence of events could represent:
                            // |on |on |off |off
                            // xxxxxxxxxxxxxx
                            //     xxxxx
                            //
                            // Reaper does not distinguish between the two. When you create the MIDI of scenario B, save the file, then open it again,
                            // you'll get scenario A. So we should not bend over backwards in supporting this kind of thing anyway. Let's do the simplest thing,
                            // the thing Reaper currently does: pick the first note off.
                            // therefore, just ignore note ons.
                            continue;
                        }
                        if (off.Type != EventType.NoteOff) continue;

                        // we have found the corresponding note off.
                        if (off.DurationSamples != null)
                        {
                            // this note off has already been visited. but it could have been because of the overlapping scenario shown above.
                            //logger.WriteLine($"!ERROR: note off has been visited multiple times while calculating durations?; track {t.Name}");
                            continue;
                        }

                        e.DurationSamples = off.TimeStamp - e.TimeStamp;
                        off.DurationSamples = e.DurationSamples; // put here to just mark that we visited this note off.

                        allDurations.Add((
                            durationSamples: e.DurationSamples.Value,
                            description: $"shortest duration {e.DurationSamples} samples in {t.Name}, event:{i} at {e.TimeStamp} samples ({(double)e.TimeStamp / 44100.0} sec)")
                            );
                        //if (!shortestDuration.HasValue || (e.DurationSamples < shortestDuration))
                        //{
                        //    shortestDuration = e.DurationSamples;
                        //    shortestDurationDesc = $"shortest duration {shortestDuration} samples in {t.Name}, event:{i} at {e.TimeStamp} samples ({(double)e.TimeStamp / 44100.0} sec)";
                        //}

                        break;
                    } // for each event

                    // was a duration calculated?
                    if (e.DurationSamples == null)
                    {
                        logger.WriteLine($"!ERROR: no duration was found. Maybe no corresponding note off event for a note on?; track {t.Name}");
                        continue;
                    }
                } // for each event in this track.

                if (pitchbendEvents.Count > 0)
                {
                    logger.WriteLine($"!Pitchbend data is not supported; track {t.Name}. {pitchbendEvents.Count} pitchbend events, starting at {pitchbendEvents.First().TimeStamp}");
                }
                if (ccEvents.Count > 0)
                {
                    logger.WriteLine($"CC events are not supported. track {t.Name}");
                }

                // populate delta encoded MIDI events ONLY for note ons.
                lastTime = 0; 
                foreach (var e in t.Events.Where(e => e.Type == EventType.NoteOn))
                {
                    if (e.DurationSamples == null)
                    {
                        logger.WriteLine($"unable to serialize note event with no duration.");
                        continue;
                    }
                    t.DeltaCodedEvents.Add(new DeltaCodedEvent()
                    {
                        TimeFromLastEvent = e.TimeStamp - lastTime,
                        DurationSamples = e.DurationSamples.Value,
                        Note = e.Note,
                        Velocity = e.Velocity
                    });

                    lastTime = e.TimeStamp;
                }
            } // for each track

            var shortestDurations = allDurations.OrderBy(x => x.durationSamples);
            foreach (var x in shortestDurations.Take(10))
            {
                logger.WriteLine(x.description);
            }
        }

        private byte FloatToByte(float value)
        {
            var temp = (int)Math.Floor((double)value * 255.0);
            if (temp < 0)
                temp = 0;
            if (temp > 255)
                temp = 255;
            return (byte) temp;
        }
    }
}
