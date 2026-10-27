#pragma once

#include "pluss/ma/stats/core.h"
#include "pluss/ma/stats/distributions.h"
#include "pluss/ma/stats/inference.h"
#include "pluss/ma/stats/regression.h"
#include "pluss/ma/stats/time_series.h"

namespace pluss {
namespace distr = ::pluss::subsp::stats::distr;
}

namespace pluss::ma {
namespace stats = ::pluss::subsp::stats;
namespace distr = ::pluss::subsp::stats::distr;
namespace inf = ::pluss::subsp::stats::inference;
namespace hyp = ::pluss::subsp::stats::inference::hypothesis;
namespace regression = ::pluss::subsp::stats::regression;
namespace reg = ::pluss::subsp::stats::regression;
namespace rng = ::pluss::subsp::stats::rng;
namespace ts = ::pluss::subsp::stats::ts;
}
