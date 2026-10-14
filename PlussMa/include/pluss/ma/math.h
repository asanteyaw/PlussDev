#pragma once

#include "pluss/ma/math/optimization.h"
#include "pluss/ma/math/integration.h"
#include "pluss/ma/math/interpolation.h"
#include "pluss/subsp/math/krylov.h"
#include "pluss/ma/math/ode.h"
#include "pluss/ma/math/random_processes.h"
#include "pluss/ma/math/linearalg.h"
#include "pluss/ma/math/finance.h"

namespace pluss {
namespace optim = ::pluss::subsp::math::optim;
namespace integ = ::pluss::subsp::math::integ;
namespace interp = ::pluss::subsp::math::interp;
namespace ode = ::pluss::subsp::math::ode;
namespace sde = ::pluss::subsp::math::sde;
namespace finance = ::pluss::subsp::math::finance;
}

namespace pluss::ma {
namespace math = ::pluss::subsp::math;
}

