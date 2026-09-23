<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f451f00f202 -->

# `pluss::ai::nn::Schedule::insert`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

## Signature

```cpp
template <typename Layer>
nn::Schedule& nn::Schedule::insert(std::size_t index, Layer&& layer);

template <typename Layer>
nn::Schedule& nn::Schedule::insert(std::size_t index, std::string name, Layer&& layer);
```

[`then`](then.md) with a position: the stage lands at `index` (shifting
every later stage right by one) instead of always at the end. Both
overloads assume `Series -> Series` -- use [`insert_as`](insert_as.md) when
a stage's real type is something else.

## Parameters

`std::size_t index`
: Where the new stage lands, in `[0, size()]` inclusive -- `index == size()`
  appends, exactly like `then`.

`Layer&& layer`, `std::string name`
: Same as [`then`](then.md).

## Returns

`Schedule&` -- a reference to `*this`, for chaining.

## Errors

- `index > size()` -- throws.
- `name` already taken -- throws.
- `layer`'s declared type does not match **either** neighbor it lands
  between (the stage now at `index - 1`, if any, and the stage now at
  `index + 1`, if any) -- throws. Unlike `then`, which only ever has one
  neighbor to satisfy (the current last stage), `insert` in the middle of
  an existing chain must satisfy both sides at once.

## Practical Examples

Three examples: inserting a compatible stage between two existing ones, the
type-mismatch error when a neighbor's declared type is not satisfied, and
what auto-naming looks like once a stage has been inserted out of call
order.

### Example 1

A `Schedule` already chaining `in` (`Fusion(3, 4)`) into `out`
(`Fusion(4, 2)`). Inserting a `Fusion(4, 4)` at index 1 fits between them --
both neighbors declare `Series -> Series`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("in", nn::Fusion(3, 4));
  net.then("out", nn::Fusion(4, 2));

  net.insert(1, "mid", nn::Fusion(4, 4));  // slots between "in" and "out" -- both neighbors are Series->Series

  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "stage " << i << ": '" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
stage 0: 'in'
stage 1: 'mid'
stage 2: 'out'
```

### Example 2

The chain check compares stage **types**, not tensor shapes (see the
category [Introduction](../Introduction.md)) -- an `RNN` stage produces a
tuple, which neither of these `Series`-only neighbors is prepared for, so
the insert is rejected before either neighbor is disturbed.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Schedule net;
  net.then("in", nn::Fusion(3, 4));
  net.then("out", nn::Fusion(4, 2));

  try {
    net.insert_as<Series, std::tuple<Series, Series>>(1, "bad", nn::RNN(4, 4));
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  std::cout << "size() is still " << net->size() << "\n";

  return 0;
}
```

Output

```text
caught: pluss::nn::Schedule type mismatch: new stage output NSt3__15tupleIJN5pluss5codem6SeriesES3_EEE is incompatible with next stage input N5pluss5codem6SeriesE
size() is still 2
```

(`NSt3__15tupleIJ...` is `std::type_info::name()`'s raw, platform-specific
mangled spelling of `std::tuple<Series, Series>` -- implementation-defined
and not meant to be read directly, only compared or logged.)

### Example 3

Auto-naming is assigned in **call order**, not final position. Two stages
added by `then()` (auto-named `"0"`, `"1"`), then a third inserted between
them by `insert()`, is auto-named `"2"` -- the counter keeps counting
regardless of where the stage ends up.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then(nn::Fusion(3, 4));   // auto-named "0"
  net.then(nn::Fusion(4, 2));   // auto-named "1"

  net.insert(1, nn::Fusion(4, 4));  // auto-named "2" -- next_auto_name_ keeps counting regardless of position

  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "stage " << i << ": '" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
stage 0: '0'
stage 1: '2'
stage 2: '1'
```

## See Also

- [`then`](then.md) -- inserts only ever at the end.
- [`insert_as`](insert_as.md) -- the same operation for a stage whose real
  type is not `Series -> Series`.
- [`erase`](erase.md) -- the inverse operation.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="input_type_at.html">
    <span class="label">Previous</span>
    <span class="title">input_type_at</span>
  </a>
  <a class="next-link" href="insert_as.html">
    <span class="label">Next</span>
    <span class="title">insert_as</span>
  </a>
</div>
<!-- apiref:nav:end -->
