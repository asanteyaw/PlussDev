#pragma once

// Backwards-compatibility shim: some code still includes SeriesImpl.h
// but we have renamed the concrete implementation to SeriesImpl.
// Provide a lightweight forwarding header so those includes keep compiling.

#include <MSharp/core/SeriesImpl.h>

namespace MSharp {
using TensorImpl = SeriesImpl;
using UndefinedTensorImpl = UndefinedSeriesImpl;
} // namespace MSharp
