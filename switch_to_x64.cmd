pushd %~dp0


@echo Make sure visual studio is closed, and press a key
@echo ** NOTE: your vst plugins dir should be:
@echo C:\root\vstpluginlinks\WaveSabre
@pause


del build\CMakeCache.txt > nul

cmake -S . -DVSTDIR=C:\root\vstpluginlinks\WaveSabre -B build

if %ERRORLEVEL% NEQ 0 (
    @echo CMake configuration failed
    @pause
    exit /b 1
)

@echo hit a key to open visual studio...

@pause

start build\WaveSabre.sln

