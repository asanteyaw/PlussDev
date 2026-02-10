#pragma once

// See NOTE: [Series vs. TensorBase]
namespace pluss {
class TensorBase;
}

namespace pluss::native {

TORCH_API bool cudnn_is_acceptable(const TensorBase& self);

} // namespace pluss::native
