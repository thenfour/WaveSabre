pushd %~dp0


@echo Make sure visual studio is closed, and press a key

@pause


del build\CMakeCache.txt

cmake -DVSTDIR=C:\root\vstpluginlinks\WaveSabre -B build -A Win32

@echo hit a key to open visual studio...

@pause

start build\WaveSabre.sln

