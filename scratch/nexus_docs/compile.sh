#!/bin/bash
# Compiles+links a single .cpp against the build-classic-docs tree, for
# verifying Nexus (and other) doc examples with real output.
# Usage: scratch/nexus_docs/compile.sh <source.cpp> <output_binary>
set -euo pipefail

REPO=/Users/yawasante/software/cxx/PlussDev
SRC="$1"
OUT="$2"

LIBS=(
  "$REPO/build-classic-docs/PlussMic/libPlussMic.a"
  "$REPO/build-classic-docs/libpluss.a"
  "$REPO/build-classic-docs/PlussPl/libPlussPl.a"
  "$REPO/build-classic-docs/PlussTils/libPlussTilsCore.a"
  "$REPO/build-classic-docs/PlussQit/libplussqit_coden.a"
  "$REPO/build-classic-docs/PlussQit/libplussqit_codem.a"
  "$REPO/build-classic-docs/PlussQit/libplussqit_codeh.a"
  "$REPO/build-classic-docs/PlussTils/view/libPlussTilsView.a"
)

/usr/bin/clang++ \
  -DACCELERATE_NEW_LAPACK=1 -DCODEH_ENABLE_DURATION_SURFACE -DCODEH_ENABLE_TIMEINDEX_SURFACE \
  -DCODEH_ENABLE_TIMELINE_SURFACE -DFMT_HEADER_ONLY=1 -DPLUSSMIC_HAS_CURL=1 -DPLUSSMIC_HAS_LIBZIP=0 \
  -DPLUSSMIC_HAS_PARQUET_CPP=0 -DPLUSSQIT_HAS_FMT=1 -DPLUSSTILS_VIEW_HAVE_GTK_WEBKIT=0 \
  -DPLUSSTILS_VIEW_HAVE_MAC_WEBKIT=1 -DPLUSSTILS_VIEW_HAVE_WIN_WEBVIEW2=0 -DPLUSSTILS_VIEW_HAVE_X11_SHELL=0 \
  -DPLUSS_HAS_ACCELERATE=1 -DPLUSS_HAS_CUDA=0 -DPLUSS_HAS_MPS=1 \
  -I"$REPO/PlussMic" -I"$REPO/PlussMic/src" -I"$REPO/PlussMic/include" -I"$REPO/PlussPl/include" \
  -I"$REPO/include" -I"$REPO/PlussQit/include" -I"$REPO/PlussQit/src" -I"$REPO/ExLibs/fmt/include" \
  -I"$REPO/PlussTils/include" -I"$REPO/PlussAI/include" -I"$REPO/PlussMa/include" \
  -I"$REPO/build-classic-docs/generated" \
  -O0 -g -std=c++20 -arch arm64 \
  -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX27.0.sdk \
  -mmacosx-version-min=26.6 \
  "$SRC" \
  "${LIBS[@]}" "${LIBS[@]}" \
  -lcurl -lsqlite3 -framework Accelerate \
  -o "$OUT"

echo "built: $OUT"
