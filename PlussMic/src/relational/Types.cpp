#include "relational/Types.h"

#include "CodeM/core/Errors.h"

namespace pluss::relational {

PullSpec PullSpec::named(std::string_view value) const {
  PLUSS_CHECK(!value.empty(), "PullSpec::named: alias cannot be empty");
  auto out = *this;
  out.alias = std::string(value);
  return out;
}

PullSpec PullSpec::windowed(std::int64_t value, std::string_view unit) const {
  PLUSS_CHECK(op != PullOp::Take, "PullSpec::windowed: take(...) cannot be windowed");
  PLUSS_CHECK(value > 0, "PullSpec::windowed: window value must be positive");
  PLUSS_CHECK(!unit.empty(), "PullSpec::windowed: window unit cannot be empty");
  auto out = *this;
  out.window = PullWindow{
      .value = value,
      .unit = std::string(unit),
      .anchor_root = false,
  };
  return out;
}

PullSpec PullSpec::root_windowed(std::int64_t value, std::string_view unit) const {
  PLUSS_CHECK(op != PullOp::Take, "PullSpec::root_windowed: take(...) cannot be windowed");
  PLUSS_CHECK(value > 0, "PullSpec::root_windowed: window value must be positive");
  PLUSS_CHECK(!unit.empty(), "PullSpec::root_windowed: window unit cannot be empty");
  auto out = *this;
  out.window = PullWindow{
      .value = value,
      .unit = std::string(unit),
      .anchor_root = true,
  };
  return out;
}

} // namespace pluss::relational
