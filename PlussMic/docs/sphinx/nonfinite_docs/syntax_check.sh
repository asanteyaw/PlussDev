#!/bin/bash
# Compiles one PlussBI documentation example into an executable. Usage: verify_example.sh SOURCE.cpp OUTPUT
# Set PLUSSDEV_EXTRA_SRCS to a space-separated list of library .cpp files to compile into the program (for checking a source change
# before the library is rebuilt). Build paths come from the PlussDev checkout and the BI build tree; edit BUILD and DEV for another machine.
set -euo pipefail
SRC="$1"
OUT="$2"
BUILD="${PLUSSDEV_BUILD:-/Users/yawasante/software/cxx/PlussDev/build-nn-optim-f64}"
DEV="${PLUSSDEV_ROOT:-/Users/yawasante/software/cxx/PlussDev}"
SDKROOT="${SDKROOT:-$(xcrun --sdk macosx --show-sdk-path)}"
/usr/bin/clang++ \
  -DACCELERATE_NEW_LAPACK=1 -DCODEH_ENABLE_DURATION_SURFACE -DCODEH_ENABLE_INDEX_ACCESSOR_SURFACE \
  -DCODEH_ENABLE_TIMELINE_SURFACE \
  -DPLUSSTILS_VIEW_HAVE_GTK_WEBKIT=0 -DPLUSSTILS_VIEW_HAVE_MAC_WEBKIT=1 \
  -DPLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2=0 -DPLUSSTILS_VIEW_HAVE_X11_SHELL=0 \
  -DPLUSS_HAS_ACCELERATE=1 -DPLUSS_HAS_CUDA=0 -DPLUSS_HAS_MPS=1 \
  -I"$DEV/PlussMic/include" -I"$DEV/PlussMic/src" \
  -I"$DEV/PlussAI/include" -I"$DEV/PlussAI" \
  -I"$DEV/PlussPl/include" -I"$DEV/PlussTils/include" \
  -I"$DEV/PlussMa/include" -I"$DEV/PlussMa/Math/Numerical/optimization" \
  -I"$BUILD/generated" \
  -I"$DEV/PlussQit/include" -I"$DEV/PlussQit/src" \
  -I"$DEV/PlussMic" \
  -I"$DEV/PlussTils/src/PlussBI/../../include" -I"$DEV/PlussTils/src/PlussBI/../../third_party/httplib" \
  -isystem /Users/yawasante/software/miniconda3/envs/cpp_env/include \
  -O0 -std=c++20 -arch arm64 -isysroot "$SDKROOT" -fsyntax-only "$SRC"
