# NOTICE

PlussII is open source software distributed under the BSD 3-Clause License.
See `LICENSE` for the license terms.

Copyright (c) 2026 asanteyaw.

## Third-party components in the release archives

The release archives link the following libraries statically so that Excel and
Parquet loading and saving work without installing anything. Each keeps its own
license, and you must comply with it when you redistribute the archives.

| Library | Used for | License |
| --- | --- | --- |
| Apache Arrow and Apache Parquet (C++) | Parquet reading and writing | Apache License 2.0 |
| Apache Thrift | Parquet metadata encoding (Arrow dependency) | Apache License 2.0 |
| libzip | Excel (.xlsx) reading and writing | BSD 3-Clause |
| zlib | Compression (libzip and Arrow dependency) | zlib License |
| zstd, lz4, snappy, brotli | Parquet column compression (Arrow dependencies) | BSD 3-Clause or MIT |
| utf8proc, re2, xsimd | String handling in Arrow | MIT, BSD 3-Clause |
| SQLite | SQL on tables | Public domain |
| curl | Loading data from http and https URLs | curl License (MIT-style) |

These libraries are built from source with vcpkg for each platform, so their
transitive dependencies carry the licenses of the pinned vcpkg ports. On macOS
and Linux, SQLite and curl come from the operating system and are not included.
The full license text of every bundled library is in the vcpkg port that
supplied it. LibTorch is never included; see `LICENSE_PYTORCH` for the notice
that accompanies the Torch bridge.
