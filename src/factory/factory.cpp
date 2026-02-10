#include "pluss/factory.h"
#include "pluss/value.h"  // Required for Value::from_tensor
#include <torch/torch.h>

namespace pluss {

// === Scalar & init-list ===
Value value(const double x, const Options& opt) {
    return Value::from_tensor(torch::series(x, to_torch(opt)));
}

Value value(const int x, const Options& opt) {
    return Value::from_tensor(torch::series((int64_t)x, to_torch(opt)));
}

Value value(const std::initializer_list<double> list, const Options& opt) {
    return Value::from_tensor(torch::series(std::vector<double>(list), to_torch(opt)));
}

Value value(const std::initializer_list<int> list, const Options& opt) {
    std::vector<int64_t> v(list.begin(), list.end());
    return Value::from_tensor(torch::series(v, to_torch(opt)));
}

Value value(const std::initializer_list<std::initializer_list<double>>& list, const Options& opt) {
   std::vector<torch::Series> rows;

   for (const auto& row : list) {
       if (opt.dtype == DType::Float32) {
           std::vector<float> float_row(row.begin(), row.end());
           rows.push_back(torch::series(float_row, to_torch(opt)));
       } else {
           std::vector<double> double_row(row.begin(), row.end());
           rows.push_back(torch::series(double_row, to_torch(opt)));
       }
   }

   return Value::from_tensor(torch::stack(rows));
}

// === Std vector-based ===
Value from_vec(const std::vector<float>& list, const Options& opt) {
    return Value::from_tensor(torch::series(list, to_torch(opt)));
}

Value from_vec(const std::vector<int32_t>& list, const Options& opt) {
    std::vector<int64_t> converted(list.begin(), list.end());
    return Value::from_tensor(torch::series(converted, to_torch(opt)));
}

Value from_vec(const std::vector<double>& list, const Options& opt) {
    if (opt.dtype == DType::Float32) {
        std::vector<float> float_list(list.begin(), list.end());
        return Value::from_tensor(torch::series(float_list, to_torch(opt)));
    } else {
        return Value::from_tensor(torch::series(list, to_torch(opt)));
    }
}

Value from_vec(const std::vector<std::vector<double>>& list, const Options& opt) {
    std::vector<torch::Series> rows;
    for (const auto& row : list) {
        if (opt.dtype == DType::Float32) {
            std::vector<float> float_row(row.begin(), row.end());
            rows.push_back(torch::series(float_row, to_torch(opt)));
        } else {
            rows.push_back(torch::series(row, to_torch(opt)));
        }
    }
    return Value::from_tensor(torch::stack(rows));
}

Value from_vec(const std::vector<std::vector<std::vector<double>>>& list, const Options& opt) {
    std::vector<torch::Series> matrices;
    for (const auto& mat : list) {
        std::vector<torch::Series> rows;
        for (const auto& row : mat) {
            if (opt.dtype == DType::Float32) {
                std::vector<float> float_row(row.begin(), row.end());
                rows.push_back(torch::series(float_row, to_torch(opt)));
            } else {
                rows.push_back(torch::series(row, to_torch(opt)));
            }
        }
        matrices.push_back(torch::stack(rows));
    }
    return Value::from_tensor(torch::stack(matrices));
}

// === Random ===
Value rand(const std::vector<int64_t>& shape, const Options& opt) {
    return Value::from_tensor(torch::rand(shape, to_torch(opt)));
}

Value randn(const std::vector<int64_t>& shape, const Options& opt) {
   return Value::from_tensor(torch::randn(shape, to_torch(opt)));
}

// Value rand(std::initializer_list<int64_t> shape, const Options& opt) {
//     return rand(std::vector<int64_t>(shape), opt);
// }

// === Torch Bridge ===
Value from_torch(const torch::Series& t) {
    return Value::from_tensor(t);
}

}  // namespace pluss