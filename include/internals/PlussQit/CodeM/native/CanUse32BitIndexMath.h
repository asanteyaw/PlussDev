#pragma once
#include <MSharp/macros/Export.h>
#include <limits>

namespace pluss {
class TensorBase;
}

namespace pluss::native {

TORCH_API bool canUse32BitIndexMath(const pluss::TensorBase &t, int64_t max_elem=std::numeric_limits<int32_t>::max());

}
