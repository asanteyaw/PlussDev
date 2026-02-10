#pragma once

// Minimal bit-packed integer types used by ScalarType list
namespace MSharp {
struct bits8 { uint8_t v; };
struct bits16 { uint16_t v; };
struct bits1x8 { uint8_t v; };
struct bits2x4 { uint8_t v; };
struct bits4x2 { uint8_t v; };
} // namespace MSharp
