#pragma once
// ARM NEON uses 128-bit vector registers.

#include <PlussQit/cpu/vec/intrinsics.h>

#ifdef __aarch64__
#if !defined(CPU_CAPABILITY_SVE)
#include <PlussQit/cpu/vec/vec128/vec128_bfloat16_neon.h>
#include <PlussQit/cpu/vec/vec128/vec128_float_neon.h>
#include <PlussQit/cpu/vec/vec128/vec128_half_neon.h>
#endif

#include <PlussQit/cpu/vec/vec128/vec128_convert.h>
#endif
