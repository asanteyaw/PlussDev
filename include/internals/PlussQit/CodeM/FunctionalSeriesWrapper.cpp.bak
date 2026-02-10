
#include <PlussQit/FunctionalTensorWrapper.h>

#include <PlussQit/FunctionalInverses.h>
#include <PlussQit/TensorUtils.h>
#include <PlussQit/WrapDimUtils.h>
#include <PlussQit/core/IListRef.h>
#include <PlussQit/core/LegacyTypeDispatch.h>
#include <MSharp/util/Exception.h>

#include <MSharp/util/irange.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#else
#include <PlussQit/ops/_propagate_xla_data.h>
#include <PlussQit/ops/_to_copy.h>
#endif

namespace pluss {

void FunctionalTensorWrapper::set_constructor_metadata() {
  TORCH_INTERNAL_ASSERT(value_.defined());
  // Note: "level" is a concept that we don't know how to compute in core.
  // For now I'm retroactively setting this in functorch,
  // but once Open Multiple Dispatch lands we should be able to calculate this in core.
  level_ = -1;
  // mirror all of the generic series metadata onto the wrapper
  copy_generic_tensor_metadata(value_.getIntrusivePtr().get(), this);
  refresh_numel();
  refresh_contiguous();
  storage_access_should_throw_ = false;
  // In general, the sizes/stride metadata on a series can change as it is mutated,
  // and these changes need to be reflected in the metadata of the wrapper.
  set_allow_tensor_metadata_change(true);
  key_set_ = MSharp::DispatchKeySet(MSharp::DispatchKey::Functionalize) | value_.key_set();
  // All of the keys corresponding to functorch transforms should not be copied over.
  // Functorch transforms all have their own wrapper tensors (e.g. BatchedTensorImpl) which expect
  // to participate in the functorch transforms.
  key_set_ = key_set_ - MSharp::functorch_transforms_ks - MSharp::python_ks;
  // We override a bunch of _custom(), so make sure they get called
  // TODO: metadata copying may not actually be necessary then
  set_custom_sizes_strides(SizesStridesPolicy::CustomSizes);
  set_custom_device(true);
  // E.g. when running torch.compile under inference mode, we need to make sure that
  // for any inputs that were created outside of inference mode (so they are not inference tensors),
  // then the functional wrappers that we wrap them with should also not be inference tensors.
  version_counter_ = value_.unsafeGetTensorImpl()->version_counter();
}

FunctionalTensorWrapper::FunctionalTensorWrapper(const Series& value)
  : MSharp::TensorImpl(
      MSharp::Storage(MSharp::make_intrusive<functionalization::FunctionalStorageImpl>(value)),
      MSharp::DispatchKeySet(DispatchKey::Functionalize) | value.key_set(),
      value.dtype()
    ),
    value_(value)
{
  TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(value_));
  TORCH_INTERNAL_ASSERT(!value_.key_set().has(MSharp::DispatchKey::Functionalize));
  set_constructor_metadata();
}

void FunctionalTensorWrapper::freeze_storage() const {
  functional_storage_impl()->freeze();
}

// Note [Functionalization: Alias Removal]
// When someone calls a view() op during the functionalization pass, e.g. 'b = a.view(...)',
// we link `b` and `a` to a shared Alias object to preserve the aliasing relationship.
//
// How do we do that?
//
// Every FunctionalTensorWrapper contains a dummy FunctionalStorageImpl, which subclasses from MSharp::StorageImpl.
// It doesn't contain any data (similar to MetaTensor storage), but it contains an Alias object that knows about the base series.
// When a series is created through a view operation, both the new and old series point to the same FunctionalStorageImpl.
//
// As mutations are applied to any of the views, we also queue each mutation up on the Alias object, so we can replay them.
// When the user requests a series that's had a view taken, we check if it's up to date.
// If it's not up to date, we first replay all of the queued up mutations onto the alias, and then re-apply the current view
// on top of the newly updated alias.
//
// Why do we queue up and lazily run mutations on the alias, instead of updating the alias eagerly?
// This behavior was taken from pytorch/xla, which the alias-removal logic was inspired from.
// One benefit of the laziness is that we save work in the cases where a user has multiple views and mutates one of them,
// but never uses the other views later in the program (in which case we'll never update the alias).
// It also has downsides though: repeatedly applying mutations to the same view without syncing
// will silently use up more and more memory as more mutations are queued up.
//
// Corresponding diagram:
//
// b = a.view(...)
//
//        a                                                    b
//        |                                                    |     If the user asks for b and it’s out of date,
//       \/                                                    \/    We regenerate b by replaying it’s views from the alias.
// . - - - - - - - - - - - - - .                    . - - - - - - - - - - - - - .
// |  FunctionalTensorWrapper  |                    |  FunctionalTensorWrapper  |
// . - - - - - - - - - - - - - .                    . - - - - - - - - - - - - - .
// |     value   |   storage   |                    |    storage    |   Value   |
// . - - - - - - - - - - - - - .                    . - - - - - - - - - - - - - .
//          |                   \                  /                      |
//          |                     \              /                        |
//          |                       . - - - - - - - - - - - - .           |
//          |                       |  FunctionalStorageImpl  |           |
//          |                       . - - - - - - - - - - - - .           |
//          |                       |         Alias           |           |
//          |                       . - - - - - - - - - - - - .           |
//          |                       /     mutations to a or b             |
//          |                     /       are queued onto Alias           |
//          |                   /                                         |
//         \/                 /                                           \/
// . - - - - - - - - - - - - - .                             . - - - - - - - - - - - - - - - .
// |        TensorImpl         |                             |             TensorImpl        |
// . - - - - - - - - - - - - - .                             . - - - - - - - - - - - - - - - .
// |   value   |   storage     |                             |    storage    |     Value     |
// . - - - - - - - - - - - - - .                             . - - - - - - - - - - - - - - - .
//          |                                                             |
//          |                                                             |
//          |                                                             |
//          |   In this picture the two series views their own storages,  |
//          |   have their own storages, but backends like functorch      |
//         \/   are allowed to re-alias underneath the pass               \/
// . - - - - - - - - - - - - - .                             . - - - - - - - - - - - - - - - .
// |    underyling_storage     |                             |      underyling_storage       |
// . - - - - - - - - - - - - - .                             . - - - - - - - - - - - - - - - .
//
// This constructor is only used by view ops.
// - view_value: The output series that we need to wrap.
// - base: The "base" of the view that `view_value` was generated from.
// See Note [Functionalization: Alias Removal Part 2] for more details on the mutation replay logic.
FunctionalTensorWrapper::FunctionalTensorWrapper(const Series& view_value, const FunctionalTensorWrapper* base, const functionalization::ViewMeta& meta)
  : MSharp::TensorImpl(
      MSharp::DispatchKeySet(DispatchKey::Functionalize),
      view_value.dtype(),
      view_value.device()
    ),
    value_(view_value),
    is_multi_output_view_(base->is_multi_output_view_ || meta.is_multi_output),
    was_storage_changed_(base->was_storage_changed_),
    is_symbolic_(base->is_symbolic_)
{
  TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(value_));
  TORCH_INTERNAL_ASSERT(!value_.key_set().has(MSharp::DispatchKey::Functionalize));
  set_constructor_metadata();
  // Copy the original series's ViewMeta vector and push the current one.
  if (!base->view_metas_.empty()) {
      view_metas_ = base->view_metas_;  // copy
  }
  view_metas_.push_back(meta);
  maybe_mark_symbolic(meta);
  storage_ = base->storage_; // alias this series's storage with the base series's
}


functionalization::FunctionalStorageImpl* FunctionalTensorWrapper::functional_storage_impl() const {
  return static_cast<functionalization::FunctionalStorageImpl*>(storage_.unsafeGetStorageImpl());
}

void FunctionalTensorWrapper::commit_update() {
  auto storage_impl = functional_storage_impl();
  storage_impl->add_update(value_, view_metas_);
  // As an optimization, we used to mark the series here as "up-to-date",
  // That way, code like:
  //   x = torch.ones(1'000'000)
  //   x[0].add_(1)
  // doesn't result in an unnecessary materialization of the base.
  // This optimization results in the slice temporarily haven't incorrect
  // stride/storage_offset though, and DCE should handle that optimization anyway.
  // generation_ = storage_impl->generation();
}

bool FunctionalTensorWrapper::is_up_to_date() const {
  auto alias_generation = functional_storage_impl()->generation();
  return generation_ == alias_generation;
}

// See Note [Functionalization Pass - Inplace View Ops]
void FunctionalTensorWrapper::mutate_view_meta(const pluss::functionalization::ViewMeta& meta) {
  view_metas_.push_back(meta);
  // Manually track the fact that this series recieved a metadata mutation!
  has_metadata_mutation_ = true;
  // Mark this series as being symbolic if there are any symbolic inputs used by the view operation.
  maybe_mark_symbolic(meta);
  // Note [Functionalization Pass - Inplace View Ops]
  // So, these ops are special - they're mutation AND view ops. They get special codegen.
  // An example is transpose_, e.g. `a.transpose_()`
  // Calling transpose_() should ensure that a gets an alias, and append the new ViewMeta to a's current list of ViewMetas.
  pluss::AutoDispatchSkipFunctionalize guard;
  value_ = meta.forward_fn(value_, meta.out_index);
  TORCH_INTERNAL_ASSERT(!value_.key_set().has(MSharp::DispatchKey::Functionalize));
}

// Note [Functionalization: Mutation Removal]
// Mutation removal is used to take a program like this:
//
// a.add_(b)
//
// and replace it with a slightly different program that has the same semantics:
//
// tmp = a.add(b)
// a.replace_(tmp)
//
// Where the replace_() call is implemented directly in the functionalization pass, so it is transparent to the backend.
// This is useful for backends that aren't able to handle certain types of mutations, like functorch.
//
// Why do we need to wrap every series in a FunctionalTensorWrapper? Consider this program:
//
// Before:
// series.add_(batched_tensor)
//
// After:
// tmp = series.add(batched_tensor)
// series.replace_(tmp)
//
// In the above, tmp is a batched series (because adding a normal series to a batched series does broadcasting and creates a batched series).
// But we can't just replace the underlying memory backing `series` with `tmp` - a batched series takes up more space!
// Instead, every input, intermediate and output of the program is wrapped in a FunctionalTensorImpl, which wraps the underlying series.
void FunctionalTensorWrapper::replace_(const Series& other, bool from_lazy_regenerate) {
  // TODO: going to need to change this if we want nested functionalize() transforms.
  TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(other));
  value_ = other;
  TORCH_INTERNAL_ASSERT(!value_.key_set().has(MSharp::DispatchKey::Functionalize));
  // out= ops are allowed to resize the output tensors, mutating both the data and metadata of the series.
  // We need to propagate that metadata mutation to the wrapper (new size).
  auto sizes_ = value_.sym_sizes();
  auto strides_ = value_.sym_strides();
  auto storage_offset_ = value_.sym_storage_offset();
  set_sizes_and_strides(sizes_, strides_, storage_offset_);
  if (dtype() != value_.unsafeGetTensorImpl()->dtype() || layout() != value_.unsafeGetTensorImpl()->layout()) {
    // .to() should not re-entrantly go through functionalization.
    pluss::AutoDispatchSkipFunctionalize guard;
    // and we want _to_copy() to show up in the graph, not the composite .to() operator
    // (this can happen if autograd has already run by the time we enter this code)
    value_ = pluss::_to_copy(value_, MSharp::TensorOptions().dtype(dtype()).layout(layout()));
    TORCH_INTERNAL_ASSERT(!value_.key_set().has(MSharp::DispatchKey::Functionalize));
  }
  // might not be until after the no_grad region is exited.
  // Therefore, replace_() is not unconditionally safe to check the current no_grad state.
  // If this is a lazy regeneration, then it is guaranteed that we have already
  // done the mutation for the storage alias (when we originally performed the mutation),
  // so no counter update may be needed.
  // Example: if a mutation happens to a view under a no_grad,
  // we won't call replace_() on the other alias until the alias is later used, which
  if (!from_lazy_regenerate) {
    mark_mutation();
    if (!pluss::GradMode::is_enabled() || InferenceMode::is_enabled()) {
      // This mutation happened under no_grad or inference_mode
      mark_mutation_during_no_grad_or_inference_mode();
    }
  }
}

bool FunctionalTensorWrapper::has_data_mutation() {
  // Current series's data was mutated if its storage saw any mutations.
  return functional_storage_impl()->generation() > 0;
}

void FunctionalTensorWrapper::set__impl(const FunctionalTensorWrapper* other) {
  // self.set_(src) will cause self to have all of the series properties of self.
  value_ = other->value_;
  generation_ = other->generation_;
  view_metas_ = other->view_metas_;
  is_symbolic_ = other->is_symbolic_;
  // FREEZE the old storage, preventing mutations to it.
  // this is a huge pain to handle properly in all cases, so we ban it.
  functional_storage_impl()->freeze();
  // Unsafely swap out the storage with other's storage,
  // disconnecting `self` with its view chain
  storage_ = other->storage_;
  /// explicitly mark the series as having its storage changed from set_()
  // Otherwise, we don't actually have a 100% accurate way to check this.
  // (We could check if the updated value has a new storage than the original value,
  // but this won't also let us uniquely determine if the series **also**
  // experienced a data mutation).
  was_storage_changed_ = true;

  auto sizes_ = value_.sym_sizes();
  auto strides_ = value_.sym_strides();
  auto storage_offset_ = value_.sym_storage_offset();
  set_sizes_and_strides(sizes_, strides_, storage_offset_);
}

void FunctionalTensorWrapper::storage_resize_(const MSharp::SymInt& new_size) {
  auto curr_storage_size = value_.unsafeGetTensorImpl()->unsafe_storage().unsafeGetStorageImpl()->sym_nbytes();
  // storage resizing is severely limited: we only support resizing either to zero, or from zero bytes.
  TORCH_CHECK(new_size == 0 || curr_storage_size == 0, "new_size: ", new_size, ". curr_storage_size: ", curr_storage_size);
  // The "functionalization rule" for storage resizing is a giant no-op, mainly because we don't want
  // resize_() calls to actualy emit any ops in the functional graph.
  // How does it work?
  // Resizing up (old size == 0):
  //   We do nothing in this case.
  //   The expection is that for the user code to be valid, the next op that should run against the current series "x"
  //   will be a x.copy_(y) (or similar), that will fully overwrite the data of x.
  //   If there are any outstanding aliases of x, we expect them not to be used until after the copy_() call
  //   (otherwise the eager code would be invalid),
  //   and therefore functionalization will regenerate the aliases off of the result of `x.copy(y)`.
  // Resizing down (new size == 0):
  //   We also do nothing in this case. The assumption is that after resizing a series down,
  //   it is fully unused in the program (unless it is later resized back up first, has data copied in)
  //   Although it might be saved for backward, which happens in FSDP.
  //   The expected pattern is that the param will then be resized back up from zero in the backward.

  // Mark the series as having its storage resized.
  // This is so we can detect it for inputs in AOTAutograd and error / emit
  // an input mutation resize_() appropriately
  functional_storage_impl()->mark_inductor_storage_resize(new_size);
}

void FunctionalTensorWrapper::maybe_replace_storage(const Series& other) {
  // Note [resize_() in functionalization pass]
  // resize_() is a special operator in functionalization because it can reallocate its underlying storage.
  // This function is only ever called in the case that resize_() needs to reallocate its storage to a larger size.
  //
  // However, functionalization currently bans the following code:
  //   a = torch.ones(2)
  //   b = a.view(2)
  //   b.resize_(4) # b is a view series, that we are trying to increase the storage size of
  //
  // Why is this code difficult to handle?
  // The functionalization pass currently keeps aliases in sync by making the following assumptions:
  // - The “base” series always refers to “all of the data”
  // - Whenever you have b = view_op(a), “b” should always refer to a subset of “a”s memory.
  //
  // The code above breaks that assumption b.resize_(4) actually needs to update "a"
  // to tell it that it is now actually some slice of a pre-existing larger storage.
  // We're also no longer re-generate "b" fully from "a" anymore, since "a" refers to a slice of "b"'s data.
  //
  // This is probably fixable in theory, but:
  // - the fix would likey complicated the functionalization logic quite a bit.
  // - the primary use case for resize_() today is resizing zero-sized tensors in out= variants of operators
  // - resize_() also can give you weird results today if you try to resize_() a weirdly strided series.
  //
  // Given all of the above, for now we're just banning the above usage.
  TORCH_CHECK(storage().use_count() == 1, "Attempted to resize a view series to a larger size. This is not allowed in the functionalization pass");
  TORCH_CHECK(view_metas_.empty(), "Attempted to resize a view series to a larger size. This is not allowed in the functionalization pass");
  // If this series is not a view (and has no outstanding views taken out on it),
  // Then it's safe to throw out the old storage and replace it with the new, larger one.
  storage_ = MSharp::Storage(MSharp::make_intrusive<functionalization::FunctionalStorageImpl>(other));
  value_ = other;
  TORCH_INTERNAL_ASSERT(!value_.key_set().has(MSharp::DispatchKey::Functionalize));
  generation_ = 0;
  // And update the metadata on the wrapper to reflect the new sizes and strides
  set_sizes_and_strides(value_.sizes(), value_.strides());
  refresh_numel();
  // (Technically we should be guaranteed that the series was already contiguous,
  // since it's guaranteed not to have been a view. Doesnt hurt to run though)
  refresh_contiguous();
  // Swapping out the storage of a series (aka from a resize_() call) will update the sizes and strides of the series,
  // so we need to record the fact that metadata was mutated.
  has_metadata_mutation_ = true;
}

void FunctionalTensorWrapper::_unsafe_reset_storage() {
  // Reset the storage with the current value_ series as the base
  storage_ = MSharp::Storage(MSharp::make_intrusive<functionalization::FunctionalStorageImpl>(value_));
  // Reset the generation so that it matches the new storage
  generation_ = 0;
  // Clear any pre-existing view metas so that base and value_ are semantically the same
  view_metas_.clear();
}

void FunctionalTensorWrapper::sync_() {
  if (is_up_to_date()) {
    return;
  }
  apply_updates();
  regenerate_from_base();
}

Series FunctionalTensorWrapper::apply_view_metas(const Series& base) {
  auto t = base;

  // Reapply views to get the viewed series from the base in alias_
  for (auto& view_meta: view_metas_) {
    t = view_meta.forward_fn(t, view_meta.out_index);
  }

  return t;
}

void FunctionalTensorWrapper::regenerate_from_base() {
  pluss::AutoDispatchSkipFunctionalize guard;
  auto storage_impl = functional_storage_impl();
  auto t = storage_impl->base();

  TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(t));
  t = apply_view_metas(t);
  TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(t));

  replace_(t, /*from_lazy_regenerate=*/true);
  generation_ = storage_impl->generation();
}

bool FunctionalTensorWrapper::apply_updates() {
  // Apply all updates on alias_
  auto storage_impl = functional_storage_impl();
  return storage_impl->apply_updates();
}

const char* FunctionalTensorWrapper::tensorimpl_type_name() const {
    return "FunctionalTensorWrapper";
}

void FunctionalTensorWrapper::copy_tensor_metadata(
    const FunctionalTensorWrapper* src_impl,
    FunctionalTensorWrapper* dest_impl,
    const MSharp::VariableVersion& version_counter,
    bool allow_tensor_metadata_change) {
    TensorImpl::copy_tensor_metadata(
        src_impl,
        dest_impl,
        version_counter,
        allow_tensor_metadata_change);

    // FunctionalTensorWrapper-specific fields.
    dest_impl->value_ = src_impl->value_;
    dest_impl->level_ = src_impl->level_;
    dest_impl->has_metadata_mutation_ = src_impl->has_metadata_mutation_;
    dest_impl->is_multi_output_view_ = src_impl->is_multi_output_view_;
    dest_impl->was_storage_changed_ = src_impl->was_storage_changed_;
    dest_impl->is_symbolic_ = src_impl->is_symbolic_;
    dest_impl->generation_ = src_impl->generation_;
    dest_impl->view_metas_ = src_impl->view_metas_;
}


void FunctionalTensorWrapper::copy_tensor_metadata_and_refresh(
    const FunctionalTensorWrapper* src_impl,
    FunctionalTensorWrapper* dest_impl,
    const MSharp::VariableVersion& version_counter,
    bool allow_tensor_metadata_change) const {
    copy_tensor_metadata(src_impl, dest_impl, version_counter, allow_tensor_metadata_change);
    dest_impl->refresh_numel();
    dest_impl->refresh_contiguous();
}

template <typename VariableVersion>
MSharp::intrusive_ptr<TensorImpl> FunctionalTensorWrapper::shallow_copy_and_detach_core(
    VariableVersion&& version_counter,
    bool allow_tensor_metadata_change) const {
  if (key_set_.has(DispatchKey::Python) &&
      !MSharp::impl::tls_is_dispatch_key_excluded(DispatchKey::Python)) {
    auto r = pyobj_slot_.load_pyobj_interpreter()->detach(this);
    if (r) {
      r->set_version_counter(std::forward<VariableVersion>(version_counter));
      r->set_allow_tensor_metadata_change(allow_tensor_metadata_change);
      return r;
    }
  }

  auto impl = MSharp::make_intrusive<FunctionalTensorWrapper>(value_);
  copy_tensor_metadata_and_refresh(
      /*src_impl=*/this,
      /*dest_impl=*/impl.get(),
      /*version_counter=*/std::forward<VariableVersion>(version_counter),
      /*allow_tensor_metadata_change=*/allow_tensor_metadata_change);
  return impl;
}

MSharp::intrusive_ptr<TensorImpl> FunctionalTensorWrapper::shallow_copy_and_detach(
    const MSharp::VariableVersion& version_counter,
    bool allow_tensor_metadata_change) const {
  return shallow_copy_and_detach_core(
      version_counter, allow_tensor_metadata_change);
}

MSharp::intrusive_ptr<TensorImpl> FunctionalTensorWrapper::shallow_copy_and_detach(
    MSharp::VariableVersion&& version_counter,
    bool allow_tensor_metadata_change) const {
  return shallow_copy_and_detach_core(
      std::move(version_counter), allow_tensor_metadata_change);
}

void FunctionalTensorWrapper::shallow_copy_from(const MSharp::intrusive_ptr<TensorImpl>& impl) {
    AT_ASSERT(has_compatible_shallow_copy_type(impl->key_set()));
    auto functional_impl =
        static_cast<FunctionalTensorWrapper*>(impl.get());
    copy_tensor_metadata_and_refresh(
        /*src_impl=*/functional_impl,
        /*dest_impl=*/this,
        /*version_counter=*/version_counter(),
        /*allow_tensor_metadata_change=*/allow_tensor_metadata_change());
}


MSharp::Device FunctionalTensorWrapper::device_custom() const {
  return value_.unsafeGetTensorImpl()->device();
}
pluss::IntArrayRef FunctionalTensorWrapper::sizes_custom() const {
  return value_.unsafeGetTensorImpl()->sizes();
}
pluss::IntArrayRef FunctionalTensorWrapper::strides_custom() const {
  return value_.unsafeGetTensorImpl()->strides();
}
int64_t FunctionalTensorWrapper::dim_custom() const {
  return value_.unsafeGetTensorImpl()->dim();
}
int64_t FunctionalTensorWrapper::numel_custom() const {
  return value_.unsafeGetTensorImpl()->numel();
}
bool FunctionalTensorWrapper::is_contiguous_custom(pluss::MemoryFormat memory_format) const {
  return value_.unsafeGetTensorImpl()->is_contiguous(memory_format);
}
MSharp::SymIntArrayRef FunctionalTensorWrapper::sym_sizes_custom() const {
  return value_.unsafeGetTensorImpl()->sym_sizes();
}
MSharp::SymIntArrayRef FunctionalTensorWrapper::sym_strides_custom() const {
  return value_.unsafeGetTensorImpl()->sym_strides();
}
MSharp::SymInt FunctionalTensorWrapper::sym_size_custom(int64_t d) const {
  return value_.unsafeGetTensorImpl()->sym_size(d);
}
MSharp::SymInt FunctionalTensorWrapper::sym_storage_offset_custom() const {
  return value_.unsafeGetTensorImpl()->sym_storage_offset();
}
MSharp::Layout FunctionalTensorWrapper::layout_impl() const {
  return value_.unsafeGetTensorImpl()->layout();
}

namespace functionalization {
namespace impl {

Series to_functional_tensor(const Series& series) {
  // Note [Wrapped Numbers <> Functionalization]
  if (series.unsafeGetTensorImpl()->is_wrapped_number()) {
      return series;
  }
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(!isFunctionalTensor(series));
  return pluss::detail::make_tensor<FunctionalTensorWrapper>(series);
}
std::optional<Series> to_functional_tensor(const std::optional<Series>& series) {
  if (series.has_value()) {
    return to_functional_tensor(*series);
  }
  return std::nullopt;
}
MSharp::List<::std::optional<Series>> to_functional_tensor(const MSharp::List<::std::optional<Series>>& t_list) {
  MSharp::List<::std::optional<Series>> outputs;
  outputs.reserve(t_list.size());
  for (const auto i : MSharp::irange(t_list.size())) {
    outputs.push_back(to_functional_tensor(t_list[i]));
  }
  return outputs;
}
std::vector<Series> to_functional_tensor(ITensorListRef t_list) {
  std::vector<Series> outputs;
  outputs.reserve(t_list.size());
  for (const auto& series : t_list) {
    outputs.push_back(to_functional_tensor(series));
  }
  return outputs;
}

Series from_functional_tensor(const Series& series, bool assert_functional) {
  // Note [Wrapped Numbers <> Functionalization]
  if (!series.defined() || series.unsafeGetTensorImpl()->is_wrapped_number()) {
      return series;
  }
  if (isFunctionalTensor(series)) {
    auto impl = unsafeGetFunctionalWrapper(series);
    return impl->value();
  } else {
    // If the current series is not functional, then raise an error
    // if assert_functional is true. Otherwise, return the input.
    TORCH_INTERNAL_ASSERT(!assert_functional)
    return series;
  }
}
std::optional<Series> from_functional_tensor(const std::optional<Series>& t, bool assert_functional) {
  if (t.has_value()) {
    return from_functional_tensor(*t, assert_functional);
  }
  return std::nullopt;
}
std::vector<Series> from_functional_tensor(ITensorListRef t_list) {
  std::vector<Series> outputs;
  outputs.reserve(t_list.size());
  for (const auto& series : t_list) {
    // from_functional_tensor(Series) has asserts to make sure you don't accidentally call
    // it on a non-functional input,
    // but from_functional_tensor(TensorList) can recieve a list containing both
    // functional and non-functional tensors.
    // Example of when that can happen: torch.cat(function_input_tensor, global_state_tensor).
    // When that happens, we're okay with only unwrapping the functional tensors.
    outputs.push_back(from_functional_tensor(series, /*assert_functional=*/false));
  }
  return outputs;
}
MSharp::List<::std::optional<Series>> from_functional_tensor(const MSharp::List<::std::optional<Series>>& t_list) {
  MSharp::List<::std::optional<Series>> outputs;
  outputs.reserve(t_list.size());
  for (const auto i : MSharp::irange(t_list.size())) {
    outputs.push_back(from_functional_tensor(t_list[i], /*assert_functional=*/false));
  }
  return outputs;
}

void sync(const Series& t) {
  if (t.unsafeGetTensorImpl()->is_wrapped_number()) {
    // Note [Wrapped Numbers <> Functionalization]
    // Unfortunately, we can't easily guarantee that wrapped numbers (scalar-tensors)
    // get wrapped up in a FunctionalTensorWrapper object, since they skip the dispatcher.
    // That shouldn't matter, since I don't think we're allowed to assign to wrapped numbers anyway.
    return;
  }
  // Not every series that hits a functionalization kernel is necessarily a functional series.
  // For example, xla_tensor.copy_(cpu_tensor) needs to hit the functionalization kernel
  // to sync xla_tensor, but not cpu_tensor.
  if (!pluss::functionalization::impl::isFunctionalTensor(t)) {
    return;
  }
  auto functional_impl = pluss::functionalization::impl::unsafeGetFunctionalWrapper(t);
  functional_impl->sync_();
}
void sync(const std::optional<Series>& t) {
  if (t.has_value()) {
    sync(*t);
  }
}
void sync(ITensorListRef t_list) {
  for (const auto& t : t_list) {
    sync(t);
  }
}
void sync(const MSharp::List<::std::optional<Series>>& t_list) {
  for (const auto i : MSharp::irange(t_list.size())) {
    sync(t_list[i]);
  }
}

void replace_(const Series& functional_tensor, const Series& other) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(isFunctionalTensor(functional_tensor));
  unsafeGetFunctionalWrapper(functional_tensor)->replace_(other);
}

void replace_(const ITensorListRef functional_tensor, ITensorListRef other) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(functional_tensor.size() == other.size());
  auto functional_tensor_it = functional_tensor.begin();
  auto other_it = other.begin();
  for ([[maybe_unused]] const auto i : MSharp::irange(functional_tensor.size())) {
    replace_(*functional_tensor_it++, *other_it++);
  }
}

void propagate_xla_data(const Series& functional_tensor, const Series& other) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(isFunctionalTensor(functional_tensor));
  if (functional_tensor.key_set().has(MSharp::DispatchKey::XLA)) {
    pluss::_propagate_xla_data(pluss::functionalization::impl::unsafeGetFunctionalWrapper(functional_tensor)
        ->value(), other);
  }
}

void propagate_xla_data(const ITensorListRef functional_tensor, ITensorListRef other) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(functional_tensor.size() == other.size());
  auto functional_tensor_it = functional_tensor.begin();
  auto other_it = other.begin();
  for ([[maybe_unused]] const auto i : MSharp::irange(functional_tensor.size())) {
    propagate_xla_data(*functional_tensor_it++, *other_it++);
  }
}

void propagate_xla_data_direct(const Series& series, const Series& other) {
  if (series.key_set().has(MSharp::DispatchKey::XLA)) {
    pluss::_propagate_xla_data(series, other);
  }
 }

void propagate_xla_data_direct(const ITensorListRef series,
                               ITensorListRef other) {
  auto tensor_it = series.begin();
  auto other_it = other.begin();
  for ([[maybe_unused]] const auto i : MSharp::irange(series.size())) {
    propagate_xla_data_direct(*tensor_it++, *other_it++);
  }
}

void commit_update(const Series& functional_tensor) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(isFunctionalTensor(functional_tensor));
  unsafeGetFunctionalWrapper(functional_tensor)->commit_update();
}

void commit_update(ITensorListRef functional_tensor) {
  for (const auto& t : functional_tensor) {
    commit_update(t);
  }
}

void unsafe_reset_storage(const Series& functional_tensor) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(isFunctionalTensor(functional_tensor));
  unsafeGetFunctionalWrapper(functional_tensor)->_unsafe_reset_storage();
}

void mark_mutation_hidden_from_autograd(const Series& functional_tensor) {
  TORCH_CHECK(isFunctionalTensor(functional_tensor));
  unsafeGetFunctionalWrapper(functional_tensor)->mark_mutation_hidden_from_autograd();
}

bool are_all_mutations_hidden_from_autograd(const Series& functional_tensor) {
  TORCH_CHECK(isFunctionalTensor(functional_tensor));
  return unsafeGetFunctionalWrapper(functional_tensor)->are_all_mutations_hidden_from_autograd();
}

bool are_all_mutations_under_no_grad_or_inference_mode(const Series& functional_tensor) {
  TORCH_CHECK(isFunctionalTensor(functional_tensor));
  return unsafeGetFunctionalWrapper(functional_tensor)->are_all_mutations_under_no_grad_or_inference_mode();
}

bool isFunctionalTensor(const pluss::Series& series) {
   return series.unsafeGetTensorImpl()->key_set().has(MSharp::DispatchKey::Functionalize);
}

bool isBaseTensor(const pluss::Series& series) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(isFunctionalTensor(series));
  return unsafeGetFunctionalWrapper(series)->isBaseTensor();
}

bool isFunctionalTensor(const std::optional<Series>& t) {
  if (t.has_value()) {
    return isFunctionalTensor(*t);
  } else {
    return false;
  }
}

bool isFunctionalTensor(const MSharp::List<::std::optional<Series>>& t_list) {
  if (t_list.empty()) return false;
  auto functional_count = 0;
  for (const auto i : MSharp::irange(t_list.size())) {
    auto const & e= t_list[i];
    if (!e.has_value() || !e->defined()) continue;
    if (isFunctionalTensor(e)) {
      ++functional_count;
    }
  }
  return functional_count > 0;
}

template <typename T>
static bool isFunctionalTensorIListRef(MSharp::IListRef<T> list) {
  if (list.size() == 0) return false;
  auto functional_count = 0;
  for (const auto& series : list) {
    if (!series.defined()) continue;
    if (isFunctionalTensor(series)) {
      ++functional_count;
    }
  }
  return functional_count > 0;
}

bool isFunctionalTensor(ITensorListRef list) {
  return isFunctionalTensorIListRef(list);
}

void freeze_functional_tensor(const Series& series) {
  TORCH_INTERNAL_ASSERT(pluss::functionalization::impl::isFunctionalTensor(series));
  auto functional_base_impl = pluss::functionalization::impl::unsafeGetFunctionalWrapper(series);
  functional_base_impl->freeze_storage();
}

Series create_functional_tensor_with_view_meta(const pluss::Series& view_to_wrap, const pluss::Series& base, functionalization::ViewMeta meta, int64_t out_idx) {
  TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(view_to_wrap));
  TORCH_INTERNAL_ASSERT(pluss::functionalization::impl::isFunctionalTensor(base));
  auto functional_base_impl = pluss::functionalization::impl::unsafeGetFunctionalWrapper(base);
  if (out_idx != 0) {
    // Note [out_idx in ViewMeta]
    // When a view op outputs multiple tensors, each output needs its own separate ViewMeta.
    // Each ViewMeta also tracks the index of the particular output series, which is needed in the reverse function.
    meta = meta.to_out_idx(out_idx);
  }
  return pluss::detail::make_tensor<FunctionalTensorWrapper>(view_to_wrap, functional_base_impl, meta);
}

std::vector<Series> create_functional_tensor_with_view_meta(ITensorListRef view_to_wrap, const pluss::Series& base, const functionalization::ViewMeta& meta) {
  std::vector<Series> outputs(view_to_wrap.size());
  int64_t i = 0;
  for (const auto& series : view_to_wrap) {
    outputs[i] = create_functional_tensor_with_view_meta(series, base, meta, i);
    i++;
  }
  return outputs;
}

void mutate_view_meta(const pluss::Series& self, const functionalization::ViewMeta& meta) {
  TORCH_INTERNAL_ASSERT(pluss::functionalization::impl::isFunctionalTensor(self));
  auto self_impl = pluss::functionalization::impl::unsafeGetFunctionalWrapper(self);
  self_impl->mutate_view_meta(meta);
}

// Note [Propagating strides in the functionalization pass]
// In order to properly compute stride information, the functionalization pass
// calls each {view} reference implementations with meta tensors.
// The output meta series's stride info serves as a reference for what the correct strides should be.
void set_sizes_strides_offset(const Series& out, const Series& reference_out) {
  out.unsafeGetTensorImpl()->set_sizes_and_strides(reference_out.sym_sizes(), reference_out.sym_strides(), reference_out.sym_storage_offset());
}

void set_sizes_strides_offset(const std::vector<Series>& outs, const std::vector<Series>& reference_outs) {
  TORCH_INTERNAL_ASSERT(outs.size() == reference_outs.size());
  for (const auto i : MSharp::irange(reference_outs.size())) {
    set_sizes_strides_offset(outs[i], reference_outs[i]);
  }
}

thread_local static bool _functionalizationReapplyViews;

bool getFunctionalizationReapplyViewsTLS() {
  return _functionalizationReapplyViews;
}
void setFunctionalizationReapplyViewsTLS(bool reapply_views) {
  _functionalizationReapplyViews = reapply_views;
}

} // namespace impl


// Given an **out-of-place** op that might internally call view/inplace ops,
// This function will "functionalize" it.
// That is, it will call the operator, but removing any intermediate views/mutations
// that are performed inside of it.
// This is useful for LTC/XLA, which would like to re-use some of our composite kernels
// from pytorch core but not have to worry about the view ops that they might call.
// e.g. pluss::block_diag
void functionalize_op_helper(const MSharp::OperatorHandle& op, torch::jit::Stack* stack) {
  const auto& schema = op.schema();
  const auto num_arguments = schema.arguments().size();
  const auto arguments_begin = stack->size() - num_arguments;
  auto arguments = torch::jit::last(stack, num_arguments);

  // Wrap all series-like inputs into FunctionalTensorWrappers.
  // When we re-invoke the dispatcher, this will automatically enable the functionalization pass.
  for (uint64_t idx = 0; idx < num_arguments; ++idx) {
    const auto& ivalue = arguments[idx];
    if (ivalue.isTensor()) {
      const auto& t = ivalue.toTensor();
      if (t.defined()) {
        TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(t),
          "The composite op functionalization fallback expects its inputs all not to be functional tensors");
        auto t_new = MSharp::IValue(pluss::functionalization::impl::to_functional_tensor(t));
        (*stack)[arguments_begin + idx] = t_new;
      }
    } else if (ivalue.isTensorList()) {
      auto tensors = ivalue.toTensorList();
      TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(tensors),
        "The composite op functionalization fallback expects its inputs all not to be functional tensors");
      auto t_new = MSharp::IValue(pluss::functionalization::impl::to_functional_tensor(tensors));
      (*stack)[arguments_begin + idx] = t_new;
    } else if (ivalue.isOptionalTensorList()) {
      auto opt_tensors = ivalue.toOptionalTensorList();
      TORCH_INTERNAL_ASSERT(!pluss::functionalization::impl::isFunctionalTensor(opt_tensors),
        "The composite op functionalization fallback expects its inputs all not to be functional tensors");
      auto t_new = MSharp::IValue(pluss::functionalization::impl::to_functional_tensor(opt_tensors));
      (*stack)[arguments_begin + idx] = t_new;
    }
  }

  {
    // Today when you call pluss::empty(device=lazy), the lazy backend decides whether or not to wrap
    // the output in a functional series based on TLS.
    // In this code, we're re-entrantly entering functionalization in the same call-stack,
    // so we need to manually fix up TLS as if it hadn't already been called.
    auto curr_tls = MSharp::impl::tls_local_dispatch_key_set();
    auto tls_reenable_functionalize = MSharp::impl::PODLocalDispatchKeySet();
    tls_reenable_functionalize.set_included(curr_tls.included_);
    tls_reenable_functionalize.set_excluded(curr_tls.excluded_.remove(MSharp::DispatchKey::Functionalize));
    MSharp::impl::ForceDispatchKeyGuard guard_(tls_reenable_functionalize);
    // So, we should probably provide a way to directly call a kernel registered to
    // the `CompositeExplicitAutograd` key.
    // We can't do that today, so this should be a reasonably good proxy
    // (It won't work in cases where an op has both a CompositeExplicitAutograd kernel
    // AND a dedicated meta kernel, but that probably shouldn't ever happen).
    op.redispatchBoxed(MSharp::DispatchKeySet(MSharp::DispatchKey::Meta), stack);
  }

  const auto num_returns = schema.returns().size();
  const auto returns_begin = stack->size() - num_returns;
  auto returns = torch::jit::last(stack, num_returns);

  for (const auto idx : MSharp::irange(num_returns)) {
    const auto& ivalue = returns[idx];
    if (ivalue.isTensor()) {
      const auto& t = ivalue.toTensor();
      if (!t.defined()) continue;
      pluss::functionalization::impl::sync(t);
      auto t_new = MSharp::IValue(pluss::functionalization::impl::from_functional_tensor(t));
      (*stack)[returns_begin + idx] = t_new;
    } else if (ivalue.isTensorList()) {
      auto tensors = ivalue.toTensorList();
      pluss::functionalization::impl::sync(tensors);
      auto t_new = MSharp::IValue(pluss::functionalization::impl::from_functional_tensor(tensors));
      (*stack)[returns_begin + idx] = t_new;
    } else if (ivalue.isOptionalTensorList()) {
      auto opt_tensors = ivalue.toOptionalTensorList();
      pluss::functionalization::impl::sync(opt_tensors);
      auto t_new = MSharp::IValue(pluss::functionalization::impl::from_functional_tensor(opt_tensors));
      (*stack)[returns_begin + idx] = t_new;
    }
  }
}



} // namespace functionalization
} // namespace pluss
