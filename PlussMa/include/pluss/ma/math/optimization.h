#pragma once

#include "pluss/subsp/math/optim.h"
#include "pluss/subsp/math/optim_fwd.h"
#include "pluss/subsp/math/optim_lsr1.h"
#include "pluss/subsp/math/linprog.h"
#include "pluss/subsp/math/milp.h"
#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/math/qp.h"
#include "pluss/subsp/math/lp/ipm.h"
#include "pluss/subsp/math/lp/mip.h"
#include "pluss/subsp/math/lp/mps.h"
#include "pluss/subsp/math/lp/series_bridge.h"
#include "pluss/subsp/math/lp/solve.h"

#if __has_include("PlussMa/Math/Numerical/optimization/dfo_solver.h")
#include "PlussMa/Math/Numerical/optimization/dfo_solver.h"
#else
#include "dfo_solver.h"
#endif

#if __has_include("PlussMa/Math/Numerical/optimization/dfo_minimizers.h")
#include "PlussMa/Math/Numerical/optimization/dfo_minimizers.h"
#else
#include "dfo_minimizers.h"
#endif

#if __has_include("PlussMa/Math/Numerical/optimization/mads_blackbox_minimizers.h")
#include "PlussMa/Math/Numerical/optimization/mads_blackbox_minimizers.h"
#else
#include "mads_blackbox_minimizers.h"
#endif

#if __has_include("PlussMa/Math/Numerical/optimization/nelder_mead_minimizer.h")
#include "PlussMa/Math/Numerical/optimization/nelder_mead_minimizer.h"
#else
#include "nelder_mead_minimizer.h"
#endif

#if __has_include("PlussMa/Math/Numerical/optimization/pattern_search_minimizer.h")
#include "PlussMa/Math/Numerical/optimization/pattern_search_minimizer.h"
#else
#include "pattern_search_minimizer.h"
#endif

namespace pluss::ma {
namespace optim = ::pluss::subsp::math::optim;
}
