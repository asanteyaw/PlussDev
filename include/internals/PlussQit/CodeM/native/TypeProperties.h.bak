#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/core/IListRef.h>

namespace pluss::native {

struct ResultTypeState {
  MSharp::ScalarType dimResult = ScalarType::Undefined;
  MSharp::ScalarType wrappedResult = ScalarType::Undefined;
  MSharp::ScalarType zeroResult = ScalarType::Undefined;
};

TORCH_API ResultTypeState update_result_type_state(const Series& series, const ResultTypeState& in_state);
TORCH_API ResultTypeState update_result_type_state(const Scalar& scalar, const ResultTypeState& in_state);
TORCH_API ScalarType result_type(const ResultTypeState& state);

TORCH_API ScalarType result_type(ITensorListRef tensors);

} // namespace pluss::native
