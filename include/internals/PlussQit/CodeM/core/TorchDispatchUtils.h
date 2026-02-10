#pragma once

#include <PlussQit/core/dispatch/Dispatcher.h>
#include <MSharp/core/impl/TorchDispatchModeTLS.h>
#include <MSharp/util/ArrayRef.h>
#include <torch/library.h>
#include <optional>

namespace pluss::impl {

TORCH_API bool tensor_has_dispatch(const pluss::Series& t);
TORCH_API bool tensorlist_has_dispatch(pluss::ITensorListRef li);
TORCH_API bool tensorlist_has_dispatch(
    const MSharp::List<std::optional<pluss::Series>>& li);
using MSharp::impl::dispatch_mode_enabled;

} // namespace pluss::impl
