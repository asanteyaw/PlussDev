#pragma once

#include <PlussQit/native/TensorIterator.h>

namespace pluss {
struct TensorIteratorBase;

namespace native {
inline namespace CPU_CAPABILITY {

void direct_copy_kernel(TensorIteratorBase &iter);
void copy_kernel(TensorIterator& iter, bool /*non_blocking*/);

}}}  // namespace pluss::native::CPU_CAPABILITY
