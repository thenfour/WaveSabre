using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using WaveSabreConvert;

namespace ProjectManager
{
    public partial class ProjectDetails : Form
    {
        public static string ConvertTreeViewToString(TreeView treeView)
        {
            StringBuilder stringBuilder = new StringBuilder();
            foreach (TreeNode node in treeView.Nodes)
            {
                BuildTreeString(node, stringBuilder, 0);
            }
            return stringBuilder.ToString();
        }

        public static void BuildTreeString(TreeNode node, StringBuilder stringBuilder, int level)
        {
            stringBuilder.Append(new string('\t', level));
            stringBuilder.AppendLine(node.Text);
            foreach (TreeNode childNode in node.Nodes)
            {
                BuildTreeString(childNode, stringBuilder, level + 1);
            }
        }

        public ProjectDetails(Song song)
        {
            InitializeComponent();
            var trackNodes = new List<TreeNode>();



            var bin = new Serializer().SerializeBinary(song, new NullLogger());
            var deviceCount = 0;

            var t = 0;
            var orderedTracks = song.Tracks.OrderBy((track) => {
                return -song.MidiLanes[track.MidiLaneId].MidiEvents.Count;
            });
            foreach (var track in orderedTracks)
            {
                var midiLane = song.MidiLanes[track.MidiLaneId];                
                var trackName = string.Format($"{t}: {track.Name} ({midiLane.MidiEvents.Count} events)");
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
            var tracksChunkNode = new TreeNode($"Chunks: Tracks: {bin.AllTrackData.GetSize()} bytes ({bin.AllTrackData.GetCompressedSize()} compressed)");
            var tracksOrdered = bin.TrackData.Select(kv => new { kv, compressedSize = kv.Value.GetCompressedSize() }).OrderByDescending(kv => kv.compressedSize);
            foreach (var kvt in tracksOrdered)
            {
                tracksChunkNode.Nodes.Add(new TreeNode($"{kvt.kv.Key}: {kvt.kv.Value.GetSize()} bytes ({kvt.compressedSize} compressed)"));
            }

            // device data
            var deviceChunkNode = new TreeNode($"Chunks: Devices: {bin.AllDeviceChunks.GetSize()} bytes ({bin.AllDeviceChunks.GetCompressedSize()} compressed)");
            var devicesOrdered = bin.DeviceChunks.Select(kv => new { kv, compressedSize = kv.Value.GetCompressedSize() }).OrderByDescending(kv => kv.compressedSize);
            foreach (var kvt in devicesOrdered)
            {
                deviceChunkNode.Nodes.Add(new TreeNode($"{kvt.kv.Key}: {kvt.kv.Value.GetSize()} bytes ({kvt.compressedSize} compressed)"));
            }

            // midi lane data
            var allMidiEventCount = song.MidiLanes.Sum(l => l.MidiEvents.Count);
            var midiLaneChunkNode = new TreeNode($"Chunks: Midi Lanes: {bin.AllMidiLanes.GetSize()} bytes ({bin.AllMidiLanes.GetCompressedSize()} compressed) ({allMidiEventCount} notes)");
            var midiLanesOrdered = bin.MidiLaneData.Select(kv => new { kv, compressedSize = kv.Value.GetCompressedSize() }).OrderByDescending(kv => kv.compressedSize);
            foreach (var kvt in midiLanesOrdered)
            {
                int iMidiLane = kvt.kv.Key;
                // find tracks which reference this midi lane
                string trackRefs = string.Join(", ", song.Tracks.Where(st => st.MidiLaneId == iMidiLane).Select(st => st.Name));
                var eventCount = song.MidiLanes[iMidiLane].MidiEvents.Count;
                midiLaneChunkNode.Nodes.Add(new TreeNode($"{kvt.kv.Key}: {kvt.kv.Value.GetSize()} bytes ({kvt.compressedSize} compressed) used by tracks {trackRefs} ({eventCount} notes)"));
            }


            treeViewDetails.Nodes.Add(new TreeNode(string.Format("Tempo: {0}", song.Tempo)));
            treeViewDetails.Nodes.Add(new TreeNode(string.Format("Duration: {0} seconds", song.Length)));
            treeViewDetails.Nodes.Add(new TreeNode($"Event scale: {1 << song.TimestampScaleLog2}"));
            treeViewDetails.Nodes.Add(new TreeNode($"Duration scale: {1 << song.NoteDurationScaleLog2}"));
            treeViewDetails.Nodes.Add(new TreeNode(string.Format("Total Device Count: {0}", deviceCount)));

            var compressRatio = ((double)bin.CompleteSong.GetCompressedSize()) * 100 / bin.CompleteSong.GetSize();
            treeViewDetails.Nodes.Add(new TreeNode($"Total Data Size: {bin.CompleteSong.GetSize()} bytes ({bin.CompleteSong.GetCompressedSize()} bytes compressed, {compressRatio.ToString("0.00")}% of uncompressed)"));

            treeViewDetails.Nodes.Add(tracks);
            treeViewDetails.Nodes.Add(deviceChunkNode);
            treeViewDetails.Nodes.Add(midiLaneChunkNode);
            treeViewDetails.Nodes.Add(tracksChunkNode);
            //treeViewDetails.ExpandAll();
            //treeViewDetails.Nodes[4].Expand();
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void btnCopy_Click(object sender, EventArgs e)
        {
            string s = ConvertTreeViewToString(treeViewDetails);
            Clipboard.SetText(s);
            MessageBox.Show($"Copied {s.Length} chars to clipboard");
        }
    }
}
