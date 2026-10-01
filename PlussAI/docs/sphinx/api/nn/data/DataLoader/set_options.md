<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5c936f3ac7a2 -->

# `pluss::loader::DataLoader::set_options`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
pluss::loader::DataLoader& pluss::loader::DataLoader::set_options(pluss::loader::DataLoaderOptions options);
```

## What It Does

Replaces this loader's entire [`DataLoaderOptions`](../DataLoaderOptions.md)
and re-validates/re-initializes against it -- the same validation and
order-generation construction itself runs, so an invalid combination
(e.g. `sampler` and `shuffle` both set) throws exactly as it would from a
constructor. Any in-flight background prefetch is stopped first.

## Parameters

`DataLoaderOptions options`
: The complete replacement options. Not merged with the existing
  options -- every field not explicitly set on `options` reverts to that
  field's own default, the same as constructing a fresh
  `DataLoaderOptions{}` and only setting what differs.

## Returns

`DataLoader&` -- a reference to `*this`, for chaining.

## Errors

Same validation as construction -- an invalid combination (mismatched
`sampler`/`shuffle`, `prefetch_factor` set with `num_workers == 0`, ...)
throws.

## Practical Examples

Three examples: changing `batch_size` on an already-constructed loader,
confirming unset fields revert to their defaults rather than carrying
over from the previous options, and an invalid combination throwing the
same way construction would.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5, 1});
  loader::DataLoader data_loader({x});

  loader::DataLoaderOptions new_options{};
  new_options.batch_size = 3;
  data_loader.set_options(new_options);

  std::cout << "batch_size() = " << data_loader.batch_size() << "\n";
  std::cout << "batch_count() = " << data_loader.batch_count() << "\n";

  return 0;
}
```

Output

```text
batch_size() = 3
batch_count() = 2
```

### Example 2

`shuffle = true` set, then replaced with options that never mention
`shuffle` -- it reverts to its own default (`false`), it is not carried
over from the first call.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoaderOptions shuffled{};
  shuffled.shuffle = true;
  loader::DataLoader data_loader({x}, shuffled);

  std::cout << "options().shuffle before = " << std::boolalpha << data_loader.options().shuffle << "\n";

  loader::DataLoaderOptions plain{};
  plain.batch_size = 1;
  data_loader.set_options(plain);

  std::cout << "options().shuffle after set_options(plain) = " << data_loader.options().shuffle << "\n";

  return 0;
}
```

Output

```text
options().shuffle before = true
options().shuffle after set_options(plain) = false
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <memory>
#include <string>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  loader::DataLoaderOptions invalid{};
  invalid.shuffle = true;
  invalid.sampler = std::make_shared<loader::SequentialSampler>();

  try {
    data_loader.set_options(invalid);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::loader::DataLoader: sampler and shuffle are mutually exclusive
```

## See Also

- [`DataLoaderOptions`](../DataLoaderOptions.md) -- every field this
  replaces.
- [`options`](../DataLoader.md#state) -- read the currently active
  options back.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_epoch.html">
    <span class="label">Previous</span>
    <span class="title">set_epoch</span>
  </a>
</div>
<!-- apiref:nav:end -->
