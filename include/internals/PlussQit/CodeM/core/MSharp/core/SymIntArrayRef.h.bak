#pragma once

#include <MSharp/core/SymInt.h>
#include <MSharp/util/ArrayRef.h>
#include <MSharp/util/DimVector.h>
#include <MSharp/util/Exception.h>
#include <MSharp/util/irange.h>
#include <cstdint>
#include <optional>

namespace MSharp {
using SymIntArrayRef = ArrayRef<SymInt>;

inline pluss::IntArrayRef asIntArrayRefUnchecked(MSharp::SymIntArrayRef ar) {
  return IntArrayRef(reinterpret_cast<const int64_t*>(ar.data()), ar.size());
}

// TODO: a SymIntArrayRef containing a heap allocated large negative integer
// can actually technically be converted to an IntArrayRef... but not with
// the non-owning API we have here.  We can't reinterpet cast; we have to
// allocate another buffer and write the integers into it.  If you need it,
// we can do it.  But I don't think you need it.

inline std::optional<pluss::IntArrayRef> asIntArrayRefSlowOpt(
    MSharp::SymIntArrayRef ar) {
  for (const MSharp::SymInt& sci : ar) {
    if (sci.is_heap_allocated()) {
      return std::nullopt;
    }
  }

  return {asIntArrayRefUnchecked(ar)};
}

inline pluss::IntArrayRef asIntArrayRefSlow(
    MSharp::SymIntArrayRef ar,
    const char* file,
    int64_t line) {
  for (const MSharp::SymInt& sci : ar) {
    TORCH_CHECK(
        !sci.is_heap_allocated(),
        file,
        ":",
        line,
        ": SymIntArrayRef expected to contain only concrete integers");
  }
  return asIntArrayRefUnchecked(ar);
}

// Even slower than asIntArrayRefSlow, as it forces an allocation for a
// destination int, BUT it is able to force specialization (it never errors)
inline MSharp::DimVector asIntArrayRefSlowAlloc(
    MSharp::SymIntArrayRef ar,
    const char* file,
    int64_t line) {
  MSharp::DimVector res(ar.size(), 0);
  for (const auto i : MSharp::irange(ar.size())) {
    res[i] = ar[i].guard_int(file, line);
  }
  return res;
}

#define C10_AS_INTARRAYREF_SLOW(a) MSharp::asIntArrayRefSlow(a, __FILE__, __LINE__)
#define C10_AS_INTARRAYREF_SLOW_ALLOC(a) \
  MSharp::asIntArrayRefSlowAlloc(a, __FILE__, __LINE__)

// Prefer using a more semantic constructor, like
// fromIntArrayRefKnownNonNegative
inline SymIntArrayRef fromIntArrayRefUnchecked(IntArrayRef array_ref) {
  return SymIntArrayRef(
      reinterpret_cast<const SymInt*>(array_ref.data()), array_ref.size());
}

inline SymIntArrayRef fromIntArrayRefKnownNonNegative(IntArrayRef array_ref) {
  return fromIntArrayRefUnchecked(array_ref);
}

inline SymIntArrayRef fromIntArrayRefSlow(IntArrayRef array_ref) {
  for (long i : array_ref) {
    TORCH_CHECK(
        SymInt::check_range(i),
        "IntArrayRef contains an int that cannot be represented as a SymInt: ",
        i);
  }
  return SymIntArrayRef(
      reinterpret_cast<const SymInt*>(array_ref.data()), array_ref.size());
}

} // namespace MSharp
