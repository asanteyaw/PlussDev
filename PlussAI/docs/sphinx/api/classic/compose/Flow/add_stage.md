<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 375fec4e6aaf -->
# `pluss::ai::classic::Flow::add_stage`

The twenty typed convenience methods — `add_standard_scaler`,
`add_one_hot_encoder`, and eighteen more, one per [`prep`](../../prep.md)
kernel — are how you add a transform stage to a [`Flow`](../Flow.md) in the
common case: you already know which stage you want. Each is the same shape:
an options struct in, an optional feature route, a `Status` out. They are
covered together on one page because they are, structurally, one operation
with twenty spellings — see [`add_named_transform`](add_named_transform.md)
for the single dynamic form these all forward to internally.

## Signature

```cpp
classic::Status classic::Flow::add_standard_scaler(
    classic::prep::StandardScalerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_minmax_scaler(
    classic::prep::MinMaxScalerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_robust_scaler(
    classic::prep::RobustScalerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_maxabs_scaler(
    classic::prep::MaxAbsScalerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_simple_imputer(
    classic::prep::SimpleImputerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_neighbor_imputer(
    classic::prep::NeighborImputerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_iterative_imputer(
    classic::prep::IterativeImputerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_one_hot_encoder(
    classic::prep::OneHotEncoderOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_ordinal_encoder(
    classic::prep::OrdinalEncoderOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_label_encoder(
    classic::prep::LabelEncoderOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_frequency_encoder(
    classic::prep::FrequencyEncoderOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_basen_encoder(
    classic::prep::BaseNEncoderOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_hashing_encoder(
    classic::prep::HashingEncoderOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_target_encoder(
    classic::prep::TargetEncoderOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_woe_encoder(
    classic::prep::WOEEncoderOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_vector_normalizer(
    classic::prep::VectorNormalizerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_quantile_map(
    classic::prep::QuantileMapOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_power_map(
    classic::prep::PowerMapOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_threshold_binarizer(
    classic::prep::ThresholdBinarizerOptions options = {}, /* feature indices */ feature_route = {});
classic::Status classic::Flow::add_polynomial_lift(
    classic::prep::PolynomialLiftOptions options = {}, /* feature indices */ feature_route = {});
```

## Parameters

`options`
: The stage's own options struct — see that struct's own page under
  [`prep`](../../prep.md) (for example
  [`StandardScalerOptions`](../../prep/scaling/StandardScalerOptions.md)) for
  every field. Defaults to that struct's own defaults.

`feature_route`
: The 0-based column indices this stage should see — pass a braced list
  (`{0, 2}`) or any range of `std::int64_t`. Omit it (the default) to route
  every column to this stage.

None of the twenty take a `remainder` parameter — a stage added this way
always uses `RouteRemainderPolicy::PassThrough` (unrouted columns continue
unchanged). Reach for
[`set_stage_remainder`](set_stage_remainder.md) afterward, or
[`add_column_stage`](add_column_stage.md) / [`add_named_transform`](add_named_transform.md)
directly, if you need `Drop`.

## Returns

A `Status`. `.ok()` on success — the stage is appended and
[`stage_count()`](../Flow.md#state) increases by one.

## Errors

- `Status::invalid_argument` — `feature_route` names a column index outside
  the flow's expected feature count (only checked once the flow has enough
  context to know that count — typically at the first `learn`, not at
  `add_*` time, since a bare `Flow` does not yet know how many features it
  will see).

## Practical Examples

A pair of routed stages added in sequence, a non-default options struct
paired with an unrouted stage, a realistic mixed-type pipeline, and every
one of the twenty typed methods called on the same `Flow` — the "twenty
spellings, one operation" claim above, exercised for real rather than
taken on faith.

### Example 1

Two typed stages, each routed to one column, added in sequence — the pattern
most pipelines use.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s1 = flow.add_standard_scaler({}, {0});
  const cls::Status s2 = flow.add_one_hot_encoder({}, {1});

  std::cout << "s1.ok() = " << std::boolalpha << s1.ok() << "\n";
  std::cout << "s2.ok() = " << s2.ok() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
s1.ok() = true
s2.ok() = true
stage_count() = 2
```

### Example 2

Passing a non-default options struct — `SimpleImputer` with an explicit
strategy — and an unrouted stage (every column).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::prep::SimpleImputerOptions imputer{};
  imputer.strategy = cls::prep::SimpleImputerStrategy::Median;

  cls::Flow flow{};
  const cls::Status s = flow.add_simple_imputer(imputer);  // no route -- every column
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";

  const std::vector<cls::TransformSpec>& specs = flow.transform_specs();
  std::cout << "feature_route.empty() = " << specs.front().feature_route.empty() << "\n";

  return 0;
}
```

Output

```text
ok = true
feature_route.empty() = true
```

### Example 3

Chaining several typed stages to build a realistic mixed-type pipeline —
scale the numeric columns, one-hot-encode the categorical column, leave
everything else untouched.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0, 1});   // two numeric columns
  flow.add_one_hot_encoder({}, {2});      // one categorical column

  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  for (const cls::TransformSpec& spec : flow.transform_specs()) {
    std::cout << "kind = " << static_cast<int>(spec.kind)
              << ", route size = " << spec.feature_route.size() << "\n";
  }

  return 0;
}
```

Output

```text
stage_count() = 2
kind = 0, route size = 2
kind = 6, route size = 1
```

### Example 4

Every one of the twenty typed methods, called once each on the same
`Flow`, with default options — the full inventory, not just the three
that happen to appear in the examples above.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  std::vector<cls::Status> results{};
  results.push_back(flow.add_standard_scaler());
  results.push_back(flow.add_minmax_scaler());
  results.push_back(flow.add_robust_scaler());
  results.push_back(flow.add_maxabs_scaler());
  results.push_back(flow.add_simple_imputer());
  results.push_back(flow.add_neighbor_imputer());
  results.push_back(flow.add_iterative_imputer());
  results.push_back(flow.add_one_hot_encoder());
  results.push_back(flow.add_ordinal_encoder());
  results.push_back(flow.add_label_encoder());
  results.push_back(flow.add_frequency_encoder());
  results.push_back(flow.add_basen_encoder());
  results.push_back(flow.add_hashing_encoder());
  results.push_back(flow.add_target_encoder());
  results.push_back(flow.add_woe_encoder());
  results.push_back(flow.add_vector_normalizer());
  results.push_back(flow.add_quantile_map());
  results.push_back(flow.add_power_map());
  results.push_back(flow.add_threshold_binarizer());
  results.push_back(flow.add_polynomial_lift());

  bool all_ok = true;
  for (const cls::Status& s : results) all_ok = all_ok && s.ok();
  std::cout << "all 20 ok() = " << std::boolalpha << all_ok << "\n";
  std::cout << "results.size() = " << results.size() << "\n";
  std::cout << "stage_count()  = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
all 20 ok() = true
results.size() = 20
stage_count()  = 20
```

## See Also

- [`add_named_transform`](add_named_transform.md) — the single dynamic form
  these all forward to.
- [`add_column_stage`](add_column_stage.md) — the same, plus naming the
  stage at construction.
- [`TransformSpec`](../TransformSpec.md) — what each call appends.
- [`prep`](../../prep.md) — every options struct these methods accept.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_named_transform.html">
    <span class="label">Previous</span>
    <span class="title">add_named_transform</span>
  </a>
  <a class="next-link" href="append_transform.html">
    <span class="label">Next</span>
    <span class="title">append_transform</span>
  </a>
</div>
<!-- apiref:nav:end -->
