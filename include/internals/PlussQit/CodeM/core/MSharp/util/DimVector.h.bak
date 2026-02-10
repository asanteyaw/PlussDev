#pragma once

#include <MSharp/core/SymInt.h>
#include <MSharp/core/impl/SizesAndStrides.h>
#include <MSharp/util/SmallVector.h>
#include <cstddef>
#include <cstdint>

namespace MSharp {

constexpr size_t kDimVectorStaticSize = C10_SIZES_AND_STRIDES_MAX_INLINE_SIZE;

/// A container for sizes or strides
using DimVector = SmallVector<int64_t, kDimVectorStaticSize>;
using SymDimVector = SmallVector<MSharp::SymInt, kDimVectorStaticSize>;

} // namespace MSharp
