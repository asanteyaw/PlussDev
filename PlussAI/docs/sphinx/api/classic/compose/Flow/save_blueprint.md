<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e25ac446706 -->

# `pluss::ai::classic::Flow::save_blueprint`

`save_blueprint` serializes a `Flow`'s **structure** — every stage's kind,
key, remainder policy, feature route, and options — into one plain-text
string. It carries no fitted parameters at all (no learned center/scale, no
model weights): pair it with
[`dump_transform_states`](dump_transform_states.md) /
[`dump_model_state`](dump_model_state.md) for a complete persisted `Flow`, or
use it alone when you only need to recreate the same *unfit* pipeline shape
elsewhere.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<std::string> classic::Flow::save_blueprint() const;
```

## Parameters

None.

## Returns

A `StatusOr<std::string>` — a versioned, line-oriented text format
(`pluss_flow_blueprint_v2`, one line per stage: kind, key, remainder,
route, serialized options). Treat the format as opaque and round-trip it only
through [`load_blueprint`](load_blueprint.md); it is not meant to be
hand-edited, though it stays plain text deliberately (diffable, loggable).
Model attachment is **not** part of the blueprint — a blueprint only ever
describes stages; reattach a model with `set_model`/`set_classifier`/
`set_regressor` after loading one.

## Errors

- `Status::invalid_argument`, `"Flow::save_blueprint does not support opaque
  append_transform stages."` — this `Flow` has a stage added via
  [`append_transform`](append_transform.md) (a hand-built `TransformerBase`
  with no declarative `TransformKind`/options to serialize). Rebuild the
  pipeline from named stages if you need it to survive a blueprint round
  trip.

## Practical Examples

### Example 1

A two-stage flow's blueprint, and the round trip through
[`load_blueprint`](load_blueprint.md) onto a fresh `Flow` — same structure,
verified via [`flow_summary`](flow_summary.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow original;
  original.add_standard_scaler({}, {0});
  original.add_one_hot_encoder({}, {1});
  original.set_stage_key(0, "scale");
  original.set_stage_key(1, "encode");

  const std::string blueprint = original.save_blueprint().value();
  std::cout << blueprint;

  cls::Flow restored;
  restored.load_blueprint(blueprint);
  std::cout << "restored stage_count = " << restored.stage_count() << "\n";
  std::cout << (restored.flow_summary() == original.flow_summary() ? "summaries match\n" : "summaries differ\n");

  return 0;
}
```

Output

```text
pluss_flow_blueprint_v2
standard_scaler|scale|pass|0|with_centering=1;with_scaling=1;variance_floor=9.9999999999999998e-13
one_hot_encoder|encode|pass|1|max_categories=20;overflow=1;include_unknown_bucket=1
restored stage_count = 2
summaries match
```

### Example 2

`append_transform` blocks `save_blueprint` — an opaque stage has nothing
declarative to serialize.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.append_transform(std::make_unique<cls::prep::StandardScaler>());
  const cls::StatusOr<std::string> blueprint = flow.save_blueprint();
  std::cout << "ok = " << std::boolalpha << blueprint.ok() << "\n";
  std::cout << "message = \"" << blueprint.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::save_blueprint does not support opaque append_transform stages."
```

### Example 3

An empty `Flow{}` (no stages) still produces a valid blueprint — just the
header line, and it round-trips cleanly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  const std::string blueprint = flow.save_blueprint().value();
  std::cout << "blueprint = \"" << blueprint << "\"\n";

  cls::Flow restored;
  const cls::Status loaded = restored.load_blueprint(blueprint);
  std::cout << "load ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "restored stage_count = " << restored.stage_count() << "\n";

  return 0;
}
```

Output

```text
blueprint = "pluss_flow_blueprint_v2
"
load ok = true
restored stage_count = 0
```

## See Also

- [`load_blueprint`](load_blueprint.md) — the restoring half.
- [`dump_transform_states`](dump_transform_states.md) /
  [`dump_model_state`](dump_model_state.md) — the fitted parameters a
  blueprint deliberately excludes.
- [`clone`](clone.md) — an in-memory deep copy, not a text format.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nested_param_keys.html">
    <span class="label">Previous</span>
    <span class="title">nested_param_keys</span>
  </a>
  <a class="next-link" href="set_classifier.html">
    <span class="label">Next</span>
    <span class="title">set_classifier</span>
  </a>
</div>
<!-- apiref:nav:end -->
