#pragma once

#include "pluss/codeh/codeh.h"

namespace pluss {

// Public factory (preferred): construct a TimeIndex from a 1D Timeline.
// Users MUST NOT construct TimeIndex directly; this is the only supported entry point.
inline pluss::codeh::TimeIndex make_timeindex(const pluss::codeh::Timeline& t, bool require_sorted = true) {
  return ::pluss::codeh::_compat::invoke<pluss::codeh::TimeIndex>("make_timeindex", t, require_sorted);
}

} // namespace pluss
