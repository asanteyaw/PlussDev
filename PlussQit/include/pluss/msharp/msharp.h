#pragma once
#include "MSharp/MSharpMacros.h"
#include "MSharp/Parallel.h"
#include "MSharp/SymInt.h"
#include "MSharp/SymBool.h"
#include "MSharp/RangeView.h"

namespace MSharp = ::pluss::msharp;

// Note: `pluss::RangeView<T>` (a template alias for `msharp::RangeView::Of<T>`)
// already exists in pluss/codem/codem.h -- that's the public spelling to use;
// no separate alias needed here.
