# WaveSabre Wiki

## Building

- WaveSabre builds have only been tested with Visual Studio 2013/2015/2017/2019 and version 3.6.6 of the VST3 SDK.
- Due to licensing requirements, WaveSabre requires you to either [download](https://web.archive.org/web/20200502121517/https://www.steinberg.net/sdk_downloads/vstsdk366_27_06_2016_build_61.zip) and copy the VST3 SDK into the build yourself. Download, extract, and copy into the *"Vst3.x"* folder. See [this readme](https://github.com/logicomacorp/WaveSabre/blob/master/Vst3.x/README) or download the VST3 SDK automatically using CMake (see below).

### CMake

WaveSabre has a new [CMake](https://cmake.org/) based build-system that can generate Visual Studio project files for you.

- Install CMake v3.11 or later
  - The latest installer [can be found here](https://cmake.org/download/#latestbinary)
  - Alternatively, you can install CMake from [Chocolatey](https://chocolatey.org/) or other package managers.
- Run `cmake -B build` to generate the Visual Studio project files and solution
  - Optionally, you can also specify `-DVSTDIR=<some-path>` to copy the VST plugins into your DAW's VST plugin directory upon build.
  - Optionally, you can specify `-DDOWNLOAD_VST3SDK=ON` to let CMake download and unpack the VST3 SDK for you.
  - You can specify `-DBUILD_VST_PLUGINS=OFF` to avoid building the plugins (useful for projects that only need the synthesizer and not the plugins).
  - You can also specify `-DVSTSDK3_DIR=<dir>` to use a VSTSDK from outside of the source-tree.
  - You can limit the tools you want to build by using `-DBUILD_WAVESABRE_CONVERT=OFF`, `-DBUILD_WAVESABRE_CONVERT_TESTS=OFF`, `-DBUILD_CONVERT_THE_FUCK=OFF` or `-DBUILD_PROJECT_MANAGER=OFF`.
  - As of Visual Studio 2019, `-A Win32` is required to generate a project with 32-bit targets.
- Open the generated solution from the build directory, and proceed as normal.

## New Device Check List

- Add device to WaveSabreCore/Devices.h
- Add device to WaveSabreConvert/Song.cs
- Add device to WaveSabrePlayer/include/SongRenderer.h
- Add device to WaveSabreStandAlongPlayer/main.cpp
- Re-run cmake to create new Vst project file
- Add Build dependencies for WaveSabreCore and WaveSabrePlayer to VST Project
- Profit!

## UPDATED new device checklist (Maj7 edition)

- create a directory in vsts
- run "switch to x64" which configures & generates projects
- notice: the Maj7Comp project exists with just a .def.
- create WaveSabre\WaveSabreCore\include\WaveSabreCore\Maj7Comp.hpp with boilerplate
  - see boilerplate folder.
- Devices.h: add `#include "Maj7Comp.hpp"`
- WaveSabreConvert/Song.cs: add `Maj7Comp` to the enum
- WaveSabrePlayer/include/SongRenderer.h: add `Maj7Comp` to the enum
- WaveSabreStandAlonePlayer/main.cpp: add `case SongRenderer::DeviceId::Maj7Comp: return new WaveSabreCore::Maj7Comp();`
- create boilerplate files (see boilerplate folder)
    - Maj7CompEditor.h
    - Maj7CompVst.cpp
    - Maj7CompVst.h
- regenerate the projects again just to test.
- add to "C:\root\git\thenfour\WaveSabre\WaveSabreCore\CMakeLists.txt": `include/WaveSabreCore/Maj7Comp.hpp`
- add the following function to WaveSabreVstLib.h: `static inline void GenerateDefaults(Maj7Comp* p) { /* nop unless needed */	}`
- the VST can now be built and used.

### once you have the params defined,

- generate a default preset in a DAW via the VST.
  - using the debug menu, export patch as C++ defaults, and now paste that into the device's default params array.
  