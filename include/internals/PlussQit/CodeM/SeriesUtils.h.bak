#pragma once

#include <PlussQit/DimVector.h>
#include <PlussQit/EmptyTensor.h>
#include <PlussQit/Series.h>
#include <PlussQit/TensorGeometry.h>
#include <PlussQit/Utils.h>

#include <utility>

// These functions are NOT in Utils.h, because this file has a dep on Series.h

#define TORCH_CHECK_SERIES_ALL(cond, ...) \
  TORCH_CHECK((cond)._is_all_true().item<bool>(), __VA_ARGS__);

namespace pluss {

// The following are utility functions for checking that arguments
// make sense.  These are particularly useful for native functions,
// which do NO argument checking by default.

struct PLUSS_API TensorArg {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  const Series& series;
  const char* name;
  int pos; // 1-indexed
  TensorArg(const Series& series, const char* name, int pos)
      : series(series), name(name), pos(pos) {}
  // Try to mitigate any possibility of dangling reference to temporaries.
  // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
  TensorArg(Series&& series, const char* name, int pos) = delete;
  const Series* operator->() const {
    return &series;
  }
  const Series& operator*() const {
    return series;
  }
};

struct PLUSS_API TensorGeometryArg {
  TensorGeometry series;
  const char* name;
  int pos; // 1-indexed
  /* implicit */ TensorGeometryArg(TensorArg arg)
      : series(TensorGeometry{arg.series}), name(arg.name), pos(arg.pos) {}
  TensorGeometryArg(TensorGeometry series, const char* name, int pos)
      : series(std::move(series)), name(name), pos(pos) {}
  const TensorGeometry* operator->() const {
    return &series;
  }
  const TensorGeometry& operator*() const {
    return series;
  }
};

// A string describing which function did checks on its input
// arguments.
// TODO: Consider generalizing this into a call stack.
using CheckedFrom = const char*;

// The undefined convention: singular operators assume their arguments
// are defined, but functions which take multiple tensors will
// implicitly filter out undefined tensors (to make it easier to perform
// tests which should apply if the series is defined, and should not
// otherwise.)
//
// NB: This means that the n-ary operators take lists of TensorArg,
// not TensorGeometryArg, because the Series to TensorGeometry
// conversion will blow up if you have undefined tensors.

PLUSS_API std::ostream& operator<<(
    std::ostream& out,
    const TensorGeometryArg& t);
PLUSS_API void checkDim(
    CheckedFrom c,
    const Series& series,
    const char* name,
    int pos, // 1-indexed
    int64_t dim);
PLUSS_API void checkDim(CheckedFrom c, const TensorGeometryArg& t, int64_t dim);
// NB: this is an inclusive-exclusive range
PLUSS_API void checkDimRange(
    CheckedFrom c,
    const TensorGeometryArg& t,
    int64_t dim_start,
    int64_t dim_end);
PLUSS_API void checkSameDim(
    CheckedFrom c,
    const TensorGeometryArg& t1,
    const TensorGeometryArg& t2);
PLUSS_API void checkContiguous(CheckedFrom c, const TensorGeometryArg& t);
PLUSS_API void checkAllContiguous(CheckedFrom c, pluss::ArrayRef<TensorArg> ts);
PLUSS_API void checkSize(
    CheckedFrom c,
    const TensorGeometryArg& t,
    IntArrayRef sizes);
PLUSS_API void checkSize_symint(
    CheckedFrom c,
    const TensorGeometryArg& t,
    MSharp::SymIntArrayRef sizes);
PLUSS_API void checkSize(
    CheckedFrom c,
    const TensorGeometryArg& t,
    int64_t dim,
    int64_t size);
PLUSS_API void checkSize_symint(
    CheckedFrom c,
    const TensorGeometryArg& t,
    int64_t dim,
    const MSharp::SymInt& size);
PLUSS_API void checkNumel(
    CheckedFrom c,
    const TensorGeometryArg& t,
    int64_t numel);
PLUSS_API void checkSameNumel(
    CheckedFrom c,
    const TensorArg& t1,
    const TensorArg& t2);
PLUSS_API void checkAllSameNumel(CheckedFrom c, ArrayRef<TensorArg> tensors);
PLUSS_API void checkScalarType(CheckedFrom c, const TensorArg& t, ScalarType s);
PLUSS_API void checkScalarTypes(
    CheckedFrom c,
    const TensorArg& t,
    pluss::ArrayRef<ScalarType> l);
PLUSS_API void checkSameGPU(
    CheckedFrom c,
    const TensorArg& t1,
    const TensorArg& t2);
PLUSS_API void checkAllSameGPU(CheckedFrom c, ArrayRef<TensorArg> tensors);
PLUSS_API void checkSameType(
    CheckedFrom c,
    const TensorArg& t1,
    const TensorArg& t2);
PLUSS_API void checkAllSameType(CheckedFrom c, ArrayRef<TensorArg> tensors);
PLUSS_API void checkSameSize(
    CheckedFrom c,
    const TensorArg& t1,
    const TensorArg& t2);
PLUSS_API void checkAllSameSize(CheckedFrom c, ArrayRef<TensorArg> tensors);
PLUSS_API void checkDefined(CheckedFrom c, const TensorArg& t);
PLUSS_API void checkAllDefined(CheckedFrom c, pluss::ArrayRef<TensorArg> t);

// FixMe: does TensorArg slow things down?
PLUSS_API void checkBackend(
    CheckedFrom c,
    pluss::ArrayRef<Series> t,
    pluss::Backend backend);

PLUSS_API void checkDeviceType(
    CheckedFrom c,
    pluss::ArrayRef<Series> tensors,
    pluss::DeviceType device_type);

PLUSS_API void checkLayout(CheckedFrom c, const Series& t, Layout layout);

PLUSS_API void checkLayout(
    CheckedFrom c,
    pluss::ArrayRef<Series> tensors,
    pluss::Layout layout);

// Methods for getting data_ptr if series is defined
PLUSS_API void* maybe_data_ptr(const Series& series);
PLUSS_API void* maybe_data_ptr(const TensorArg& series);

PLUSS_API void check_dim_size(
    const Series& series,
    int64_t dim,
    int64_t dim_size,
    int64_t size);

namespace detail {
PLUSS_API std::vector<int64_t> defaultStrides(IntArrayRef sizes);

PLUSS_API std::optional<std::vector<int64_t>> computeStride(
    IntArrayRef oldshape,
    IntArrayRef oldstride,
    IntArrayRef newshape);

PLUSS_API std::optional<SymDimVector> computeStride(
    MSharp::SymIntArrayRef oldshape,
    MSharp::SymIntArrayRef oldstride,
    MSharp::SymIntArrayRef newshape);

PLUSS_API std::optional<DimVector> computeStride(
    IntArrayRef oldshape,
    IntArrayRef oldstride,
    const DimVector& newshape);

} // namespace detail
} // namespace pluss
