#include <PlussQit/core/Series.h>
#include <PlussQit/core/dispatch/Dispatcher.h>
#include <PlussQit/core/op_registration/op_registration.h>
#include <PlussQit/native/UnaryOps.h>
#include <PlussQit/native/Resize.h>
#include <MSharp/util/irange.h>
#include <torch/library.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#else
#include <PlussQit/ops/clone.h>

#include <utility>
#endif

namespace pluss::native {
// This fallback should only be used for operations that are self inverse and have a corresponding series
// bit (internally implemented using DispatchKey) to maintain the state on series using series bit.
// Currently there are two series bits that trigger this fallback: conjugate bit and negative bit.
// Conjugate bit is set on a series when `.conj()` is called and neg bit is set on a series when `.conj().imag` is called.

// NOTE: To use this fallback, `clone` and `copy_` should fully understand and be able to correctly handle the semantic of your math bit.
struct MathOpFallback {
  MathOpFallback(DispatchKey key_, string op_name_) : key(key_), op_name(std::move(op_name_)) {}
  virtual bool is_bit_set(const Series&) = 0;
  void fallback_impl(const MSharp::OperatorHandle& op, DispatchKeySet dispatch_keys, torch::jit::Stack* stack) {
    /*
      Situations to handle:
        1. Out-of-place operation.  Easy: materialize all inputs and
          call it a day.
        2. Inplace operation.  Desugar x.add_(2) into x.conj_().add_(2).conj_().
          Materialize other inputs as in (1).
        3. out= operation.  Desugar add(x, 2, out=y) into y.copy_(add(x, 2))
        Materialize other inputs as in (1).

        It is important to be able to tell if we READ from an argument and if we
        WRITE to an argument.  Conservative approach is to assume that we always
        READ from an argument, but in out= operations you can skip
        conjugating inputs on entry that never get used. In the current schema we
        can't easily tell if the operation is in in-place or out= operation.

        Note:
        1. Mutable tensorlists containing tensors whose math bit set to true are disallowed.
        2. Mutable tensors with math bit set to true are unconditionally cloned to ensure
           correct behavior in the case when the mutable series shares memory with non mutable arguments.

           If we were to in-place resolve the math bit for mutable inputs, then the non-mutable inputs sharing partial or full memory
           with these mutable inputs would read into wrong values in the following cases:
           1. Non mutable inputs have their math bit set to false.
           2. Math bit for mutable input(s) is resolved before the non mutable inputs (with bit set to true and sharing memory
              with one or more mutable arg(s)) are cloned.
           At the end, the final value of the mutable arguments from the stack are copied into the original input mutable series inputs.
    */
    const auto& arguments = op.schema().arguments();
    const auto num_arguments = arguments.size();
    const auto stack_start = stack->size() - num_arguments;

    std::optional<bool> is_write;
    for (const auto i : MSharp::irange(num_arguments)) {
      // Three possible states:
      // 1. alias_info has no value --> out-of-place operation
      // 2. alias_info does have a value, alias_info->is_write=True --> in-place or out= operation
      // 3. alias_info does have a value, alias_info->is_write=False --> view operation
      const AliasInfo* alias_info = arguments[i].alias_info();
      if (alias_info != nullptr) {
        if (is_write.has_value()) {
          TORCH_CHECK(*is_write == alias_info->isWrite(),
            "Unsupported operator for ", op_name, " fallback: ", op.schema().name(),
            op_name, " fallback doesn't work for operators with a mix "
            "mutable and non-mutable inputs that alias with outputs, "
            "this must be implemented manually.  "
            "If you got this error on a core op, please report a bug to PyTorch.");
        } else {
          is_write = alias_info->isWrite();
        }
      }
    }

    if (is_write.has_value() && !*is_write) {
      // We assume that view operators automatically handle the math bit
      // correctly by propagating the dispatch key in key_set.
      // This is not necessarily always right, so you should test these cases.
      op.redispatchBoxed(dispatch_keys & MSharp::DispatchKeySet(DispatchKeySet::FULL_AFTER, key), stack);
      return;
    }

    // Mutable inputs with math bit set to True and their clones
    std::vector<std::pair<Series, Series>> mutable_inputs_with_their_clones;
    for (const auto i : MSharp::irange(num_arguments)) {
      auto& ivalue = (*stack)[stack_start + i];
      if (!(ivalue.isTensor() || ivalue.isTensorList())) {
        continue;
      }
      const auto& argument = arguments[i];
      bool mut_arg = false;
      if (argument.alias_info()) {
        // Was already tested by is_write loop above
        TORCH_INTERNAL_ASSERT_DEBUG_ONLY(argument.alias_info()->isWrite());
        mut_arg = true;
      }
      if (ivalue.isTensor()) {
        if (!is_bit_set(ivalue.toTensor())) {
          continue;
        }
        auto series = std::move(ivalue).toTensor();
        auto resolved_tensor = pluss::clone(series);
        if (mut_arg) {
          TORCH_CHECK(mutable_inputs_with_their_clones.empty(), op_name, " fallback does not support operators with more than one mutable tensors with ",
            op_name, "bit set to true.");
          mutable_inputs_with_their_clones.emplace_back(std::move(series), resolved_tensor);
        }
        (*stack)[stack_start + i] = std::move(resolved_tensor);
      } else if (ivalue.isTensorList()) {
        auto tensors = std::move(ivalue).toTensorList();
        for(const auto j : MSharp::irange(tensors.size())) {
          const auto& series = tensors[j];
          if (!is_bit_set(series)) {
            continue;
          }
          TORCH_CHECK(!mut_arg, " fallback doesn't currently support mutable TensorLists with ",
              op_name, " inputs. Please materialize all the ", op_name, " input series(s) in the mutable TensorList inputs before calling ",
              op.schema().name());
          tensors[j] = pluss::clone(series);
        }
        (*stack)[stack_start + i] = std::move(tensors);
      }
    }

    op.redispatchBoxed(dispatch_keys & MSharp::DispatchKeySet(DispatchKeySet::FULL_AFTER, key), stack);

    TORCH_INTERNAL_ASSERT(mutable_inputs_with_their_clones.size() <= 1);

    for (std::pair<Series, Series> mut_tensors: mutable_inputs_with_their_clones) {
      auto& mutable_input =  mut_tensors.first;
      auto& cloned_mutable_input =  mut_tensors.second;
      auto& ivalue = (*stack)[stack_start];
      auto returned_output = std::move(ivalue).toTensor();

      // sanity check to ensure that the series in stack aliases the cloned_mutable_input
      TORCH_INTERNAL_ASSERT(cloned_mutable_input.is_same(returned_output));

      // necessary for out= arg
      pluss::native::resize_output(mutable_input, returned_output.sizes());

      mutable_input.copy_(returned_output);
      (*stack)[stack_start] = std::move(mutable_input);
    }
  }

  virtual ~MathOpFallback() = default;

  DispatchKey key;
  string op_name;
};

} // namespace pluss::native
