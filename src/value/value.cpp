#include "pluss/value.h"
#include "pluss/error_utils.h"
#include "pluss/value_options.h"
#include <torch/torch.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace pluss {

    struct Value::Impl {
        torch::Tensor t;
    
        Impl() : t(torch::empty({})) {}
        explicit Impl(const torch::Tensor& tensor) : t(tensor.clone()) {}
        Impl(const Impl& other) : t(other.t.clone()) {}
        Impl(Impl&& other) noexcept : t(std::move(other.t)) {}
    
        Impl& operator=(const Impl& other) {
            t = other.t.clone();
            return *this;
        }
        Impl& operator=(Impl&& other) noexcept {
            t = std::move(other.t);
            return *this;
        }
    
        ~Impl() = default;
    
        std::string shape_str() const {
            std::ostringstream oss;
            oss << "[";
            for (int64_t i = 0; i < t.dim(); ++i) {
                oss << t.size(i);
                if (i != t.dim() - 1) oss << ", ";
            }
            oss << "]";
            return oss.str();
        }
    
        std::vector<int64_t> shape() const {
            return t.sizes().vec();
        }
    
        std::string summary(int precision = 4) const {
            std::ostringstream oss;
            oss << "Value(" << shape_str() << ", dtype=" << t.dtype() << ")\n";
        
            const int64_t dim = t.dim();
        
            if (t.numel() == 1 && dim == 0) {
                oss << std::fixed << std::setprecision(precision) << t.item<double>();
                return oss.str();
            }
        
            oss << std::fixed << std::setprecision(precision);
        
            const int64_t limit = 5;
        
            if (dim == 1) {
                oss << "[";
                for (int64_t i = 0; i < t.size(0); ++i) {
                    if (i == limit && t.size(0) > limit * 2) {
                        oss << "...";
                        i = t.size(0) - limit;
                        if (i < limit) i = limit;  // Ensure no overlap
                        if (i < t.size(0)) oss << ", ";
                    }
                    oss << t[i].item<double>();
                    if (i != t.size(0) - 1) oss << ", ";
                }
                oss << "]";
            } else if (dim == 2) {
                for (int64_t i = 0; i < t.size(0); ++i) {
                    oss << " ";
                    for (int64_t j = 0; j < t.size(1); ++j) {
                        oss << std::setw(8) << t[i][j].item<double>();
                    }
                    oss << "\n";
                }
            } else {
                oss << "(Tensor with dimension " << dim << " not yet supported in summary)";
            }
        
            return oss.str();
        }
    };

// === Private Constructor + Internal Factory ===
Value::Value(std::unique_ptr<Impl> impl)
    : _impl(std::move(impl)) {}

std::unique_ptr<Value::Impl> Value::make_from_tensor(const torch::Tensor& t) {
    return std::make_unique<Impl>(t);
}

// === Rule of 5 ===
Value::Value() : _impl(std::make_unique<Impl>()) {}
Value::Value(const Value& other) : _impl(std::make_unique<Impl>(*other._impl)) {}
Value::Value(Value&& other) noexcept = default;
Value& Value::operator=(const Value& other) {
    _impl = std::make_unique<Impl>(*other._impl);
    return *this;
}
Value& Value::operator=(Value&& other) noexcept = default;
Value::~Value() = default;

// === Tensor Operations ===
Value Value::pow(double exponent) const {
    return attempt([&] {
        return Value(make_from_tensor(_impl->t.pow(exponent)));
    });
}

Value Value::sqrt() const {
    return attempt([&] {
        return Value(make_from_tensor(_impl->t.sqrt()));
    });
}

Value Value::root(int64_t n) const {
    return attempt([&] {
        if (n == 0)
            throw PlussError("Root degree cannot be zero.");
        return Value(make_from_tensor(_impl->t.pow(1.0 / n)));
    });
}

Value Value::var(int64_t dim, bool unbiased, bool keepdim) const {
    return attempt([&] {
        auto out = (dim == -1)
            ? _impl->t.var(unbiased)
            : _impl->t.var(dim, unbiased, keepdim);
        return Value(make_from_tensor(out));
    });
}

Value Value::std(int64_t dim, bool unbiased, bool keepdim) const {
    return var(dim, unbiased, keepdim).sqrt(); // already wrapped
}

Value Value::mean(int64_t dim, bool keepdim) const {
    return attempt([&] {
        auto out = (dim == -1)
            ? _impl->t.mean()
            : _impl->t.mean(dim, keepdim);
        return Value(make_from_tensor(out));
    });
}

Value Value::stack(const Value& other, int64_t dim) const {
    return attempt([&] {
        return Value(make_from_tensor(torch::stack({_impl->t, other._impl->t}, dim)));
    });
}

Value Value::stack(const std::vector<Value>& values, int64_t dim) {
    return attempt([&] {
        std::vector<torch::Tensor> tensors;
        tensors.reserve(values.size());
        for (const auto& v : values)
            tensors.push_back(v._impl->t);
        return Value(make_from_tensor(torch::stack(tensors, dim)));
    });
}
// === End Tensor Operations ===

// === Introspection ===
std::string Value::shape_str() const { return _impl->shape_str(); }
std::vector<int64_t> Value::shape() const { return _impl->shape(); }
std::string Value::summary(int precision) const { return _impl->summary(precision); }

// === Pretty Print ===
std::ostream& operator<<(std::ostream& os, const Value& v) {
    return os << v.summary();
}
// public access to private make_from_tensor
Value Value::from_tensor(const torch::Tensor& t) {
    return Value(make_from_tensor(t));
}

}  // namespace pluss