#pragma once

// Compatibility shim: include the internal generated SeriesBase and map the
// legacy `at::TensorBase` name to `pluss::SeriesBase` for compatibility.
#include <internals/PlussQit/CodeM/core/SeriesBase.h>

namespace at {
using TensorBase = ::pluss::SeriesBase;
}
