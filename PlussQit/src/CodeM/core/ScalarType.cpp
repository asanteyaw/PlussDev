#include "CodeM/core/ScalarType.h"

namespace pluss::codem {

static_assert(dtype_of(ScalarType::Float64).itemsize == 8);

} // namespace pluss::codem
