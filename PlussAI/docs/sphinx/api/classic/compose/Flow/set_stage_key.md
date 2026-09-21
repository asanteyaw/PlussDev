<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6d8752f03c94 -->
# `pluss::ai::classic::Flow::set_stage_key`

`set_stage_key` renames a [`Flow`](../Flow.md) stage after it has already
been added — an alternative to naming it up front with
[`add_column_stage`](add_column_stage.md). Unlike
[`set_stage_route`](set_stage_route.md)/[`set_stage_remainder`](set_stage_remainder.md),
renaming does **not** reset [`trained()`](../Flow.md#state) — a stage's name
is bookkeeping for [`nested_param`](nested_param.md) lookups, not part of
what a fit depends on.

## Signature

```cpp
classic::Status classic::Flow::set_stage_key(std::int64_t stage_index, std::string stage_key);
```

## Parameters

`stage_index` — `std::int64_t`, 0-based
: Which stage to rename — an index into [`transform_specs()`](../TransformSpec.md).

`stage_key` — `std::string`
: The new name. Pass `""` to clear a stage's name back to unnamed. A
  non-empty value must not collide with any other stage's current
  `stage_key`.

## Returns

A `Status`. `.ok()` on success — the stage's
[`TransformSpec::stage_key`](../TransformSpec.md#state) updates immediately.

## Errors

- `Status::invalid_argument` — `stage_index` is negative or `>=`
  [`stage_count()`](../Flow.md#state).
- `Status::invalid_argument` — `stage_key` is non-empty and already used by a
  different stage.

## Practical Examples

### Example 1

Naming a stage that was added unnamed.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  const cls::Status s = flow.set_stage_key(0, "income_scaler");
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_key = " << flow.transform_specs().front().stage_key << "\n";

  return 0;
}
```

Output

```text
ok = true
stage_key = income_scaler
```

### Example 2

Both failure modes: a duplicate name, and an out-of-range index.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.add_one_hot_encoder({}, {1});
  flow.set_stage_key(0, "scale_col0");

  const cls::Status dup = flow.set_stage_key(1, "scale_col0");
  std::cout << "duplicate key ok = " << std::boolalpha << dup.ok() << "\n";
  std::cout << "duplicate key message = \"" << dup.message << "\"\n";

  const cls::Status out_of_range = flow.set_stage_key(5, "whatever");
  std::cout << "out-of-range ok = " << out_of_range.ok() << "\n";
  std::cout << "out-of-range message = \"" << out_of_range.message << "\"\n";

  return 0;
}
```

Output

```text
duplicate key ok = false
duplicate key message = "Flow::set_stage_key requires unique stage_key values."
out-of-range ok = false
out-of-range message = "Flow::set_stage_key stage index out of range."
```

### Example 3

Renaming (or clearing, with `""`) a stage's key does **not** reset
`trained()` — unlike [`set_stage_route`](set_stage_route.md) and
[`set_stage_remainder`](set_stage_remainder.md), which do.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.set_stage_key(0, "income_scaler");

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch batch{};
  batch.x = x;
  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "learn_map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "trained() after learn_map() = " << flow.trained() << "\n";

  flow.set_stage_key(0, "");  // clear the name
  std::cout << "stage_key after clearing = \"" << flow.transform_specs().front().stage_key << "\"\n";
  std::cout << "trained() after set_stage_key() = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
learn_map ok = true
trained() after learn_map() = true
stage_key after clearing = ""
trained() after set_stage_key() = true
```

## See Also

- [`add_column_stage`](add_column_stage.md) — name a stage at construction
  instead of after the fact.
- [`set_stage_route`](set_stage_route.md) / [`set_stage_remainder`](set_stage_remainder.md)
  — the two sibling mutators that do reset `trained()`.
- [`nested_param`](nested_param.md) — where a `stage_key` is used to address
  a parameter.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_regressor.html">
    <span class="label">Previous</span>
    <span class="title">set_regressor</span>
  </a>
  <a class="next-link" href="set_stage_remainder.html">
    <span class="label">Next</span>
    <span class="title">set_stage_remainder</span>
  </a>
</div>
<!-- apiref:nav:end -->
