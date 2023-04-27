# Maj7

A polyphonic megasynthesizer-sampler specifically designed for 32kb executable music. It is aimed at combining the functionality of Slaughter, Specimen, Adultery, and Falcon, and adding a rich modulation system plus a lot more.

<img src="https://user-images.githubusercontent.com/3672094/234365003-46f482f6-8bc1-4b6b-af6e-54e6a9bd1aea.png" height="600">

## Quick intro

The idea behind this WaveSabre device is that by combining device features into 1 synth, we can save code size by reducing duplicated overhead of multiple synthesizers. And with that space savings, we can spend it on fancy things like bandlimiting oscillators, fancy-pants envelopes and modulation system.

### Regarding workflow and code size ###

Before you begin writing your track: Maj7 contains all the features of Slaughter, Specimen, Adultery, and Falcon, but much much much more. So if your project uses Maj7, then ALL tracks should begin with Maj7, and no other synth devices should be used. Those other synth devices are not obsolete though exactly. Maj7 is like 9kb of compressed code size while the others combine to around 5kb. So if you can't afford a few kb (which for a 64kb may well be the case), you may not be able to commit to using Maj7.

In other words, before starting to write your track, decide already if you're going to risk the code size of Maj7.

### What is the payoff to using Maj7?

You will have the freedom to create using a workflow much like any other conventional music, without thinking too much about the underlying tech. You will have a huge amount of features to play with, all in one device.

The older WaveSabre devices are very small, and therefore carry limitations. They contain very basic modulations and few core features. Maj7, on the other hand, is designed to be a fully-featured high-fidelity synthesizer as powerful as Massive, FM8, Serum, combined.

Some example scenarios where Maj7 makes life easier:

* Drumkit building, where each key corresponds to a different sample or oscillator.
* When experimenting and building sounds, there are just so many features; maybe the biggest is how powerful modulations are
* When building a patch library, Macro knobs (think Massive) make reusing patches more manageable and clear
* It's just convenient and comfortable. The GUI has a lot of visual feedback, and the overall workflow is forgiving, intuitive, and familiar to other widely-used synths.

## Tour of features

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
* So this is a LOT of features, and therefore it's a lot of code. Something like 9kb of code, compressed. On the other hand it replaces at least 4 existing WaveSabre devices.

## techniques / tutorials

* velocity
* Drumkits
* Unisono modulation

## Detailed documentation of features

* interface
** keyboard, mouse
** knob indicators
** typing values
** menus
** sections
* quality setting
* signal path
** oscillator (phase mod, hard sync)
** pre-fm amplifier
** filter
* modulations
** curves
** source ranges
** sources & destinations
** destination range
** advanced view
** auxilliary signal
** built-in modulations
** compound modulations
** voice-level vs global


## Technical info

* WaveSabre chunk optimization
* ProjectManager new features

## FAQ





