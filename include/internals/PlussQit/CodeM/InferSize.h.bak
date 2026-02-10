#pragma once

#include <PlussQit/DimVector.h>
#include <MSharp/core/ScalarType.h>
#include <MSharp/core/SymIntArrayRef.h>
#include <MSharp/util/DimVector.h>
#include <optional>
#include <sstream>
#include <vector>

namespace pluss {

// Infers the size of a dim with size -1, if it exists. Also checks that new
// shape is compatible with the number of elements.
//
// templated to handle std::vector<int64_t> and DimVector use cases, see
// below
//
template <typename InputArrayRef, typename NumelType, typename ResultVec>
inline void infer_size_impl(
    InputArrayRef shape,
    NumelType numel,
    ResultVec& res) {
  NumelType newsize = 1;
  // N.B. this is an index, not a sym dim!
  std::optional<int64_t> infer_dim;
  for (int64_t dim = 0, ndim = shape.size(); dim != ndim; dim++) {
    if (shape[dim] == -1) {
      if (infer_dim) {
        throw std::runtime_error("only one dimension can be inferred");
      }
      infer_dim = dim;
    } else if (shape[dim] >= 0) {
      newsize *= shape[dim];
    } else {
      TORCH_CHECK(false, "invalid shape dimension ", shape[dim]);
    }
  }

  if (TORCH_GUARD_SIZE_OBLIVIOUS(sym_eq(numel, newsize)) ||
      (infer_dim && newsize > 0 && numel % newsize == 0)) {
    if (infer_dim) {
      // We have a degree of freedom here to select the dimension size; follow
      // NumPy semantics and just bail.  However, a nice error message is needed
      // because users often use `view` as a way to flatten & unflatten
      // dimensions and will otherwise be confused why
      //   empty_tensor.view( 0, 0)
      // works yet
      //   empty_tensor.view(-1, 0)
      // doesn't.
      TORCH_CHECK(
          newsize != 0,
          "cannot reshape series of 0 elements into shape ",
          shape,
          " because the unspecified dimension size -1 can be any "
          "value and is ambiguous");
      res[*infer_dim] = numel / newsize;
    }
    return;
  }

  std::ostringstream ss;
  ss << "shape '" << shape << "' is invalid for input of size " << numel;
  throw std::runtime_error(ss.str());
}

inline std::vector<int64_t> infer_size(IntArrayRef shape, int64_t numel) {
  auto res = shape.vec();
  infer_size_impl(shape, numel, res);
  return res;
}

inline pluss::DimVector infer_size_dv(IntArrayRef shape, int64_t numel) {
  auto res = pluss::DimVector(shape);
  infer_size_impl(shape, numel, res);
  return res;
}

inline pluss::SymDimVector infer_size_dv(
    MSharp::SymIntArrayRef shape,
    MSharp::SymInt numel) {
  auto res = pluss::SymDimVector(shape);
  infer_size_impl<MSharp::SymIntArrayRef, MSharp::SymInt, pluss::SymDimVector>(
      shape, std::move(numel), res);
  return res;
}

} // namespace pluss
