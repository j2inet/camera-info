# camera-info

Visual Studio C++ console app that uses Media Foundation to enumerate connected cameras,
list output formats and resolutions, and print details to `std::wcout` with ANSI color codes.

# Signed Binaries

Signed binaries for this application are included in this repository. You will find them in the 
[bin](bin) folder. Within there are two subfolders for an [ARM64 version](bin/release-arm64) version 
of the application and an [Intel Architecture x64](bin/release-x64) version of the build. I have not
tested the ARM version. I am making it available since it appears that Windows on ARM may be more common 
in the common years and the effort to produce an ARM binary now is next to nothing. 

From [J2i.net, LLC](https://j2i.net), 2026
