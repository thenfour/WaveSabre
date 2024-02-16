using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Remoting.Messaging;
using System.Security.Principal;
using System.Text;
using static System.Net.Mime.MediaTypeNames;

namespace ReaperParser.ReaperElements
{
    [ReaperTag("REAPER_PROJECT")]
    public class ReaperProject : ReaperElement
    {
        public float SomeVersion { get; set; }
        public string Version { get; set; }
        public int ProjectId { get; set; }

        [ReaperTag("RIPPLE")]
        public int Ripple { get; set; }
        [ReaperTag("AUTOXFADE")]
        public int AuroCrossFade { get; set; }
        [ReaperTag("PANLAW")]
        public int PanLaw { get; set; }
        [ReaperTag("SAMPLERATE")]
        public int SampleRate { get; set; }
        [ReaperTag("LOOP")]
        public bool Loop { get; set; }

        [ReaperTag("TEMPOENVLOCKMODE")]
        public int TempoEnvelopeLockMode { get; set; }

        public ReaperMasterVolumePan MasterVolume { get; set; }
        public ReaperTempo Tempo { get; set; }
        public TempoEnvelopeEx TempoEnvelope { get; set; }
        public ReaperSelection Selection { get; set; }
        public ReaperZoom Zoom { get; set; }
        public List<ReaperTrack> Tracks { get; } = new List<ReaperTrack>();
        public List<ReaperMasterFxChain> MasterEffectsChain { get; } = new List<ReaperMasterFxChain>();

        public List<ReaperMarker> Markers { get; } = new List<ReaperMarker>();

        public List<ReaperRegion> Regions { get; } = new List<ReaperRegion>();

        // after parsing, we have a bunch of markers but in order to use regions in a more intuitive way, some transformation should be done.
        public void NormalizeRegions()
        {
            // regions are actually just a pair of markers. so examine markers to generate a list of regions.
            Dictionary<int, ReaperRegion> rgnDict = new Dictionary<int, ReaperRegion>();
            foreach (var m in Markers)
            {
                if (!m.IsRegion) continue;
                if (!rgnDict.ContainsKey(m.ID))
                {
                    rgnDict.Add(m.ID, new ReaperRegion());
                    rgnDict[m.ID].ID = m.ID;
                }
                if (!string.IsNullOrEmpty(m.Name) && m.Name != "\"\"") // region end markers have a name of `""`
                {
                    rgnDict[m.ID].Name = m.Name;
                }
                rgnDict[m.ID].Times.Add(m.Time);
                rgnDict[m.ID].Markers.Add(m);
            }

            // the region dict now will contain a lot of stuff that may not be regions.
            // filter out IDs which aren't regions.
            var regions = rgnDict.Values.Where(v => v.IsValidRegion);

            this.Regions.AddRange(regions);
        }
    }

    public class ReaperRegion
    {
        public int ID { get; set; }
        public List<double> Times { get; } = new List<double>();
        public List<ReaperMarker> Markers { get; } = new List<ReaperMarker>();
        public bool IsValidRegion { get { 
                return (Times.Count == 2)
                    && (TimeLength > 0)
                    && Markers.Any(m => m.IsRegion) // this is not the case. normal markers also have type "R".
                    ;
            } }
        public double TimeLength {  get { return TimeEnd - TimeStart;  } }
        public string Name { get; set; }
        public double TimeStart { get { return Times.Min(); } }
        public double TimeEnd { get { return Times.Max(); } }
    }


    /*
     let's look at markers.

  MARKER 1 0.25 "" 1 0 1 R {C8D5E1E8-8D40-442C-8BB1-D00A8BA03ADE} 0
  MARKER 1 17.5 "" 1
  MARKER 1 5.25 "" 0 0 1 R {3856C760-FB6D-4634-AAE2-D035EC3D6A94} 0
  MARKER 3 9.75 "" 0 0 1 R {5FFCE4D7-AD02-461E-9FEA-660F8DFCFB9F} 0
  MARKER 2 13.5 "" 0 0 1 R {FA832776-DFAA-4082-AB37-B3883DE25762} 0
  MARKER 2 20.5 "two - blue" 1 33489152 1 R {0167C27A-2C26-4153-B023-923843E1F974} 0
  MARKER 2 22.5 "" 1

    this is from a brand new project, reaper v7.09, 2024-02-16,
    where i added 2 regions, and 3 markers.

    note: marker IDs are reused between markers & regions.
    note: but they are separated by the flags field.

  MARKER 1 0.25 "" 1 0 1 R {C8D5E1E8-8D40-442C-8BB1-D00A8BA03ADE} 0
         ^id           |   |                                      |
           ^position
                ^name  |   |                                      |
                   ^flags? i believe this is flags. confirmed that reaper responds to this; 0=marker, 1=region.
                     ^color                                       |
                       ^?? always 1
                         ^type? always R                          |
                           ^guid
                                                                  ^?? always 0

     */

    [ReaperTag("MARKER")]
    public class ReaperMarker : ReaperElement
    {
        public int ID { get; set; }
        public double Time { get; set; }
        public string Name { get; set; }

        /*
         according to something:
0 (no flags): This is the default value and indicates that no flags have been set for the marker.
1 (region): This flag indicates that the marker is a region marker, which can be used to define a region of the timeline.
2 (loop): This flag indicates that the marker is a loop point, which can be used to define a section of the timeline that should be looped.
4 (use marker name for REX): This flag is used when exporting a marker to a REX file, and indicates that the name of the marker should be used as the name of the REX file.
8 (sync point): This flag indicates that the marker is a sync point, which can be used to synchronize playback with external devices.
16 (selection endpoint): This flag indicates that the marker is the endpoint of a time selection.
        */
        public int Flags { get; set; }

        //public double Beats { get; set; }

        public int Color { get; set; }
        public int AlwaysOne { get; set; }


        public string Type { get; set; } // "R"
        public string Guid { get; set; }
        public int AlwaysZero { get; set; }

        public bool IsRegion {  get { return (Flags & 1) == 1; } }
    }


    [ReaperTag("SELECTION")]
        public class ReaperSelection : ReaperElement
        {
            public double Start { get; set; }
            public double End { get; set; }
        }
        [ReaperTag("TEMPO")]
        public class ReaperTempo : ReaperElement
        {
            public float BPM { get; set; }
            public int Beats { get; set; }
        public int Bars { get; set; }
    }

    [ReaperTag("ZOOM")]
    public class ReaperZoom : ReaperElement
    {
        public float ZoomSize { get; set; }
        public float ZoomA { get; set; }
        public float ZoomB { get; set; }
    }

    [ReaperTag("MASTER_VOLUME")]
    public class ReaperMasterVolumePan : ReaperElement
    {
        public float Volume { get; set; }
        public float Pan { get; set; }
        public float VolumeX { get; set; }
        public float VolumeY { get; set; }
    }

    [ReaperTag("TEMPOENVEX")]
    public class TempoEnvelopeEx : ReaperElement
    {
        public TempoEnvelopeVisability Visibility { get; set; }
        public List<ReaperAutomationPoint> Points { get; } = new List<ReaperAutomationPoint>();
    }

    [ReaperTag("VIS")]
    public class TempoEnvelopeVisability : ReaperElement
    {
        public bool IsVisible { get; set; } // 
        public int PointType { get; set; } // continuous or points
        public bool DisplayColor { get; set; }
    }

    [ReaperTag("ACT")]
    public class TempoEnvelopeActive : ReaperElement
    {
        public int A { get; set; }
        public int B { get; set; }
    }

    [ReaperTag("MASTERFXLIST")]
    public class ReaperMasterFxChain : ReaperElement
    {
        [ReaperTag("FXID")]
        public string Fxid { get; set; }
        [ReaperTag("WET")]
        public float Wet { get; set; }
        public ReaperVst Vst { get; set; }
        public List<ReaperAutomation> Automations { get; set; }


        [ReaperTag("BYPASS")]
        public string _bypass { get; set; }
        public bool IsBypassed
        {
            get
            {
                return string.Equals(_bypass, "1");
            }
        }

        public ReaperMasterFxChain()
        {
            Automations = new List<ReaperAutomation>();
        }

        // override for customer parse of data
        public override void CompleteParse()
        {
            // clean up empty reaper effect chains
            // wanted to do this a better way, but the file format is lame
            if (this.Fxid == null)
            {
                var project = (ReaperProject)this.ParentElement;
                project.MasterEffectsChain.Remove(this);
            }
        }
    }
}
