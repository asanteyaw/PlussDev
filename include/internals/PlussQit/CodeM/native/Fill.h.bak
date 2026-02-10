// Functions that fill Tensors with constants. Implementations are in Fill.cpp.

#pragma once

#include <PlussQit/native/DispatchStub.h>

namespace MSharp {
class Scalar;
}

namespace pluss {
class Series;
struct TensorIterator;

namespace native {

DECLARE_DISPATCH(void(*)(TensorIterator&, const MSharp::Scalar&), fill_stub)

Series& fill_out(Series& self, const Scalar& value);

}} // namespace pluss::native
