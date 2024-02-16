

general dev notes and workflow
-----------------------------------

  * main dev VS solution : `build\WaveSabre.sln`
    * vsts get built to `C:\root\vstpluginlinks\WaveSabre` ; this is specified in the `switch_to_x86.cmd` scripts.
  * only `x86` or `x64` build is active at a time. they are binary compatible wrt the song payload / device chunks.
  * launch project manager: 
  * drag & drop your reaper `.rpp` onto project mgr; `copy .hpp`.
  * paste & replace the whole `rendered.hpp` in the `WaveSabreExecutableMusicPlayer` project.
  * Check some configuration:
    * see `#define WS_EXEPLAYER_RELEASE_FEATURES` which enables / disables some useful features during development.
    * also search `SetThreadPriority` if you want to disable thread prioritization (bogs down system which is annoying during some debugging)
    * see also `gpRenderer = new SongRenderer(&gSong, mProcessorCount);` if you want to control thread count.
  * typical fx chain includes only the following devices:
    * maj7
    * leveller (EQ)
    * maj7 width (stereo image)
    * echo (delay)
    * cathedral (reverb)
    * smasher (compression)
  

size optimization & performance notes
-----------------------------------

  * x86 is smaller than x64 (by how much?)
  * comp tracks with lots of chords are especially bad for compression.
  * using modulations, delays, etc, can help a lot with note optimization; delays may reduce the need for repeating chords etc.
  * use `#fixedvelocity` in track names to elide serializing velocities.

optimization numbers
-----------------------------------

  * with typical devices and empty song data, size = 19856 (x86, minsizerel, squishy, unpadded size).

measuring the size of a device's code is tricky because much code is shared between modules. if you
measure the diff between "no devices" and "just Leveller" for example, you'll get a value which is more than the useful value.
a better way to measure is to measure with ALL devices enabled, and then disable the one you're curious about. then you're seeing
the code unique to that module.

Using the possize (from 0 devices to the 1 in question), and the abovementioned method (negsize), with the above mentioned devices:

| Device    | Possize | Negsize |
|-----------|---------|---------|
| Maj7      | 10160   | 8516    |
| Leveller  | 1848    | 524     |
| Maj7Width | 1608    | 188     |
| Echo      | 1936    | 360     |
| Cathedral | 2140    | 916     |
| Smasher   | 1616    | 408     |


Use Project Manager to analyze note / patch size info.

Measuring code size with SizeBench
-----------------------------------

Use [SizeBench](https://github.com/microsoft/SizeBench). But you can't use it on the `MinSizeRel` build because PDB info is required, and this configuration doesn't output debug info for size reasons. So use `RelWithDebugInfo`, and open the binary + PDB in sizebench.

There you can see a lot of info about individual classes / functions / symbols.

Note: Smaller code here does not necessarily mean smaller code after squishy. Even some smaller code will output larger after compression, because of entropy.

Note: also, it's again deceptive because much code is inlined or again shared in ways PDBs don't quite understand.

The most reliable way is to just DO an optimization or just remove some hunks of code and see the result.



tenfour's changes for revision 2023
-----------------------------------

* project mgr
    * more info shown in project mgr (compressed size, and breakdown by midi lane / device / track)
    * ability to copy project info as text
    * ability to copy complete render .hpp file
    * warnings for unsupported features
    * selectable options for song bounds
* Pitchbend and midi CC automation is now supported.
* wavesabre tag now marks the beginning of a song chunk
* EXE music project
* VST improvements
  * Now using ImGui for its flexibility
  * Ability to type in values
  * Shift+drag = fine adjust
  * Smasher now shows level meters for input, output, compression, and threshold.
  * You can see now how compressible a device chunk will be.
  * VST chunks are now different than minsize chunks; this means VSTs are stored as text-based JSON key-value pairs therefore:
    * can be copied/pasted from clipboard, or saved/loaded from disk
    * will remain compatible no matter how the minsize serialization format evolves.
* New devices
    * Maj7 synthesizer (this is the biggest change IMO)
    * Width device for stereo field adjustments including narrowing of bass freqs
    * Scissor now supports tanh saturation
* Size optimizations
    * MIDI LANES
        * Track names can now have directives; the first directive is "#fixedvelocity" where velocities are left out of the chunk.
        * De-interleaving event / midi note / velocity, in order to put like data with like.
        * Time values now stored as var-length ints instead of int32.
    * PARAM chunks
        * Now storing params as diff of default value
        * optimization stage lets the device set values explicitly to 0 when they're not used
        * params are now stored as 16-bit signed values which are slightly more compressible than floats.
        * defaults now stored in code as a single blob
    * CODE SIZE
        * Biquad filter / other hand-optimizations
        * Using memset to set arrays of floats to 0
        * Using a new method of accessing params which reduces amount of code (a few kb, compressed!), and unifies a lot of params (ParamAccessor)
* Rendering speed optimizations
    * Deep tracks now prioritized for processing first
    * Proper non-busy locking in the node graph runner (rewrite of graph runner)


Maj7
----

A polyphonic megasynthesizer specifically designed for 32kb executable music.

* 4 fully high quality (polyblep bandlimiting) oscillators
* Hard-sync and FM supported on all oscillators
* 4 samplers which can use loaded samples or GM.DLS, each with key zone (i.e. drum kit support)
* 4 LFOs, including noise with adjustable HP and LP
* 10 modulation envelopes (DAHDSR, one-shot, note retrig, each stage with adjustable curve)
* 40 user-configurable modulations
    * Each with range, curve, mapping settings
    * Each with a side-chain (think Serum) which attenuates the mod strength based on another mod source (e.g. modulate env -> volume, and side-chain with velocity)
* Full 4op FM matrix
* 2 stereo filters available, with key tracking frequency support
* 7 macro knobs (think NI Massive) which can be used as modulation sources
* It's fairly performant, but there are quality settings for big projects

So this is a LOT of features, and therefore it's a lot of code. Something like 9kb of code, compressed. On the other hand it replaces at least 4 existing WaveSabre devices.

techniques

Wish list
---------

* pitch modulation in delay plugin
* tests


