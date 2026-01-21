#pragma once

#include <c10/core/Scalar.h>
#include <c10/util/ArrayRef.h>
#include <c10/core/MemoryFormat.h>
#include <c10/core/Device.h>
#include <c10/core/SymInt.h>
#include <c10/core/SymIntArrayRef.h>
#include <ATen/core/Dimname.h>
#include <ATen/core/Generator.h>
#include <torch/csrc/api/include/torch/types.h>
#include <torch/csrc/api/include/torch/detail/TensorDataContainer.h>
#include <optional>
#include <vector>
#include <string>

namespace pluss {

// Forward declaration
class Series;

class Scalar : public at::Scalar {
private:
    at::Scalar scalar_;
public:
    Scalar() = default;
    Scalar(int value) : scalar_(value) {}
    Scalar(int64_t value) : scalar_(value) {}
    Scalar(double value) : scalar_(value) {}
    Scalar(bool value) : scalar_(value) {}
    Scalar(const at::Scalar& scalar) : scalar_(scalar) {}
    // operator at::Scalar() const { return scalar_; }
};

template <typename T>
class ArrayRef {
private:
    std::vector<T> data_;
public:
    ArrayRef() = default;
    ArrayRef(const std::vector<T>& data) : data_(data) {}
    ArrayRef(std::initializer_list<T> data) : data_(data) {}
    ArrayRef(const c10::ArrayRef<T>& ref) : data_(ref.begin(), ref.end()) {}
    operator c10::ArrayRef<T>() const { return c10::ArrayRef<T>(data_); }

    size_t size() const { return data_.size(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }    
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
};

class IntArrayRef {
private:
    std::vector<int64_t> data_;
public:
    IntArrayRef() = default;
    IntArrayRef(const std::vector<int64_t>& data) : data_(data) {}
    IntArrayRef(std::initializer_list<int64_t> data) : data_(data) {}
    IntArrayRef(const at::IntArrayRef& ref) : data_(ref.begin(), ref.end()) {}
    operator at::IntArrayRef() const { return at::IntArrayRef(data_); }
};

class OptionalIntArrayRef {
    private:
        std::optional<IntArrayRef> data_;
    
    public:
        OptionalIntArrayRef() = default;
        OptionalIntArrayRef(const IntArrayRef& data) : data_(data) {}
        OptionalIntArrayRef(std::nullopt_t) : data_(std::nullopt) {}
    
        operator at::OptionalIntArrayRef() const {
            return data_.has_value() ? at::OptionalIntArrayRef(data_.value()) : at::OptionalIntArrayRef();
        }
    
        bool has_value() const { return data_.has_value(); }
    
        const IntArrayRef& value() const {
            if (!data_) throw std::runtime_error("OptionalIntArrayRef has no value");
            return data_.value();
        }
    
        const IntArrayRef& operator*() const { return value(); }
    
        const IntArrayRef* operator->() const {
            if (!data_) throw std::runtime_error("OptionalIntArrayRef has no value");
            return &data_.value();
        }
};

class SymInt {
private:
    c10::SymInt symint_;
public:
    SymInt() = default;
    SymInt(int64_t value) : symint_(value) {}
    SymInt(const c10::SymInt& symint) : symint_(symint) {}
    operator c10::SymInt() const { return symint_; }
};

class SymIntArrayRef {
private:
    std::vector<c10::SymInt> data_;
public:
    SymIntArrayRef() = default;
    SymIntArrayRef(const std::vector<c10::SymInt>& data) : data_(data) {}
    SymIntArrayRef(std::initializer_list<c10::SymInt> data) : data_(data) {}
    SymIntArrayRef(const c10::SymIntArrayRef& ref) : data_(ref.begin(), ref.end()) {}
    operator c10::SymIntArrayRef() const { return c10::SymIntArrayRef(data_); }
};

class OptionalSymIntArrayRef {
private:
    std::optional<SymIntArrayRef> data_;
public:
    OptionalSymIntArrayRef() = default;
    OptionalSymIntArrayRef(const SymIntArrayRef& data) : data_(data) {}
    OptionalSymIntArrayRef(std::nullopt_t) : data_(std::nullopt) {}
    operator at::OptionalSymIntArrayRef() const {
        return data_.has_value() ? at::OptionalSymIntArrayRef(data_.value()) : at::OptionalSymIntArrayRef();
    }
};

class Dimname {
private:
    at::Dimname dimname_;
public:
    Dimname() = delete;
    Dimname(const at::Dimname& dimname) : dimname_(dimname) {}
    static Dimname fromSymbol(const std::string& name) {
        return Dimname(at::Dimname::fromSymbol(at::Symbol::fromQualString(name)));
    }
    operator at::Dimname() const { return dimname_; }
};

class DimnameList {
private:
    std::vector<Dimname> dimnames_;
public:
    DimnameList() = default;
    DimnameList(const std::vector<Dimname>& dimnames) : dimnames_(dimnames) {}
    DimnameList(std::initializer_list<Dimname> dimnames) : dimnames_(dimnames) {}
    operator at::DimnameList() const {
        std::vector<at::Dimname> result;
        for (const auto& d : dimnames_) result.push_back(d);
        return at::DimnameList(result);
    }
};

class MemoryFormat {
private:
    at::MemoryFormat format_;
public:
    MemoryFormat(at::MemoryFormat format) : format_(format) {}
    operator at::MemoryFormat() const { return format_; }
    bool operator==(const MemoryFormat& other) const { return format_ == other.format_; }
    bool operator!=(const MemoryFormat& other) const { return format_ != other.format_; }
    static const MemoryFormat Contiguous;
    static const MemoryFormat Preserve;
    static const MemoryFormat ChannelsLast;
    static const MemoryFormat ChannelsLast3d;
};
inline const MemoryFormat MemoryFormat::Contiguous(at::MemoryFormat::Contiguous);
inline const MemoryFormat MemoryFormat::Preserve(at::MemoryFormat::Preserve);
inline const MemoryFormat MemoryFormat::ChannelsLast(at::MemoryFormat::ChannelsLast);
inline const MemoryFormat MemoryFormat::ChannelsLast3d(at::MemoryFormat::ChannelsLast3d);

class Device {
private:
    at::Device device_;
public:
    Device() = delete;
    Device(const std::string& s) : device_(s) {}
    Device(const at::Device& d) : device_(d) {}
    operator at::Device() const { return device_; }
};

class Generator {
private:
    at::Generator generator_;
public:
    Generator() = default;
    Generator(const at::Generator& g) : generator_(g) {}
    operator at::Generator() const { return generator_; }
};

// class SeriesDataContainer {
//     private:
//         torch::detail::TensorDataContainer container_;
    
//     public:
//         SeriesDataContainer() = default;
    
//         // Allow construction from any valid tensor-like input (1D, vector, scalar, etc.)
//         template <typename T>
//         SeriesDataContainer(T&& data)
//             noexcept(noexcept(torch::detail::TensorDataContainer(std::forward<T>(data))))
//             : container_(std::forward<T>(data)) {}
    
//         // Special-case initializer list (like {1.0, 2.0, 3.0}) to forward directly to torch container
//         template <typename T>
//         SeriesDataContainer(std::initializer_list<T> list)
//             : container_(torch::detail::TensorDataContainer(list)) {}
    
//         // Implicit conversion to torch::detail::TensorDataContainer
//         operator torch::detail::TensorDataContainer() const { return container_; }
    
//         // Convert to Tensor using torch's internal logic
//         at::Tensor convert_to_tensor(const at::TensorOptions& opts) const {
//             return container_.convert_to_tensor(opts);
//         }
//     };
class SeriesDataContainer : public torch::detail::TensorDataContainer {
    public:
        using torch::detail::TensorDataContainer::TensorDataContainer; // 🪄 inherit all constructors
};

struct ScalarType {
    private:
        at::ScalarType scalar_type_;
    
    public:
        ScalarType() = default;
        ScalarType(at::ScalarType type) : scalar_type_(type) {}
        ScalarType(int type) : scalar_type_(static_cast<at::ScalarType>(type)) {}
    
        // Implicit conversion to at::ScalarType
        operator at::ScalarType() const { return scalar_type_; }
    
        // Implicit conversion from your old enum-style constants
        static const ScalarType Byte, Char, Short, Int, Long, Half, Float, Double,
                                ComplexHalf, ComplexFloat, ComplexDouble, Bool,
                                QInt8, QUInt8, QInt32, BFloat16, QUInt4x2, QUInt2x4,
                                Undefined, NumOptions;
};
    
// Define constants
inline const ScalarType ScalarType::Byte(at::ScalarType::Byte);
inline const ScalarType ScalarType::Char(at::ScalarType::Char);
inline const ScalarType ScalarType::Short(at::ScalarType::Short);
inline const ScalarType ScalarType::Int(at::ScalarType::Int);
inline const ScalarType ScalarType::Long(at::ScalarType::Long);
inline const ScalarType ScalarType::Half(at::ScalarType::Half);
inline const ScalarType ScalarType::Float(at::ScalarType::Float);
inline const ScalarType ScalarType::Double(at::ScalarType::Double);
inline const ScalarType ScalarType::ComplexHalf(at::ScalarType::ComplexHalf);
inline const ScalarType ScalarType::ComplexFloat(at::ScalarType::ComplexFloat);
inline const ScalarType ScalarType::ComplexDouble(at::ScalarType::ComplexDouble);
inline const ScalarType ScalarType::Bool(at::ScalarType::Bool);
inline const ScalarType ScalarType::QInt8(at::ScalarType::QInt8);
inline const ScalarType ScalarType::QUInt8(at::ScalarType::QUInt8);
inline const ScalarType ScalarType::QInt32(at::ScalarType::QInt32);
inline const ScalarType ScalarType::BFloat16(at::ScalarType::BFloat16);
inline const ScalarType ScalarType::QUInt4x2(at::ScalarType::QUInt4x2);
inline const ScalarType ScalarType::QUInt2x4(at::ScalarType::QUInt2x4);
inline const ScalarType ScalarType::Undefined(at::ScalarType::Undefined);
inline const ScalarType ScalarType::NumOptions(at::ScalarType::NumOptions);

struct SeriesOptions {
    private:
        std::optional<at::ScalarType> dtype_;
        std::optional<at::Layout> layout_;
        std::optional<at::Device> device_;
        std::optional<bool> requires_grad_;
        std::optional<bool> pinned_memory_;
        std::optional<at::MemoryFormat> memory_format_;
        std::optional<bool> is_sparse_;
        std::optional<bool> is_sparse_compressed_;
        std::optional<bool> is_sparse_csr_;
    
    public:
        SeriesOptions& dtype(at::ScalarType t) { dtype_ = t; return *this; }
        SeriesOptions& layout(at::Layout l) { layout_ = l; return *this; }
        SeriesOptions& device(at::Device d) { device_ = d; return *this; }
        SeriesOptions& requires_grad(bool r) { requires_grad_ = r; return *this; }
        SeriesOptions& pinned_memory(bool p) { pinned_memory_ = p; return *this; }
        SeriesOptions& memory_format(at::MemoryFormat f) { memory_format_ = f; return *this; }
        SeriesOptions& sparse(bool v = true) { is_sparse_ = v; return *this; }
        SeriesOptions& sparse_compressed(bool v = true) { is_sparse_compressed_ = v; return *this; }
        SeriesOptions& sparse_csr(bool v = true) { is_sparse_csr_ = v; return *this; }
    
        std::optional<caffe2::TypeMeta> type_meta_opt() const {
            return dtype_ ? std::optional<caffe2::TypeMeta>(c10::scalarTypeToTypeMeta(*dtype_)) : std::nullopt;
        }
    
        operator at::TensorOptions() const {
            at::TensorOptions opt;
            if (dtype_) opt = opt.dtype(*dtype_);
            if (layout_) opt = opt.layout(*layout_);
            if (device_) opt = opt.device(*device_);
            if (requires_grad_) opt = opt.requires_grad(*requires_grad_);
            if (pinned_memory_) opt = opt.pinned_memory(*pinned_memory_);
            if (memory_format_) opt = opt.memory_format(*memory_format_);
            return opt;
        }
    
        std::optional<at::ScalarType> dtype_opt() const { return dtype_; }
        std::optional<at::Layout> layout_opt() const { return layout_; }
        std::optional<at::Device> device_opt() const { return device_; }
        std::optional<bool> requires_grad_opt() const { return requires_grad_; }
        std::optional<bool> pinned_memory_opt() const { return pinned_memory_; }
        std::optional<at::MemoryFormat> memory_format_opt() const { return memory_format_; }
        std::optional<bool> is_sparse_opt() const { return is_sparse_; }
        std::optional<bool> is_sparse_compressed_opt() const { return is_sparse_compressed_; }
        std::optional<bool> is_sparse_csr_opt() const { return is_sparse_csr_; }
    
        bool has_dtype() const { return dtype_.has_value(); }
        bool has_layout() const { return layout_.has_value(); }
        bool has_device() const { return device_.has_value(); }
        bool has_requires_grad() const { return requires_grad_.has_value(); }
        bool has_pinned_memory() const { return pinned_memory_.has_value(); }
        bool has_memory_format() const { return memory_format_.has_value(); }
    };
    
    // 

template<typename T>
using optional = std::optional<T>;
using string_view = c10::string_view;
using Layout = at::Layout;
using QScheme = at::QScheme;
using Storage = at::Storage;
using Stream = c10::Stream;
constexpr auto nullopt = std::nullopt;

template <typename In, typename Out>
std::vector<Out> map_to_vector(const ArrayRef<In>& input, const std::function<Out(const In&)>& fn) {
    std::vector<Out> out;
    out.reserve(input.size());
    for (const auto& val : input)
        out.push_back(fn(val));
    return out;
}

} // namespace pluss

