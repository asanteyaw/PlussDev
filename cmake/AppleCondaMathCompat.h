#pragma once

// Conda clang's libc++ can be paired with a newer Xcode SDK whose math.h
// expects Clang's resource float.h to provide these macros.  In that mixed
// toolchain the delegation can miss, leaving libc++ headers uncompilable.
#ifndef INFINITY
#define INFINITY __builtin_huge_valf()
#endif

#ifndef NAN
#define NAN __builtin_nanf("0x7fc00000")
#endif
