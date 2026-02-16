#pragma once

// Flame: shared low-level utilities used by CodeN (text).
// We intentionally reuse the implementation from MSharp to avoid duplicated
// ownership/ABI semantics across the framework.
//
// Namespace policy:
//   - pluss::flame is the "c10-like" foundation for CodeN
//   - pluss::msharp remains the original home of the implementations
//
// This header provides stable aliases so CodeN can depend on Flame without
// tying itself to the MSharp naming.

#include "MSharp/IntrusivePtr.h"
#include "MSharp/SnapPtr.h"

namespace pluss::flame {

using pluss::msharp::intrusive_ptr;
using pluss::msharp::make_intrusive;
namespace raw = pluss::msharp::raw;

// RefCounted base for intrusive objects.
using pluss::msharp::RefCounted;

} // namespace pluss::flame
