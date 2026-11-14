#pragma once

#include "pluss/subsp/actuarial/actuarial_models.h"
#include "pluss/subsp/actuarial/core.h"
#include "pluss/subsp/actuarial/life_contingencies.h"
#include "pluss/subsp/actuarial/loss.h"
#include "pluss/subsp/actuarial/loss_models.h"
#include "pluss/subsp/actuarial/pricing.h"
#include "pluss/subsp/actuarial/reserving.h"
#include "pluss/subsp/actuarial/valuation.h"

namespace pluss::subsp::actuarial {
namespace life = life_contingencies;
namespace models = actuarial_models;
namespace losses = loss_models;
} // namespace pluss::subsp::actuarial

namespace pluss::ma {
namespace actuarial = ::pluss::subsp::actuarial;
}
