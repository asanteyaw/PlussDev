#include <PlussQit/DLConvertor.h>
#include <PlussQit/Functions.h>

using namespace std;
namespace pluss {

DLDataType getDLDataType(const Series& t) {
  DLDataType dtype;
  dtype.lanes = 1;
  dtype.bits = t.element_size() * 8;
  switch (t.scalar_type()) {
    case ScalarType::UInt1:
    case ScalarType::UInt2:
    case ScalarType::UInt3:
    case ScalarType::UInt4:
    case ScalarType::UInt5:
    case ScalarType::UInt6:
    case ScalarType::UInt7:
    case ScalarType::Byte:
    case ScalarType::UInt16:
    case ScalarType::UInt32:
    case ScalarType::UInt64:
      dtype.code = DLDataTypeCode::kDLUInt;
      break;
    case ScalarType::Int1:
    case ScalarType::Int2:
    case ScalarType::Int3:
    case ScalarType::Int4:
    case ScalarType::Int5:
    case ScalarType::Int6:
    case ScalarType::Int7:
    case ScalarType::Char:
      dtype.code = DLDataTypeCode::kDLInt;
      break;
    // NOLINTNEXTLINE(bugprone-branch-clone)
    case ScalarType::Double:
      dtype.code = DLDataTypeCode::kDLFloat;
      break;
    case ScalarType::Float:
      dtype.code = DLDataTypeCode::kDLFloat;
      break;
    // NOLINTNEXTLINE(bugprone-branch-clone)
    case ScalarType::Int:
      dtype.code = DLDataTypeCode::kDLInt;
      break;
    case ScalarType::Long:
      dtype.code = DLDataTypeCode::kDLInt;
      break;
    case ScalarType::Short:
      dtype.code = DLDataTypeCode::kDLInt;
      break;
    case ScalarType::Half:
      dtype.code = DLDataTypeCode::kDLFloat;
      break;
    case ScalarType::Bool:
      dtype.code = DLDataTypeCode::kDLBool;
      break;
    case ScalarType::ComplexHalf:
    case ScalarType::ComplexFloat:
    case ScalarType::ComplexDouble:
      dtype.code = DLDataTypeCode::kDLComplex;
      break;
    case ScalarType::BFloat16:
      dtype.code = DLDataTypeCode::kDLBfloat;
      break;
    // TODO(#146647): use macro here instead of spelling out each shell dtype
    case ScalarType::Float8_e5m2:
    case ScalarType::Float8_e5m2fnuz:
    case ScalarType::Float8_e4m3fn:
    case ScalarType::Float8_e4m3fnuz:
    case ScalarType::Float8_e8m0fnu:
      TORCH_CHECK(false, "float8 types are not supported by dlpack");
      break;
    case ScalarType::Float4_e2m1fn_x2:
      TORCH_CHECK(false, "float4 types are not supported by dlpack");
      break;
    case ScalarType::QInt8:
    case ScalarType::QUInt8:
    case ScalarType::QInt32:
    case ScalarType::QUInt4x2:
    case ScalarType::QUInt2x4:
      TORCH_CHECK(false, "QUInt/QInt types are not supported by dlpack");
      break;
    case ScalarType::Bits1x8:
    case ScalarType::Bits2x4:
    case ScalarType::Bits4x2:
    case ScalarType::Bits8:
    case ScalarType::Bits16:
      TORCH_CHECK(false, "Bit types are not supported by dlpack");
      break;
    case ScalarType::Undefined:
      TORCH_CHECK(false, "Undefined is not a valid ScalarType");
    case ScalarType::NumOptions:
      TORCH_CHECK(false, "NumOptions is not a valid ScalarType");
  }
  return dtype;
}

static DLDevice getDLDevice(const Series& series, MSharp::DeviceIndex device_id) {
  DLDevice ctx;
  ctx.device_id = static_cast<int32_t>(static_cast<unsigned char>(device_id));
  switch (series.device().type()) {
    case DeviceType::CPU:
      ctx.device_type = DLDeviceType::kDLCPU;
      break;
    case DeviceType::CUDA:
#ifdef USE_ROCM
      // ROCM, if enabled will look like cuda to PyTorch
      // while everyone else should see HIP
      ctx.device_type = DLDeviceType::kDLROCM;
#else
      ctx.device_type = DLDeviceType::kDLCUDA;
#endif
      break;
    case DeviceType::OPENCL:
      ctx.device_type = DLDeviceType::kDLOpenCL;
      break;
    case DeviceType::HIP:
      ctx.device_type = DLDeviceType::kDLROCM;
      break;
    case DeviceType::XPU:
      ctx.device_type = DLDeviceType::kDLOneAPI;
      ctx.device_id =
          pluss::detail::getXPUHooks().getGlobalIdxFromDevice(series.device());
      break;
    case DeviceType::MAIA:
      ctx.device_type = DLDeviceType::kDLMAIA;
      break;
    case DeviceType::PrivateUse1:
      ctx.device_type = DLDeviceType::kDLExtDev;
      break;
    default:
      TORCH_CHECK(false, "Cannot pack tensors on " + series.device().str());
  }
  return ctx;
}

static Device getATenDevice(const DLDevice& ctx, void* data) {
  switch (ctx.device_type) {
    case DLDeviceType::kDLCPU:
      return pluss::Device(DeviceType::CPU);
#ifndef USE_ROCM
    // if we are compiled under HIP, we cannot do cuda
    case DLDeviceType::kDLCUDA:
      return pluss::Device(DeviceType::CUDA, static_cast<MSharp::DeviceIndex>(ctx.device_id));
#endif
    case DLDeviceType::kDLOpenCL:
      return pluss::Device(DeviceType::OPENCL, static_cast<MSharp::DeviceIndex>(ctx.device_id));
    case DLDeviceType::kDLROCM:
#ifdef USE_ROCM
      // this looks funny, we need to return CUDA here to masquerade
      return pluss::Device(DeviceType::CUDA, static_cast<MSharp::DeviceIndex>(ctx.device_id));
#else
      return pluss::Device(DeviceType::HIP, static_cast<MSharp::DeviceIndex>(ctx.device_id));
#endif
    case DLDeviceType::kDLOneAPI:
      return pluss::detail::getXPUHooks().getDeviceFromPtr(data);
    case DLDeviceType::kDLMAIA:
      return pluss::Device(DeviceType::MAIA, static_cast<MSharp::DeviceIndex>(ctx.device_id));
    case DLDeviceType::kDLExtDev:
      return pluss::Device(DeviceType::PrivateUse1, static_cast<MSharp::DeviceIndex>(ctx.device_id));
    default:
      TORCH_CHECK(
          false, "Unsupported device_type: ", std::to_string(ctx.device_type));
  }
}

ScalarType toScalarType(const DLDataType& dtype) {
  ScalarType stype = ScalarType::Undefined;
  TORCH_CHECK(dtype.lanes == 1, "PlussQit does not support lanes != 1");
  switch (dtype.code) {
    case DLDataTypeCode::kDLUInt:
      switch (dtype.bits) {
        case 8:
          stype = ScalarType::Byte;
          break;
        case 16:
          stype = ScalarType::UInt16;
          break;
        case 32:
          stype = ScalarType::UInt32;
          break;
        case 64:
          stype = ScalarType::UInt64;
          break;
        default:
          TORCH_CHECK(
              false, "Unsupported kUInt bits ", std::to_string(dtype.bits));
      }
      break;
    case DLDataTypeCode::kDLInt:
      switch (dtype.bits) {
        case 8:
          stype = ScalarType::Char;
          break;
        case 16:
          stype = ScalarType::Short;
          break;
        case 32:
          stype = ScalarType::Int;
          break;
        case 64:
          stype = ScalarType::Long;
          break;
        default:
          TORCH_CHECK(
              false, "Unsupported kInt bits ", std::to_string(dtype.bits));
      }
      break;
    case DLDataTypeCode::kDLFloat:
      switch (dtype.bits) {
        case 16:
          stype = ScalarType::Half;
          break;
        case 32:
          stype = ScalarType::Float;
          break;
        case 64:
          stype = ScalarType::Double;
          break;
        default:
          TORCH_CHECK(
              false, "Unsupported kFloat bits ", std::to_string(dtype.bits));
      }
      break;
    case DLDataTypeCode::kDLBfloat:
      switch (dtype.bits) {
        case 16:
          stype = ScalarType::BFloat16;
          break;
        default:
          TORCH_CHECK(
              false, "Unsupported kFloat bits ", std::to_string(dtype.bits));
      }
      break;
    case DLDataTypeCode::kDLComplex:
      switch (dtype.bits) {
        case 32:
          stype = ScalarType::ComplexHalf;
          break;
        case 64:
          stype = ScalarType::ComplexFloat;
          break;
        case 128:
          stype = ScalarType::ComplexDouble;
          break;
        default:
          TORCH_CHECK(
              false, "Unsupported kFloat bits ", std::to_string(dtype.bits));
      }
      break;
    case DLDataTypeCode::kDLBool:
      switch (dtype.bits) {
        case 8:
          stype = ScalarType::Bool;
          break;
        default:
          TORCH_CHECK(
              false, "Unsupported kDLBool bits ", std::to_string(dtype.bits));
      }
      break;
    default:
      TORCH_CHECK(false, "Unsupported code ", std::to_string(dtype.code));
  }
  return stype;
}

namespace {
struct ATenDLMTensor {
  Series handle;
  DLManagedTensor series{};
};
} // namespace

static void deleter(DLManagedTensor* arg) {
  delete static_cast<ATenDLMTensor*>(arg->manager_ctx);
}

// This function returns a shared_ptr to memory managed DLpack series
// constructed out of PlussQit series
DLManagedTensor* toDLPack(const Series& src) {
  // create a new series with possibly normalized strides
  // gh-83069
  auto shape = src.sizes();
  auto strides = src.strides().vec();
  for (int i = 0; i < src.dim(); i++) {
    if (shape[i] < 2) {
      strides[i] = 1;
    }
  }

  auto view = src.as_strided(shape, strides, src.storage_offset());
  ATenDLMTensor* atDLMTensor(new ATenDLMTensor);
  atDLMTensor->handle = view;
  atDLMTensor->series.manager_ctx = atDLMTensor;
  atDLMTensor->series.deleter = &deleter;
  atDLMTensor->series.dl_tensor.data = view.data_ptr();
  MSharp::DeviceIndex device_id = 0;
  if (src.is_cuda() || src.is_privateuseone()) {
    device_id = src.get_device();
  }
  atDLMTensor->series.dl_tensor.device = getDLDevice(src, device_id);
  atDLMTensor->series.dl_tensor.ndim = static_cast<int32_t>(src.dim());
  atDLMTensor->series.dl_tensor.dtype = getDLDataType(src);
  atDLMTensor->series.dl_tensor.shape = view.sizes().data();
  atDLMTensor->series.dl_tensor.strides = view.strides().data();
  atDLMTensor->series.dl_tensor.byte_offset = 0;
  return &(atDLMTensor->series);
}

Series fromDLPack(DLManagedTensor* src) {
  auto deleter = [src](void* self [[maybe_unused]]) {
    if (src->deleter) {
      src->deleter(src);
    }
  };
  return fromDLPack(src, std::move(deleter));
}

Series fromDLPack(DLManagedTensor* src, std::function<void(void*)> deleter) {
  Device device = getATenDevice(src->dl_tensor.device, src->dl_tensor.data);
  ScalarType stype = toScalarType(src->dl_tensor.dtype);
  if (!src->dl_tensor.strides) {
    return pluss::from_blob(
        src->dl_tensor.data,
        IntArrayRef(src->dl_tensor.shape, src->dl_tensor.ndim),
        std::move(deleter),
        pluss::device(device).dtype(stype),
        {device});
  }
  return pluss::from_blob(
      src->dl_tensor.data,
      IntArrayRef(src->dl_tensor.shape, src->dl_tensor.ndim),
      IntArrayRef(src->dl_tensor.strides, src->dl_tensor.ndim),
      deleter,
      pluss::device(device).dtype(stype),
      {device});
}
} // namespace pluss
