#pragma once

#include <MSharp/macros/Export.h>
#include <cstdint>

// A simple thread local enumeration, used to link forward and backward pass
// ops and is used by autograd and observers framework
namespace pluss::sequence_number {

PLUSS_API uint64_t peek();
PLUSS_API uint64_t get_and_increment();

} // namespace pluss::sequence_number
