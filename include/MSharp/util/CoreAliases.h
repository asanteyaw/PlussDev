#pragma once

namespace MSharp {
namespace pluss {
// Core type aliases: expose a small set of core MSharp types under
// MSharp::pluss::*. Only alias types that are guaranteed to be
// declared by headers that include this file from Allocator.h.
using DataPtr = ::MSharp::DataPtr;
using Allocator = ::MSharp::Allocator;
using Device = ::MSharp::Device;
using DeviceType = ::MSharp::DeviceType;
// Aliases for utility ArrayRef types from the global namespace
template <typename T>
using ArrayRef = ::ArrayRef<T>;
using IntArrayRef = ::IntArrayRef;
} // namespace pluss
} // namespace MSharp

// Top-level compatibility aliases: map a few commonly expected symbols
// into the MSharp namespace so internal headers that refer to them
// (e.g. intrusive_ptr_target, intrusive_ptr, Series, TensorImpl)
// will resolve.
namespace MSharp {
// intrusive_ptr target and smart pointer are implemented in `pluss`.
using intrusive_ptr_target = ::pluss::intrusive_ptr_target;
template <typename T>
using intrusive_ptr = ::pluss::intrusive_ptr<T>;

// Series/Tensor aliases: forward to pluss equivalents
using Series = ::pluss::Series;
using TensorBase = ::pluss::TensorBase;
using TensorImpl = ::pluss::TensorImpl;

// Symmetric/alias types used in symbolic shapes
using SymDimVector = ::MSharp::DimVector;
using SymInt = ::MSharp::SymInt;
} // namespace MSharp
