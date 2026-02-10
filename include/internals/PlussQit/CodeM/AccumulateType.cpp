#include <PlussQit/AccumulateType.h>

namespace pluss {

MSharp::ScalarType toAccumulateType(MSharp::ScalarType type, MSharp::DeviceType device) {
  switch (type) {
#define DEFINE_CASE(scalar_t, TypeNum)                                                             \
    case ScalarType::TypeNum:                                                                      \
      switch (device) {                                                                            \
        case DeviceType::CUDA:                                                                     \
          return CppTypeToScalarType<pluss::acc_type_device<scalar_t, MSharp::DeviceType::CUDA>>::value; \
        case DeviceType::XPU:                                                                      \
          return CppTypeToScalarType<pluss::acc_type_device<scalar_t, MSharp::DeviceType::XPU>>::value;  \
        case DeviceType::MPS:                                                                      \
          return CppTypeToScalarType<pluss::acc_type_device<scalar_t, MSharp::DeviceType::MPS>>::value;  \
        default:                                                                                   \
          return CppTypeToScalarType<pluss::acc_type_device<scalar_t, MSharp::DeviceType::CPU>>::value;  \
      }

    AT_FORALL_SCALAR_TYPES_WITH_COMPLEX_EXCEPT_COMPLEX_HALF_F8NZ(DEFINE_CASE)
#undef DEFINE_CASE

    default: TORCH_INTERNAL_ASSERT(false, "Unrecognized ScalarType: ", type);
  }
}

MSharp::ScalarType toAccumulateType(MSharp::ScalarType type, bool is_cuda) {
  return is_cuda ? toAccumulateType(type, MSharp::DeviceType::CUDA) : toAccumulateType(type, MSharp::DeviceType::CPU);
}

}
