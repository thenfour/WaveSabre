﻿using ReaperParser.ReaperElements;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WaveSabreConvert
{
    public class ReaperTrackLink
    {
        public Song.Track Track;
        public List<ReaperTrackLinkReceive> Receives;

        public ReaperTrackLink(Song.Track track)
        {
            Track = track;
            Receives = new List<ReaperTrackLinkReceive>();
        }
    }

    public class ReaperTrackLinkReceive
    {
        public ReaperTrackLink SendingTrack;
        public int ReceivingChannelIndex;
        public float Volume;
    }

    public class ReaperConverter
    {
        ILog logger;
        ReaperProject project;
        private List<Song.Track> projectTracks;
        private List<ReaperTrackLink> linkedTracks, visitedTracks, orderedTracks;
        private double trackStart;

        public Song Process(ReaperProject project, ILog logger, ConvertOptions options)
        {
            this.logger = logger;
            this.project = project;

            var song = new Song();

            song.SampleRate = project.SampleRate;
            
            song.Tempo = (int)project.Tempo.BPM;

            if (project.TempoEnvelope.Visibility.IsVisible)
            {
                logger.WriteLine("ERROR: This project has a tempo map which is not supported. Even if there's only 1 point, it can mess everything up. Delete the tempo envelope entirely and try again.");
            }

            switch (options.mBoundsMode)
            {
                case BoundsMode.Selection:
                    // TODO (but do we care?)
                //if (project.Selection.Start > project.Selection.End)
                //    trackStart = project.Selection.End;
                //else
                //    trackStart = project.Selection.Start;
                //break;
                case BoundsMode.Regions:
                    if (project.Regions.Count < 1)
                    {
                        logger.WriteLine("ERROR: This project has no regions defined; unable to determine song bounds");
                        return null;
                    }
                    trackStart = project.Regions.Min((rgn) => { return rgn.TimeStart; });
                    break;
                case BoundsMode.Events:
                    trackStart = 0;
                    // todo: check last event
                    break;
            }

            projectTracks = new List<Song.Track>();
            visitedTracks = new List<ReaperTrackLink>();
            orderedTracks = new List<ReaperTrackLink>();

            var master = CreateMaster();
            projectTracks.Add(master);

            var currentBus = master;

            int busLevel = 0;
            foreach (var reaperTrack in project.Tracks)
            {
                var track = CreateTrack(reaperTrack);
                projectTracks.Add(track);
                var trackIndex = projectTracks.IndexOf(track);

                // bus fun!
                switch (reaperTrack.BusConfig.BusMode)
                {
                    case ReaperBusMode.Current:
                        // push this track to current bus
                        currentBus.Receives.Add(new Song.Receive()
                        {
                            ReceivingChannelIndex = 0,
                            SendingTrackIndex = trackIndex,
                            Volume = 1.0f
                        });
                        break;
                    case ReaperBusMode.OpenBus:
                        // create new bus
                        currentBus.Receives.Add(new Song.Receive()
                        {
                            ReceivingChannelIndex = 0,
                            SendingTrackIndex = trackIndex,
                            Volume = 1.0f
                        });
                        busLevel += reaperTrack.BusConfig.BusIcrement;
                        currentBus = track;
                        break;
                    case ReaperBusMode.CloseBus:
                        // drop back to master
                        currentBus.Receives.Add(new Song.Receive()
                        {
                            ReceivingChannelIndex = 0,
                            SendingTrackIndex = trackIndex,
                            Volume = 1.0f
                        });
                        currentBus = master;
                        busLevel += reaperTrack.BusConfig.BusIcrement;
                        break;
                }
            }

            // all tracks added but with track Id's instead of referencable objects
            // create linkedTracks so recevies are based on object rather than Id
            linkedTracks = new List<ReaperTrackLink>();

            foreach (var track in projectTracks)
                linkedTracks.Add(new ReaperTrackLink(track));

            foreach (var track in linkedTracks)
            { 
                foreach (var receive in track.Track.Receives)
                {
                    track.Receives.Add(new ReaperTrackLinkReceive()
                    {
                        SendingTrack = linkedTracks[receive.SendingTrackIndex],
                        ReceivingChannelIndex = receive.ReceivingChannelIndex,
                        Volume = receive.Volume
                    });
                }
            }

            // now visit the master and order the tracks using DFS
            VisitTrack(linkedTracks[0]);

            // tracks now correctly ordered so rebuild the Id's
            foreach (var track in orderedTracks)
            {
                track.Track.Receives = new List<Song.Receive>();
                foreach (var receive in track.Receives)
                {
                    track.Track.Receives.Add(new Song.Receive()
                    {
                        ReceivingChannelIndex = receive.ReceivingChannelIndex,
                        Volume = receive.Volume,
                        SendingTrackIndex = orderedTracks.IndexOf(receive.SendingTrack)
                    });
                }
                
                song.Tracks.Add(track.Track);
            }

            switch (options.mBoundsMode)
            {
                case BoundsMode.Events:
                    // TODO.
                    //trackStart = 0;
                    //var lastEvent = 0;
                    //foreach (var t in song.Tracks)
                    //{
                    //    if (t.Events.Count > 0)
                    //    {
                    //        var thisLastEvent = t.Events.Max(e => e.TimeStamp);
                    //        if (thisLastEvent > lastEvent)
                    //            lastEvent = thisLastEvent;
                    //    }
                    //}
                    //song.Length = (double)lastEvent / (double)project.SampleRate;

                    //break;
                case BoundsMode.Selection:
                    //if (project.Selection.Start < project.Selection.End)
                    //    song.Length = project.Selection.End - project.Selection.Start;
                    //else
                    //    song.Length = project.Selection.Start - project.Selection.End;

                    //logger.WriteLine("INFO: track start and end points taken from selection");
                    //break;
                case BoundsMode.Regions:
                    var songEnd = project.Regions.Max((rgn) => { return rgn.TimeEnd; });
                    song.Length = songEnd - trackStart;
                    break;
            }

            return song;
        }

        void VisitTrack(ReaperTrackLink projectTrack)
        {
            if (visitedTracks.Contains(projectTrack)) return;

            visitedTracks.Add(projectTrack);
            foreach (var projectReceive in projectTrack.Receives)
            {
                if (projectReceive.Volume > 0.0)
                { 
                    VisitTrack(projectReceive.SendingTrack);
                }
            }
            orderedTracks.Add(projectTrack);
        }

        public Song.Track CreateTrack(ReaperTrack reaperTrack)
        {
            var track = new Song.Track();
            track.Name = reaperTrack.TrackName;
            track.Volume = reaperTrack.VolumePanning.Volume;

            if (reaperTrack.VolumePanning.Pan != 0)
                logger.WriteLine("WARNING: Pan value {0} on track {1} unsupported", reaperTrack.VolumePanning.Pan, reaperTrack.TrackName);

            // add receives
            foreach (var r in reaperTrack.Receives)
            {
                if (r.ReceiveFader != ReaperFader.PostFader)
                {
                    logger.WriteLine("WARNING: Receive fader type of {0} on track {1} unsupported", r.ReceiveFader, reaperTrack.TrackName);
                }

                var receive = new Song.Receive();
                receive.SendingTrackIndex = r.ReceiveTrackId + 1;       // master always zero, so tracks are out by one
                receive.ReceivingChannelIndex = r.DestinationChannelIndex;
                receive.Volume = r.Volume;
                track.Receives.Add(receive);
            }

            foreach (var f in reaperTrack.EffectsChain)
            {
                if (f.Vst == null)
                {
                    logger.WriteLine($"Track {reaperTrack.TrackName} has no VST?");
                    continue;
                }
                if (f.Wet != 0)
                    logger.WriteLine("WARNING: Wet value for effect {0} on track {1} unsupported", f.Vst.VstFile, reaperTrack.TrackName);

                Song.Device device = null;
                
                Song.DeviceId deviceId;
                if (Enum.TryParse<Song.DeviceId>(f.Vst.VstFile.Replace(".dll", "").Replace(".64", ""), out deviceId))
                {
                    device = new Song.Device();
                    device.Id = deviceId;
                    device.Chunk = f.Vst.VstChunkData;
                }
                if (device == null)
                {
                    logger.WriteLine("ERROR: Device skipped (unsupported plugin): " + f.Vst.VstFile);
                }
                else
                {
                    if (f.IsBypassed)
                    {
                        logger.WriteLine("ERROR: Bypassed devices cause problems: " + f.Vst.VstName);
                    }

                    track.Devices.Add(device);
                    var deviceIndex = track.Devices.IndexOf(device);
                    foreach (var a in f.Automations)
                    {
                        track.Automations.Add(ConvertAutomation(a, deviceIndex, reaperTrack.TrackName));
                    }

                }
                track.Events = ConvertMidi(reaperTrack.MediaItems, reaperTrack);
            }

            return track;
        }

        private List<Song.Event> ConvertMidi(List<ReaperMediaItem> reaperMedia, ReaperTrack track)
        {
            var events = new List<Song.Event>();

            foreach (var mediaItem in reaperMedia)
            {
                var itemPosition = SecondsToSamples(mediaItem.Position - trackStart);
                if (itemPosition < 0)
                    continue;

                // this is not "start offset"; it's slip offset. it's the position within the media item that this instance begins on.
                //var itemStart = SecondsToSamples(mediaItem.StartOffest);
                if (mediaItem.StartOffest.Offset2 != 0)
                {
                    logger.WriteLine($"ERROR: media item midi offsets are not supported; audio will not be aligned. Glue media items before conversion to ensure SOFFS xxx 0. Found in track {track.TrackName}");
                }
                if (mediaItem.PlayRate.Rate != 1 || mediaItem.PlayRate.Semitones != 0)
                {
                    logger.WriteLine($"ERROR: media item play rate != 1 is not supported; audio will not be aligned. Found in track {track.TrackName}");
                }
                if (mediaItem.FadeIn.Duration != 0)
                {
                    logger.WriteLine($"ERROR: media item fade in is not supported. Found in track {track.TrackName}");
                }
                if (mediaItem.FadeOut.Duration != 0)
                {
                    logger.WriteLine($"ERROR: media item fade out is not supported. Found in track {track.TrackName}");
                }
                if (mediaItem.VolumePanning.Volume != 1)
                {
                    logger.WriteLine($"ERROR: media item volume not supported. Found in track {track.TrackName}");
                }
                var slipOffsetSamples = SecondsToSamples(mediaItem.StartOffest.Offset1);
                var itemEnd = slipOffsetSamples + SecondsToSamples(mediaItem.Length);

                foreach (var source in mediaItem.MediaSource)
                {
                    if (source.MediaType == "MIDI")
                    {
                        bool loop = true;
                        var position = 0;
                        var activeNotes = new bool[128];

                        if (source.IgnoreTempo)
                        {
                            logger.WriteLine($"ERROR: Ignore tempo in media item source is unsupported. Found in track {track.TrackName}");
                        }

                        while (loop)
                        {
                            foreach (var e in source.MidiEvents)
                            {
                                position += e.PositionDelta;
                                var eventPosition = PositionToSamples((float)position / (float)source.MidiConfig.NoteSize / (float)project.Tempo.Beats);
                                //eventPosition -= slipOffsetSamples; // convert position-in-media-item to position-in-song.
                                var timestamp = eventPosition + itemPosition - slipOffsetSamples;

                                if (eventPosition < slipOffsetSamples)      // before instance is in view.
                                    continue;

                                if (eventPosition >= itemEnd)        // end of item close all notes and be done with ti
                                {
                                    for (var note = 0; note < 128; note++)
                                    {
                                        if (activeNotes[note])
                                        {
                                            events.Add(new Song.Event()
                                            {
                                                TimeStamp = itemEnd + itemPosition - slipOffsetSamples,
                                                Note = (byte)note,
                                                Velocity = (byte)0,
                                                Type = Song.EventType.NoteOff
                                            });
                                        }
                                    }
                                    loop = false;
                                    break;
                                }

                                if (e.MidiEvent == ReaperNoteEvent.NoteOn)
                                {
                                    activeNotes[e.Note] = true;
                                    events.Add(new Song.Event()
                                    {
                                        TimeStamp = timestamp,
                                        Note = (byte)e.Note,
                                        Velocity = (byte)e.Velocity,
                                        Type = Song.EventType.NoteOn
                                    });
                                }
                                else if (e.MidiEvent == ReaperNoteEvent.NoteOff)
                                {
                                    activeNotes[e.Note] = false;
                                    events.Add(new Song.Event()
                                    {
                                        TimeStamp = timestamp,
                                        Note = (byte)e.Note,
                                        Velocity = (byte)e.Velocity,
                                        Type = Song.EventType.NoteOff
                                    });
                                }
                                else if (e.MidiEvent == ReaperNoteEvent.CC)
                                {
                                    events.Add(new Song.Event()
                                    {
                                        TimeStamp = timestamp,
                                        Note = (byte)e.Note,
                                        Velocity = (byte)e.Velocity,
                                        Type = Song.EventType.CC,
                                    });

                                }
                                else if (e.MidiEvent == ReaperNoteEvent.PitchBend)
                                {
                                    events.Add(new Song.Event()
                                    {
                                        TimeStamp = timestamp,
                                        Note = (byte)e.Note,
                                        Velocity = (byte)e.Velocity,
                                        Type = Song.EventType.PitchBend,
                                    });
                                }
                                else
                                {
                                    Console.WriteLine($"Unknown reaper event: {e.MidiEvent}");
                                }
                            }
                        }
                    }
                    else
                    {
                        Console.WriteLine($"Unknown reaper media type: {source.MediaType}");
                    }
                }
            }

            events.Sort((a, b) =>
            {
                if (a.TimeStamp > b.TimeStamp) return 1;
                if (a.TimeStamp < b.TimeStamp) return -1;
                if (a.Type == Song.EventType.NoteOn && b.Type == Song.EventType.NoteOff) return 1;
                if (a.Type == Song.EventType.NoteOff && b.Type == Song.EventType.NoteOn) return -1;
                return 0;
            });

            return events;
        }

        public Song.Track CreateMaster()
        {
            var master = new Song.Track();
            master.Name = "Master";
            master.Volume = project.MasterVolume.Volume;

            foreach (var f in project.MasterEffectsChain)
            {
                Song.Device device = null;

                Song.DeviceId deviceId;
                if (Enum.TryParse<Song.DeviceId>(f.Vst.VstFile.Replace(".dll", "").Replace(".64", ""), out deviceId))
                {
                    device = new Song.Device();
                    device.Id = deviceId;
                    device.Chunk = f.Vst.VstChunkData;
                }
                if (device == null)
                {
                    logger.WriteLine("ERROR: Device skipped (unsupported plugin): " + f.Vst.VstFile);
                }
                else
                {
                    if (f.IsBypassed)
                    {
                        logger.WriteLine("ERROR: Bypassed devices cause problems: " + f.ParentElement.ElementName);
                    }

                    master.Devices.Add(device);
                    var deviceIndex = master.Devices.IndexOf(device);
                    foreach (var a in f.Automations)
                    {
                        master.Automations.Add(ConvertAutomation(a, deviceIndex, "Master"));
                    }
                }
            }

            return master;
        }

        private Song.Automation ConvertAutomation(ReaperAutomation reaperAutomation, int deviceIndex, string trackName)
        {
            var auto = new Song.Automation();
            auto.DeviceIndex = deviceIndex;
            auto.ParamId = reaperAutomation.ParameterId;
            auto.Points = new List<Song.Point>();
            foreach (var p in reaperAutomation.Points)
            {
                if (p.Position < trackStart)
                    continue;

                if (p.Shape != ReaperPointShape.Linear)
                {
                    logger.WriteLine("WARNING: Automation Shape {0} unsupported on Master ", p.Shape.ToString());
                }

                auto.Points.Add(new Song.Point()
                {
                    TimeStamp = SecondsToSamples(p.Position - trackStart),
                    Value = p.Value
                });
            }

            return auto;
        }

        public int PositionToSamples(float position)
        {
            double samples = (((60.0 / project.Tempo.BPM) * project.SampleRate) * project.Tempo.Beats) * position;
            return (int)samples;
        }

        public int SecondsToSamples(double seconds)
        {
            return (int)((double)project.SampleRate * (double)seconds);
        }
    }
}
