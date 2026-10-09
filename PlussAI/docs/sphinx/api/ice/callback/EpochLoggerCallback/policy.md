<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d4afc0672f63 -->
# `pluss::ai::ice::EpochLoggerCallback::policy`

[Back to `pluss::ai::ice::EpochLoggerCallback`](../EpochLoggerCallback.md)

## Signature

```cpp
const pluss::ai::ice::ProgressPolicy& pluss::ai::ice::EpochLoggerCallback::policy() const noexcept;
```

## What It Does

Returns the [`ProgressPolicy`](../../options/ProgressPolicy.md) the logger was constructed
with. The logger copies the policy at construction, so the value returned here is the
logger's own copy: changing the caller's policy struct afterward does not change what the
logger reports.

## Parameters

None.

## Returns

A `const` reference to the policy, valid for the life of the callback.

## Errors

None. The method is `noexcept`.

## Practical Examples

### Example 1

Reading back the cadence and the metric order a logger was built with.

```cpp
#include <pluss/ai/ice.h>

#include <iostream>
#include <sstream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ProgressPolicy policy{};
  policy.log_every_n_epochs = 5;
  policy.metrics = {"val_loss", "loss"};
  std::ostringstream log;
  ice::EpochLoggerCallback logger(policy, log);

  std::cout << "every = " << logger.policy().log_every_n_epochs << "\n";
  std::cout << "metrics =";
  for (const std::string& name : logger.policy().metrics) std::cout << " " << name;
  std::cout << "\n";
  return 0;
}
```

Output

```text
every = 5
metrics = val_loss loss
```

### Example 2

The default policy. A logger built without arguments reports silently to the default
stream and has automatic installation switched off.

```cpp
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const ice::EpochLoggerCallback logger{};
  std::cout << "verbose = " << logger.policy().verbose << "\n";
  std::cout << "log_every_n_epochs = " << logger.policy().log_every_n_epochs << "\n";
  return 0;
}
```

Output

```text
verbose = 0
log_every_n_epochs = 1
```

### Example 3

The copy. Changing the original policy after construction does not change the logger's
policy, which the program shows by comparing the two cadences.

```cpp
#include <pluss/ai/ice.h>

#include <iostream>
#include <sstream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::ProgressPolicy policy{};
  policy.log_every_n_epochs = 2;
  std::ostringstream log;
  ice::EpochLoggerCallback logger(policy, log);
  policy.log_every_n_epochs = 9;

  std::cout << "caller = " << policy.log_every_n_epochs << ", logger = " << logger.policy().log_every_n_epochs << "\n";
  return 0;
}
```

Output

```text
caller = 9, logger = 2
```

## See Also

- [`EpochLoggerCallback`](../EpochLoggerCallback.md) -- the logger this method belongs to.
- [`ProgressPolicy`](../../options/ProgressPolicy.md) -- the fields the returned policy carries.
