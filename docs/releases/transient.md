# PlussII 0.0.5 — Transient

This release refreshes the public C++ distribution, the documentation source, and the interactive table viewer. All downloadable archives are built and validated on their matching operating system before publication.

## Highlights

- Expanded the DataFrame and Atlas documentation, including corrected C++ signature order and updated API and guide examples.
- Improved workbook support: `save_excel` now accepts a complete workbook, with multiple named sheets written to one `.xlsx` file.
- Refined catalog, archive, and Atlas depth operations and their examples. The public `aggregate_over_catalogs` and `aggregate_over_frames` names become `aggregate_depth`.
- Refreshed the PlussView table themes. **Default** is the cobalt and coral palette; Ember, Nardo, Brooklands, Gulf, Obsidian, and Indigo remain selectable. Native is reserved for embedded rendering.
- Added automated Steady and Transient distribution channels. Steady is a versioned release; Transient is the latest validated development build.
- Corrected cross-platform `nextafter` linkage so Linux and Windows consumer projects do not reference the macOS-only MPS kernel.
- Restored DataFrame plotting navigation and named accessor references by the methods users call.

## Compatibility

The C++ distribution requires CMake 3.28 or newer and a C++20 compiler. Select the ZIP for your operating system and architecture. The Windows archive targets Visual Studio 2022 with ClangCL and the Release configuration; the Linux archives are built on Ubuntu 24.04. See the download page for dependency and consumer-project commands.

The `aggregate_depth` rename is a source compatibility change for callers of the former depth aggregation methods. Update those call sites when moving from 0.0.4.
