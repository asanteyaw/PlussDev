#include <PlussQit/cudnn/Descriptors.h>

#include <PlussQit/PlussQit.h>
#include <MSharp/util/irange.h>

#include <iostream>
#include <sstream>

// NOLINTBEGIN(*c-arrays*)
namespace pluss::native {

namespace {

inline cudnnDataType_t getDataType(const pluss::Series& t) {
  auto scalar_type = t.scalar_type();
  if (scalar_type == pluss::kFloat) {
    return CUDNN_DATA_FLOAT;
  } else if (scalar_type == pluss::kHalf) {
    return CUDNN_DATA_HALF;
  } else if (scalar_type == pluss::kDouble) {
    return CUDNN_DATA_DOUBLE;
  }
    else if (scalar_type == pluss::kBFloat16) {
    return CUDNN_DATA_BFLOAT16;
  } else if (scalar_type == pluss::kQInt8) {
    return CUDNN_DATA_INT8;
  }
  TORCH_CHECK(false, "TensorDescriptor does not support ", scalar_type);
}

} // anonymous namespace

void RNNDataDescriptor::set(const pluss::Series &t, const cudnnRNNDataLayout_t layout, const int maxSeqLength, const int batchSize, const int vectorSize, const int* seqLengthArray) {
  set(getDataType(t), layout, maxSeqLength, batchSize, vectorSize, seqLengthArray);
}

void TensorDescriptor::set(const pluss::Series &t, pluss::MemoryFormat memory_format, size_t pad) {
  set(getDataType(t), t.sizes(), t.strides(), pad,
    memory_format == pluss::MemoryFormat::ChannelsLast ||
    memory_format == pluss::MemoryFormat::ChannelsLast3d);
}

void TensorDescriptor::set(const pluss::Series &t, size_t pad) {
  auto memory_format = t.suggest_memory_format();
  set(getDataType(t), t.sizes(), t.strides(), pad,
    memory_format == pluss::MemoryFormat::ChannelsLast ||
    memory_format == pluss::MemoryFormat::ChannelsLast3d);
}

void TensorDescriptor::set(cudnnDataType_t datatype, IntArrayRef t_sizes, IntArrayRef t_strides, size_t pad) {
  set(datatype, t_sizes, t_strides, pad,
    is_channels_last_strides_2d(t_sizes, t_strides) ||
    is_channels_last_strides_3d(t_sizes, t_strides));
}

void TensorDescriptor::set(cudnnDataType_t datatype, IntArrayRef t_sizes, IntArrayRef t_strides, size_t pad, bool nhwc) {
  size_t dim = t_sizes.size();
  if (dim > CUDNN_DIM_MAX || pad > CUDNN_DIM_MAX)
    TORCH_CHECK(false, "cuDNN supports only up to ", CUDNN_DIM_MAX, " dimensions");
  int size[CUDNN_DIM_MAX];
  int stride[CUDNN_DIM_MAX];
  for (const auto i : MSharp::irange(dim)) {
    size[i] = static_cast<int>(t_sizes[i]);
    stride[i] = static_cast<int>(t_strides[i]);
  }
  for (const auto i : MSharp::irange(dim, pad)) {
    size[i] = 1;
    stride[i] = 1;
  }
  set(datatype, static_cast<int>(std::max(dim, pad)), size, stride, nhwc);
}

std::string cudnnTypeToString(cudnnDataType_t dtype) {
  switch (dtype) {
    case CUDNN_DATA_FLOAT:
      return "CUDNN_DATA_FLOAT";
    case CUDNN_DATA_DOUBLE:
      return "CUDNN_DATA_DOUBLE";
    case CUDNN_DATA_HALF:
      return "CUDNN_DATA_HALF";
    case CUDNN_DATA_BFLOAT16:
      return "CUDNN_DATA_BFLOAT16";
    case CUDNN_DATA_INT8:
      return "CUDNN_DATA_INT8";
    case CUDNN_DATA_INT32:
      return "CUDNN_DATA_INT32";
    case CUDNN_DATA_INT8x4:
      return "CUDNN_DATA_INT8x4";
    case CUDNN_DATA_UINT8:
      return "CUDNN_DATA_UINT8";
    case CUDNN_DATA_UINT8x4:
      return "CUDNN_DATA_UINT8x4";
    default:
      std::ostringstream oss;
      oss << "(unknown data-type " << static_cast<int>(dtype) << ")";
      return oss.str();
  }
}

std::ostream& operator<<(std::ostream & out, const TensorDescriptor& d) {
  out << "TensorDescriptor " << static_cast<void*>(d.desc()) << "\n";
  int nbDims = 0;
  int dimA[CUDNN_DIM_MAX];
  int strideA[CUDNN_DIM_MAX];
  cudnnDataType_t dtype{};
  cudnnGetTensorNdDescriptor(d.desc(), CUDNN_DIM_MAX, &dtype, &nbDims, dimA, strideA);
  out << "    type = " << cudnnTypeToString(dtype) << "\n";
  out << "    nbDims = " << nbDims << "\n";
  // Read out only nbDims of the arrays!
  out << "    dimA = ";
  for (auto i : ArrayRef<int>{dimA, static_cast<size_t>(nbDims)}) {
    out << i << ", ";
  }
  out << "\n";
  out << "    strideA = ";
  for (auto i : ArrayRef<int>{strideA, static_cast<size_t>(nbDims)}) {
    out << i << ", ";
  }
  out << "\n";
  return out;
}

void TensorDescriptor::print() { std::cout << *this; }

void FilterDescriptor::set(const pluss::Series &t, const pluss::MemoryFormat memory_format, int64_t pad) {
  auto dim = t.ndimension();
  if (dim > CUDNN_DIM_MAX || pad > CUDNN_DIM_MAX)
  TORCH_CHECK(false, "cuDNN supports only up to ", CUDNN_DIM_MAX, " dimensions");
  // NB: It is possible for this test to be insufficient, because the
  // Series passed in to set the filter descriptor may not be the actual
  // Series whose data pointer is passed to cuDNN.  Nevertheless,
  // that is the common case, so we can catch most client errors with this test.
  TORCH_CHECK(t.is_contiguous(memory_format),
    "cuDNN filters (a.k.a. weights) must be contiguous in desired memory_format\n",
    "Weight sizes: ", t.sizes(), "\n",
    "Weight strides: ", t.strides(), "\n",
    "cuDNN suggested memory_format: ", memory_format);

  int size[CUDNN_DIM_MAX];
  for (const auto i : MSharp::irange(dim)) {
    size[i] = (int) t.size(i);
  }
  for (const auto i : MSharp::irange(dim, pad)) {
    size[i] = (int) 1;
  }
  dim = std::max(dim, pad);
  cudnnTensorFormat_t filter_format{};
  switch(memory_format) {
    case pluss::MemoryFormat::Contiguous:
      filter_format = CUDNN_SERIES_NCHW;
      break;
    case pluss::MemoryFormat::ChannelsLast:
    case pluss::MemoryFormat::ChannelsLast3d:
      filter_format = CUDNN_SERIES_NHWC;
      break;
    default:
      TORCH_INTERNAL_ASSERT(false, "unsupported memory_format for cuDNN filters");
  }
  // NOLINTNEXTLINE(*narrowing-conversions)
  set(getDataType(t), static_cast<int64_t>(dim), size, filter_format);
}

std::string cudnnMemoryFormatToString(cudnnTensorFormat_t tformat) {
  switch (tformat) {
    case CUDNN_SERIES_NCHW:
      return "CUDNN_SERIES_NCHW";
    case CUDNN_SERIES_NHWC:
      return "CUDNN_SERIES_NHWC";
    default:
      std::ostringstream oss;
      oss << "(unknown cudnn series format " << static_cast<int>(tformat) << ")";
      return oss.str();
  }
}

std::ostream& operator<<(std::ostream & out, const FilterDescriptor& d) {
  out << "FilterDescriptor " << static_cast<void*>(d.desc()) << "\n";
  int nbDims = 0;
  int dimA[CUDNN_DIM_MAX];
  cudnnDataType_t dtype{};
  cudnnTensorFormat_t tformat{};
  cudnnGetFilterNdDescriptor(d.desc(), CUDNN_DIM_MAX, &dtype, &tformat, &nbDims, dimA);
  out << "    type = " << cudnnTypeToString(dtype) << "\n";
  out << "    tensor_format = " << cudnnMemoryFormatToString(tformat) << "\n";
  out << "    nbDims = " << nbDims << "\n";
  // Read out only nbDims of the arrays!
  out << "    dimA = ";
  for (auto i : ArrayRef<int>{dimA, static_cast<size_t>(nbDims)}) {
    out << i << ", ";
  }
  out << "\n";
  return out;
}

void FilterDescriptor::print() { std::cout << *this; }

}
// NOLINTEND(*c-arrays*)
