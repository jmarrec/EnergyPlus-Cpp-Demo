# Example C++ to drive EnergyPlus

## A FTXUI-based CLI that uses the EnergyPlus C/C++ API layer to run a file, with SQL Queries built-in

This was a proof of concept  / demonstration that started at the end of 2022 when I added an export of a CMake Configuration target ot EnergyPlus so one could link to EnergyPlus more easily (see [NREL/EnergyPlus#9712](https://github.com/NatLabRockies/EnergyPlus/pull/9712)) and wanted to try out [FTXUI](https://github.com/ArthurSonzogni/FTXUI))

[![C++ CI](https://github.com/jmarrec/EnergyPlus-Cpp-Demo/actions/workflows/build.yml/badge.svg)](https://github.com/jmarrec/EnergyPlus-Cpp-Demo/actions/workflows/build.yml)
[![clang-format](https://github.com/jmarrec/EnergyPlus-Cpp-Demo/actions/workflows/clang-format-check.yml/badge.svg)](https://github.com/jmarrec/EnergyPlus-Cpp-Demo/actions/workflows/clang-format-check.yml)
[![cppcheck](https://github.com/jmarrec/EnergyPlus-Cpp-Demo/actions/workflows/cppcheck.yml/badge.svg)](https://github.com/jmarrec/EnergyPlus-Cpp-Demo/actions/workflows/cppcheck.yml)

![demo GIF](EnergyPlus-Cpp-Demo.gif)

## Usage

This started on one of my branches that added exporting a cmake configuration target so one could link to EnergyPlus more easily (https://github.com/NatLabRockies/EnergyPlus/pull/9712) but now E+ has it out of the box since EnergyPlus v23.1.0, so you can build and link against the regular installer!

```shell
epcli -w /path/to/weather.epw -d out 1ZoneUncontrolled.idf
```

Then follow the UI.

## Building

Make sure you have conan installed: `pip install conan` (tested with >= 2, <= 3)

```shell
conan install . --output-folder=build --build=missing -c tools.cmake.cmaketoolchain:generator=Ninja -s compiler.cppstd=20 -s build_type=Release
cmake --preset conan-release -Denergyplus_DIR:PATH=/Applications/EnergyPlus-25-2-0/lib/cmake/energyplus
cmake --build --preset conan-release
./build/Products/epcli -w /path/to/weather.epw -d out 1ZoneUncontrolled.idf
```

You can copy 1ZoneUncontrolled.idf from the ExampleFiles, and add `Output:SQLite, SimpleAndTabular;` so you have access to the SQL reports
