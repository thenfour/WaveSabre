
# Maj7 MBC Multiband compressor - Limiter - Saturator

Maj7 MBC is a high quality and feature-rich multi-band compressor and saturator. All the features pro audio engineers are used to are implemented in a robust and stable way.

Note: because of the design, it can completely replace all compression needs. For size-optimization, if you're going to use a multiband compressor at all, then don't use any other compressors.

Note: because this includes a drive parameter per band, it can also replace Maj Sat, saving a lot of code size.

Note: Some features are disabled for code size.

## Background / Justification

For size reasons, devices should have the right amount of features to avoid redundancy or device overhead. We want to avoid having multiple compressor devices, but typical "production quality" mixing necessitates a few different compressor/dynamics needs:

* Utility compressors for shaping transients, taming tracks. Separate stereo operation, fast response times.
* Bus compressors for gluing. Slower response times, stronger stereo linking, and range from aggressive to transparent.
* Multiband compression, mostly used for mastering.
* Master Limiting, for final transient handling in a transparent way, overall loudness management, standard conformance.

Is a multiband compressor overkill for a size-optimized demoscene prod? Yes absolutely, but there are also good reasons to include it. Saturation/excitation often wants to be band-independent, so there are multiple justifications for these crossovers.

One of the ideas behind the Maj7 suite is that it will output a production-quality prod, and in that case MB compression feels like a necessity. This means the output can be ready for a final release straight out of WaveSabre, without having to go back and rework a "final mix" replacing devices with high quality Fab Filter plugins.

With the right set of features, we can try to cover the most ground.


## Features

  * Single or multi-band compression & saturation
  * Compression with thresh, ratio, knee, attack, release
    * attack is instant, useful for naive brickwall limiting
  * Channel linking
  * Filtering of sidechain signal
  * Soft clipping, to soften the sound of over-limit brickwalling.

## Parameters

This is basically the same as Maj7 Comp, except split into 3 frequency bands. You can control the frequency crossovers. The crossover slope is fixed at 24db/oct.

### Drive

This is a bonus feature which adds almost no code size, and for simple cases will totally replace Maj7 Sat.

### Threshold

The dB value where compression is applied. If threshold is -20dB, then signals detected above -20dB will have the ratio applied and attenuated.

Typically adjust threshold to find the sweet spot where the compressor is both engaging and disengaging. Too low, and the signal will always get squashed. Too high and the compressor will never engage.

### Attack

How fast will the compression take effect, when it crosses the threshold.

The exact millisecond value is approximate. Attack is a curve and putting an exact millisecond value on it is not possible.

Actual attack time is affected by the detection method. If using peak, then an attack of 0ms will indeed apply compression instantly. If using RMS, an attack of 0ms will still take a bit of time to engage.

Attack in general affects the transients of the sound. An instant attack can brickwall transients, killing them. Slower attack lets them through.

Too fast of an attack, and you may lose wanted transients, like the attack of drums. It will make the sound completely squashed or suffocated (that can also be the intention).

Too slow of an attack, and the compressor never quite reaches its target compression, and the result doesn't feel refined or comfortable.

### Release

When the signal drops below the threshold, how fast does the attenuation go away.

Like attack, this is affected by detection method. When using a large RMS window for example, release will be much slower to respond and execute.

Release in general affects the rhythm of the compression effect. In a drum track, you would rarely want release to be longer than a beat; you want the compression to recover fully before being engaged again.

Too slow of a release and the signal never "recovers", always being pushed down into compression land. A form of suffocation like the sound can never resurface. It's important to avoid this because even if it sounds OK, after a short pause of silence, the gain will recover and be unnaturally loud, then compress again and not recover again until another pause. You'll know this is happening if attenuation is constant (see the VU meters).

Release time will play a big role in if the result sounds "pumping".

Too fast of a release and you may hear the rapid gain changes too audibly. Extremely short release can result in waveform distortion due to extremely fast gain changes.

Note: a perfectly instant release is not possible. The fastest practical release is a few milliseconds.

### Ratio

When the threshold is crossed, this is how much to attenuate the signal. Higher values = more attenuation. A ratio of 1 will apply no compression at all. Refer to the transfer curve graphic; it's true to the actual compressor functionality.

Tip: If you want more compression, ask yourself if it's best to adjust ratio or threshold. Threshold let's more audio be affected, ratio affects that audio. When I adjust threshold & ratio, I start with an extremely high ratio, so I can clearly hear what's being compressed and what's not. Adjust the threshold to find the loudness range i want to compress, THEN adjust ratio to find how extreme I want the effect.

### Knee

This extends the threshold down, and soften the curve. Again refer to the transfer curve graphic to see exactly what's going on. The idea is to form a smoother transition from "non-compressed signal" to "compressed signal". It can help make the compression more transparent, by blurring the distinction between compressed & uncompressed signal.

0dB means no knee; the signal is not compressed until exactly the threshold, then full ratio kicks in.

Too "hard" of a knee (like 0dB) and the compression may not be as transparent as you want. Maybe some transients don't quite hit the threshold and sound unnaturally compressed or uncompressed. Blur the line by softening the knee.

Dialing in the correct knee is difficult because the result can be subtle. Some refer to the character of knee as "naturalness". Try listening for transients and how it engages the compressor, and how much compression character is apparent. Try listening to the diff signal to hear which parts of the signal are affected by changing knee size.

### Stereo linking

When 0, it means left and right stereo channels are processed independently. When 100%, it means the compressor reacts to the mono signal, and both channels have equal attenuation applied. Usually something in between is desired.

Too wide (e.g. 0%), and you may hear unnatural distracting compression happening in only 1 channel at a time. Refer to the history graph window to see the attenuation between left & right signals; when they are very different, the effect can be undesirable.


### Peak-RMS (disabled in some variations)

When this is set to 0ms, peak detection is used. This allows for instant attack, where not even 1 sample will escape compression. Peak is actually almost always usable (RMS is rarely *needed*).

But when you need RMS, bring this value up to set the RMS window size. Like attack & release parameters, this is not *exactly* a millisecond duration but an approximation.

Increasing RMS window basically tells the compressor to look at the bigger picture and not get caught up on little transients. The result is that attack & release will be lazier, but on some material it may feel more accurate.

When do you need RMS? RMS window can smooth out transients before compression. If compression feels erratic or inconsistent on material that has a lot of transients (in an extreme example, crackly record sound), increasing RMS window is a way to tell the compressor to act on the overall loudness rather than those transients.

### Highpass & Lowpass Frequency / Q

 (Lowpass is disabled in some variations)

The input detection can have lowpass & highpass filters applied, in order to avoid responding to certain frequencies. Most common is when a kick drum is dominating the response of the compressor on a drum bus. In this case, adjust the highpass filter to reduce the impact of low frequencies on the compression.

Refer to the frequency response graph; it's calculated based on the actual filters in the effect, not contrived.

To hear the filtered detection signal, listen to the "Sidechain" signal.

Note: when frequencies are out of range, the filters are fully bypassed.

### Drive (saturation)

When 0dB or below, this disables drive.

This adds odd harmonic saturation. It's effectively the "clean tape" setting you'd find ind Fab Filter Saturn.

### Makeup gain

This is disabled in some builds; the idea is that if you don't have parallel processing then there's no reason for this extra gain stage. Just adjust the output gain.

This is the amount of gain to apply after compression to bring the signal to the same level as the input signal. This is not quite the same as output gain, because of where it's applied during processing.

If you're not using parallel compression, then this is really the same as output gain. But in a parallel processing setting, setting proper makeup gain allows an even dry-wet parallel mix, where dry & wet are both equal loudness.

### Dry-wet mix (Parallel processing)

Dialing in the dry signal (by reducing the dry-wet mix parameter) allows parallel compression. One way to think of it is the transfer curve takes on a new shape.

Parallel compression is used for many musical functions, from making the compression more natural or cohesive to blending in extreme distorting compression on the original signal.

Note: parallel processing can result in a behavior that resembles some other compressors like the original Smasher, Waves C1, or MCompressor, which don't offer parallel processing but that's how their curve looks.

### Input gain

Adjusts the loudness of the incoming signal. Don't use this as a substitute for threshold! It's just a utility, unlikely to be used much. Maybe if the incoming signal is in a loudness range that makes it hard to dial in proper compression, and then you'd adjust the output gain to compensate?

### Output gain

Similarly, this adjusts the loudness of the outgoing signal. Again mostly as a utility.

This is NOT makeup gain. For gain-staging please use makeup gain, which is intended for loudness matching the original signal.

### Output signal

Here you can select the signal that's actually output by the device. I find "diff" very useful sometime as another way to hear the character of the compression being applied. "Sidechain" allows hearing the incoming signal with filtering applied.

### Soft clipping enable

This enables the soft clipping processing stage, which is designed as a mastering tool, but could be useful for brickwall limiting of any kind.

If this is disabled, clipping is not performed, and over-unity output is possible without clipping.

When this is enabled, output will never exceed unity, which makes this useful as a last processing stage in a master bus.

The VU meter to the right shows the amount of clipping that's been attenuated. If a signal is not over-unity, this meter will be empty. The meter only registers something if the input signal was clipping. This is useful for mastering.

Note: Even if the signal is not clipping, shaping is occurring above the selected threshold.

Note: Compressor output gain is applied before soft clipping. It means compressor output gain is effectively an input gain to the soft clipper, for dialing in final loudness.

### Soft clip threshold

Like Maj7 Sat, this threshold determines the amplitude at which a signal has shaping applied. Below the threshold, the signal will be linear and you won't hear the saturation that is natural with soft clipping.

Above the threshold will have soft clipping applied. For individual tracks this is not likely to be a big deal; `-inf` would be common. But for mastering, it can shape the signal a bit too much and feel heavy-handed, even if clipping is not occurring.

As the saturation range shrinks (threshold approaching `0dB`), the clipping will take more and more the shape of hard clipping, which is quite ugly, but reduces the influence of the wave shaper.

If this is too low, you might feel that the signal is becoming too saturated, too thick. But if it's too close to unity (`0dB`), then clipping results in quite ugly hard clipping which sounds like non-musical tearing or glitching.

So for a master bus soft clipper, this threshold control is actually very important to the final dynamic character.

For a master bus, around -9 to -6dB seems to be a sweet spot. You want something that engages in loud parts, but mostly stays out of the way during non-critical passages.

### Soft clip output (maximum output level)

After soft clipping, the output range is attenuated using this parameter. Output level will never exceed this.

Note: this is of course in the digital domain, with no oversampling. True peak is not even close to being in scope of this project.

## Known issues

* annoyance: the dB scale of the VU Meters is different than the scale of the history view & transfer curve. it's a bit confusing when you look at the legends of one and thinking it applies to the other.

## GUI

### History graph

This shows the ongoing signal levels. You can select the checkboxes to determine what gets shown.

* Input (gray): The incoming signal, with input gain applied (effectively the dry signal)
* Detector (yellow) The signal that the compressor is using to calculate attenuation
* Attenuation (green): The amount of attenuation applied to the signal
* Output (blue): The wet signal
* Left: Show left signals. All these signals have left and right versions (left is bright, right is darker)
* Right: similar to left.

Note: The vertical scale matches the transfer curve graph, but NOT the VU Meters.

### Transfer curve

This shows the mapping across loudness levels. A gray line shows the threshold level, and a 1:1 diagonal mapping.

The live signal is also tracked here: when no compression is applied, the indicator is gray. When compression is applied, it turns red.

Note: The vertical scale matches the history graph, but NOT the VU Meters.

### VU Meters

They're not perfect, but they do the job.

* Input Left
* Input Right
* Attenuation left amount, in red
* Attenuation right amount, in red
* Output Left
* Output Right


## Processing flow

  * input gain
  * band splitting
  * for each band,
    * input gain
    * channel linking
    * highpass filter, if enabled
    * compression stage
    * output gain
  * output gain
  * soft clipping
    * waveshaping
    * output gain



## Usage as a saturator / exciter

MBC contains all the parts of both a compressor and saturator. The multi-band processing and process flow make it very cheap to combine the two operations.

The main saturation control is the "Drive" knob buried among the compressor controls.

So the workflow for using this as a saturator are:

1. enable compressor band (either single band or multi-band)
2. set ratio to 1 which forces compression to not attenuate
3. adjust drive as necessary; this is your saturation control.

Saturation can be applied to the 3 bands independently. Note that adding saturation will introduce frequencies that are outside of that band's crossover boundary.

## Usage as a limiter

Several features are combined to allow MBC to act as a limiter: 

* Instant attack times
* High ratios
* Soft clipping

To use MBC as a limiter,

- Be in single band mode. While you can do multiband compression in a limiter workflow, it probably doesn't make much sense. Do your MB processing, THEN do a single-band limiting stage.
- Ratio set to maximum (50:1)
- Attack to 0, to "catch" transients
- Other compressor params:
  - Gentle threshold and knee to maximize transparency, but wide enough that transients are hidden through treatment.
  - Probably don't add drive; that kind of treatment should come earlier in the chain.
  - Channel link probably leave default for transparency. Drop to 0 if you want to catch transients perfectly without the other channel interrupting.
- Enable soft clipping, to guarantee samples don't exceed unity.
  - Threshold & output defaults are probably fine; they're intended for limiting.




## Why no Mid-Side processing?

Mid-Side processing was part of the original design but removed for a number of practical reasons.

You cannot just process M/S signals in the way we do stereo, because the loudness is not balanced between channels as they are in stereo. Thus, threshold is meaningless in M/S mode, unless you do further treatment of the scenario:

1. Offer additional parameters to balance the sound (like a "compensation panning"). This is not satisfactory, because actually M/S are likely to want different parameters altogether (like ratio).
2. Offer yet more params: separate thresh/ratio/knee for M and S channels. Ozone's Vintage Compressor does this. But this is just too much for a size-optimized device.
3. Selectable mid-only or side-only processing. Could be done.

In the end, M/S compression mode is never a deal-breaking requirement so let's leave it out.

Note that many high quality compressors (Kotelnikov for example) don't offer M/S at all.
