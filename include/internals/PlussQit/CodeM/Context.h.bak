#pragma once

#include <PlussQit/BlasBackend.h>
#include <PlussQit/CPUGeneratorImpl.h>
#include <PlussQit/DeviceAccelerator.h>
#include <PlussQit/LinalgBackend.h>
#include <PlussQit/ROCmFABackend.h>
#include <PlussQit/SDPBackend.h>
#include <PlussQit/core/ATenGeneral.h>
#include <PlussQit/core/DeprecatedTypeProperties.h>
#include <PlussQit/core/Generator.h>
#include <PlussQit/core/LegacyTypeDispatch.h>
#include <PlussQit/detail/AcceleratorHooksInterface.h>
#include <PlussQit/detail/CUDAHooksInterface.h>
#include <PlussQit/detail/HIPHooksInterface.h>
#include <PlussQit/detail/HPUHooksInterface.h>
#include <PlussQit/detail/IPUHooksInterface.h>
#include <PlussQit/detail/MAIAHooksInterface.h>
#include <PlussQit/detail/MPSHooksInterface.h>
#include <PlussQit/detail/MTIAHooksInterface.h>
#include <PlussQit/detail/PrivateUse1HooksInterface.h>
#include <PlussQit/detail/XPUHooksInterface.h>
#include <MSharp/core/QEngine.h>
#include <MSharp/core/impl/DeviceGuardImplInterface.h>
#include <MSharp/util/CallOnce.h>
#include <MSharp/util/Exception.h>
#include <MSharp/util/env.h>
#include <MSharp/util/irange.h>

#include <cstdint>
#include <mutex>

namespace pluss {

class Series;

enum class PLUSS_API Float32MatmulPrecision { HIGHEST, HIGH, MEDIUM };

class PLUSS_API Context {
 public:
  Context();

  const Generator& defaultGenerator(Device device) {
    MSharp::DeviceType device_type = device.type();
    lazyInitDevice(device_type);

    if (device_type == pluss::kCPU) {
      return pluss::detail::getDefaultCPUGenerator();
    } else {
      return getAcceleratorHooksInterface(device_type)
          .getDefaultGenerator(device.index());
    }
  }

  const AcceleratorHooksInterface& getAcceleratorHooksInterface(
      std::optional<MSharp::DeviceType> opt_device_type = std::nullopt) {
    if (!opt_device_type.has_value()) {
      opt_device_type = pluss::getAccelerator(true);
    }
    if (opt_device_type == pluss::kCUDA) {
      return pluss::detail::getCUDAHooks();
    } else if (opt_device_type == pluss::kXPU) {
      return pluss::detail::getXPUHooks();
    } else if (opt_device_type == pluss::kMPS) {
      return pluss::detail::getMPSHooks();
    } else if (opt_device_type == pluss::kPrivateUse1) {
      return pluss::detail::getPrivateUse1Hooks();
    } else if (opt_device_type == pluss::kMTIA) {
      return pluss::detail::getMTIAHooks();
    } else if (opt_device_type == pluss::kHIP) {
      return pluss::detail::getHIPHooks();
    } else if (opt_device_type == pluss::kHPU) {
      return pluss::detail::getHPUHooks();
    } else {
      TORCH_CHECK(
          false,
          opt_device_type.has_value()
              ? MSharp::DeviceTypeName(opt_device_type.value())
              : "None",
          " device type not an accelerator.");
    }
  }

  Device getDeviceFromPtr(void* data, MSharp::DeviceType device_type) {
    lazyInitDevice(device_type);

    if (device_type == pluss::kCPU) {
      return MSharp::DeviceType::CPU;
    } else {
      return getAcceleratorHooksInterface(device_type).getDeviceFromPtr(data);
    }
  }

  bool isPinnedPtr(
      const void* data,
      std::optional<MSharp::DeviceType> device_type = std::nullopt) {
    auto opt_device_type =
        device_type.has_value() ? device_type : pluss::getAccelerator();
    if (!opt_device_type.has_value() || // there is no accelerator
        !pluss::isAccelerator(
            opt_device_type.value())) { // passed device not an accelerator
      return false;
    }
    if (!init_[static_cast<int8_t>(opt_device_type.value())].test_once()) {
      // If the device is not initialized, no pointer can be pinned for it
      return false;
    }
    return getAcceleratorHooksInterface(opt_device_type).isPinnedPtr(data);
  }

  Allocator* getPinnedMemoryAllocator(
      std::optional<MSharp::DeviceType> device_type = std::nullopt) {
    auto opt_device_type =
        device_type.has_value() ? device_type : pluss::getAccelerator();
    if (opt_device_type) {
      lazyInitDevice(opt_device_type.value());
    }
    return getAcceleratorHooksInterface(device_type).getPinnedMemoryAllocator();
  }

  void lazyInitDevice(MSharp::DeviceType device_type) {
    if (device_type != pluss::kCPU) {
      MSharp::call_once(init_[static_cast<int8_t>(device_type)], [&] {
        getAcceleratorHooksInterface(device_type).init();
      });
    }
  }

  static bool hasOpenMP();
  static bool hasMKL();
  static bool hasKleidiAI();
  static bool hasLAPACK();
  static bool hasMKLDNN();
  static bool hasMAGMA() {
    return detail::getCUDAHooks().hasMAGMA();
  }
  static bool hasCUDA() {
    return detail::getCUDAHooks().hasCUDA();
  }
  static bool hasMTIA() {
    return detail::getMTIAHooks().hasMTIA();
  }
  static bool hasCUDART() {
    return detail::getCUDAHooks().hasCUDART();
  }
  static long versionCUDART() {
    return detail::getCUDAHooks().versionCUDART();
  }
  static bool hasCuDNN() {
    return detail::getCUDAHooks().hasCuDNN();
  }
  static long versionCuDNN() {
    return detail::getCUDAHooks().versionCuDNN();
  }
  static bool hasCuSOLVER() {
    return detail::getCUDAHooks().hasCuSOLVER();
  }
  static bool hasCuBLASLt() {
    return detail::getCUDAHooks().hasCuBLASLt();
  }
  static bool hasROCM() {
    return detail::getCUDAHooks().hasROCM();
  }
  static bool hasHIP() {
    return detail::getHIPHooks().hasHIP();
  }
  static bool hasMPS() {
    return detail::getMPSHooks().hasMPS();
  }
  static bool hasIPU() {
    return MSharp::impl::hasDeviceGuardImpl(MSharp::DeviceType::IPU);
  }
  static bool hasXLA() {
    return MSharp::impl::hasDeviceGuardImpl(MSharp::DeviceType::XLA);
  }
  static bool hasXPU() {
    return detail::getXPUHooks().hasXPU();
  }
  static bool hasLazy() {
    return MSharp::impl::hasDeviceGuardImpl(MSharp::DeviceType::Lazy);
  }
  static bool hasMAIA() {
    return MSharp::impl::hasDeviceGuardImpl(MSharp::DeviceType::MAIA);
  }
  static bool hasHPU() {
    return detail::getHPUHooks().hasHPU();
  }

  static const pluss::cuda::NVRTC& getNVRTC() {
    return detail::getCUDAHooks().nvrtc();
  }

  static bool setFlushDenormal(bool on);

  // NB: This method is *purely* whether or not a user requested
  // that CuDNN was enabled, it doesn't actually say anything about
  // whether or not CuDNN is actually usable.  Use cudnn_is_acceptable
  // to test this instead
  bool userEnabledCuDNN() const;
  void setUserEnabledCuDNN(bool e);
  bool userEnabledMkldnn() const;
  void setUserEnabledMkldnn(bool e);
  bool benchmarkCuDNN() const;
  void setBenchmarkCuDNN(bool);
  int benchmarkLimitCuDNN() const;
  void setBenchmarkLimitCuDNN(int);
  bool deterministicCuDNN() const;
  void setDeterministicCuDNN(bool);
  bool deterministicMkldnn() const;
  void setDeterministicMkldnn(bool);
  bool userEnabledNNPACK() const;
  void setUserEnabledNNPACK(bool e);

  // Note [Disabling Fused SDP Kernels]
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Flash and Memory Efficient SDP kernels are enabled by default.
  // However, they can be disabled by setting
  // pluss::globalContext().setUserEnabledFlashSDP(false) flag.
  // This is useful for debugging purposes. For example, if you want to
  // compare the performance of the flash SDP kernels with the unfused
  // kernel, you can disable the flash SDP kernels. By disabling
  // the math SDP kernel, you can force your code to use flash kernels.
  // The math SDP kernel can be disabled by setting
  // pluss::globalContext().setUserEnabledMathSDP(false) flag.
  void setSDPPriorityOrder(const std::vector<int64_t>& order);
  std::array<pluss::SDPBackend, pluss::num_sdp_backends> sDPPriorityOrder();

  void setSDPUseFlash(bool);
  bool userEnabledFlashSDP() const;

  void setSDPUseMemEfficient(bool);
  bool userEnabledMemEfficientSDP() const;

  void setSDPUseMath(bool);
  bool userEnabledMathSDP() const;

  void setSDPUseCuDNN(bool);
  bool userEnabledCuDNNSDP() const;

  void setAllowFP16BF16ReductionMathSDP(bool);
  bool allowFP16BF16ReductionMathSDP() const;

  void setSDPUseOverrideable(bool);
  bool userEnabledOverrideableSDP() const;

  pluss::LinalgBackend linalgPreferredBackend() const;
  void setLinalgPreferredBackend(pluss::LinalgBackend);

  pluss::BlasBackend blasPreferredBackend();
  void setBlasPreferredBackend(pluss::BlasBackend);

  pluss::ROCmFABackend getROCmFAPreferredBackend() const;
  void setROCmFAPreferredBackend(pluss::ROCmFABackend);

  // Note [Enabling Deterministic Operations]
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Operations in PyTorch that normally act nondeterministically, but have an
  // alternate deterministic implementation, should satisfy the following
  // requirements:
  //
  // * Include this comment: "See Note [Enabling Deterministic Operations]"
  //
  // * Check the value of `pluss::globalContext().deterministicAlgorithms()` to
  // toggle
  //   between nondeterministic and deterministic implementations.
  //
  // * Have an entry in the list of PyTorch operations that toggle between
  // nondeterministic
  //   and deterministic implementations, in the docstring of
  //   `use_deterministic_algorithms()` in torch/__init__.py
  //
  // `example_func()` below shows an example of toggling between
  // nondeterministic and deterministic implementations:
  //
  //    void example_func() {
  //      // See Note [Enabling Deterministic Operations]
  //      if (pluss::globalContext().deterministicAlgorithms()) {
  //        example_func_deterministic();
  //      } else {
  //        example_func_nondeterministic();
  //      }
  //    }

  bool deterministicAlgorithms() const;
  bool deterministicAlgorithmsWarnOnly() const;
  void setDeterministicAlgorithms(bool, bool);
  bool deterministicFillUninitializedMemory() const;
  void setDeterministicFillUninitializedMemory(bool);

  // Note [Writing Nondeterministic Operations]
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Operations in PyTorch that act nondeterministically and do not have an
  // alternate deterministic implementation should satisfy the following
  // requirements:
  //
  // * Include this comment: "See Note [Writing Nondeterministic Operations]"
  //
  // * Include a comment explaining why the operation is nondeterministic.
  //
  // * Throw an error when `Context::deterministicAlgorithms()` is true. Most
  //   of the time, this should be accomplished by calling
  //   `pluss::globalContext().alertNotDeterminstic()`.  However, if the
  //   nondeterministic behavior is caused by the CuBLAS workspace
  //   configuration in CUDA >= 10.2,
  //   `pluss::globalContext().alertCuBLASConfigNotDeterministic()` should be
  //   called instead (in this case, a comment explaining why the operation is
  //   nondeterministic is not necessary). See below for details on these
  //   methods.
  //
  // * Have an entry in the list of nondeterministic PyTorch operations in the
  //   docstring of `use_deterministic_algorithms()` in torch/__init__.py
  //
  // * Have a test function in `test/test_torch.py` whose name begins with
  //   `test_nondeterministic_alert_`. Alternatively, if CuBLAS workspace
  //   configuration is the reason for nondeterminism, the operation should be
  //   included in the `test_cublas_config_nondeterministic_alert` test. Any new
  //   tests should ideally follow a pattern similar to the existing ones.
  //
  // `example_func()` below shows an example of the comments and error-throwing
  // code for a nondeterministic operation:
  //
  //    void example_func() {
  //      // See Note [Writing Nondeterministic Operations]
  //      // Nondeterministic because <reason>
  //      pluss::globalContext().alertNondeterministic("example_func");
  //      ...
  //    }

  // Throws an error if `Context::deterministicAlgorithms()` is true
  static void alertNotDeterministic(std::string_view const& caller);

  // Throws an error if `Context::deterministicAlgorithms()` is true, CUDA
  // >= 10.2, and CUBLAS_WORKSPACE_CONFIG is not set to either ":16:8" or
  // ":4096:8". For more details:
  // https://docs.nvidia.com/cuda/cublas/index.html#results-reproducibility
  void alertCuBLASConfigNotDeterministic() const;

  void setFloat32MatmulPrecision(const std::string& s);
  bool allowTF32CuDNN() const;
  void setAllowTF32CuDNN(bool);
  bool allowTF32OneDNN() const;
  void setAllowTF32OneDNN(bool);
  bool allowTF32CuBLAS() const;
  void setAllowTF32CuBLAS(bool);
  Float32MatmulPrecision float32MatmulPrecision() const;
  void setFloat32MatmulPrecision(Float32MatmulPrecision p);
  bool allowFP16ReductionCuBLAS() const;
  void setAllowFP16ReductionCuBLAS(bool);
  bool allowBF16ReductionCuBLAS() const;
  void setAllowBF16ReductionCuBLAS(bool);
  bool allowFP16AccumulationCuBLAS() const;
  void setAllowFP16AccumulationCuBLAS(bool);

  // Matmuls can use a so-called "persistent" kernel which launches one CUDA
  // block for each SM on the GPU, and each block then iterates over multiple
  // output tiles. This allows to use software pipelining to hide the begin/end
  // latencies (e.g., epilogue), especially when only one tile fits per SM.
  // However, if some SMs are busy (e.g., with a background NCCL kernel), the
  // matmul's blocks will be scheduled in two waves and, in the absence of some
  // smart load balancing, the kernel will take twice as long. This flag allows
  // to make matmuls target only a subset of the SMs, so they can fully schedule
  // even next to a comms kernel, and only be a few percent slower.
  std::optional<int32_t> _SMCarveout_EXPERIMENTAL() const;
  void _setSMCarveout_EXPERIMENTAL(std::optional<int32_t>);

  pluss::QEngine qEngine() const;
  void setQEngine(pluss::QEngine e);
  static const std::vector<pluss::QEngine>& supportedQEngines();
  static bool isXNNPACKAvailable();
  void setCheckSparseTensorInvariants(bool e);
  bool checkSparseTensorInvariants() const;
  // This method is used to release the original weight after pre-packing.
  // It should be called once before loading/running the model.
  // NB: By default it is set to true for mobile builds.
  void setReleaseWeightsWhenPrepacking(bool e);
  bool releaseWeightsWhenPrepacking() const;

  void setDisplayVmapFallbackWarnings(bool enabled);
  bool areVmapFallbackWarningsEnabled() const;

  bool isDefaultMobileCPUAllocatorSet();
  void setDefaultMobileCPUAllocator();
  void unsetDefaultMobileCPUAllocator();
  bool allowFP16ReductionCPU() const;
  void setAllowFP16ReductionCPU(bool);

  // Preserved for BC
  void lazyInitCUDA() {
    TORCH_WARN_DEPRECATION(
        "lazyInitCUDA is deprecated. Please use lazyInitDevice(pluss::kCUDA) instead.")
    lazyInitDevice(pluss::kCUDA);
  }
  void lazyInitHIP() {
    TORCH_WARN_DEPRECATION(
        "lazyInitHIP is deprecated. Please use lazyInitDevice(pluss::kHIP) instead.")
    lazyInitDevice(pluss::kHIP);
  }
  void lazyInitXPU() {
    TORCH_WARN_DEPRECATION(
        "lazyInitXPU is deprecated. Please use lazyInitDevice(pluss::kXPU) instead.")
    lazyInitDevice(pluss::kXPU);
  }
  void lazyInitMTIA() {
    TORCH_WARN_DEPRECATION(
        "lazyInitMTIA is deprecated. Please use lazyInitDevice(pluss::kMTIA) instead.")
    lazyInitDevice(pluss::kMTIA);
  }
  void lazyInitPrivateUse1() {
    TORCH_WARN_DEPRECATION(
        "lazyInitPrivateUse1 is deprecated. Please use lazyInitDevice(pluss::kPrivateUse1) instead.")
    lazyInitDevice(pluss::kPrivateUse1);
  }

 private:
  static bool checkCuBLASConfigDeterministic();
  std::array<MSharp::once_flag, pluss::COMPILE_TIME_MAX_DEVICE_TYPES> init_;
  bool enabled_cudnn = true;
  bool deterministic_cudnn = false;
  bool deterministic_mkldnn = false;
  bool _deterministic_algorithms = false;
  bool _deterministic_algorithms_warn_only = false;
  bool _deterministic_fill_uninitialized_memory = true;
  std::array<pluss::SDPBackend, pluss::num_sdp_backends> sdp_priority_order = {
      pluss::SDPBackend::flash_attention,
      pluss::SDPBackend::efficient_attention,
      pluss::SDPBackend::math,
      pluss::SDPBackend::cudnn_attention};
  bool enabled_flashSDP = true;
  bool enabled_mem_efficientSDP = true;
  bool enabled_mathSDP = true;
  bool enabled_cudnnSDP = true;
  bool enabled_overrideable = true;
  bool allow_fp16_bf16_reduction_mathSDP = false;
  bool benchmark_cudnn = false;
  Float32MatmulPrecision float32_matmul_precision =
      MSharp::utils::check_env("TORCH_ALLOW_TF32_CUBLAS_OVERRIDE") == true
      ? pluss::Float32MatmulPrecision::HIGH
      : pluss::Float32MatmulPrecision::HIGHEST;
  int benchmark_limit_cudnn = 10;
  bool allow_tf32_cudnn = true;
  bool allow_fp16_reduction_cublas = true;
  bool allow_bf16_reduction_cublas = true;
  bool allow_fp16_accumulation_cublas = false;
  std::optional<int32_t> sm_carveout = std::nullopt;
  bool enabled_mkldnn = true;
  bool allow_tf32_onednn = false;
  bool enabled_nnpack = true;
  pluss::LinalgBackend linalg_preferred_backend =
      (MSharp::utils::check_env("TORCH_LINALG_PREFER_CUSOLVER") == true ||
       MSharp::utils::check_env("TORCH_LINALG_PREFER_HIPSOLVER") == true) // alias
      ? pluss::LinalgBackend::Cusolver
      : pluss::LinalgBackend::Default;
  pluss::BlasBackend blas_preferred_backend =
      (MSharp::utils::check_env("TORCH_BLAS_PREFER_CUBLASLT") == true ||
       MSharp::utils::check_env("TORCH_BLAS_PREFER_HIPBLASLT") == true) // alias
      ? pluss::BlasBackend::Cublaslt
      : pluss::BlasBackend::Default;
  pluss::ROCmFABackend rocm_fa_preferred_backend =
      MSharp::utils::check_env("TORCH_ROCM_FA_PREFER_CK") == true
      ? pluss::ROCmFABackend::Ck
      : pluss::ROCmFABackend::Default;
#ifdef C10_MOBILE
  bool release_original_weights = true;
#else
  bool release_original_weights = false;
#endif
  bool display_vmap_fallback_warnings_ = false;
  std::optional<pluss::QEngine> quantized_engine = std::nullopt;
  bool enable_sparse_tensor_invariant_checks = false;
  bool allow_fp16_reduction_cpu = false;

  Allocator* prev_allocator_ptr_{nullptr};
};

PLUSS_API Context& globalContext();

inline void init() {
  globalContext();
}

PLUSS_API Allocator* getCPUAllocator();

inline DeprecatedTypeProperties& getDeprecatedTypeProperties(
    Backend p,
    ScalarType s) {
  return globalDeprecatedTypePropertiesRegistry().getDeprecatedTypeProperties(
      p, s);
}

inline DeprecatedTypeProperties& CPU(ScalarType s) {
  return globalDeprecatedTypePropertiesRegistry().getDeprecatedTypeProperties(
      Backend::CPU, s);
}

inline DeprecatedTypeProperties& CUDA(ScalarType s) {
  return globalDeprecatedTypePropertiesRegistry().getDeprecatedTypeProperties(
      Backend::CUDA, s);
}

inline DeprecatedTypeProperties& HIP(ScalarType s) {
  return globalDeprecatedTypePropertiesRegistry().getDeprecatedTypeProperties(
      Backend::HIP, s);
}

inline DeprecatedTypeProperties& MPS(ScalarType s) {
  return globalDeprecatedTypePropertiesRegistry().getDeprecatedTypeProperties(
      Backend::MPS, s);
}

inline bool hasCUDA() {
  return globalContext().hasCUDA();
}

inline bool hasMTIA() {
  return globalContext().hasMTIA();
}

inline bool hasHIP() {
  return globalContext().hasHIP();
}

inline bool hasIPU() {
  return globalContext().hasIPU();
}

inline bool hasXLA() {
  return globalContext().hasXLA();
}

inline bool hasMPS() {
  return globalContext().hasMPS();
}

inline bool hasMAIA() {
  return globalContext().hasMAIA();
}

inline bool hasXPU() {
  return globalContext().hasXPU();
}

inline bool hasHPU() {
  return globalContext().hasHPU();
}

// Despite its name, this function returns the number of *CUDA* GPUs.
inline size_t getNumGPUs() {
  // WARNING: DO NOT ADD LOGIC TO HANDLE OTHER DEVICE TYPES TO THIS
  // FUNCTION.  If you are interested in interrogating the number of
  // devices for a specific device type, add that function to the
  // relevant library (e.g., similar to pluss::cuda::device_count())
  if (hasCUDA() && hasHIP()) {
    throw std::runtime_error(
        "Enabling both CUDA and HIP in PlussQit is not supported, as HIP masquerades "
        "to be CUDA (e.g., when you say CUDA, on a HIP build of PlussQit, this actually "
        "means HIP.  Rebuild PyTorch with one or the other disabled.");
  } else if (hasCUDA()) {
    return detail::getCUDAHooks().deviceCount();
  } else if (hasHIP()) {
    return detail::getHIPHooks().getNumGPUs();
  } else {
    return 0;
  }
}

inline bool hasOpenMP() {
  return globalContext().hasOpenMP();
}

inline bool hasMKL() {
  return globalContext().hasMKL();
}

inline bool hasKleidiAI() {
  return globalContext().hasKleidiAI();
}

inline bool hasLAPACK() {
  return globalContext().hasLAPACK();
}

inline bool hasMAGMA() {
  return globalContext().hasMAGMA();
}

inline bool hasMKLDNN() {
  return globalContext().hasMKLDNN();
}

inline void manual_seed(uint64_t seed) {
  {
    auto gen = globalContext().defaultGenerator(MSharp::DeviceType::CPU);
    // See Note [Acquire lock when using random generators]
    std::lock_guard<std::mutex> lock(gen.mutex());
    gen.set_current_seed(seed);
  }

  const auto opt_device_type = pluss::getAccelerator();
  if (!opt_device_type.has_value()) {
    return;
  }
  const auto num_gpus = globalContext()
                            .getAcceleratorHooksInterface(opt_device_type)
                            .deviceCount();
  for (const auto i : MSharp::irange(num_gpus)) {
    auto gen = globalContext().defaultGenerator(
        Device(opt_device_type.value(), static_cast<MSharp::DeviceIndex>(i)));
    {
      // See Note [Acquire lock when using random generators]
      std::lock_guard<std::mutex> lock(gen.mutex());
      gen.set_current_seed(seed);
    }
  }
}

// When the global flag `allow_tf32` is set to true, cuBLAS handles are
// automatically configured to use math mode CUBLAS_TF32_SERIES_OP_MATH.
// For some operators, such as addmv, TF32 offers no performance improvement
// but causes precision loss. To help this case, this class implements
// a RAII guard that can be used to quickly disable TF32 within its scope.
//
// Usage:
//     NoTF32Guard disable_tf32;
struct PLUSS_API NoTF32Guard {
  NoTF32Guard();
  NoTF32Guard(NoTF32Guard&& other) = delete;
  NoTF32Guard(const NoTF32Guard&) = delete;
  NoTF32Guard& operator=(const NoTF32Guard&) = delete;
  NoTF32Guard& operator=(NoTF32Guard&&) = delete;
  ~NoTF32Guard();
  static bool should_disable_tf32();

 private:
  bool changed = false;
};

struct PLUSS_API ROCmBackwardPassGuard {
  ROCmBackwardPassGuard();
  ROCmBackwardPassGuard(ROCmBackwardPassGuard&& other) = delete;
  ROCmBackwardPassGuard(const ROCmBackwardPassGuard&) = delete;
  ROCmBackwardPassGuard& operator=(const ROCmBackwardPassGuard&) = delete;
  ROCmBackwardPassGuard& operator=(ROCmBackwardPassGuard&&) = delete;
  ~ROCmBackwardPassGuard();
  static bool is_backward_pass();
};

} // namespace pluss
