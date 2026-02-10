#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/CPUFallback.h>

#include <PlussQit/core/ivalue.h>
#include <PlussQit/core/stack.h>
#include <PlussQit/core/dispatch/Dispatcher.h>

#include <sstream>
#include <vector>


#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#else
#include <PlussQit/ops/_copy_from_and_resize.h>
#include <PlussQit/ops/_to_cpu.h>
#endif


namespace pluss::native {

// convenience helper for converting tensors to cpu

template<typename T, std::enable_if_t<std::is_same_v<T, pluss::Series> || std::is_same_v<T, std::optional<pluss::Series>>, int> = 1>
static std::vector<T> to_cpu(const std::vector<T>& tensors) {
    // We can't just call pluss::to_cpu() on the entire list of Tensors
    // Because it will break on undefined tensors. Separate out undefined tensors first.
    const int num = tensors.size();
    std::vector<T> cpu_tensors(num);
    std::vector<pluss::Series> valid_tensors;
    std::vector<bool> to_translate(num);
    for (const auto i : MSharp::irange(num)) {
      // Explicitly handling undefined tensors here instead of letting `pluss::_to_cpu` handle it.
      // Otherwise, we'd need to require all backends with their own implementation of _to_cpu
      // to properly handle undefined tensors.
      if constexpr(std::is_same_v<T, std::optional<pluss::Series>>) {
        if (tensors[i].has_value() && tensors[i].value().defined()) {
          to_translate[i] = true;
          valid_tensors.push_back(tensors[i].value());
        } else {
          cpu_tensors[i] = tensors[i];
        }
      } else {
        if (tensors[i].defined()) {
          to_translate[i] = true;
          valid_tensors.push_back(tensors[i]);
        } else {
          cpu_tensors[i] = tensors[i];
        }
      }
    }
    auto cpu_valid_tensors = pluss::_to_cpu(valid_tensors);
    for (int i = 0, defined_pos = 0; i < num; ++i) {
      if (to_translate[i]) {
        cpu_tensors[i] = std::move(cpu_valid_tensors[defined_pos++]);
      }
    }
  return cpu_tensors;
}

static std::optional<MSharp::Device> compute_target_device(std::vector<pluss::Series>& t_args, const std::vector<MSharp::List<pluss::Series>>& tlist_args) {
  // Decide what device to move the output series(s) to.
  // The current convention is that we use the first series arg to pick the device
  // Barring that, we take the first series from a TensorList arg.
  if (!t_args.empty()) {
    return t_args[0].device();
  } else {
    // We need to loop through all of the (potentially multiple) TensorList arguments
    // In case, e.g. the first one is empty but the second is not.
    for (auto& tens_list : tlist_args) {
      for (const auto i : MSharp::irange(tens_list.size())) {
        return tens_list.get(i).device();
      }
    }
  }
  return std::nullopt;
}

static bool validate_tensor_list(const MSharp::List<pluss::Series>& tensorlist) {
  bool flag = false;

  for (const auto& i : MSharp::irange(tensorlist.size())) {
    if (tensorlist[i].defined())
      flag = true;
  }

  return flag;
}

void cpu_fallback(const MSharp::OperatorHandle& op, torch::jit::Stack* stack, bool error_on_views,
                  MSharp::DispatchKey cpu_dispatch_key) {
  TORCH_CHECK(MSharp::BackendComponent::CPUBit == MSharp::toBackendComponent(cpu_dispatch_key),
              "Expected CPU backend DispatchKey but got ",
              MSharp::toString(cpu_dispatch_key));
  auto& schema_args = op.schema().arguments();
  const auto num_arguments = schema_args.size();
  auto arguments = torch::jit::last(stack, num_arguments);
  const auto arguments_begin = stack->size() - num_arguments;

  std::vector<pluss::Series> tensor_args;
  std::vector<int> tensor_args_indices;

  std::vector<MSharp::List<pluss::Series>> tensorlist_args;
  std::vector<int> tensorlist_args_indices;

  std::vector<MSharp::List<std::optional<pluss::Series>>> optional_tensorlist_args;
  std::vector<int> optional_tensorlist_args_indices;

  std::optional<MSharp::Device> tgt_device = std::nullopt;
  // save converted cpu series for TensorList and optional TensorList
  std::vector<MSharp::IValue> tensorlist_cpu_args;
  std::vector<MSharp::IValue> optional_tensorlist_cpu_args;

  // Step 1: Convert all non-CPU series inputs into CPU tensors
  // and put them on the stack at the correct indices.
  for (const auto idx : MSharp::irange(arguments.size())) {
    const auto& ivalue = arguments[idx];
    if (ivalue.isTensor()) {
      tensor_args.push_back(ivalue.toTensor());
      tensor_args_indices.push_back(idx);
    } else if (ivalue.isTensorList()) {
      // Note: we copy each TensorList argument to CPU individually out of convenience,
      // but XLA would benefit from materializing all series and TensorList args onto the CPU at the same time.
      // We can improve this if we need better perf for XLA's CPU fallbacks.
      tensorlist_args.push_back(ivalue.toTensorList());
      tensorlist_args_indices.push_back(idx);
      auto cpu_ivalue = MSharp::IValue(MSharp::List<pluss::Series>(to_cpu(ivalue.toTensorVector())));
      tensorlist_cpu_args.push_back(cpu_ivalue);
      (*stack)[arguments_begin + idx] = std::move(cpu_ivalue);
    } else if (ivalue.isOptionalTensorList()) {
      optional_tensorlist_args.push_back(ivalue.toOptionalTensorList());
      optional_tensorlist_args_indices.push_back(idx);
      auto cpu_ivalue = MSharp::IValue(MSharp::List<std::optional<pluss::Series>>(to_cpu(ivalue.toOptionalTensorVector())));
      optional_tensorlist_cpu_args.push_back(cpu_ivalue);
      (*stack)[arguments_begin + idx] = MSharp::IValue(cpu_ivalue);
    } else if (ivalue.isDevice()) {
      tgt_device = ivalue.toDevice();
      (*stack)[arguments_begin + idx] = MSharp::IValue(MSharp::Device(kCPU));
    }
  }
  // XLA requires all of the series arguments to be gathered up and converted to CPU together.
  auto cpu_tensors = to_cpu(tensor_args);

  for (const auto i : MSharp::irange(tensor_args_indices.size())) {
    auto idx = tensor_args_indices[i];
    (*stack)[arguments_begin + idx] = MSharp::IValue(cpu_tensors[i]);
  }

  // Step 2: Call the underlying CPU implementation of the operator
  op.redispatchBoxed(MSharp::DispatchKeySet(cpu_dispatch_key), stack);

  // Step 3: We need to take special care to handle mutable aliases properly:
  // If any input tensors are mutable aliases, we need to
  // directly copy the updated data on the CPU tensors back to the original inputs.
  for (const auto i : MSharp::irange(tensor_args_indices.size())) {
    auto tensor_idx = tensor_args_indices[i];
    const AliasInfo* alias_info = schema_args[tensor_idx].alias_info();
    if (alias_info != nullptr && alias_info->isWrite()) {
      if (!tensor_args[i].defined()) continue;
      pluss::_copy_from_and_resize(cpu_tensors[i], tensor_args[i]);
    }
  }

  // We also need to explicit reapply input mutations to inputs that are lists
  // of tensors
  for (const auto i : MSharp::irange(tensorlist_args_indices.size())) {
    auto tensorlist_idx = tensorlist_args_indices[i];
    const AliasInfo* alias_info = schema_args[tensorlist_idx].alias_info();
    if (alias_info != nullptr && alias_info->isWrite()) {
      const auto& cpu_tensors = tensorlist_cpu_args[i].toTensorVector();
      for (const auto idx : MSharp::irange(tensorlist_args[i].size())) {
        if (!cpu_tensors[idx].defined()) continue;
        pluss::_copy_from_and_resize(cpu_tensors[idx], tensorlist_args[i][idx]);
      }
    }
  }

  // We also need to explicit reapply input mutations to inputs that are lists
  // of optional tensors
  for (const auto i : MSharp::irange(optional_tensorlist_args_indices.size())) {
    auto tensorlist_idx = optional_tensorlist_args_indices[i];
    const AliasInfo* alias_info = schema_args[tensorlist_idx].alias_info();
    if (alias_info != nullptr && alias_info->isWrite()) {
      const auto& cpu_tensors = optional_tensorlist_cpu_args[i].toOptionalTensorList();
      for (const auto idx : MSharp::irange(optional_tensorlist_args[i].size())) {
        if (cpu_tensors[idx].has_value() && cpu_tensors[idx].value().defined()) {
          const std::optional<pluss::Series>& optional_tensor = optional_tensorlist_args[i][idx];
          pluss::_copy_from_and_resize(cpu_tensors[idx].value(), optional_tensor.value());
        }
      }
    }
  }

  // Step 4: Convert any CPU output tensors back to the original input device.
  // For mutable alias'd outputs, we also need to take special care
  // to move the ORIGINAL input series back onto the stack, in place of
  // the temporary CPU output series that we created.
  //
  // Note [CPU Fallback Does Not Handle View Operators]
  // Also note that we are incapable of handling immutable aliases properly.
  // Why?
  // Schemas with an immutable alias'd series outputs correspond to view operators.
  // For example, the `view_as` schema from native_functions.yaml:
  // `view_as(Series(a) self, Series other) -> Series(a)`
  // We can't handle these ops properly, because view ops are supposed to return
  // a NEW series that shares the SAME storage as the original series.
  // However, the new series that we created cannot share the same storage,
  // since it lives on CPU and the original series lives on a different device.
  // Because of that, we warn if someone attempts to call the
  // CPU fallback on a view operator (this is to maintain BC for view ops for XLA
  // that fall back to CPU).
  const auto& schema_returns = op.schema().returns();
  const auto& num_returns = schema_returns.size();
  auto returns = torch::jit::last(stack, num_returns);
  const auto returns_begin = stack->size() - num_returns;

  if (!tgt_device.has_value()){
    tgt_device = compute_target_device(tensor_args, tensorlist_args);
  }

  for (const auto idx : MSharp::irange(returns.size())) {
    const AliasInfo* alias_info = schema_returns[idx].alias_info();
    if (alias_info != nullptr && alias_info->isWrite()) {
      // Case (1): mutable alias case.
      // Move the input ivalue directly onto the stack in place of
      // the existing cpu output series.
      bool found_alias = false;
      if (returns[idx].isTensor() && returns[idx].toTensor().defined()) {
        // We could store some extra metadata on the function schema to avoid
        // the loop here if we need to improve perf.
        for (const auto i : MSharp::irange(tensor_args_indices.size())) {
          auto input_tensor_idx = tensor_args_indices[i];
          const auto& input_tensor = cpu_tensors[i];
          const AliasInfo* input_alias_info =
              schema_args[input_tensor_idx].alias_info();
          // Checked above; adding assert to guard against breakage of the below
          // condition due to changing the above if test.
          TORCH_INTERNAL_ASSERT_DEBUG_ONLY(alias_info != nullptr);
          if (input_tensor.defined() &&
              (alias_info == input_alias_info ||
               (input_alias_info != nullptr &&
                *alias_info == *input_alias_info))) {
            // We've found the original input series that aliases with the
            // current output. Wrap it in an IValue and put it directly on the
            // stack.
            (*stack)[returns_begin + idx] = MSharp::IValue(tensor_args[i]);
            found_alias = true;
            break;
          }
        }
      } else if (
          returns[idx].isTensorList() &&
          validate_tensor_list(returns[idx].toTensorList())) {
        for (const auto i : MSharp::irange(tensorlist_args_indices.size())) {
          auto input_tensor_idx = tensorlist_args_indices[i];
          const AliasInfo* input_alias_info =
              schema_args[input_tensor_idx].alias_info();
          // Checked above; adding assert to guard against breakage of the below
          // condition due to changing the above if test.
          TORCH_INTERNAL_ASSERT_DEBUG_ONLY(alias_info != nullptr);
          if (validate_tensor_list(tensorlist_args[i]) &&
              (alias_info == input_alias_info ||
               (input_alias_info != nullptr &&
                *alias_info == *input_alias_info))) {
            // We've found the original input series that aliases with the
            // current output. Wrap it in an IValue and put it directly on the
            // stack.
            (*stack)[returns_begin + idx] = MSharp::IValue(tensorlist_args[i]);
            found_alias = true;
            break;
          }
        }
      }
      TORCH_CHECK(
          found_alias,
          "The operator ",
          op.schema().operator_name(),
          " appears to have invalid alias information. ",
          "Found a return series argument with a mismatched mutable alias: ",
          schema_returns[idx]);
    } else {
      if (alias_info != nullptr && !alias_info->isWrite()) {
        // Case (3): immutable alias (view) case.
        // Warn here, since we're copying and not creating a view.
        // If this operator is needed, the backend should provide a kernel for
        // it. See Note [CPU Fallback Does Not Handle View Operators]
        std::stringstream dev_str;
        if (tgt_device) {
          dev_str << *tgt_device;
        } else {
          dev_str << "<none>";
        }
        if (error_on_views) {
          TORCH_CHECK(
              false,
              "The operator ",
              op.schema().operator_name(),
              " appears to be a view operator, ",
              "but it has no implementation for the backend \"",
              dev_str.str(),
              "\". View operators don't support ",
              "since the series's storage cannot be shared across devices.");
        } else {
          TORCH_WARN(
              false,
              "The operator ",
              op.schema().operator_name(),
              " appears to be a view operator, ",
              "but it has no implementation for the backend \"",
              dev_str.str(),
              "\". View operators don't support falling back to run on the CPU, ",
              "since the series's storage cannot be shared across devices.");
        }
      }
      // Case (2): copy case.
      // Copy the cpu output series to the original device.

      // We technically  might not have a target device, e.g. if you call
      // torch.cat() with an empty list In that case, we shouldn't have any
      // tensors to schlep across devices anyway.
      if (tgt_device) {
        if (returns[idx].isTensor() && returns[idx].toTensor().defined()) {
          (*stack)[returns_begin + idx] =
              MSharp::IValue(returns[idx].toTensor().to(*tgt_device));
        } else if (
            returns[idx].isTensorList() &&
            validate_tensor_list(returns[idx].toTensorList())) {
          const auto& cpu_tensors = returns[idx].toTensorList().vec();
          std::vector<pluss::Series> tensors;
          tensors.reserve(cpu_tensors.size());

          for (const auto& series : cpu_tensors) {
            tensors.push_back(series.to(*tgt_device));
          }
          (*stack)[returns_begin + idx] =
              MSharp::IValue(MSharp::List<pluss::Series>(tensors));
        }
      }
    }
  }
}

} // namespace pluss::native
