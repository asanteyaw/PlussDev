#include <MSharp/core/DispatchKey.h>
#include <MSharp/core/DispatchKeySet.h>

#include <regex>
#include <unordered_map>

namespace MSharp {

const char* toString(BackendComponent t) {
  switch (t) {
    case BackendComponent::CPUBit:
      return "CPUBit";
    case BackendComponent::CUDABit:
      return "CUDABit";
    case BackendComponent::HIPBit:
      return "HIPBit";
    case BackendComponent::XLABit:
      return "XLABit";
    case BackendComponent::LazyBit:
      return "LazyBit";
    case BackendComponent::MetaBit:
      return "MetaBit";
    case BackendComponent::XPUBit:
      return "XPUBit";
    case BackendComponent::IPUBit:
      return "IPUBit";
    case BackendComponent::MPSBit:
      return "MPSBit";
    case BackendComponent::HPUBit:
      return "HPUBit";
    case BackendComponent::VEBit:
      return "VEBit";
    case BackendComponent::MTIABit:
      return "MTIA";
    case BackendComponent::MAIABit:
      return "MAIA";
    case BackendComponent::PrivateUse1Bit:
      return "PrivateUse1Bit";
    case BackendComponent::PrivateUse2Bit:
      return "PrivateUse2Bit";
    case BackendComponent::PrivateUse3Bit:
      return "PrivateUse3Bit";
    case BackendComponent::InvalidBit:
      return "InvalidBit";
    default:
      return "UNKNOWN_BACKEND_BIT";
  }
}

BackendComponent toBackendComponent(DeviceType device_type) {
  switch (device_type) {
#define DO_CASE(device, _)                          \
  case DeviceType::device: {                        \
    return toBackendComponent(DispatchKey::device); \
  }
    C10_FORALL_BACKEND_DEVICE_TYPES(DO_CASE, unused)
#undef DO_CASE
    default:
      return BackendComponent::InvalidBit;
  }
}

const char* toString(DispatchKey t) {
  switch (t) {
    case DispatchKey::Undefined:
      return "Undefined";

    case DispatchKey::Dense:
      return "Dense";
    case DispatchKey::FPGA:
      return "FPGA";
    case DispatchKey::MAIA:
      return "MAIA";
    case DispatchKey::Vulkan:
      return "Vulkan";
    case DispatchKey::Metal:
      return "Metal";

    case DispatchKey::Lazy:
      return "Lazy";
    case DispatchKey::MPS:
      return "MPS";
    case DispatchKey::HPU:
      return "HPU";
    case DispatchKey::MTIA:
      return "MTIA";

    case DispatchKey::Quantized:
      return "Quantized";
    case DispatchKey::QuantizedPrivateUse1:
      return "QuantizedPrivateUse1";
    case DispatchKey::CustomRNGKeyId:
      return "CustomRNGKeyId";
    case DispatchKey::MkldnnCPU:
      return "MkldnnCPU";

    case DispatchKey::Sparse:
      return "Sparse";

    case DispatchKey::SparseCsr:
      return "SparseCsr";

    case DispatchKey::NestedTensor:
      return "NestedTensor";

    case DispatchKey::BackendSelect:
      return "BackendSelect";

    case DispatchKey::Python:
      return "Python";

    case DispatchKey::Fake:
      return "Fake";
    case DispatchKey::FuncTorchDynamicLayerBackMode:
      return "FuncTorchDynamicLayerBackMode";

    case DispatchKey::Functionalize:
      return "Functionalize";

    case DispatchKey::Named:
      return "Named";

    case DispatchKey::Conjugate:
      return "Conjugate";
    case DispatchKey::Negative:
      return "Negative";
    case DispatchKey::ZeroTensor:
      return "ZeroTensor";

    case DispatchKey::ADInplaceOrView:
      return "ADInplaceOrView";

    case DispatchKey::AutogradOther:
      return "AutogradOther";
    case DispatchKey::AutogradFunctionality:
      return "AutogradFunctionality";
    case DispatchKey::AutogradNestedTensor:
      return "AutogradNestedTensor";

    case DispatchKey::Tracer:
      return "Tracer";

    case DispatchKey::AutocastCPU:
      return "AutocastCPU";
    case DispatchKey::AutocastMTIA:
      return "AutocastMTIA";
    case DispatchKey::AutocastMAIA:
      return "AutocastMAIA";
    case DispatchKey::AutocastXPU:
      return "AutocastXPU";
    case DispatchKey::AutocastIPU:
      return "AutocastIPU";
    case DispatchKey::AutocastHPU:
      return "AutocastHPU";
    case DispatchKey::AutocastCUDA:
      return "AutocastCUDA";
    case DispatchKey::AutocastXLA:
      return "AutocastXLA";
    case DispatchKey::AutocastPrivateUse1:
      return "AutocastPrivateUse1";
    case DispatchKey::AutocastMPS:
      return "AutocastMPS";

    case DispatchKey::FuncTorchBatched:
      return "FuncTorchBatched";
    case DispatchKey::BatchedNestedTensor:
      return "BatchedNestedTensor";
    case DispatchKey::FuncTorchVmapMode:
      return "FuncTorchVmapMode";

    case DispatchKey::Batched:
      return "Batched";
    case DispatchKey::VmapMode:
      return "VmapMode";

    case DispatchKey::FuncTorchGradWrapper:
      return "FuncTorchGradWrapper";

    case DispatchKey::DeferredInit:
      return "DeferredInit";
    case DispatchKey::PythonTLSSnapshot:
      return "PythonTLSSnapshot";

    // Note [Out-of-tree vmap+grad prototype]
    // The following keys are used in the implementation of the out-of-tree
    // composable functions transforms (vmap+grad) prototype that lives at
    // https://github.com/zou3519/functorch
    // We plan on eventually upstreaming the prototype into core, at which
    // point it will have a different design that should use fewer keys.
    case DispatchKey::FuncTorchDynamicLayerFrontMode:
      return "FuncTorchDynamicLayerFrontMode";

    case DispatchKey::TESTING_ONLY_GenericWrapper:
      return "TESTING_ONLY_GenericWrapper";

    case DispatchKey::TESTING_ONLY_GenericMode:
      return "TESTING_ONLY_GenericMode";

    case DispatchKey::PreDispatch:
      return "PreDispatch";

    case DispatchKey::PythonDispatcher:
      return "PythonDispatcher";

      // Aliases

    case DispatchKey::Autograd:
      return "Autograd";
    case DispatchKey::CompositeImplicitAutograd:
      return "CompositeImplicitAutograd";
    case DispatchKey::CompositeImplicitAutogradNestedTensor:
      return "CompositeImplicitAutogradNestedTensor";
    case DispatchKey::CompositeExplicitAutograd:
      return "CompositeExplicitAutograd";
    case DispatchKey::CompositeExplicitAutogradNonFunctional:
      return "CompositeExplicitAutogradNonFunctional";
    case DispatchKey::FuncTorchBatchedDecomposition:
      return "FuncTorchBatchedDecomposition";

      // Per-backend dispatch keys

    default:
      auto bc = toBackendComponent(t);
      auto fk = toFunctionalityKey(t);

      switch (fk) {
#define ENTRY(backend, functionality)  \
  case BackendComponent::backend##Bit: \
    return #functionality #backend;

#define FORALL_BC(dkname, prefix)                  \
  case DispatchKey::dkname:                        \
    switch (bc) {                                  \
      C10_FORALL_BACKEND_COMPONENTS(ENTRY, prefix) \
      default:                                     \
        return #prefix "Undefined";                \
    }

        C10_FORALL_FUNCTIONALITY_KEYS(FORALL_BC)

        default:
          switch (bc) {
            C10_FORALL_BACKEND_COMPONENTS(ENTRY, Unknown)
            default:
              return "UnknownUnknown";
          }

#undef FORALL_BC
#undef ENTRY
      }
  }
}

std::ostream& operator<<(std::ostream& str, DispatchKey rhs) {
  return str << toString(rhs);
}
std::ostream& operator<<(std::ostream& str, BackendComponent rhs) {
  return str << toString(rhs);
}

DispatchKey getAutogradKeyFromBackend(BackendComponent k) {
  // We want this to return an autograd key. We're relying on the fact that
  // getAutogradRelatedKeySetFromBackend returns an autograd key +
  // ADInplaceOrView, and autograd has higher precedence. The core mapping from
  // backend -> autograd key lives in `getAutogradRelatedKeySetFromBackend`
  // instead of here for performance. `getAutogradRelatedKeySetFromBackend` is a
  // hotpath function, and we want to make sure that it doesn't have to
  // construct any DispatchKeySets at runtime.
  return getAutogradRelatedKeySetFromBackend(k).highestPriorityTypeId();
}

MSharp::DispatchKey parseDispatchKey(const std::string& k) {
  static std::unordered_map<std::string, MSharp::DispatchKey> key_map = {
      {"Undefined", MSharp::DispatchKey::Undefined},
      {"Dense", MSharp::DispatchKey::Dense},
      {"FPGA", MSharp::DispatchKey::FPGA},
      {"MAIA", MSharp::DispatchKey::MAIA},
      {"MPS", MSharp::DispatchKey::MPS},
      {"Vulkan", MSharp::DispatchKey::Vulkan},
      {"Metal", MSharp::DispatchKey::Metal},
      {"VE", MSharp::DispatchKey::VE},
      {"Meta", MSharp::DispatchKey::Meta},
      {"Quantized", MSharp::DispatchKey::Quantized},
      {"CustomRNGKeyId", MSharp::DispatchKey::CustomRNGKeyId},
      {"MkldnnCPU", MSharp::DispatchKey::MkldnnCPU},
      {"Sparse", MSharp::DispatchKey::Sparse},
      {"SparseCsr", MSharp::DispatchKey::SparseCsr},
      {"BackendSelect", MSharp::DispatchKey::BackendSelect},
      {"Python", MSharp::DispatchKey::Python},
      {"PythonTLSSnapshot", MSharp::DispatchKey::PythonTLSSnapshot},
      {"Fake", MSharp::DispatchKey::Fake},
      {"Named", MSharp::DispatchKey::Named},
      {"Conjugate", MSharp::DispatchKey::Conjugate},
      {"Negative", MSharp::DispatchKey::Negative},
      {"ZeroTensor", MSharp::DispatchKey::ZeroTensor},
      {"FuncTorchDynamicLayerBackMode",
       MSharp::DispatchKey::FuncTorchDynamicLayerBackMode},
      {"Functionalize", MSharp::DispatchKey::Functionalize},
      {"ADInplaceOrView", MSharp::DispatchKey::ADInplaceOrView},
      {"AutogradOther", MSharp::DispatchKey::AutogradOther},
      {"AutogradFunctionality", MSharp::DispatchKey::AutogradFunctionality},
      {"AutogradNestedTensor", MSharp::DispatchKey::AutogradNestedTensor},
      {"Tracer", MSharp::DispatchKey::Tracer},
      {"AutocastCPU", MSharp::DispatchKey::AutocastCPU},
      {"AutocastMTIA", MSharp::DispatchKey::AutocastMTIA},
      {"AutocastMAIA", MSharp::DispatchKey::AutocastMAIA},
      {"AutocastXPU", MSharp::DispatchKey::AutocastXPU},
      {"AutocastIPU", MSharp::DispatchKey::AutocastIPU},
      {"AutocastHPU", MSharp::DispatchKey::AutocastHPU},
      {"AutocastCUDA", MSharp::DispatchKey::AutocastCUDA},
      {"AutocastXLA", MSharp::DispatchKey::AutocastXLA},
      {"AutocastPrivateUse1", MSharp::DispatchKey::AutocastPrivateUse1},
      {"AutocastMPS", MSharp::DispatchKey::AutocastMPS},
      {"FuncTorchBatched", MSharp::DispatchKey::FuncTorchBatched},
      {"BatchedNestedTensor", MSharp::DispatchKey::BatchedNestedTensor},
      {"FuncTorchVmapMode", MSharp::DispatchKey::FuncTorchVmapMode},
      {"Batched", MSharp::DispatchKey::Batched},
      {"VmapMode", MSharp::DispatchKey::VmapMode},
      {"DeferredInit", MSharp::DispatchKey::DeferredInit},
      {"FuncTorchGradWrapper", MSharp::DispatchKey::FuncTorchGradWrapper},
      {"FuncTorchDynamicLayerFrontMode",
       MSharp::DispatchKey::FuncTorchDynamicLayerFrontMode},
      {"TESTING_ONLY_GenericWrapper",
       MSharp::DispatchKey::TESTING_ONLY_GenericWrapper},
      {"TESTING_ONLY_GenericMode", MSharp::DispatchKey::TESTING_ONLY_GenericMode},
      {"PythonDispatcher", MSharp::DispatchKey::PythonDispatcher},
      {"PreDispatch", MSharp::DispatchKey::PreDispatch},

      {"CPU", MSharp::DispatchKey::CPU},
      {"CUDA", MSharp::DispatchKey::CUDA},
      {"HIP", MSharp::DispatchKey::HIP},
      {"XLA", MSharp::DispatchKey::XLA},
      {"MPS", MSharp::DispatchKey::MPS},
      {"XPU", MSharp::DispatchKey::XPU},
      {"IPU", MSharp::DispatchKey::IPU},
      {"HPU", MSharp::DispatchKey::HPU},
      {"Lazy", MSharp::DispatchKey::Lazy},
      {"MTIA", MSharp::DispatchKey::MTIA},
      {"NestedTensor", MSharp::DispatchKey::NestedTensor},
      {"NestedTensorCPU", MSharp::DispatchKey::NestedTensorCPU},
      {"NestedTensorCUDA", MSharp::DispatchKey::NestedTensorCUDA},
      {"NestedTensorXPU", MSharp::DispatchKey::NestedTensorXPU},
      {"NestedTensorHPU", MSharp::DispatchKey::NestedTensorHPU},
      {"NestedTensorMeta", MSharp::DispatchKey::NestedTensorMeta},
      {"NestedTensorPrivateUse1", MSharp::DispatchKey::NestedTensorPrivateUse1},
      {"PrivateUse1", MSharp::DispatchKey::PrivateUse1},
      {"PrivateUse2", MSharp::DispatchKey::PrivateUse2},
      {"PrivateUse3", MSharp::DispatchKey::PrivateUse3},

      {"QuantizedCPU", MSharp::DispatchKey::QuantizedCPU},
      {"QuantizedCUDA", MSharp::DispatchKey::QuantizedCUDA},
      {"QuantizedXPU", MSharp::DispatchKey::QuantizedXPU},
      {"QuantizedPrivateUse1", MSharp::DispatchKey::QuantizedPrivateUse1},

      {"SparseCPU", MSharp::DispatchKey::SparseCPU},
      {"SparseCUDA", MSharp::DispatchKey::SparseCUDA},
      {"SparseHIP", MSharp::DispatchKey::SparseHIP},
      {"SparseXPU", MSharp::DispatchKey::SparseXPU},
      {"SparseVE", MSharp::DispatchKey::SparseVE},
      {"SparseMeta", MSharp::DispatchKey::SparseMeta},
      {"SparsePrivateUse1", MSharp::DispatchKey::SparsePrivateUse1},

      {"SparseCsrCPU", MSharp::DispatchKey::SparseCsrCPU},
      {"SparseCsrCUDA", MSharp::DispatchKey::SparseCsrCUDA},
      {"SparseCsrHIP", MSharp::DispatchKey::SparseCsrHIP},
      {"SparseCsrXPU", MSharp::DispatchKey::SparseCsrXPU},
      {"SparseCsrVE", MSharp::DispatchKey::SparseCsrVE},
      {"SparseCsrMeta", MSharp::DispatchKey::SparseCsrMeta},
      {"SparseCsrPrivateUse1", MSharp::DispatchKey::SparseCsrPrivateUse1},

      {"AutogradCPU", MSharp::DispatchKey::AutogradCPU},
      {"AutogradCUDA", MSharp::DispatchKey::AutogradCUDA},
      {"AutogradXLA", MSharp::DispatchKey::AutogradXLA},
      {"AutogradLazy", MSharp::DispatchKey::AutogradLazy},
      {"AutogradMeta", MSharp::DispatchKey::AutogradMeta},
      {"AutogradIPU", MSharp::DispatchKey::AutogradIPU},
      {"AutogradXPU", MSharp::DispatchKey::AutogradXPU},
      {"AutogradMPS", MSharp::DispatchKey::AutogradMPS},
      {"AutogradHPU", MSharp::DispatchKey::AutogradHPU},
      {"AutogradPrivateUse1", MSharp::DispatchKey::AutogradPrivateUse1},
      {"AutogradPrivateUse2", MSharp::DispatchKey::AutogradPrivateUse2},
      {"AutogradPrivateUse3", MSharp::DispatchKey::AutogradPrivateUse3},

      {"Autograd", MSharp::DispatchKey::Autograd},
      {"CompositeImplicitAutograd",
       MSharp::DispatchKey::CompositeImplicitAutograd},
      {"CompositeImplicitAutogradNestedTensor",
       MSharp::DispatchKey::CompositeImplicitAutogradNestedTensor},
      {"CompositeExplicitAutograd",
       MSharp::DispatchKey::CompositeExplicitAutograd},
      {"CompositeExplicitAutogradNonFunctional",
       MSharp::DispatchKey::CompositeExplicitAutogradNonFunctional},
      {"FuncTorchBatchedDecomposition",
       MSharp::DispatchKey::FuncTorchBatchedDecomposition},
  };
  auto it = key_map.find(k);
  if (it == key_map.end() && MSharp::get_privateuse1_backend() != "PrivateUse1") {
    std::string pu1_backend_name = MSharp::get_privateuse1_backend();
    std::transform(
        pu1_backend_name.begin(),
        pu1_backend_name.end(),
        pu1_backend_name.begin(),
        ::toupper);
    std::string processed_k =
        std::regex_replace(k, std::regex(pu1_backend_name), "PrivateUse1");
    it = key_map.find(processed_k);
  }
  TORCH_CHECK(it != key_map.end(), "could not parse dispatch key: ", k);
  return it->second;
}

} // namespace MSharp
