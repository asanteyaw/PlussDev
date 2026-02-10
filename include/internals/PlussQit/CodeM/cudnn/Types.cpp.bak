#include <PlussQit/cudnn/Types.h>

#include <PlussQit/PlussQit.h>

namespace pluss::native {

cudnnDataType_t getCudnnDataTypeFromScalarType(const pluss::ScalarType dtype) {
  if (dtype == MSharp::kQInt8 || dtype == pluss::kChar) {
    return CUDNN_DATA_INT8;
  } else if (dtype == pluss::kFloat) {
    return CUDNN_DATA_FLOAT;
  } else if (dtype == pluss::kDouble) {
    return CUDNN_DATA_DOUBLE;
  } else if (dtype == pluss::kHalf) {
    return CUDNN_DATA_HALF;
  } else if (dtype == pluss::kBFloat16) {
    return CUDNN_DATA_BFLOAT16;
  } else if (dtype == pluss::kInt) {
    return CUDNN_DATA_INT32;
  } else if (dtype == pluss::kByte) {
    return CUDNN_DATA_UINT8;
  }
  std::string msg("getCudnnDataTypeFromScalarType() not supported for ");
  msg += toString(dtype);
  throw std::runtime_error(msg);
}

cudnnDataType_t getCudnnDataType(const pluss::Series& series) {
  return getCudnnDataTypeFromScalarType(series.scalar_type());
}

int64_t cudnn_version() {
  return CUDNN_VERSION;
}

} // namespace pluss::native
