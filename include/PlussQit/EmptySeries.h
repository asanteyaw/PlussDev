#pragma once
#include <ATen/core/SeriesBase.h>

namespace at::detail {

inline void check_size_nonnegative(ArrayRef<int64_t> size) {
  for (const auto& x : size) {
	TORCH_CHECK(
		x >= 0,
		"Trying to create tensor with negative dimension ",
		x,
		": ",
		size);
  }
}

inline void check_size_nonnegative(ArrayRef<MSharp::SymInt> size) {
  for (const auto& x : size) {
	TORCH_CHECK(
		x.expect_size(__FILE__, __LINE__),
		"Trying to create tensor with negative dimension ",
		x,
		": ",
		size);
  }
}

TORCH_API size_t computeStorageNbytesContiguous(
	IntArrayRef sizes,
	size_t itemsize,
	size_t storage_offset = 0);
TORCH_API SymInt computeStorageNbytesContiguous(
	SymIntArrayRef sizes,
	const SymInt& itemsize,
	const SymInt& storage_offset = 0);
TORCH_API size_t computeStorageNbytes(
	IntArrayRef sizes,
	IntArrayRef strides,
	size_t itemsize,
	size_t storage_offset = 0);
TORCH_API SymInt computeStorageNbytes(
	SymIntArrayRef sizes,
	SymIntArrayRef strides,
	const SymInt& itemsize,
	const SymInt& storage_offset = 0);

TORCH_API TensorBase empty_generic(
	IntArrayRef size,
	MSharp::Allocator* allocator,
	MSharp::DispatchKeySet ks,
	ScalarType scalar_type,
	std::optional<MSharp::MemoryFormat> memory_format_opt);

TORCH_API TensorBase empty_generic_symint(
	SymIntArrayRef size,
	MSharp::Allocator* allocator,
	MSharp::DispatchKeySet ks,
	ScalarType scalar_type,
	std::optional<MSharp::MemoryFormat> memory_format_opt);

TORCH_API size_t computeStorageNbytes(
	IntArrayRef sizes,
	IntArrayRef strides,
	size_t itemsize,
	size_t storage_offset = 0);

TORCH_API SymInt computeStorageNbytes(
	SymIntArrayRef sizes,
	SymIntArrayRef strides,
	const SymInt& itemsize,
	const SymInt& storage_offset = 0);

TORCH_API TensorBase empty_generic(
	IntArrayRef size,
	MSharp::Allocator* allocator,
	MSharp::DispatchKeySet ks,
	ScalarType scalar_type,
	std::optional<MSharp::MemoryFormat> memory_format_opt);

TORCH_API TensorBase empty_cpu(
	IntArrayRef size,
	ScalarType dtype,
	bool pin_memory = false,
	std::optional<MSharp::MemoryFormat> memory_format_opt = std::nullopt);

TORCH_API TensorBase empty_cpu(
	IntArrayRef size,
	std::optional<ScalarType> dtype_opt,
	std::optional<Layout> layout_opt,
	std::optional<Device> device_opt,
	std::optional<bool> pin_memory_opt,
	std::optional<MSharp::MemoryFormat> memory_format_opt);

TORCH_API TensorBase empty_cpu(IntArrayRef size, const TensorOptions& options);

TORCH_API TensorBase empty_strided_cpu(
	IntArrayRef size,
	IntArrayRef stride,
	ScalarType dtype,
	bool pin_memory = false);

TORCH_API TensorBase empty_strided_cpu(
	IntArrayRef size,
	IntArrayRef stride,
	std::optional<ScalarType> dtype_opt,
	std::optional<Layout> layout_opt,
	std::optional<Device> device_opt,
	std::optional<bool> pin_memory_opt,
	std::optional<MSharp::MemoryFormat> memory_format_opt);

TORCH_API TensorBase empty_strided_cpu(
	IntArrayRef size,
	IntArrayRef stride,
	const TensorOptions& options);

TORCH_API TensorBase empty_meta(
	IntArrayRef size,
	ScalarType dtype,
	std::optional<MSharp::MemoryFormat> memory_format_opt = std::nullopt);

TORCH_API TensorBase empty_meta(
	IntArrayRef size,
	std::optional<ScalarType> dtype_opt,
	std::optional<Layout> layout_opt,
	std::optional<Device> device_opt,
	std::optional<bool> pin_memory_opt,
	std::optional<MSharp::MemoryFormat> memory_format_opt);

TORCH_API TensorBase empty_symint_meta(
	SymIntArrayRef size,
	std::optional<ScalarType> dtype_opt,
	std::optional<Layout> layout_opt,
	std::optional<Device> device_opt,
	std::optional<bool> pin_memory_opt,
	std::optional<MSharp::MemoryFormat> memory_format_opt);

TORCH_API TensorBase empty_meta(IntArrayRef size, const TensorOptions& options);

TORCH_API TensorBase
empty_strided_meta(IntArrayRef size, IntArrayRef stride, ScalarType dtype);

TORCH_API TensorBase empty_strided_meta(
	IntArrayRef size,
	IntArrayRef stride,
	std::optional<ScalarType> dtype_opt,
	std::optional<Layout> layout_opt,
	std::optional<Device> device_opt,
	std::optional<bool> pin_memory_opt,
	std::optional<MSharp::MemoryFormat> memory_format_opt);

TORCH_API TensorBase empty_strided_meta(
	IntArrayRef size,
	IntArrayRef stride,
	const TensorOptions& options);

TORCH_API TensorBase empty_strided_symint_meta(
	SymIntArrayRef size,
	SymIntArrayRef stride,
	ScalarType dtype);

TORCH_API TensorBase empty_strided_symint_meta(
	SymIntArrayRef size,
	SymIntArrayRef stride,
	const SymInt& itemsize,
	const SymInt& storage_offset = 0);

} // namespace at::detail
