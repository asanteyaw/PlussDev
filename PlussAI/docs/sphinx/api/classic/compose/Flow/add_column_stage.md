<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 151d9474f28b -->
# `pluss::ai::classic::Flow::add_column_stage`

`add_column_stage` is [`add_named_transform`](add_named_transform.md) with
`stage_key` promoted to the first, required parameter instead of a trailing
default — reach for this form when naming the stage is the point of the
call, not an afterthought.

## Signature

```cpp
classic::Status classic::Flow::add_column_stage(
    std::string stage_key,
    classic::TransformKind kind,
    const classic::TransformOptions& options,
    /* feature indices */ feature_route = {},
    classic::RouteRemainderPolicy remainder = classic::RouteRemainderPolicy::PassThrough);
```

## What It Does

The same work [`add_named_transform`](add_named_transform.md) does --
appends a transform stage built from `kind`/`options` -- with `stage_key`
moved from a trailing default to a required leading parameter, so a call
site that cares about naming the stage reads that intent up front rather
than as an easily-missed trailing argument.

## Parameters

`stage_key` — `std::string`, required (no default)
: The stage's name — see [`TransformSpec::stage_key`](../TransformSpec.md#state).
  Required here, unlike [`add_named_transform`](add_named_transform.md) where
  it defaults to empty.

`kind`, `options`, `feature_route`, `remainder`
: Identical to [`add_named_transform`](add_named_transform.md) — see that
  page for each.

## Returns

A `Status`. `.ok()` on success; the stage is appended, named, and
[`stage_count()`](../Flow.md#state) increases by one.

## Errors

Identical to [`add_named_transform`](add_named_transform.md#errors) — a
`kind` / `options` mismatch, or an out-of-range `feature_route`.

## Practical Examples

### Example 1

Naming a stage at construction, then finding it by name through
[`nested_param_keys`](nested_param.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.add_column_stage(
      "income_scaler", cls::TransformKind::StandardScaler, cls::prep::StandardScalerOptions{}, {0});
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

Two named stages in one flow, each independently addressable by
`stage_key`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_column_stage("scale_numeric", cls::TransformKind::StandardScaler,
                        cls::prep::StandardScalerOptions{}, {0, 1});
  flow.add_column_stage("encode_category", cls::TransformKind::OneHotEncoder,
                        cls::prep::OneHotEncoderOptions{}, {2});

  for (const cls::TransformSpec& spec : flow.transform_specs()) {
    std::cout << "stage_key = " << spec.stage_key << ", route size = " << spec.feature_route.size()
              << "\n";
  }

  return 0;
}
```

Output

```text
stage_key = scale_numeric, route size = 2
stage_key = encode_category, route size = 1
```

### Example 3

A `kind` / `options` mismatch, the same failure mode as
[`add_named_transform`](add_named_transform.md), verified here too since
this overload validates independently.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.add_column_stage(
      "bad_stage", cls::TransformKind::OneHotEncoder, cls::prep::StandardScalerOptions{});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";
  std::cout << "stage_count() after the failed add = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::add_named_transform: options does not match kind=one_hot_encoder."
stage_count() after the failed add = 0
```

## See Also

- [`add_named_transform`](add_named_transform.md) — the same call, with
  `stage_key` optional and trailing.
- [`set_stage_key`](set_stage_key.md) — rename a stage after the fact.
- [`nested_param`](nested_param.md) — reading/writing a parameter by
  `stage_key`-qualified path.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="add_feature_transformer.html">
    <span class="label">Next</span>
    <span class="title">add_feature_transformer</span>
  </a>
</div>
<!-- apiref:nav:end -->
