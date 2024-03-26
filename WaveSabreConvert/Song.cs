using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

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
                d.Chunk = Utils.ConvertDeviceChunk(d.Id, d.Chunk);
            }

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
                        case EventType.CC: // ignore.
                            continue;
                        case EventType.PitchBend:
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
                    }

                    // was a duration calculated?
                    if (e.DurationSamples == null)
                    {
                        logger.WriteLine($"!ERROR: no duration was found. Maybe no corresponding note off event for a note on?; track {t.Name}");
                        continue;
                    }
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
