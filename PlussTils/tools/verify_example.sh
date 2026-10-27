#!/bin/bash
# Compiles one PlussBI documentation example into an executable. Usage: verify_example.sh SOURCE.cpp OUTPUT
# Set PLUSSDEV_EXTRA_SRCS to a space-separated list of library .cpp files to compile into the program (for checking a source change
# before the library is rebuilt). Build paths come from the PlussDev checkout and the BI build tree; edit BUILD and DEV for another machine.
set -euo pipefail
SRC="$1"
OUT="$2"
BUILD="${PLUSSDEV_BUILD:-/Users/yawasante/software/cxx/PlussDev/build-nn-optim-f64}"
DEV="${PLUSSDEV_ROOT:-/Users/yawasante/software/cxx/PlussDev}"
/usr/bin/clang++ \
  -DACCELERATE_NEW_LAPACK=1 -DCODEH_ENABLE_DURATION_SURFACE -DCODEH_ENABLE_INDEX_ACCESSOR_SURFACE \
  -DCODEH_ENABLE_TIMELINE_SURFACE -DFMT_HEADER_ONLY=1 -DPLUSSQIT_HAS_FMT=1 \
  -DPLUSSTILS_VIEW_HAVE_GTK_WEBKIT=0 -DPLUSSTILS_VIEW_HAVE_MAC_WEBKIT=1 \
  -DPLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2=0 -DPLUSSTILS_VIEW_HAVE_X11_SHELL=0 \
  -DPLUSS_HAS_ACCELERATE=1 -DPLUSS_HAS_CUDA=0 -DPLUSS_HAS_MPS=1 \
  -I"$DEV/PlussMic/include" -I"$DEV/PlussMic/src" \
  -I"$DEV/PlussAI/include" -I"$DEV/PlussAI" \
  -I"$DEV/PlussPl/include" -I"$DEV/PlussTils/include" \
  -I"$DEV/PlussMa/include" -I"$DEV/PlussMa/Math/Numerical/optimization" \
  -I"$BUILD/generated" \
  -I"$DEV/PlussQit/include" -I"$DEV/PlussQit/src" \
  -I"$DEV/PlussQit/../ExLibs/fmt/include" \
  -I"$DEV/PlussMic" \
  -I"$DEV/PlussTils/src/PlussBI/../../include" -I"$DEV/PlussTils/src/PlussBI/../../third_party/httplib" \
  -isystem /Users/yawasante/software/miniconda3/envs/cpp_env/include \
  -O1 -std=c++20 -arch arm64 \
  -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX27.0.sdk \
  -mmacosx-version-min=26.6 \
  -o "$OUT" "$SRC" ${PLUSSDEV_EXTRA_SRCS:-} \
  -Wl,-search_paths_first -Wl,-headerpad_max_install_names \
  -Wl,-rpath,/Users/yawasante/software/miniconda3/envs/cpp_env/lib \
  "$BUILD/libpluss.a" "$BUILD/PlussMic/libPlussMic.a" \
  /Users/yawasante/software/miniconda3/envs/cpp_env/lib/libparquet.1200.0.0.dylib \
  /Users/yawasante/software/miniconda3/envs/cpp_env/lib/libarrow.1200.0.0.dylib \
  /Users/yawasante/software/miniconda3/envs/cpp_env/lib/libcurl.dylib \
  "$BUILD/PlussPl/libPlussPl.a" "$BUILD/libpluss.a" \
  "$BUILD/PlussQit/libplussqit_codeh.a" "$BUILD/PlussQit/libplussqit_codem.a" \
  -framework Accelerate -framework MetalPerformanceShaders -framework Foundation -framework MetalPerformanceShadersGraph \
  "$BUILD/PlussQit/libplussqit_coden.a" /opt/homebrew/lib/libunistring.dylib \
  "$BUILD/PlussTils/libPlussTilsCore.a" "$BUILD/PlussTils/view/libPlussTilsView.a" \
  /Users/yawasante/software/miniconda3/envs/cpp_env/lib/libzip.dylib \
  -framework Cocoa -framework UniformTypeIdentifiers -framework WebKit -framework QuartzCore \
  -framework CoreText -framework CoreFoundation -framework CoreGraphics -framework CoreVideo -framework Metal \
  "$BUILD/PlussTils/src/PlussBI/libPlussBI.a" \
  /Users/yawasante/software/miniconda3/envs/cpp_env/lib/libsqlite3.dylib
