<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 263fb921f896 -->
# `pluss::ai::classic::Flow::add_named_transform`

`add_named_transform` is the single dynamic form every typed convenience
method on [`add_stage`](add_stage.md) forwards to: a `TransformKind` value
plus its matching `TransformOptions`, rather than a named method per kind.
Reach for it when the stage kind is not known at compile time — read from
configuration, built in a loop over a list of kinds — otherwise the typed
methods on [`add_stage`](add_stage.md) are more readable for a kind you
already know by name.

## Signature

```cpp
classic::Status classic::Flow::add_named_transform(
    classic::TransformKind kind,
    const classic::TransformOptions& options,
    std::string stage_key = {},
    /* feature indices */ feature_route = {},
    classic::RouteRemainderPolicy remainder = classic::RouteRemainderPolicy::PassThrough);
```

## Parameters

`kind` — `TransformKind`
: Which [`prep`](../../prep.md) stage to add. See [`TransformSpec`](../TransformSpec.md#signature)
  for the full list of values.

`options` — `TransformOptions`
: The matching options struct, held in the `TransformOptions` variant —
  construct the concrete struct (`prep::StandardScalerOptions{}`, …) and it
  converts implicitly. Passing an `options` alternative that does not match
  `kind` fails (see Errors).

`stage_key` — `std::string`, default `""`
: An optional name for the stage — see [`TransformSpec::stage_key`](../TransformSpec.md#state).

`feature_route`, default every column
: As every `add_*` method — the 0-based column indices this stage sees.

`remainder` — `RouteRemainderPolicy`, default `PassThrough`
: Unlike the twenty typed methods on [`add_stage`](add_stage.md), this form
  lets you set `Drop` directly, without a follow-up
  [`set_stage_remainder`](set_stage_remainder.md) call.

## Returns

A `Status`. `.ok()` on success; the stage is appended and
[`stage_count()`](../Flow.md#state) increases by one.

## Errors

- `Status::invalid_argument` — `options` holds an alternative that does not
  match `kind`.
- The same feature-route validity caveat as [`add_stage`](add_stage.md#errors).

## Practical Examples

### Example 1

Building a stage from a `kind` value read as if from configuration, rather
than naming the typed method directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::TransformKind kind = cls::TransformKind::StandardScaler;  // "read from configuration"
  const cls::TransformOptions options = cls::prep::StandardScalerOptions{};

  cls::Flow flow{};
  const cls::Status s = flow.add_named_transform(kind, options);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
ok = true
stage_count() = 1
```

### Example 2

`remainder = Drop` — a column not named by the route is removed from the
flow's output entirely, which none of the typed `add_stage` methods can do
directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_named_transform(cls::TransformKind::StandardScaler, cls::prep::StandardScalerOptions{},
                           "scale_col0", {0}, cls::RouteRemainderPolicy::Drop);

  const cls::TransformSpec& spec = flow.transform_specs().front();
  std::cout << "stage_key = " << spec.stage_key << "\n";
  std::cout << "remainder == Drop: " << std::boolalpha
            << (spec.remainder == cls::RouteRemainderPolicy::Drop) << "\n";

  return 0;
}
```

Output

```text
stage_key = scale_col0
remainder == Drop: true
```

### Example 3

A `kind` / `options` mismatch — passing `OneHotEncoderOptions` for
`TransformKind::StandardScaler`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.add_named_transform(
      cls::TransformKind::StandardScaler, cls::prep::OneHotEncoderOptions{});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::add_named_transform: options does not match kind=standard_scaler."
```

## See Also

- [`add_stage`](add_stage.md) — the twenty typed spellings of this same call.
- [`add_column_stage`](add_column_stage.md) — the same, with `stage_key` as
  the first parameter instead of a trailing default.
- [`TransformSpec`](../TransformSpec.md) — what each call appends.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_function_transformer.html">
    <span class="label">Previous</span>
    <span class="title">add_function_transformer</span>
  </a>
  <a class="next-link" href="add_stage.html">
    <span class="label">Next</span>
    <span class="title">add_stage</span>
  </a>
</div>
<!-- apiref:nav:end -->
