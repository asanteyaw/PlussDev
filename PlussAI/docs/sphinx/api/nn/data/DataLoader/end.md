<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6ce155fdd28d -->

# `pluss::loader::DataLoader::end`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
pluss::loader::DataLoader::iterator pluss::loader::DataLoader::end();
```

## What It Does

Returns the end-of-range sentinel iterator -- called implicitly by
range-for, compared against on every loop iteration to decide when to
stop. Unlike [`begin`](begin.md), `end` never resets or reshuffles
anything; it is a pure, stateless comparison target, positioned at
`batch_count()`.

## Parameters

None.

## Returns

`DataLoader::iterator` -- positioned one past the last batch.
Comparing any iterator returned by [`begin`](begin.md) against this one
(`!=`) is what range-for's own loop condition checks.

## Errors

None. `end` has no preconditions and never throws.

## Practical Examples

Three examples: the implicit range-for use every other example already
relies on, an explicit manual loop using `end()` as the stop condition,
and confirming two calls to `end()` compare equal to each other.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 1;
  loader::DataLoader data_loader({x}, options);

  for (auto batch : data_loader) {
    std::cout << "batch: " << batch.x() << "\n";
  }

  return 0;
}
```

Output

```text
batch: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[1]]
batch: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2]]
batch: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[3]]
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 1;
  loader::DataLoader data_loader({x}, options);

  for (auto it = data_loader.begin(); it != data_loader.end(); ++it) {
    std::cout << "manual: " << (*it).x() << "\n";
  }

  return 0;
}
```

Output

```text
manual: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[1]]
manual: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2]]
manual: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[3]]
```

### Example 3

Two separate calls to `end()` compare equal -- it carries no state of its
own beyond the fixed `batch_count()` position.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  std::cout << "end() == end(): " << std::boolalpha << (data_loader.end() == data_loader.end()) << "\n";

  return 0;
}
```

Output

```text
end() == end(): true
```

## See Also

- [`begin`](begin.md) -- the matching start-of-range iterator.
- [`batch_count`](batch_count.md) -- the position `end()` is fixed at.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="begin.html">
    <span class="label">Previous</span>
    <span class="title">begin</span>
  </a>
  <a class="next-link" href="epoch.html">
    <span class="label">Next</span>
    <span class="title">epoch</span>
  </a>
</div>
<!-- apiref:nav:end -->
