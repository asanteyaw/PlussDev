#include "pluss/value_options.h"
#include <stdexcept>

namespace pluss {

torch::TensorOptions to_torch(const Options& opt) {
    torch::TensorOptions torch_opt;

    if (opt.device == "cuda") {
        torch_opt = torch_opt.device(torch::kCUDA);
    } else {
        torch_opt = torch_opt.device(torch::kCPU);
    }

    switch (opt.dtype) {
        case DType::Float32: torch_opt = torch_opt.dtype(torch::kFloat32); break;
        case DType::Float64: torch_opt = torch_opt.dtype(torch::kFloat64); break;
        case DType::Int16:   torch_opt = torch_opt.dtype(torch::kInt16);   break;
        case DType::Int32:   torch_opt = torch_opt.dtype(torch::kInt32);   break;
        case DType::Int64:   torch_opt = torch_opt.dtype(torch::kInt64);   break;
        case DType::Default: torch_opt = torch_opt.dtype(torch::kFloat64); break;
        default:
            throw std::invalid_argument("Unknown dtype in Options.");
    }

    torch_opt = torch_opt.requires_grad(opt.requires_grad);
    return torch_opt;
}

}  // namespace pluss