#pragma once

#include "pluss/subsp/math/function.h"
#include "pluss/subsp/math/operators.h"
#include "pluss/subsp/math/calculus_io.h"
#include "pluss/ma/math/optimization.h"
#include "pluss/ma/math/integration.h"
#include "pluss/ma/math/signal.h"
#include "pluss/ma/math/interpolation.h"
#include "pluss/subsp/math/krylov.h"
#include "pluss/subsp/math/multigrid.h"
#include "pluss/subsp/math/linear_operator.h"
#include "pluss/subsp/math/state.h"
#include "pluss/ma/math/ode.h"
#include "pluss/ma/math/random_processes.h"
#include "pluss/ma/math/linearalg.h"
#include "pluss/ma/math/finance.h"

namespace pluss::ma {
namespace math = ::pluss::subsp::math;
}

namespace pluss {
namespace optim = ::pluss::subsp::math::optim;
namespace integ = ::pluss::subsp::math::integ;
namespace interp = ::pluss::subsp::math::interp;
namespace ode = ::pluss::subsp::math::ode;
namespace finance = ::pluss::subsp::math::finance;
namespace math = ::pluss::subsp::math;
namespace operators = ::pluss::subsp::math::operators;
}

