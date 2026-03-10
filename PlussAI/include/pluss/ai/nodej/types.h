#pragma once

#include "pluss/core.h"
#include "pluss/subsp/math/diff.h"
#include "pluss/subsp/math/optim.h"
#include "pluss/subsp/stats/distr.h"
#include "pluss/subsp/stats/ts.h"

#include <type_traits>
#include <vector>

namespace pluss::ai::nodej {

// NodeJ's non-negotiable core numeric type.
using Series = ::pluss::Series;
using SeriesPack = std::vector<Series>;
using ParameterPack = ::pluss::subsp::math::optim::ParameterPack<Series>;

namespace diff = ::pluss::subsp::math::diff;
namespace optim = ::pluss::subsp::math::optim;
namespace distr = ::pluss::subsp::stats::distr;
namespace ts = ::pluss::subsp::stats::ts;

static_assert(std::is_same_v<Series, ::pluss::codem::Series>,
              "NodeJ requires pluss::Series to alias pluss::codem::Series.");

} // namespace pluss::ai::nodej
