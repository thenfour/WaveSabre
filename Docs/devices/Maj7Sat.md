
# Maj7 Sat - Multiband Waveshaping Saturation and Distortion

Maj7 Sat is a saturation / distortion device. Replaces WaveSabre's "Scissor", this has finer control over the sound and response, more akin to FabFilter Saturn, or MSaturate, or Ozone Exciter.

The saturation effect is effectively a waveshaping transform, for the purpose of introducing higher harmonics.

Various types of clipping / soft clipping, folding, and even harmonics are supported.

## Background / Justification

Saturation is a subtle effect but is important in making digital music sound warmer and more lively, reacting in nonlinear ways to the source. Scissor mostly suffered from being too much in the middle-ground of quality and features. I would rather have a dead-simple saturation plugin with few intuitive controls, or go full-blown to give the control that saturation deserves.

Whether it's justified to add so much code is hard to say at the moment.

## Parameters

### Crossover frequency 1 (Hz) / Crossover frequency 2 (Hz)

Maj7 Sat operates in 3 independent frequency bands, allowing you to apply the effect to different frequency ranges independently. Common example scenarios:

1. when applying saturation to the whole signal feels imbalanced like low frequencies are hogging too much drive. Reduce or disable saturation in low frequencies
2. Different frequency ranges react to different saturation models differently; using the different bands lets you more tightly hone in the correct settings for the right place.

Select the frequency ranges by setting the 2 crossover frequencies.

### Crossover slope

Frequency bands are not perfectly cutoff. Cutoff lets you control how much "bleed" there is between frequency bands. The steeper the slope, the more separate the bands will sound.

Too gentle, and you won't be able to separate the bands to the degree you need. Too extreme and the separation may be unnaturally abrupt and the effect's separation will be too noticeable.


### Input gain

I guess you can use this as a global "drive" parameter, but it's not designed for that; this is a utility for correcting incoming signals. Usually leave it default.

### Dry/Wet Master

This can be used to affect the wet/dry (parallel processing) mix of the whole effect. This parameter is applied by all bands, along with their individual dry/wet mix parameter.

### Output gain

Similar to input gain, this is mostly a utility unrelated to the main features of this device. Mostly leave it at default and gain stage using other parameters.

### Per Band: Enable

Enables / disables applying the saturation effect to this band. All saturation parameters will therefore be bypassed. Output gain for this band is still applied.

### Per Band: Mute / Solo

For configuring the crossovers, or focusing on 1 band, you can use these like typical mute/solo controls.

### Per Band: Threshold

This applies the saturation effect only above the specified threshold. It can help limit the application of saturation to a certain peaking amplitude.

Use this to limit saturation to transients, or to apply saturation but in a cleaner way. You can even think of this as a sort of instant gate for saturation.

So a threshold of `-inf` will apply the saturation curve along the whole input range. All input samples will be "waveshaped" and non-linearities applied. This can be fine for heavy distortion, or a very subtle saturation that doesn't get in the way.

A threshold of near `0dB` would reduce the "area" that nonlinearities are applied. Below this level, there would be no effect applied.

Finding the right threshold can help restrict the saturation in a way that sounds clean and transparent, "biting" the right amount.

### Per Band: Drive

Pre-gain before waveshaping is applied, for the purpose of controlling the intensity of saturation.

### Per Band: Model

The waveshaping transfer curve to be applied. They are in order from gentle to extreme.

Clipping algorithms are first in the list, where all clipped signals will be clamped.

Folding algorithms are last, where the wave folds back on itself in various ways over unity.

These all introduce odd harmonics.

Admittedly, some of these are pretty subtle and might be removed for the sake of code size. The most distinct are:

* Thru: Applies no waveshaping; no harmonics are added at this stage.
* Tanh: Warm analog-style saturation that's hard to make sound bad. Careful not to allow it to muddy your sound.
* Divclip: A more aggressive saturation that still sounds very musical and dynamic.
* Hard clip: Ugly noisy peppery digital hard clipping. It basically sounds like a mistake.
* Folding in general sounds very aggressive and should be used for extreme effects
* Linear (triangle) fold is the most naive folding algorithm, and basically sounds broken.


### Per Band: Analog

Mixes in even harmonics. Because the waveshaping models add odd harmonics, this is a way to instead add even harmonics.

This is applied even when the saturation model is `thru`.

Applying even harmonics can easily go from warm and subtle, to noisy and glitchy. I would not use this as a distortion effect but rather as a way to fatten up the sound in the subtle way.

### Per Band: PanMode

How the left & right channels are processed: in stereo (L/R), or mid/side.

### Per Band: EffectPan

In stereo, this can apply saturation at different levels in the left or right channels (rare).

In mid/side, this will balance saturation between the center channel (mono channel) or side channel (the stereo image). I find it almost always more natural to process mid/side, however because mid/side may have different levels, it can be tricky to dial in the right balance of settings.

### Per Band: Makeup Gain

This is a gain control used to compensate for the drive, analog, and model effect, which all tend to increase the loudness of the signal in a way that can't be automatically corrected. This knob allows you to compensate for the loudness increase.

Note: This is not the same as the band's output gain, even though it's very close. Makeup gain is applied before wet/dry, so the idea is to use makeup gain to match the loudness of the dry signal.

### Per Band: Dry/Wet

Mixes the saturated signal into the unsaturated (dry) signal.

For a good parameter sweep, make sure makeup gain has been set to match the level of the saturated signal to the loudness of the dry signal.

### Per Band: Output gain

Final output gain applied for this frequency band. Usually leave this at 0 and instead use makeup gain.



## Justifications

### Why no oversampling?

The new algorithm requires more processing and more state per band per sample. Oversampling would be significantly heavier than Scissor. Saturation is already a rather subtle effect; adding oversampling is something only worth doing if we really run into a problem.

### Why no "diff" signal?

It's always tempting to hear a "diff" to hear the effect that's applied to the incoming signal, without the original signal. To hear the character or strength of the effect. So why isn't this a feature?

Because of the various internal gain staging, it's tricky to find an actual "diff" signal that would be useful. A diff signal tends to just sound like bits of noise that doesn't help make decisions in the way it does for a compressor.

It also would add code size, and an extra parameter. For a fringe feature that's not useful, not worth.
