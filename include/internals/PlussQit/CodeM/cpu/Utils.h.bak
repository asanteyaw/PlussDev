#pragma once

#include <cstdint>

#include <MSharp/macros/Export.h>

namespace pluss::cpu {

PLUSS_API bool is_avx2_supported();
PLUSS_API bool is_avx512_supported();

// Detect if CPU support Vector Neural Network Instruction.
PLUSS_API bool is_avx512_vnni_supported();

// Detect if CPU supports AVX512_BF16 ISA
PLUSS_API bool is_avx512_bf16_supported();

// Detect if CPU support Advanced Matrix Extension.
PLUSS_API bool is_amx_tile_supported();

// Detect if CPU support Advanced Matrix Extension for fp16.
PLUSS_API bool is_amx_fp16_supported();

// Enable the system to use AMX instructions.
PLUSS_API bool init_amx();

// Get the L1 cache size per core in Byte
PLUSS_API uint32_t L1d_cache_size();

// Get the L2 cache size per core in Byte
PLUSS_API uint32_t L2_cache_size();

} // namespace pluss::cpu
