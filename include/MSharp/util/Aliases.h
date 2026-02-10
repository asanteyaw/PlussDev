#pragma once

#include <MSharp/util/BFloat16.h>
#include <MSharp/util/Float4_e2m1fn_x2.h>
#include <MSharp/util/Float8_e4m3fn.h>
#include <MSharp/util/Float8_e4m3fnuz.h>
#include <MSharp/util/Float8_e5m2.h>
#include <MSharp/util/Float8_e5m2fnuz.h>
#include <MSharp/util/Float8_e8m0fnu.h>
#include <MSharp/util/Half.h>
#include <MSharp/util/bits.h>
#include <MSharp/util/complex.h>
#include <MSharp/util/qint8.h>
#include <MSharp/util/qint32.h>
#include <MSharp/util/quint8.h>
#include <MSharp/util/quint2x4.h>
#include <MSharp/util/quint4x2.h>

// Bring pluss and c10 types into MSharp namespace for compatibility
namespace MSharp {
using Half = ::pluss::Half;
using Float4_e2m1fn_x2 = ::pluss::Float4_e2m1fn_x2;
using Float8_e4m3fn = ::pluss::Float8_e4m3fn;
using Float8_e4m3fnuz = ::pluss::Float8_e4m3fnuz;
using Float8_e5m2 = ::pluss::Float8_e5m2;
using Float8_e5m2fnuz = ::pluss::Float8_e5m2fnuz;
using Float8_e8m0fnu = ::pluss::Float8_e8m0fnu;

} // namespace MSharp

// Also provide nested `pluss` inside `MSharp` so lookups like
// `MSharp::pluss::Half` resolve correctly when code is inside
// the `MSharp` namespace and performs unqualified `pluss::` lookups.
namespace MSharp { namespace pluss {
using Half = ::pluss::Half;
using BFloat16 = ::pluss::BFloat16;
using Float8_e5m2 = ::pluss::Float8_e5m2;
using Float8_e4m3fn = ::pluss::Float8_e4m3fn;
using Float8_e5m2fnuz = ::pluss::Float8_e5m2fnuz;
using Float8_e4m3fnuz = ::pluss::Float8_e4m3fnuz;
using Float8_e8m0fnu = ::pluss::Float8_e8m0fnu;
using Float4_e2m1fn_x2 = ::pluss::Float4_e2m1fn_x2;
} }

// Note: `MSharp::impl` aliases that depend on implementation `impl` are
// added inside `ScalarType.h` after the implementation details are defined
// to avoid ordering issues.

// Provide explicit `MSharp` nested namespaces that import the same type
// names from the global `::MSharp` / `::pluss` namespaces. This avoids
// namespace aliasing (which conflicted with inlined headers) while
// allowing unqualified `MSharp::` and `pluss::` references to resolve.

// (No additional MSharp aliases here; top-level MSharp mappings
// already provide the necessary using-decls.)

namespace pluss {
using Half = ::pluss::Half;
using BFloat16 = ::pluss::BFloat16;
using Float8_e5m2 = ::pluss::Float8_e5m2;
using Float8_e4m3fn = ::pluss::Float8_e4m3fn;
using Float8_e5m2fnuz = ::pluss::Float8_e5m2fnuz;
using Float8_e4m3fnuz = ::pluss::Float8_e4m3fnuz;
using Float8_e8m0fnu = ::pluss::Float8_e8m0fnu;
using Float4_e2m1fn_x2 = ::pluss::Float4_e2m1fn_x2;
} // namespace pluss
