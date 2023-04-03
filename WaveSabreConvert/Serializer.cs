using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace WaveSabreConvert
{
    public class Serializer
    {
        public string Serialize(Song song)
        {
            var sb = new StringBuilder();

            sb.AppendLine("#include <WaveSabreCore.h>");
            sb.AppendLine("#include <WaveSabrePlayerLib.h>");
            sb.AppendLine();

            SerializeFactory(sb, song);
            sb.AppendLine();

            SerializeBlob(sb, song);
            sb.AppendLine();

            sb.AppendLine("SongRenderer::Song Song = {");
            sb.AppendLine("\tSongFactory,");
            sb.AppendLine("\tSongBlob");
            sb.AppendLine("};");

            return sb.ToString();
        }

        public BinaryOutput SerializeBinary(Song song)
        {
            return CreateBinary(song);
        }

        void SerializeFactory(StringBuilder sb, Song song)
        {
            sb.AppendLine("WaveSabreCore::Device *SongFactory(SongRenderer::DeviceId id)");
            sb.AppendLine("{");

            var devicesUsed = new List<Song.DeviceId>();

            // We want to output these in the same order as they're listed
            //  in the DeviceId enum for debugging purposes
            foreach (var id in (Song.DeviceId[])Enum.GetValues(typeof(Song.DeviceId)))
            {
                if (song.Tracks.Any(track => track.Devices.Any(device => device.Id == id)))
                    devicesUsed.Add(id);
            }

            if (devicesUsed.Count > 0)
            {
                sb.AppendLine("\tswitch (id)");
                sb.AppendLine("\t{");

                foreach (var id in devicesUsed)
                {
                    var name = id.ToString();
                    sb.AppendLine("\tcase SongRenderer::DeviceId::" + name + ": return new WaveSabreCore::" + name + "();");
                }

                sb.AppendLine("\t}");
            }

            sb.AppendLine("\treturn nullptr;");

            sb.AppendLine("}");
        }

        public class BinaryStream
        {
            public MemoryStream stream = new MemoryStream();
            public BinaryWriter writer;
            public byte[] GetByteArray()
            {
                return stream.ToArray();
            }
            public int GetCompressedSize()
            {
                return Utils.WaveSabreTestCompression(GetByteArray());
            }
            public int GetSize()
            {
                return (int)stream.Length;
            }
            public BinaryStream()
            {
                writer = new BinaryWriter(stream);
            }
        }

        public class BinaryOutput
        {
            public BinaryStream CompleteSong = new BinaryStream();
            public Dictionary<Song.DeviceId, BinaryStream> DeviceChunks = new Dictionary<Song.DeviceId, BinaryStream>();
            public Dictionary<int, BinaryStream> MidiLaneData = new Dictionary<int, BinaryStream>();
            public Dictionary<string, BinaryStream> TrackData = new Dictionary<string, BinaryStream>();

            ///////////////////////////////////////////////////
            public void Write(int n)
            {
                this.CompleteSong.writer.Write(n);
            }
            public void Write(double n)
            {
                this.CompleteSong.writer.Write(n);
            }
            public void Write(byte[] n)
            {
                this.CompleteSong.writer.Write(n);
            }
            public void Write(byte n)
            {
                this.CompleteSong.writer.Write(n);
            }


            ///////////////////////////////////////////////////
            public void WriteToTrack(string trackID, int n)
            {
                this.CompleteSong.writer.Write(n);
                if (!TrackData.ContainsKey(trackID))
                {
                    TrackData[trackID] = new BinaryStream();
                }
                TrackData[trackID].writer.Write(n);
            }
            public void WriteToTrack(string trackID, double n)
            {
                this.CompleteSong.writer.Write(n);
                if (!TrackData.ContainsKey(trackID))
                {
                    TrackData[trackID] = new BinaryStream();
                }
                TrackData[trackID].writer.Write(n);
            }
            public void WriteToTrack(string trackID, float n)
            {
                this.CompleteSong.writer.Write(n);
                if (!TrackData.ContainsKey(trackID))
                {
                    TrackData[trackID] = new BinaryStream();
                }
                TrackData[trackID].writer.Write(n);
            }
            public void WriteToTrack(string trackID, byte[] n)
            {
                this.CompleteSong.writer.Write(n);
                if (!TrackData.ContainsKey(trackID))
                {
                    TrackData[trackID] = new BinaryStream();
                }
                TrackData[trackID].writer.Write(n);
            }
            public void WriteToTrack(string trackID, byte n)
            {
                this.CompleteSong.writer.Write(n);
                if (!TrackData.ContainsKey(trackID))
                {
                    TrackData[trackID] = new BinaryStream();
                }
                TrackData[trackID].writer.Write(n);
            }

            ///////////////////////////////////////////////////
            public void Write(Song.DeviceId d, int n)
            {
                this.CompleteSong.writer.Write(n);
                if (!DeviceChunks.ContainsKey(d))
                {
                    DeviceChunks[d] = new BinaryStream();
                }
                DeviceChunks[d].writer.Write(n);
            }
            public void Write(Song.DeviceId d, double n)
            {
                this.CompleteSong.writer.Write(n);
                if (!DeviceChunks.ContainsKey(d))
                {
                    DeviceChunks[d] = new BinaryStream();
                }
                DeviceChunks[d].writer.Write(n);
            }
            public void Write(Song.DeviceId d, float n)
            {
                this.CompleteSong.writer.Write(n);
                if (!DeviceChunks.ContainsKey(d))
                {
                    DeviceChunks[d] = new BinaryStream();
                }
                DeviceChunks[d].writer.Write(n);
            }
            public void Write(Song.DeviceId d, byte[] n)
            {
                this.CompleteSong.writer.Write(n);
                if (!DeviceChunks.ContainsKey(d))
                {
                    DeviceChunks[d] = new BinaryStream();
                }
                DeviceChunks[d].writer.Write(n);
            }
            public void Write(Song.DeviceId d, byte n)
            {
                this.CompleteSong.writer.Write(n);
                if (!DeviceChunks.ContainsKey(d))
                {
                    DeviceChunks[d] = new BinaryStream();
                }
                DeviceChunks[d].writer.Write(n);
            }


            ///////////////////////////////////////////////////
            public void WriteMidiLane(int iMidiLane, int n)
            {
                this.CompleteSong.writer.Write(n);
                if (!MidiLaneData.ContainsKey(iMidiLane))
                {
                    MidiLaneData[iMidiLane] = new BinaryStream();
                }
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLane(int iMidiLane, double n)
            {
                this.CompleteSong.writer.Write(n);
                if (!MidiLaneData.ContainsKey(iMidiLane))
                {
                    MidiLaneData[iMidiLane] = new BinaryStream();
                }
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLane(int iMidiLane, float n)
            {
                this.CompleteSong.writer.Write(n);
                if (!MidiLaneData.ContainsKey(iMidiLane))
                {
                    MidiLaneData[iMidiLane] = new BinaryStream();
                }
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLane(int iMidiLane, byte[] n)
            {
                this.CompleteSong.writer.Write(n);
                if (!MidiLaneData.ContainsKey(iMidiLane))
                {
                    MidiLaneData[iMidiLane] = new BinaryStream();
                }
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLane(int iMidiLane, byte n)
            {
                this.CompleteSong.writer.Write(n);
                if (!MidiLaneData.ContainsKey(iMidiLane))
                {
                    MidiLaneData[iMidiLane] = new BinaryStream();
                }
                MidiLaneData[iMidiLane].writer.Write(n);
            }
        }

        BinaryOutput CreateBinary(Song song)
        {
            BinaryOutput writer = new BinaryOutput();

            // song settings
            writer.Write(song.Tempo);
            writer.Write(song.SampleRate);
            writer.Write(song.Length);

            // serialize all devices
            writer.Write(song.Devices.Count);
            foreach (var device in song.Devices)
            {
                writer.Write(device.Id, (byte)device.Id);
                writer.Write(device.Id, device.Chunk.Length);
                writer.Write(device.Id, device.Chunk);
            }

            // serialize all midi lanes
            writer.Write(song.MidiLanes.Count);
            //foreach (var midiLane in song.MidiLanes)
            for (int iMidiLane = 0; iMidiLane < song.MidiLanes.Count; ++ iMidiLane)
            {
                var midiLane = song.MidiLanes[iMidiLane];
                writer.WriteMidiLane(iMidiLane, midiLane.MidiEvents.Count);
                foreach (var e in midiLane.MidiEvents)
                {
                    writer.WriteMidiLane(iMidiLane, e.TimeFromLastEvent);
                }
                foreach (var e in midiLane.MidiEvents)
                {
                    switch (e.Type)
                    {
                        case Song.EventType.NoteOn:
                            writer.WriteMidiLane(iMidiLane, (byte)0);
                            break;
                        case Song.EventType.NoteOff:
                            writer.WriteMidiLane(iMidiLane, (byte)1);
                            break;
                        case Song.EventType.CC:
                            writer.WriteMidiLane(iMidiLane, (byte)2);
                            break;
                        case Song.EventType.PitchBend:
                            writer.WriteMidiLane(iMidiLane, (byte)3);
                            break;
                        default:
                            throw new Exception("Unsupported event type");
                    }
                }
                foreach (var e in midiLane.MidiEvents)
                {
                    writer.Write(e.Note);
                }
                foreach (var e in midiLane.MidiEvents)
                {
                    writer.Write(e.Velocity);
                }
            }

            // serialize each track
            writer.Write(song.Tracks.Count);
            //foreach (var track in song.Tracks)
            for (int iTrack = 0; iTrack < song.Tracks.Count; ++ iTrack)
            {
                var track = song.Tracks[iTrack];
                string trackID = $"{track.Name} #{iTrack}";
                writer.WriteToTrack(trackID, track.Volume);

                writer.WriteToTrack(trackID, track.Receives.Count);
                foreach (var receive in track.Receives)
                {
                    writer.WriteToTrack(trackID, receive.SendingTrackIndex);
                    writer.WriteToTrack(trackID, receive.ReceivingChannelIndex);
                    writer.WriteToTrack(trackID, receive.Volume);
                }

                writer.WriteToTrack(trackID, track.Devices.Count);
                foreach (var deviceId in track.DeviceIndices)
                {
                    writer.WriteToTrack(trackID, deviceId);
                }

                writer.WriteToTrack(trackID, track.MidiLaneId);

                writer.WriteToTrack(trackID, track.Automations.Count);
                foreach (var automation in track.Automations)
                {
                    writer.WriteToTrack(trackID, automation.DeviceIndex);
                    writer.WriteToTrack(trackID, automation.ParamId);
                    writer.WriteToTrack(trackID, automation.DeltaCodedPoints.Count);
                    foreach (var point in automation.DeltaCodedPoints)
                    {
                        writer.WriteToTrack(trackID, point.TimeFromLastPoint);
                        writer.WriteToTrack(trackID, point.Value);
                    }
                }
            }

            return writer;
        }

        void SerializeBlob(StringBuilder sb, Song song)
        {
            var blob = CreateBinary(song).CompleteSong.GetByteArray();

            sb.AppendLine("const unsigned char SongBlob[] =");
            sb.Append("{");
            int numsPerLine = 10;
            for (int i = 0; i < blob.Length; i++)
            {
                if (i < blob.Length - 1 && (i % numsPerLine) == 0)
                {
                    sb.AppendLine();
                    sb.Append("\t");
                }
                sb.Append("0x" + blob[i].ToString("x2") + ",");
                if (i < blob.Length - 1 && (i % numsPerLine) != numsPerLine - 1) sb.Append(" ");
            }
            sb.AppendLine();
            sb.AppendLine("};");
        }
    }
}
