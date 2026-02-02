#pragma once

// Factory functions for creating Series objects with pluss types instead of at:: types

#include <ATen/core/ATen_fwd.h>
#include <c10/util/ArrayRef.h>
#include <pluss/SeriesBody.h>    // Testing SeriesOps.h instead
#include <pluss/Types.h>
#include <torch/csrc/autograd/variable.h>
#include <ATen/core/Tensor.h>
#include <ATen/TracerMode.h>
#include <ATen/core/grad_mode.h>
#include <functional>
// #include <initializer_list>
// #include <utility>

namespace pluss {

/// Creates a Series from a TensorDataContainer
/// This is the main function to create Series objects, similar to torch::tensor
// inline Series value(const SeriesDataContainer& tensor_data_container, const SeriesOptions& options = {}) {
//   at::Tensor tensor = tensor_data_container.convert_to_tensor(at::TensorOptions(options).requires_grad(::std::nullopt));
//   return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
// }
inline Series value(SeriesDataContainer data_container, const SeriesOptions& options = {}) {
  return Series(torch::autograd::make_variable(data_container.convert_to_tensor(at::TensorOptions(options).requires_grad(::std::nullopt)),
  at::TensorOptions(options).requires_grad()));
}

// inline Series value(torch::detail::TensorDataContainer tensor_data_container, const at::TensorOptions& options = {}) {
//   return Series(torch::autograd::make_variable(
//     // note: we remove the requires_grad setting from the TensorOptions because
//     // it is ignored anyways (and we actually have an assertion that it isn't set
//     // which would fail otherwise). We handle requires_grad explicitly here
//     // instead of passing it through to the kernel.
//     tensor_data_container.convert_to_tensor(options.requires_grad(::std::nullopt)),
//     options.requires_grad()));
// }

/// Creates a Series from an existing at::Tensor
/// This function allows easy conversion from torch tensors to Series objects
inline Series from_torch(const at::Tensor& tensor, bool requires_grad = false) {
  return Series(torch::autograd::Variable(tensor).set_requires_grad(requires_grad));
}

// Helper function to process options configuration
inline SeriesOptions process_options_config(const std::unordered_map<std::string, std::string>& config) {
  SeriesOptions opts;

  for (const auto& [key, value] : config) {
      if (key == "dtype") {
          if (value == "uint8") opts.dtype(ScalarType::Byte);
          else if (value == "int8") opts.dtype(ScalarType::Char);
          else if (value == "int16") opts.dtype(ScalarType::Short);
          else if (value == "int32" || value == "int") opts.dtype(ScalarType::Int);
          else if (value == "int64" || value == "long") opts.dtype(ScalarType::Long);
          else if (value == "float16" || value == "half") opts.dtype(ScalarType::Half);
          else if (value == "float32" || value == "float") opts.dtype(ScalarType::Float);
          else if (value == "float64" || value == "double") opts.dtype(ScalarType::Double);
          else if (value == "bfloat16") opts.dtype(ScalarType::BFloat16);
          else if (value == "bool") opts.dtype(ScalarType::Bool);
          else if (value == "complex32" || value == "complexhalf") opts.dtype(ScalarType::ComplexHalf);
          else if (value == "complex64" || value == "complexfloat") opts.dtype(ScalarType::ComplexFloat);
          else if (value == "complex128" || value == "complexdouble") opts.dtype(ScalarType::ComplexDouble);
          else if (value == "qint8") opts.dtype(ScalarType::QInt8);
          else if (value == "quint8") opts.dtype(ScalarType::QUInt8);
          else if (value == "qint32") opts.dtype(ScalarType::QInt32);
          else if (value == "quint4x2") opts.dtype(ScalarType::QUInt4x2);
          else if (value == "quint2x4") opts.dtype(ScalarType::QUInt2x4);
          else if (value == "undefined") opts.dtype(ScalarType::Undefined);
          else if (value == "numoptions") opts.dtype(ScalarType::NumOptions);
          else throw std::invalid_argument("Invalid dtype value: " + value);
      }

      else if (key == "device") {
          try {
              opts.device(at::Device(value));  // handles cuda, cuda:0, cpu, etc.
          } catch (const c10::Error& e) {
              throw std::invalid_argument("Invalid device string: " + value);
          }
      }

      else if (key == "requires_grad") {
          opts.requires_grad(value == "true");
      }

      else if (key == "pinned_memory") {
          opts.pinned_memory(value == "true");
      }

      else if (key == "layout") {
          if (value == "strided") opts.layout(at::kStrided);
          else if (value == "sparse") opts.layout(at::kSparse);
          else if (value == "sparse_csr") opts.layout(at::kSparseCsr);
          else throw std::invalid_argument("Invalid layout value: " + value);
      }

      else if (key == "memory_format") {
          if (value == "contiguous") opts.memory_format(at::MemoryFormat::Contiguous);
          else if (value == "preserve") opts.memory_format(at::MemoryFormat::Preserve);
          else if (value == "channels_last") opts.memory_format(at::MemoryFormat::ChannelsLast);
          else if (value == "channels_last_3d") opts.memory_format(at::MemoryFormat::ChannelsLast3d);
          else throw std::invalid_argument("Invalid memory_format value: " + value);
      }

      else if (key == "sparse") {
          opts.sparse(value == "true");
      }

      else if (key == "sparse_compressed") {
          opts.sparse_compressed(value == "true");
      }

      else if (key == "sparse_csr") {
          opts.sparse_csr(value == "true");
      }

      else {
          throw std::invalid_argument("Unknown SeriesOptions key: " + key);
      }
  }

  return opts;
}

// Original function that takes an unordered_map
inline SeriesOptions config(const std::unordered_map<std::string, std::string>& config) {
  return process_options_config(config);
}

// Overload that takes an initializer list of strings
// inline SeriesOptions config(std::initializer_list<std::string> list) {
//   if (list.size() % 2 != 0) {
//     throw std::invalid_argument("options_config initializer list must have an even number of elements (key-value pairs)");
//   }
  
//   std::unordered_map<std::string, std::string> config;
//   auto it = list.begin();
//   while (it != list.end()) {
//     std::string key = *it++;
//     std::string value = *it++;
//     config[key] = value;
//   }
  
//   return process_options_config(config);
// }

template <typename T>
std::string to_string_value(const T& value) {
    std::ostringstream oss;
    oss << value;  // relies on operator<<
    return oss.str();
}

template <typename T>
inline SeriesOptions config(std::initializer_list<std::pair<const char*, T>> list) {
    std::unordered_map<std::string, std::string> map;
    for (const auto& [key, val] : list)
        map.emplace(key, to_string_value(val));
    return config(map);
}

/// A generic deleter function.
using Deleter = std::function<void(void*)>;

/// Exposes the given `data` as a `Series` without taking ownership of the
/// original data. `sizes` should specify the shape of the tensor, `strides` the
/// stride in each dimension. The `deleter` function (a
/// `std::function<void(void*)>`) will be called on the `data` when the Series
/// data would normally be deallocated. The `TensorOptions` specify additional
/// configuration options for the returned tensor, such as what type to
/// interpret the `data` as.
inline Series from_blob(
    void* data,
    const IntArrayRef& sizes,
    const IntArrayRef& strides,
    const Deleter& deleter,
    const SeriesOptions& options = {}) {
  at::Tensor tensor = ([&]() {
    at::AutoDispatchBelowAutograd guard;  // TODO: remove
    at::tracer::impl::NoTracerDispatchMode tracer_guard;
    return at::from_blob(data, sizes, strides, deleter, at::TensorOptions(options).requires_grad(::std::nullopt));
  })();
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

/// Exposes the given `data` as a `Series` without taking ownership of the
/// original data. `sizes` should specify the shape of the tensor, `strides` the
/// stride in each dimension. The `TensorOptions`
/// specify additional configuration options for the returned tensor, such as
/// what type to interpret the `data` as.
inline Series from_blob(
    void* data,
    const IntArrayRef& sizes,
    const IntArrayRef& strides,
    const SeriesOptions& options = {}) {
  at::Tensor tensor = ([&]() {
    at::AutoDispatchBelowAutograd guard;  // TODO: remove
    at::tracer::impl::NoTracerDispatchMode tracer_guard;
    return at::from_blob(data, sizes, strides, at::TensorOptions(options).requires_grad(::std::nullopt));
  })();
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

/// Exposes the given `data` as a `Series` without taking ownership of the
/// original data. `sizes` should specify the shape of the tensor. The `deleter`
/// (a `std::function<void(void*)>`) function will be called on the `data` when
/// the Series data would normally be deallocated. The `TensorOptions` specify
/// additional configuration options for the returned tensor, such as what type
/// to interpret the `data` as.
inline Series from_blob(
    void* data,
    const IntArrayRef& sizes,
    const Deleter& deleter,
    const SeriesOptions& options = {}) {
  at::Tensor tensor = ([&]() {
    at::AutoDispatchBelowAutograd guard;  // TODO: remove
    at::tracer::impl::NoTracerDispatchMode tracer_guard;
    return at::from_blob(data, sizes, deleter, at::TensorOptions(options).requires_grad(::std::nullopt));
  })();
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

/// Exposes the given `data` as a `Series` without taking ownership of the
/// original data. `sizes` should specify the shape of the tensor. The
/// `TensorOptions` specify additional configuration options for the returned
/// tensor, such as what type to interpret the `data` as.
inline Series from_blob(
    void* data,
    const IntArrayRef& sizes,
    const SeriesOptions& options = {}) {
  at::Tensor tensor = ([&]() {
    at::AutoDispatchBelowAutograd guard;  // TODO: remove
    at::tracer::impl::NoTracerDispatchMode tracer_guard;
    return at::from_blob(data, sizes, at::TensorOptions(options).requires_grad(::std::nullopt));
  })();
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

// Factory functions for creating Series objects from various ATen functions
inline Series zeros(const IntArrayRef& size, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::zeros(size, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series ones(const IntArrayRef& size, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::ones(size, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series rand(const IntArrayRef& size, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::rand(size, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series randn(const IntArrayRef& size, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::randn(size, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series arange(const Scalar& end, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::arange(end, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series arange(const Scalar& start, const Scalar& end, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::arange(start, end, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series arange(const Scalar& start, const Scalar& end, const Scalar& step, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::arange(start, end, step, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series linspace(const Scalar& start, const Scalar& end, int64_t steps, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::linspace(start, end, steps, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series eye(int64_t n, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::eye(n, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series eye(int64_t n, int64_t m, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::eye(n, m, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series empty(const IntArrayRef& size, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::empty(size, at::TensorOptions(options).requires_grad(::std::nullopt), 
                               memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series full(const IntArrayRef& size, const Scalar& fill_value, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::full(size, fill_value, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

// Additional factory functions

inline Series _make_dep_token(const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::_make_dep_token(at::TensorOptions(options).requires_grad(::std::nullopt), 
                                         memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series _cudnn_init_dropout_state(double dropout, bool train, int64_t dropout_seed, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::_cudnn_init_dropout_state(dropout, train, dropout_seed, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series bartlett_window(int64_t window_length, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::bartlett_window(window_length, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series bartlett_window(int64_t window_length, bool periodic, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::bartlett_window(window_length, periodic, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series blackman_window(int64_t window_length, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::blackman_window(window_length, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series blackman_window(int64_t window_length, bool periodic, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::blackman_window(window_length, periodic, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series empty(const IntArrayRef& size, const optional<DimnameList>& names, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::empty(size, names.has_value() ? optional<at::DimnameList>(*names) : optional<at::DimnameList>{}, 
                               at::TensorOptions(options).requires_grad(::std::nullopt), 
                               memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series empty_symint(const SymIntArrayRef& size, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::empty_symint(size, at::TensorOptions(options).requires_grad(::std::nullopt), 
                                      memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series empty_permuted(const IntArrayRef& size, const IntArrayRef& physical_layout, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::empty_permuted(size, physical_layout, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series empty_permuted_symint(const SymIntArrayRef& size, const IntArrayRef& physical_layout, const SeriesOptions& options = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::empty_permuted_symint(size, physical_layout, at::TensorOptions(options).requires_grad(::std::nullopt));
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series _empty_affine_quantized(const IntArrayRef& size, const SeriesOptions& options = {}, double scale = 1, int64_t zero_point = 0, 
                                     const optional<MemoryFormat>& memory_format = optional<MemoryFormat>{MemoryFormat::Contiguous}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::_empty_affine_quantized(size, at::TensorOptions(options).requires_grad(::std::nullopt), scale, zero_point, 
                                                 memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

// Add more factory functions as needed...

// The last few important ones
inline Series zeros_like(const Series& self, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::zeros_like(self.tensor(), at::TensorOptions(options).requires_grad(::std::nullopt), 
                                    memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series ones_like(const Series& self, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::ones_like(self.tensor(), at::TensorOptions(options).requires_grad(::std::nullopt), 
                                   memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series empty_like(const Series& self, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::empty_like(self.tensor(), at::TensorOptions(options).requires_grad(::std::nullopt), 
                                    memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series full_like(const Series& self, const Scalar& fill_value, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::full_like(self.tensor(), fill_value, at::TensorOptions(options).requires_grad(::std::nullopt), 
                                   memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series rand_like(const Series& self, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::rand_like(self.tensor(), at::TensorOptions(options).requires_grad(::std::nullopt), 
                                   memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series randn_like(const Series& self, const SeriesOptions& options = {}, const optional<MemoryFormat>& memory_format = {}) {
  at::AutoDispatchBelowADInplaceOrView guard;
  at::Tensor tensor = at::randn_like(self.tensor(), at::TensorOptions(options).requires_grad(::std::nullopt), 
                                    memory_format.has_value() ? optional<at::MemoryFormat>(*memory_format) : optional<at::MemoryFormat>{});
  return Series(torch::autograd::Variable(tensor).set_requires_grad(options.requires_grad_opt().value_or(false)));
}

inline Series pile(SeriesList series, int64_t dim=0) {
  // at::TensorList tensors;
  auto tensor_vec = map_to_vector<Series, torch::Tensor>(series, [](const Series& s) {
    return s.tensor();
  });
  return Series(at::_ops::stack::call(tensor_vec, dim));
}

inline Series unite(const SeriesList& inputs, int64_t dim = 0) {
  Series stacked = pile(inputs, dim);
  if (stacked.dim() > dim && stacked.shape(dim) == static_cast<int64_t>(inputs.size())) {
      return stacked.squeeze(dim);
  }
  return stacked;
}

} // namespace pluss
