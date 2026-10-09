#include "CodeH/core/TimelineIndexAccessor.h"

#include "CodeH/compat/Access.h"
#include "CodeH/compat/Invoke.h"

namespace pluss::codeh {

pluss::codem::Series TimelineIndexAccessor::join_asof(const TimelineIndexAccessor& right,
                                          AsofDirection direction,
                                          const Duration* tolerance,
                                          bool allow_exact_matches) const {
  if (tolerance) {
    return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
        "join_asof", *this, {right, direction, *tolerance, allow_exact_matches});
  }
  return ::pluss::codeh::_compat::invoke<pluss::codem::Series>(
      "join_asof", *this,
      {right, direction, ::pluss::codeh::_compat::Arg{}, allow_exact_matches});
}

} // namespace pluss::codeh
