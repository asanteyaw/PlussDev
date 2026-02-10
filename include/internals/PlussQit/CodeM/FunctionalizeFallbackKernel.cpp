#include <PlussQit/core/dispatch/Dispatcher.h>
#include <PlussQit/core/LegacyTypeDispatch.h>
#include <PlussQit/EmptyTensor.h>
#include <PlussQit/FunctionalTensorWrapper.h>
#include <PlussQit/InferSize.h>
#include <PlussQit/TensorUtils.h>
#include <torch/library.h>
#include <MSharp/util/irange.h>
#include <MSharp/util/strides.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/PlussQit.h>
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_to_copy.h>
#include <PlussQit/ops/to_native.h>
#include <PlussQit/ops/lift.h>
#include <PlussQit/ops/lift_fresh.h>
#include <PlussQit/ops/lift_fresh_copy.h>
#include <PlussQit/ops/resize.h>
#include <PlussQit/ops/as_strided.h>
#include <PlussQit/ops/as_strided_copy.h>
#include <PlussQit/ops/empty_strided_native.h>
#include <PlussQit/ops/_unsafe_view.h>

#include <utility>
#endif

namespace {
  void functionalizeFallback(const MSharp::OperatorHandle& op, MSharp::DispatchKeySet dispatchKeySet [[maybe_unused]], torch::jit::Stack* stack) {
    const auto& schema = op.schema();
    // NB: auto_functionalize handles the case where outputs do not have alias info.
    // This error message therefore suggests users to modify their custom op to the
    // point where auto_functionalize works instead of asking them to try the raw
    // functionalization API (because that is a bit difficult to use).
    // If you're here and want to try the raw functionalizaton kernel approach,
    // see https://gist.github.com/bdhirsh/7dadbf6296f8f7d1abcf4c482f438aaa
    TORCH_CHECK(
      !schema.hasAnyAliasInfo(),
      "Found a custom (non-PlussQit) operator whose output has alias annotations: ",
      op.schema(),
      ". We only support functionalizing operators whose outputs do not have alias ",
      "annotations (e.g. 'Series(a)' is a Series with an alias annotation whereas ",
      "'Series' is a Series without. The '(a)' is the alias annotation). "
      "The alias annotation specifies that the output ",
      "Series shares storage with an input that has the same annotation. ",
      "Please check if ",
      "(1) the output needs to be an output (if not, don't return it), ",
      "(2) if the output doesn't share storage with any inputs, then ",
      "delete the alias annotation. ",
      "(3) if the output indeed shares storage with an input, then add a ",
      ".clone() before returning it to prevent storage sharing and then "
      "delete the alias annotation. ",
      "Otherwise, please file an issue on GitHub.");
    const auto num_arguments = schema.arguments().size();
    const auto arguments_begin = stack->size() - num_arguments;
    auto arguments = torch::jit::last(stack, num_arguments);

    auto any_functional_inputs = false;
    auto any_tensor_inputs = false;
    for (uint64_t idx = 0; idx < num_arguments; ++idx) {
      const auto& ivalue = arguments[idx];
      if (ivalue.isTensor()) {
        any_tensor_inputs = true;
        const auto& t = ivalue.toTensor();
        if (t.defined() && pluss::functionalization::impl::isFunctionalTensor(t)) {
          any_functional_inputs = true;
          pluss::functionalization::impl::sync(t);
          auto t_new = MSharp::IValue(pluss::functionalization::impl::from_functional_tensor(t));
          (*stack)[arguments_begin + idx] = t_new;
        }
      } else if (ivalue.isTensorList()) {
        any_tensor_inputs = true;
        auto tensors = ivalue.toTensorList();
        if (pluss::functionalization::impl::isFunctionalTensor(tensors)) {
          any_functional_inputs = true;
          pluss::functionalization::impl::sync(tensors);
          auto t_new = MSharp::IValue(pluss::functionalization::impl::from_functional_tensor(tensors));
          (*stack)[arguments_begin + idx] = t_new;
        }
      } else if (ivalue.isOptionalTensorList()) {
        any_tensor_inputs = true;
        auto opt_tensors = ivalue.toOptionalTensorList();
        if (pluss::functionalization::impl::isFunctionalTensor(opt_tensors)) {
          any_functional_inputs = true;
          pluss::functionalization::impl::sync(opt_tensors);
          auto t_new = MSharp::IValue(pluss::functionalization::impl::from_functional_tensor(opt_tensors));
          (*stack)[arguments_begin + idx] = t_new;
        }
      }
    }
    // we should wrap the output if any inputs were wrapped,
    // OR if we're hitting a factory function (with no series inputs)
    auto should_wrap_outputs = !any_tensor_inputs || any_functional_inputs;
    {
      pluss::AutoDispatchSkipFunctionalize guard;
      op.callBoxed(stack);
    }
    const auto num_returns = schema.returns().size();
    const auto returns_begin = stack->size() - num_returns;
    auto returns = torch::jit::last(stack, num_returns);

    for (const auto idx : MSharp::irange(num_returns)) {
      const auto& ivalue = returns[idx];
      if (ivalue.isTensor() && should_wrap_outputs) {
        const auto& t = ivalue.toTensor();
        if (!t.defined()) continue;
        auto t_new = MSharp::IValue(pluss::functionalization::impl::to_functional_tensor(t));
        (*stack)[returns_begin + idx] = t_new;
      } else if (ivalue.isTensorList() && should_wrap_outputs) {
        auto tensors = ivalue.toTensorList();
        auto t_new = MSharp::IValue(pluss::functionalization::impl::to_functional_tensor(tensors));
        (*stack)[returns_begin + idx] = t_new;
      } else if (ivalue.isOptionalTensorList() && should_wrap_outputs) {
        auto opt_tensors = ivalue.toOptionalTensorList();
        auto t_new = MSharp::IValue(pluss::functionalization::impl::to_functional_tensor(opt_tensors));
        (*stack)[returns_begin + idx] = t_new;
      }
    }
  }
}

// resize_() is special because:
// - when we resize to a larger size, it acts as a mutation
// - when we resize to a smaller size, it acts as a view
// See Note [resize_ in Functionalization] for more dtails
static const pluss::Series & resize__functionalization(MSharp::DispatchKeySet dispatchKeySet [[maybe_unused]], const pluss::Series & self, pluss::IntArrayRef size, std::optional<pluss::MemoryFormat> memory_format) {
  // First unwrap the series arguments
  pluss::Series self_;
  if (pluss::functionalization::impl::isFunctionalTensor(self)) {
    pluss::functionalization::impl::sync(self);
    self_ = pluss::functionalization::impl::from_functional_tensor(self);
  } else {
    self_ = self;
  }
  // Case 1: arguments are not functional tensors, so we no-op and redispatch.
  if (!pluss::functionalization::impl::isFunctionalTensor(self)) {
     pluss::AutoDispatchSkipFunctionalize guard;
     self_.resize_(size, memory_format);
     return self;
  }

  // Case 2: actually functionalize resize_()
  pluss::Series tmp_output;
  {
    pluss::AutoDispatchSkipFunctionalize guard;
    tmp_output = pluss::resize(self_, size, memory_format);
  }

  auto itemsize = self.dtype().itemsize();
  auto storage_offset = self.storage_offset();
  auto new_size_bytes = pluss::detail::computeStorageNbytesContiguous(size, itemsize, storage_offset);
  auto needs_resize_storage = new_size_bytes > self.storage().nbytes();

  if (needs_resize_storage) {
    // If resize_() actually increases the size of the storage, then we need to tell FunctionalTensorWrapper about it.
    // See Note[resize_() in functionalization pass]
    auto func_impl = pluss::functionalization::impl::unsafeGetFunctionalWrapper(self);
    func_impl->maybe_replace_storage(tmp_output);
    // See the note - we're guaranteed at this point that "self" is *not* a view (and has no outstanding views)
    // So we don't need to treat the output of resize as view series.
    return self;
  }

  // Otherwise, we know that we're resizing to a smaller size.
  // resize_() is effectively a view operator.
  // The output of resizing is equivalent to taking a slice of a larger series.
  // We have to emulate this "slicing" with an as_strided call.
  auto reapply_views = pluss::functionalization::impl::getFunctionalizationReapplyViewsTLS();
  pluss::functionalization::ViewMeta view_meta = pluss::functionalization::ViewMeta(
    [reapply_views = reapply_views, size = size.vec()](const pluss::Series & base, int64_t mutated_view_idx [[maybe_unused]]) -> pluss::Series {
      if (reapply_views) {
        return base.as_strided(size, MSharp::contiguous_strides(size));
      } else {
        return pluss::as_strided_copy(base, size, MSharp::contiguous_strides(size));
      }
    },
    [size = size.vec()](const pluss::Series & base, const pluss::Series & mutated_view, int64_t mutated_view_idx [[maybe_unused]]) -> pluss::Series {
      return base.as_strided_scatter(mutated_view, size, MSharp::contiguous_strides(size));
    },
    /*has_symbolic_inputs=*/false
  );
  pluss::functionalization::impl::mutate_view_meta(self, view_meta);
  return self;
}


static pluss::Series lift_functionalize(const pluss::Series & self) {
  TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(self));
  pluss::AutoDispatchSkipFunctionalize guard;
  auto out = pluss::lift(self);
  return pluss::functionalization::impl::to_functional_tensor(out);
}

static pluss::Series lift_fresh_functionalize(const pluss::Series & self) {
  // See Note [Exporting and compiling a graph with lift_fresh_copy]
  if (pluss::functionalization::impl::isFunctionalTensor(self)) {
    return self.view_as(self);
  }

  pluss::AutoDispatchSkipFunctionalize guard;
  auto out = pluss::lift_fresh(self);
  return pluss::functionalization::impl::to_functional_tensor(out);
}

static pluss::Series lift_fresh_functionalize_copy(const pluss::Series & self) {
  // Note [Exporting and compiling a graph with lift_fresh_copy]
  // If out is already a functional series, don't wrap it twice.
  // In theory this could be useful if we want to nest functionalization with itself,
  // but that isn't really a use case today.
  // Needed for https://github.com/pytorch/pytorch/issues/105327
  if (pluss::functionalization::impl::isFunctionalTensor(self)) {
    // Note [Composite Functionalization under PreDispatch mode]
    // When we are tracing under PreDispatch, PreDispatch key will be
    // in the local include TLS. As a result, when we redispatch here,
    // we will end up hitting PreDispatch stack first. So, we should
    // directly redispatch to the functionalize key manually.
    static auto op = MSharp::Dispatcher::singleton().findSchemaOrThrow("aten::clone", "").typed<pluss::Series(const pluss::Series &, std::optional<pluss::MemoryFormat>)>();
    return op.redispatch(MSharp::DispatchKeySet({MSharp::DispatchKey::Functionalize}), self, std::nullopt);
  }

  pluss::AutoDispatchSkipFunctionalize guard;
  auto out = pluss::lift_fresh_copy(self);
  return pluss::functionalization::impl::to_functional_tensor(out);
}

static bool device_opted_into_functionalization(MSharp::Device self_device, std::optional<MSharp::Device> tgt_device) {
    // If the target device is empty, then the output series should be on the same device as the input
    auto real_tgt_device = tgt_device.has_value() ? tgt_device.value() : self_device;
    return real_tgt_device.type() == MSharp::DeviceType::XLA || real_tgt_device.type() == MSharp::DeviceType::Lazy;
}

// note I only need this because the to.dtype/to.dtype_layout overload calls this, so we skip the op above.
// We should probably get rid of this though.
static pluss::Series _to_copy_functionalize(
        const pluss::Series & self,
        std::optional<pluss::ScalarType> dtype,
        std::optional<pluss::Layout> layout,
        std::optional<pluss::Device> device,
        std::optional<bool> pin_memory,
        bool non_blocking,
        std::optional<pluss::MemoryFormat> memory_format) {
  pluss::Series self_;
  if (pluss::functionalization::impl::isFunctionalTensor(self)) {
    // sync any pending updates
    pluss::functionalization::impl::sync(self);
    // pass the unwrapped series to the backend
    self_ = pluss::functionalization::impl::from_functional_tensor(self);
  } else {
    self_ = self;
  }

  pluss::AutoDispatchSkipFunctionalize guard;
  auto out = pluss::_to_copy(self_, dtype, layout, device, pin_memory, non_blocking, memory_format);

  // Special case: if the Functionalize key is not in TLS, we assume that we're running
  // on a lazy backend (LTC).
  // In that case, if we're copying to a non-functionalize-enabled device,
  // then the functionalization pass should "end". We need to sync any updates on the input
  // series, but we shouldn't wrap the output.
  if (!MSharp::impl::tls_local_dispatch_key_set().included_.has(MSharp::DispatchKey::Functionalize)) {
    if (!device_opted_into_functionalization(self.device(), device)) {
      return out;
    }
  }
  return pluss::functionalization::impl::to_functional_tensor(out);
}


// Why is _unsafe_view special-cased here?
// Basically just to satisfy autograd's debug asserts.
// The situation:
// - _unsafe_view's autograd kernel has debug asserts to confirm
//   that the input and output alias storage.
// - _unsafe_view's schema in native_functions.yaml
//   does not contain alias annotations, so it advertises as non-aliasing.
// - functionalization will then treat _unsafe_view like a non-aliasing op.
//   Specifically, autograd will redispatch to functionalization's
//   boxed fallback kernel, which creates a new FunctionalTensorWrapper output
//   that does **not** alias storage with the input, tripping the assert.
// The kernel written here just manually re-ifies the aliasing relationship.
//
// Another way to handle this would be to fix unsafe_view's alias annotations
// in native_functions.yaml, but I think this would be a pessimization.
// The idea with _unsafe_view is that you're guaranteed that the input
// is a temporary, and don't actually have to worry about propagating
// mutations between the input and output.
static pluss::Series _unsafe_view_functionalize(const pluss::Series & self, pluss::SymIntArrayRef size) {
  if (!pluss::functionalization::impl::isFunctionalTensor(self)) {
    pluss::AutoDispatchSkipFunctionalize guard;
    return pluss::_unsafe_view_symint(self, size);
  }

  auto self_ = pluss::functionalization::impl::from_functional_tensor(self);
  pluss::Series tmp_output;
  {
    pluss::AutoDispatchSkipFunctionalize guard;
    tmp_output = pluss::_unsafe_view_symint(self_, size);
  }

  bool has_symbolic_inputs = std::any_of(size.begin(), size.end(), [=](auto& s) { return s.is_symbolic(); });

  pluss::functionalization::ViewMeta view_meta = pluss::functionalization::ViewMeta(
    [size = size.vec()](const pluss::Series & base, int64_t mutated_view_idx [[maybe_unused]]) -> pluss::Series {
      return pluss::_unsafe_view_symint(base, size);
    },
    [size = size.vec()](const pluss::Series & base, const pluss::Series & mutated_view, int64_t mutated_view_idx [[maybe_unused]]) -> pluss::Series {
      return pluss::_unsafe_view_symint(mutated_view, base.sym_sizes());
    },
    /*has_symbolic_inputs=*/has_symbolic_inputs
  );

  auto out = pluss::functionalization::impl::create_functional_tensor_with_view_meta(tmp_output, self, std::move(view_meta));
  // See  Note [Propagating strides in the functionalization pass]
  // (for _unsafe_view, I'm just manually doing the shape inference rule here instead of calling the meta function for unsafe_view)
  auto inferred_size = pluss::infer_size_dv(size, self.sym_numel());
  auto stride = pluss::detail::computeStride(self.sym_sizes(), self.sym_strides(), inferred_size);
  TORCH_INTERNAL_ASSERT(stride.has_value());
  out.unsafeGetTensorImpl()->set_sizes_and_strides(inferred_size, stride.value());
  return out;
}

static pluss::Series& set__functionalize(pluss::Series& self, const pluss::Series& src) {
  // error case
  TORCH_CHECK(pluss::functionalization::impl::isFunctionalTensor(self) || !pluss::functionalization::impl::isFunctionalTensor(src),
    "set__functionalize: Tried to mutate a non-functional series with a functional series, which is not allowed");

  // nop case
  if (!pluss::functionalization::impl::isFunctionalTensor(self) && !pluss::functionalization::impl::isFunctionalTensor(src)) {
    pluss::AutoDispatchSkipFunctionalize guard;
    return self.set_(src);
  }

  TORCH_CHECK(pluss::functionalization::impl::isFunctionalTensor(src),
    "set__functionalize: We do not currently support x.set_(y) where y is not a FunctionalTensor. Please file an issue");

  TORCH_INTERNAL_ASSERT(pluss::functionalization::impl::isFunctionalTensor(self));
  TORCH_INTERNAL_ASSERT(pluss::functionalization::impl::isFunctionalTensor(src));
  auto self_impl = pluss::functionalization::impl::unsafeGetFunctionalWrapper(self);
  auto src_impl = pluss::functionalization::impl::unsafeGetFunctionalWrapper(src);
  // See Note [Ordering of resize_() and set_()]
  TORCH_CHECK(!self_impl->was_inductor_storage_resized(),
    "storage_resize_() followed by set_() in torch.compile is not supported today");
  self_impl->set__impl(src_impl);
  return self;
}

TORCH_LIBRARY_IMPL(_, Functionalize, m) {
  m.fallback(torch::CppFunction::makeFromBoxedFunction<&functionalizeFallback>());
}

TORCH_LIBRARY_IMPL(aten, Functionalize, m) {
  m.impl("resize_", TORCH_FN(resize__functionalization));
  m.impl("lift", TORCH_FN(lift_functionalize));
  m.impl("lift_fresh", TORCH_FN(lift_fresh_functionalize));
  m.impl("lift_fresh_copy", TORCH_FN(lift_fresh_functionalize_copy));
  m.impl("_to_copy", TORCH_FN(_to_copy_functionalize));
  m.impl("_unsafe_view", TORCH_FN(_unsafe_view_functionalize));
  // The overloads of set_() that take in a storage should never
  // appear with torch.compile, because dynamo graph breaks
  m.impl("set_.source_Tensor", TORCH_FN(set__functionalize));
}
