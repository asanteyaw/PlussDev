#pragma once

#include "pluss/msharp/msharp.h"
#include "CodeM/CodeM.h"

// Optional: LibTorch interop (enable PLUSS_ENABLE_TORCH)
#ifdef PLUSS_ENABLE_TORCH
#include <pluss/codem/torch_interop.h>
#endif

namespace pluss {
  using Series = codem::Series;
  using Config = codem::Config;
  using ScalarType = codem::ScalarType;
  using Platform = codem::Platform;
  using BlobDeleter = codem::BlobDeleter;

} // namespace pluss
