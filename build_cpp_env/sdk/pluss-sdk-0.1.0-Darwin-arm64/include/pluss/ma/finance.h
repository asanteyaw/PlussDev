#pragma once

#include "pluss/subsp/math/finance/filtering.h"
#include "pluss/subsp/math/finance/hedging.h"
#include "pluss/subsp/math/finance/module.h"
#include "pluss/subsp/math/finance/models.h"
#include "pluss/subsp/math/finance/objects.h"
#include "pluss/subsp/math/finance/pricing.h"
#include "pluss/subsp/math/finance/rates.h"
#include "pluss/subsp/math/finance/volatility.h"
#include "pluss/subsp/math/finance/vol_bridge.h"
#include "pluss/codem/losses.h"

namespace pluss::ma {
namespace finance = ::pluss::subsp::math::finance;
namespace vol = ::pluss::subsp::math::finance::volatility;
namespace rate = ::pluss::subsp::math::finance::rates;
namespace mdl = ::pluss::subsp::math::finance::models;
namespace mod = ::pluss::subsp::math::finance::module;
namespace obj = ::pluss::subsp::math::finance::object;
namespace prc = ::pluss::subsp::math::finance::pricing;
namespace hdg = ::pluss::subsp::math::finance::hedging;
namespace flt = ::pluss::subsp::math::finance::filtering;
}
