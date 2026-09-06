#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  // steps()/days() are both plain row-offset wrappers today (NodeJ's
  // TSCrossVal kernel is sample-index based) -- the two names exist so
  // intent reads clearly at the call site even though the values are
  // interchangeable right now.
  workflow::TimeOptions time_options{};
  time_options.index = "observed_at";
  time_options.horizon = workflow::steps(3);
  time_options.gap = workflow::days(1);
  time_options.lookback = workflow::steps(30);

  workflow::validate_time_options(time_options, "example");
  std::cout << "has_time_index = " << workflow::has_time_index(time_options) << "\n";

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.time(time_options);
  std::cout << "time_aware = " << wf.time_aware() << "\n";
  std::cout << "time_options().index = " << wf.time_options()->index
            << " horizon=" << wf.time_options()->horizon
            << " gap=" << wf.time_options()->gap
            << " lookback=" << wf.time_options()->lookback << "\n";

  // horizon/gap/lookback cannot be negative -- validate_time_options throws
  // before any fold assignment sees the bad value.
  workflow::TimeOptions invalid{};
  invalid.index = "observed_at";
  invalid.horizon = -1;
  try {
    workflow::validate_time_options(invalid, "example(invalid)");
    std::cout << "validate_time_options: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "validate_time_options threw: " << e.what() << "\n";
  }

  return 0;
}
