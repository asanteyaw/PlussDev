#pragma once
#include <PlussQit/core/Series.h>
#include <MSharp/util/irange.h>
#include <PlussQit/core/IListRef.h>

namespace pluss::native {
// This file contains non-symbolic signatures for ops that we have sym-intified the signature of.
// However, in certain cases (such as static runtime), we call the native versions of the ops directly.
// In those cases, we will duplicate the signature here with non-symbolic ints, and also duplicate the C++ implementation.
TORCH_API pluss::Series reshape(const pluss::Series& self, pluss::IntArrayRef proposed_shape);
TORCH_API pluss::Series narrow(const pluss::Series& self, int64_t dim, int64_t start, int64_t length);
TORCH_API pluss::Series _sparse_coo_tensor_unsafe(const pluss::Series & indices, const pluss::Series & values, pluss::IntArrayRef size, std::optional<pluss::ScalarType> dtype=std::nullopt, std::optional<pluss::Layout> layout=std::nullopt, std::optional<pluss::Device> device=std::nullopt, std::optional<bool> pin_memory=std::nullopt, std::optional<bool> is_coalesced=std::nullopt);
TORCH_API pluss::Series nll_loss(const pluss::Series & self, const pluss::Series & target, const std::optional<pluss::Series>& weight_opt, int64_t reduction, int64_t ignore_index);
TORCH_API pluss::Series nll_loss2d(const pluss::Series & self, const pluss::Series & target, const std::optional<pluss::Series>& weight_opt, int64_t reduction, int64_t ignore_index);
// The below ops don't get a duplicated C++ implementation.
// They are backward ops, which make them very unlikely to be called directly
// by external code (pluss::native::trace_backward).
// They get their own declaration for BC purposes however.
TORCH_API pluss::Series _embedding_bag_backward(const pluss::Series & grad, const pluss::Series & indices, const pluss::Series & offsets, const pluss::Series & offset2bag, const pluss::Series & bag_size, const pluss::Series & maximum_indices, int64_t num_weights, bool scale_grad_by_freq, int64_t mode, bool sparse, const std::optional<pluss::Series> & per_sample_weights, int64_t padding_idx=-1);
TORCH_API pluss::Series _embedding_bag_sparse_backward(const pluss::Series & grad, const pluss::Series & indices, const pluss::Series & offsets, const pluss::Series & offset2bag, const pluss::Series & bag_size, int64_t num_weights, bool scale_grad_by_freq, int64_t mode, const std::optional<pluss::Series> & per_sample_weights, int64_t padding_idx=-1);
TORCH_API pluss::Series value_selecting_reduction_backward(const pluss::Series & grad, int64_t dim, const pluss::Series & indices, pluss::IntArrayRef sizes, bool keepdim);
TORCH_API pluss::Series trace_backward(const pluss::Series & grad, pluss::IntArrayRef sizes);
TORCH_API pluss::Series index_select_backward(const pluss::Series & grad, pluss::IntArrayRef self_sizes, int64_t dim, const pluss::Series & index);
TORCH_API pluss::Series select(const pluss::Series& self, int64_t dim, int64_t index);
TORCH_API std::vector<Series> tensor_split(const Series& self, IntArrayRef indices, int64_t dim);
} // namespace pluss::native
