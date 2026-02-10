#pragma once

#include <PlussQit/core/ivalue.h>
#include <PlussQit/core/stack.h>
#include <PlussQit/core/boxing/KernelFunction.h>
#include <PlussQit/core/dispatch/Dispatcher.h>
#include <MSharp/util/Metaprogramming.h>
#include <torch/library.h>

namespace pluss::native {

// This function implements a boxed fallback to CPU.
// External backends can add their own custom logging on top if it to customize their own CPU fallbacks.
TORCH_API void cpu_fallback(const MSharp::OperatorHandle& op, torch::jit::Stack* stack, bool error_on_views = false,
                            MSharp::DispatchKey cpu_dispatch_key = MSharp::DispatchKey::CPU);

// This is a helper function that backends can use to directly call their boxed CPU fallback
// TODO: update and add a usage example after https://github.com/pytorch/pytorch/pull/58092 lands.
template<MSharp::KernelFunction::BoxedKernelFunction* fallback_fn, class Op, bool symint, class ReturnType, class... ParameterTypes>
struct _call_fallback_fn final {};

template<MSharp::KernelFunction::BoxedKernelFunction* fallback_fn, class Op, bool symint, class ReturnType, class... ParameterTypes>
struct _call_fallback_fn<fallback_fn, Op, symint, ReturnType(ParameterTypes...)> final {
    static ReturnType call(typename MSharp::maybe_keep_symint<symint, ParameterTypes>::type... args) {
        auto op = MSharp::Dispatcher::singleton()
            // TODO: figure out how to make compiler happy without dynamic casts
            .findSchemaOrThrow((const char*) Op::name, (const char*) Op::overload_name)
            //.findSchemaOrThrow("a", "b")
            .typed<ReturnType (typename MSharp::maybe_keep_symint<symint, ParameterTypes>::type...)>();
        return MSharp::impl::BoxedKernelWrapper<ReturnType (typename MSharp::maybe_keep_symint<symint, ParameterTypes>::type...)>::call(
            MSharp::BoxedKernel::makeFromFunction<fallback_fn>(),
            op,
            MSharp::DispatchKeySet(), // we know that the cpu_fallback doesn't use the dispatch keyset.
            // TODO: get std::forward<> to work
            args...
            );
    }
};

template<MSharp::KernelFunction::BoxedKernelFunction* fallback_fn, class Op>
using call_fallback_fn_symint = _call_fallback_fn<fallback_fn, Op, true, typename Op::schema>;

template<MSharp::KernelFunction::BoxedKernelFunction* fallback_fn, class Op>
using call_fallback_fn = _call_fallback_fn<fallback_fn, Op, false, typename Op::schema>;

} // namespace pluss::native
