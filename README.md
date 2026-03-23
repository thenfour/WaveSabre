## Intro

Maj7 is a toolchain for writing size-optimized "executable music".

Maj7 is a fork of [WaveSabre](https://github.com/logicomacorp/WaveSabre/tree/master). It is a software synthesizer and effect library, with toolchain for size-optimized executable music.

### Why use Maj7 over WaveSabre?

Maj7 has a slightly different intended use than WaveSabre. While WS is specifically for 64kb demos, Maj7 is intended to write 32kb Executable music. Here we care more about audio quality and musical expression in a standalone executable, with no intention of integrating into a 64kb demo. Therefore Maj7 fills more space than WS, but is intended to be just the right size for high quality music.

With Maj7 you can expect some advantages over WS:

- Higher quality audio effects. Everything from the anti-aliased oscillators, phase-coherent crossovers, meticulously crafted compression algorithms, have been improved from the WaveSabre stock devices.
- More conventional workflow. Param curves and ranges, modulation options, visual feedback, have all been tweaked in the favor of usability familiar to musicians working with NI Massive, FabFilter effects, etc.

And disadvantages:

- Consumes more space (though still comfortably fits in a 32kb EXE music entry)

## Usage

In general, the workflow is like this:

1. Write a song in your DAW of choice using only the `Maj7` VST plugins.
2. Use `Project Manager` to convert to a `.hpp` file
3. Drop in the `hpp` and build the executable music project.


## Development

Please see [our docs](./Docs/Home.md#building) for prerequisites, building, etc

## Questions?

It's quite robust and usable, however because it's a 1-man show, there's no announcements of fixes / breaking changes or anything. Feel free to contact [tenfour](https://twitter.com/tenfour2) for questions / whatever:

  * [Discord](https://discord.gg/kkf9gQfKAd)
  * [Twitter](https://twitter.com/tenfour2)

## Size optimization

* SizeBench is best for profiling. build min-size-rel-with-debug-info
* see `WaveSabreCoreFeatures.hpp` to disable features in the code. it's not always clear from the VST editors which things are disabled.
* use directives in track names to elide data (like `#fixedvelocity`, `#oneshot`, `#fixednote`) -- (oneshot is possibly not currently supported)

## Directives in track names

* `#fixedvelocity` indicates that the track's midi data doesn't require including velocity info. useful for sfx, arps...
  often it's optimal to not use velocity (that gets serialized with every note), and instead use a parameter automation lane for things like sfx.
* `#oneshot` indicates that the track not need to serialize note off information. it's commented out for some reason.
* `#fixednote` things like snare drums, kick drums, may not need note value info.

### notes:

- `enum class ... : uint8_t` does not usually save space. code is spent on widening/narrowing args; storage padding is not that critical

### msvcrt / build-msvcrt

- the `msvcrt` project is a tiny shim that compensates for the fact that we are `NODEFAULTLIB`.
- the `build-msvcrt` project generates a `.lib` from a `.def` so we can minimally link against the OS msvcrt.dll
