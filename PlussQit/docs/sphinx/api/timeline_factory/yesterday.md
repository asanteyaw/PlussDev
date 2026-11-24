<!-- pluss-hand-authored -->
# pluss::timeline::yesterday

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::yesterday()
```

## The Calendar Day before Today

`yesterday()` returns a rank-zero, one-element Timeline at UTC midnight at the start of the calendar day before the one that is current when the function is called. It is the same as taking `today()` and moving it back by one day. Like `today()`, the result is a captured instant: it does not change at the next midnight.

The function has no arguments. The boundary is always UTC midnight, so it does not depend on the process timezone. For a boundary in another timezone, project the result with `tz_convert()`.

## Example 1: One Day From Today

A hard-coded date would be wrong the next day, so the program checks the relationship. The difference of the day numbers is one whenever it runs.

```cpp
// The days around today.
#include <pluss/pluss.h>

#include <iostream>

int main() {
  const auto yesterday = pluss::timeline::yesterday();
  const auto today = pluss::timeline::today();
  const auto tomorrow = pluss::timeline::tomorrow();
  const double before = today.epoch_days().item<double>() - yesterday.epoch_days().item<double>();
  const double after = tomorrow.epoch_days().item<double>() - today.epoch_days().item<double>();
  std::cout << "days from yesterday to today: " << before << "\n";
  std::cout << "days from today to tomorrow:  " << after << "\n";
}
```

### Output

```text
days from yesterday to today: 1
days from today to tomorrow:  1
```

## See Also

- [`today`](today.md): the start of the current day.
- [`tomorrow`](tomorrow.md): the day after today.
- [`now`](now.md): the current instant, not rounded to a day.
