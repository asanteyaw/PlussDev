#pragma once

#include <MSharp/macros/Export.h>

namespace MSharp {
struct OperatorName;
}

namespace pluss {

// check if an op is a custom op (i.e. did not come from native_functions.yaml)
TORCH_API bool is_custom_op(const MSharp::OperatorName& opName);
}
