
# Maj7 Width

Maj7 Width is a stereo image manipulator, with all the features a pro engineer expects for controlling the stereo image of a recording.

## Background / Justification

Polished mixes pay careful attention to manage the stereo image at the track, bus, and master level, in order to:

* Correct for aggressive stereo separation of synthesizers or effects like reverb
* Correct for side-effects of stereo recorded source materials like panning, or rotation.
* Similar to an EQ, stereo image can be used to make tracks interact better between each other.
* Narrow low frequencies to avoid out-of-control unmanageable muddy bass.
* Maintain a mono-compatibile mix by keeping tracks in their place in the stereo image.

## Parameters

### Left / Right Source

This determines where the left and right channels come from. Swapping them, you would effectively swap left and right channels. Setting the left source to +1 (full value) would result in a mono mix which is entirely the right channel.

Can be useful for using only the left channel as the mono source when narrowing for example, or swapping left / right.

### Rotation

Performs a 2D rotation around the "origin". This doesn't translate *directly* to aural behaviors, however it does have a distinctive sound and utility in stereo imaging. It can be useful when combining microphone sources, or for special effects.

### Side HPF (highpass filter of the side channel)

During processing, the audio is split into Mid/Side components; this applies a high pass filter to the side channel. The result is a narrowing of low frequencies.

At the moment this filter is fixed at a gentle 6db/oct slope, so you may need to adjust it further than you expect. Use your ears.

### Mid / Side Balance

Recombining the sound from Mid/Side back to stereo, we can adjust the balance of mid & side here.

* When full left (-1), only the Mid (mono) mix is output.
* When 0, mid and side are output equally (normal/default behavior)
* When full right (+1), only the side mix is output. No mono/center content is output

### Pan

Output panning, this is applied after mid-side processing.

### Output gain

Loudness of the outgoing signal. Mostly as a utility, because this effect shouldn't affect the output gain much.

## Todo

Mostly this is complete; the only things I'm currently considering is:

* true multiband operation like most pro imaging products
