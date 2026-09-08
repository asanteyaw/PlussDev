<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0e9f61edb47a -->

# `lymph::workflow::Policy::apply` and `empty`

[Back to `lymph::workflow::Policy`](../Policy.md)

`apply` runs the whole admission contract against a frame and returns the
admitted frame. It is the one method `Workflow` calls — once per raw-frame
verb, before the held `Model` sees anything. `empty()` reports whether a
`Policy` has any rules at all.

## Signature

```cpp
pluss::table::DataFrame lymph::workflow::Policy::apply(const pluss::table::DataFrame& frame) const;
bool lymph::workflow::Policy::empty() const;
```

## Parameters

`const pluss::table::DataFrame& frame`
: The raw frame to admit. `apply` does not mutate it — it returns a new
  frame.

## Returns

`apply(frame)`
: The admitted frame, produced in a fixed order:

  1. **Validate presence** — every column named in `require` / `drop` /
     `reject_missing` / `allow_missing` / `null_filter_columns` must exist,
     or `apply` throws naming the first missing one.
  2. **Check `forbid`** — a forbidden column present and not also in `drop`
     throws.
  3. **Apply `drop`** — remove those columns.
  4. **Apply `reject_missing`** — drop rows null in those columns.
  5. **Apply the legacy `filter_nulls` sweep** — if enabled.

`empty()`
: `true` only when every rule list is empty and `filter_nulls` is `false`. A
  `Workflow` built with an empty `Policy` is a pure pass-through.

## Errors

- `"missing required raw column \"<name>\""` — a named column is absent
  (step 1).
- `"forbidden raw column \"<name>\" is present; ..."` — step 2.

## Practical Examples

### Example 1

A `Policy` combining a column rule and a row rule — `apply` runs both, in
order.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const double nan_val = std::numeric_limits<double>::quiet_NaN();
  const DataFrame raw({
      {"customer_id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6}, {6}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1}, {6}, f64))},
      {"default", Thread(pluss::make_series({0.0, nan_val, 0.0, 1.0, 1.0, nan_val}, {6}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});
  policy.require({"income", "balance", "default"});
  policy.reject({"default"});

  const DataFrame admitted = policy.apply(raw);
  std::cout << "raw:      " << raw.nfeatures() << " features x " << raw.ncases() << " rows\n";
  std::cout << "admitted: " << admitted.nfeatures() << " features x " << admitted.ncases()
            << " rows  (customer_id dropped; 2 null-default rows dropped)\n";

  return 0;
}
```

Output

```text
raw:      4 features x 6 rows
admitted: 3 features x 4 rows  (customer_id dropped; 2 null-default rows dropped)
```

### Example 2

`empty()` and the pass-through case.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9}, {4}, f64))},
  });

  workflow::Policy blank{};
  std::cout << "Policy{}.empty()        = " << std::boolalpha << blank.empty() << "\n";
  const DataFrame passed = blank.apply(raw);
  std::cout << "pass-through unchanged  = "
            << (passed.nfeatures() == raw.nfeatures() && passed.ncases() == raw.ncases()) << "\n";

  workflow::Policy one_rule{};
  one_rule.drop({"balance"});
  std::cout << "after drop(): empty()   = " << one_rule.empty() << "\n";

  return 0;
}
```

Output

```text
Policy{}.empty()        = true
pass-through unchanged  = true
after drop(): empty()   = false
```

### Example 3

`apply` is replayed on every `Workflow` verb — a model trained through a
`Policy` still has it applied to fresh inference data.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train_raw({
      {"customer_id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6}, {6}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 3.6, 4.0, 4.4}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 1.8, 2.1, 2.4}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(train_raw, data_options);

  const DataFrame incoming({
      {"customer_id", Thread(pluss::make_series({7, 8, 9}, {3}, f64))},
      {"income", Thread(pluss::make_series({1.2, 4.2, 1.6}, {3}, f64))},
      {"balance", Thread(pluss::make_series({0.3, 2.2, 0.5}, {3}, f64))},
  });
  std::cout << "pred = " << trained.infer(incoming, data_options) << "\n";

  return 0;
}
```

Output

```text
pred = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
```

## See Also

- [`columns`](columns.md) / [`rows`](rows.md) — the rules `apply` runs.
- [`Workflow`](../Workflow.md) — calls `apply` on every raw-frame verb.
- [`Workflow::save`](../Workflow/save.md) — persists the `Policy` so a
  reloaded `Workflow` applies the identical contract.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="columns.html">
    <span class="label">Next</span>
    <span class="title">columns</span>
  </a>
</div>
<!-- apiref:nav:end -->
