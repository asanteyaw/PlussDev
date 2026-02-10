#pragma once

#include <ATen/core/Generator.h>

#include <MSharp/core/Allocator.h>
#include <MSharp/util/Exception.h>
#include <MSharp/util/Registry.h>

#include <ATen/detail/AcceleratorHooksInterface.h>

// NB: dummy argument to suppress "ISO C++11 requires at least one argument
// for the "..." in a variadic macro"
struct TORCH_API CUDAHooksArgs {};

TORCH_DECLARE_REGISTRY(CUDAHooksRegistry, CUDAHooksInterface, CUDAHooksArgs);
#define REGISTER_CUDA_HOOKS(clsname) \
	C10_REGISTER_CLASS(CUDAHooksRegistry, clsname, clsname)

namespace detail {
TORCH_API const CUDAHooksInterface& getCUDAHooks();
} // namespace detail
