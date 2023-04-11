﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using static WaveSabreConvert.Song;

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
            public void WriteVarUint32(UInt32 value)
            {
                do
                {
                    byte n = (byte)(value & 0x7f);
                    value >>= 7;
                    if (value != 0)
                    {
                        n |= 0x80;
                    }
                    this.writer.Write(n);
                } while (value != 0);
            }
        }

        public class BinaryOutput
        {
            public BinaryStream CompleteSong = new BinaryStream();
            public BinaryStream AllDeviceChunks = new BinaryStream();
            public BinaryStream AllMidiLanes = new BinaryStream();
            public BinaryStream AllTrackData = new BinaryStream();
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
            public void WriteVarUint32(UInt32 value)
            {
                this.CompleteSong.WriteVarUint32(value);
            }


            ///////////////////////////////////////////////////
            public void WriteToTrack(string trackID, int n)
            {
                if (!TrackData.ContainsKey(trackID)) TrackData[trackID] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                TrackData[trackID].writer.Write(n);
                this.AllTrackData.writer.Write(n);
            }
            public void WriteToTrack(string trackID, double n)
            {
                if (!TrackData.ContainsKey(trackID)) TrackData[trackID] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                TrackData[trackID].writer.Write(n);
                this.AllTrackData.writer.Write(n);
            }
            public void WriteToTrack(string trackID, float n)
            {
                if (!TrackData.ContainsKey(trackID)) TrackData[trackID] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                TrackData[trackID].writer.Write(n);
                this.AllTrackData.writer.Write(n);
            }
            public void WriteToTrack(string trackID, byte[] n)
            {
                if (!TrackData.ContainsKey(trackID)) TrackData[trackID] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                TrackData[trackID].writer.Write(n);
                this.AllTrackData.writer.Write(n);
            }
            public void WriteToTrack(string trackID, byte n)
            {
                if (!TrackData.ContainsKey(trackID)) TrackData[trackID] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                TrackData[trackID].writer.Write(n);
                this.AllTrackData.writer.Write(n);
            }
            public void WriteToTrackVarUint32(string trackID, UInt32 n)
            {
                if (!TrackData.ContainsKey(trackID)) TrackData[trackID] = new BinaryStream();
                this.CompleteSong.WriteVarUint32(n);
                TrackData[trackID].WriteVarUint32(n);
                this.AllTrackData.WriteVarUint32(n);
            }

            ///////////////////////////////////////////////////
            public void Write(Song.DeviceId d, int n)
            {
                if (!DeviceChunks.ContainsKey(d)) DeviceChunks[d] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllDeviceChunks.writer.Write(n);
                DeviceChunks[d].writer.Write(n);
            }
            public void Write(Song.DeviceId d, double n)
            {
                if (!DeviceChunks.ContainsKey(d)) DeviceChunks[d] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllDeviceChunks.writer.Write(n);
                DeviceChunks[d].writer.Write(n);
            }
            public void Write(Song.DeviceId d, float n)
            {
                if (!DeviceChunks.ContainsKey(d)) DeviceChunks[d] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllDeviceChunks.writer.Write(n);
                DeviceChunks[d].writer.Write(n);
            }
            public void Write(Song.DeviceId d, byte[] n)
            {
                if (!DeviceChunks.ContainsKey(d)) DeviceChunks[d] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllDeviceChunks.writer.Write(n);
                DeviceChunks[d].writer.Write(n);
            }
            public void Write(Song.DeviceId d, byte n)
            {
                if (!DeviceChunks.ContainsKey(d)) DeviceChunks[d] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllDeviceChunks.writer.Write(n);
                DeviceChunks[d].writer.Write(n);
            }
            public void WriteVarUint32(Song.DeviceId d, UInt32 n)
            {
                if (!DeviceChunks.ContainsKey(d)) DeviceChunks[d] = new BinaryStream();
                this.CompleteSong.WriteVarUint32(n);
                this.AllDeviceChunks.WriteVarUint32(n);
                DeviceChunks[d].WriteVarUint32(n);
            }


            ///////////////////////////////////////////////////
            public void WriteMidiLane(int iMidiLane, int n)
            {
                if (!MidiLaneData.ContainsKey(iMidiLane)) MidiLaneData[iMidiLane] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllMidiLanes.writer.Write(n);
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLaneVarUint32(int iMidiLane, UInt32 value)
            {
                if (!MidiLaneData.ContainsKey(iMidiLane)) MidiLaneData[iMidiLane] = new BinaryStream();
                this.CompleteSong.WriteVarUint32(value);
                this.AllMidiLanes.WriteVarUint32(value);
                MidiLaneData[iMidiLane].WriteVarUint32(value);
            }
            public void WriteMidiLane(int iMidiLane, UInt32 n)
            {
                if (!MidiLaneData.ContainsKey(iMidiLane)) MidiLaneData[iMidiLane] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllMidiLanes.writer.Write(n);
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLane(int iMidiLane, double n)
            {
                if (!MidiLaneData.ContainsKey(iMidiLane)) MidiLaneData[iMidiLane] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllMidiLanes.writer.Write(n);
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLane(int iMidiLane, float n)
            {
                if (!MidiLaneData.ContainsKey(iMidiLane)) MidiLaneData[iMidiLane] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllMidiLanes.writer.Write(n);
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLane(int iMidiLane, byte[] n)
            {
                if (!MidiLaneData.ContainsKey(iMidiLane)) MidiLaneData[iMidiLane] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllMidiLanes.writer.Write(n);
                MidiLaneData[iMidiLane].writer.Write(n);
            }
            public void WriteMidiLane(int iMidiLane, byte n)
            {
                if (!MidiLaneData.ContainsKey(iMidiLane)) MidiLaneData[iMidiLane] = new BinaryStream();
                this.CompleteSong.writer.Write(n);
                this.AllMidiLanes.writer.Write(n);
                MidiLaneData[iMidiLane].writer.Write(n);
            }
        }

        BinaryOutput CreateBinary(Song song)
        {
            BinaryOutput writer = new BinaryOutput();

            // song header.
            // TODO: Organize header version numbers in some structured way. But currently it's not easy to predict what
            // would be the best way to do this so stay simple until the next change is required, and clean this up.
            writer.Write(("WSBR0001".Select(ch => Convert.ToByte(ch)).ToArray()));

            // song settings
            writer.Write(song.Tempo);
            writer.Write(song.SampleRate);
            writer.Write(song.Length);

            // serialize all devices
            writer.Write(song.Devices.Count);
            foreach (var device in song.Devices)
            {
                writer.Write(device.Id, (byte)device.Id);
                writer.WriteVarUint32(device.Id, (UInt32)device.Chunk.Length);
                writer.Write(device.Id, device.Chunk);
            }

            // serialize all midi lanes
            writer.Write(song.MidiLanes.Count);
            for (int iMidiLane = 0; iMidiLane < song.MidiLanes.Count; ++ iMidiLane)
            {
                var midiLane = song.MidiLanes[iMidiLane];

                // write whether note ons incur velocities
                // find all tracks using this
                bool fixedVel = song.Tracks.Where(t => t.MidiLaneId == iMidiLane).Any(t => t.Name.ToLowerInvariant().Contains("#fixedvelocity"));
                writer.WriteMidiLane(iMidiLane, (byte)(fixedVel ? 1 : 0));

                writer.WriteMidiLane(iMidiLane, midiLane.MidiEvents.Count);
                foreach (var e in midiLane.MidiEvents)
                {
                    if (e.TimeFromLastEvent < 0)
                    {
                        Console.WriteLine($"Negative time deltas break things. wut?");
                    }
                    int eventType = (int)e.Type;
                    if (eventType < 0 || eventType > 3)
                    {
                        Console.WriteLine($"ERROR: unsupportable event type");
                    }
                    UInt32 t = (UInt32)e.TimeFromLastEvent;
                    t <<= 2;
                    t |= (UInt32)eventType;
                    writer.WriteMidiLaneVarUint32(iMidiLane, t);
                }
                foreach (var e in midiLane.MidiEvents)
                {
                    writer.WriteMidiLane(iMidiLane, (byte)e.Note);
                }
                foreach (var e in midiLane.MidiEvents)
                {
                    switch (e.Type)
                    {
                        case EventType.NoteOff:
                            continue;
                        case EventType.NoteOn:
                            if (fixedVel)
                            {
                                continue;
                            }
                            writer.WriteMidiLane(iMidiLane, (byte)e.Velocity);
                            break;
                        case EventType.CC:
                        default:
                            writer.WriteMidiLane(iMidiLane, (byte)e.Velocity);
                            break;
                    }

                }
            }

            // serialize each track
            writer.Write(song.Tracks.Count);
            for (int iTrack = 0; iTrack < song.Tracks.Count; ++ iTrack)
            {
                var track = song.Tracks[iTrack];
                string trackID = $"{track.Name} #{iTrack}";
                writer.WriteToTrack(trackID, track.Volume);

                writer.WriteToTrackVarUint32(trackID, (UInt32)track.Receives.Count);
                foreach (var receive in track.Receives)
                {
                    writer.WriteToTrackVarUint32(trackID, (UInt32)receive.SendingTrackIndex);
                    writer.WriteToTrackVarUint32(trackID, (UInt32)receive.ReceivingChannelIndex);
                    writer.WriteToTrack(trackID, receive.Volume);
                }

                writer.WriteToTrackVarUint32(trackID, (UInt32)track.Devices.Count);
                foreach (var deviceId in track.DeviceIndices)
                {
                    writer.WriteToTrackVarUint32(trackID, (UInt32)deviceId);
                }

                writer.WriteToTrackVarUint32(trackID, (UInt32)track.MidiLaneId);

                writer.WriteToTrackVarUint32(trackID, (UInt32)track.Automations.Count);
                foreach (var automation in track.Automations)
                {
                    writer.WriteToTrackVarUint32(trackID, (UInt32)automation.DeviceIndex);
                    writer.WriteToTrackVarUint32(trackID, (UInt32)automation.ParamId);
                    writer.WriteToTrackVarUint32(trackID, (UInt32)automation.DeltaCodedPoints.Count);
                    foreach (var point in automation.DeltaCodedPoints)
                    {
                        writer.WriteToTrackVarUint32(trackID, (UInt32)point.TimeFromLastPoint);
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
