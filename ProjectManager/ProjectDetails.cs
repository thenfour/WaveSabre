﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using WaveSabreConvert;

namespace ProjectManager
{
    public partial class ProjectDetails : Form
    {
        public ProjectDetails(Song song)
        {
            InitializeComponent();
            var trackNodes = new List<TreeNode>();

            var bin = new Serializer().SerializeBinary(song);
            var deviceCount = 0;

            var t = 0;
            foreach (var track in song.Tracks)
            {
                var trackName = string.Format("{0}: {1}", t, track.Name, track);
                var trackNode = new TreeNode(trackName);
                var deviceNodes = new List<TreeNode>();

                if (track.Receives.Count > 0)
                {
                    var recNode = new TreeNode(string.Format("Track receives {0}", track.Receives.Count));

                    foreach (var rec in track.Receives)
                    {
                        var tid = rec.SendingTrackIndex;
                        recNode.Nodes.Add(string.Format("{0}: {1} / Channel Index {2}", tid, song.Tracks[tid].Name, rec.ReceivingChannelIndex));
                    }

                    trackNode.Nodes.Add(recNode);
                }

                var d = 0;
                foreach (var device in track.Devices)
                {
                    deviceCount++;
                    var autoNodes = new List<TreeNode>();
                    var deviceName = string.Format("{0}: {1} ({2} bytes)", d, device.Id, device.Chunk.Length);
                    if (track.Automations.Count > 0)
                    {
                        var autos = track.Automations.Where(a => a.DeviceIndex == d).ToList();
                        if (autos.Count > 0)
                        {
                            var autoHead = new TreeNode(string.Format("Automation Lanes: {0}", autos.Count));
                            foreach (var auto in autos)
                            {
                                autoHead.Nodes.Add(new TreeNode(string.Format("Param Id: {0} / Points {1}", auto.ParamId, auto.DeltaCodedPoints.Count)));
                            }
                            autoNodes.Add(autoHead);
                        }
                    }
                    var deviceNode = new TreeNode(deviceName, autoNodes.ToArray());
                    deviceNodes.Add(deviceNode);
                    d++;
                }

                trackNode.Nodes.Add(new TreeNode(string.Format("Midi Events: {0}", track.Events.Count)));
                trackNode.Nodes.Add(new TreeNode("Devices", deviceNodes.ToArray()));
                trackNodes.Add(trackNode);
                t++;
            }
            var tracks = new TreeNode(string.Format("Tracks: {0}", song.Tracks.Count), trackNodes.ToArray());

            // track data
            var tracksChunkNode = new TreeNode("Chunks: Tracks");
            var tracksOrdered = bin.TrackData.Select(kv => new { kv, compressedSize = kv.Value.GetCompressedSize() }).OrderByDescending(kv => kv.compressedSize);
            foreach (var kvt in tracksOrdered)
            {
                tracksChunkNode.Nodes.Add(new TreeNode($"{kvt.kv.Key}: {kvt.kv.Value.GetSize()} bytes ({kvt.compressedSize} compressed)"));
            }

            // device data
            var deviceChunkNode = new TreeNode("Chunks: Devices");
            var devicesOrdered = bin.DeviceChunks.Select(kv => new { kv, compressedSize = kv.Value.GetCompressedSize() }).OrderByDescending(kv => kv.compressedSize);
            foreach (var kvt in devicesOrdered)
            {
                deviceChunkNode.Nodes.Add(new TreeNode($"{kvt.kv.Key}: {kvt.kv.Value.GetSize()} bytes ({kvt.compressedSize} compressed)"));
            }

            // midi lane data
            var midiLaneChunkNode = new TreeNode("Chunks: Midi Lanes");
            var midiLanesOrdered = bin.MidiLaneData.Select(kv => new { kv, compressedSize = kv.Value.GetCompressedSize() }).OrderByDescending(kv => kv.compressedSize);
            foreach (var kvt in midiLanesOrdered)
            {
                int iMidiLane = kvt.kv.Key;
                // find tracks which reference this midi lane
                string trackRefs = string.Join(", ", song.Tracks.Where(st => st.MidiLaneId == iMidiLane).Select(st => st.Name));
                midiLaneChunkNode.Nodes.Add(new TreeNode($"{kvt.kv.Key}: {kvt.kv.Value.GetSize()} bytes ({kvt.compressedSize} compressed) used by tracks {trackRefs}"));
            }


            treeViewDetails.Nodes.Add(new TreeNode(string.Format("Tempo: {0}", song.Tempo)));
            treeViewDetails.Nodes.Add(new TreeNode(string.Format("Duration: {0} seconds", song.Length)));
            treeViewDetails.Nodes.Add(new TreeNode(string.Format("Total Device Count: {0}", deviceCount)));
            treeViewDetails.Nodes.Add(new TreeNode(string.Format("Total Data Size: {0} bytes ({1} bytes compressed)", bin.CompleteSong.GetSize(), bin.CompleteSong.GetCompressedSize())));

            treeViewDetails.Nodes.Add(tracks);
            treeViewDetails.Nodes.Add(deviceChunkNode);
            treeViewDetails.Nodes.Add(midiLaneChunkNode);
            treeViewDetails.Nodes.Add(tracksChunkNode);
            treeViewDetails.ExpandAll();
            //treeViewDetails.Nodes[4].Expand();
        }
    }
}
