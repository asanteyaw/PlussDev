#!/bin/bash
# Compile + link just test_series_contraction.cpp against an existing libplussqit_codem.a build tree.
# Usage: build_contraction_test.sh BUILD_DIR [TEST_FILE]   (e.g. build-contraction)
set -euo pipefail
B="$(cd "$1" && pwd)"; R="$(cd "$(dirname "$0")/../.." && pwd)"
CE=/Users/yawasante/software/miniconda3/envs/cpp_env
SDK=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX27.0.sdk
/usr/bin/clang++ -DACCELERATE_NEW_LAPACK=1 -DFMT_HEADER_ONLY=1 -DPLUSSQIT_HAS_FMT=1 -DPLUSS_HAS_ACCELERATE=1 \
  -DPLUSS_HAS_CUDA=0 -DPLUSS_HAS_MPS=1 -I"$B/generated" -I"$R/src" -I"$R/include" -I"$R/../PlussTils/include" \
  -I"$R/../ExLibs/fmt/include" -isystem $CE/include -O1 -std=c++20 -arch arm64 -isysroot $SDK -mmacosx-version-min=26.6 \
  -o "$B/${2:-test_series_contraction.cpp}.bin" "$R/tests/CodeM/${2:-test_series_contraction.cpp}" \
  -Wl,-rpath,$CE/lib "$B/libplussqit_codem.a" $CE/lib/libCatch2Main.3.13.0.dylib $CE/lib/libCatch2.3.13.0.dylib \
  -framework Accelerate -framework Metal -framework MetalPerformanceShaders -framework Foundation \
  -framework MetalPerformanceShadersGraph /opt/homebrew/lib/libunistring.dylib
