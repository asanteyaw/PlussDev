<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 53f5157b6a27 -->

# `pluss::ai::classic::Flow::load_blueprint`

`load_blueprint` rebuilds a `Flow`'s stages from a
[`save_blueprint()`](save_blueprint.md) string — clearing whatever stages the
`Flow` already had first. It restores structure only (stage kinds, keys,
routes, options), never fitted parameters; a `Flow` built this way is
unfit (`trained() == false`) until you `learn`/`learn_map` it, or separately
restore state via [`load_transform_states`](load_transform_states.md).

## Signature

```cpp
classic::Status classic::Flow::load_blueprint(std::string_view text);
```

## Parameters

`text` — `std::string_view`
: A string previously produced by [`save_blueprint()`](save_blueprint.md).
  Both the current (`pluss_flow_blueprint_v2`) and an older
  (`pluss_flow_blueprint_v1`, no per-stage options) format are accepted.

## Returns

A `Status`. `.ok()` on success; `stage_count()` and `flow_summary()` reflect
the restored structure afterward. **Any stages this `Flow` already had are
discarded first** (`load_blueprint` calls `clear_transforms()` internally
before parsing), even if `text` turns out to be malformed partway through —
do not assume the previous stages survive a failed `load_blueprint` call.

## Errors

- `Status::invalid_argument`, `"Flow::load_blueprint requires a non-empty
  blueprint."` — `text` is empty.
- `Status::invalid_argument`, `"Flow::load_blueprint invalid blueprint
  header."` — the first line is neither blueprint version header.
- `Status::invalid_argument`, `"Flow::load_blueprint malformed stage line."` /
  `"...unknown stage kind."` / `"...invalid remainder policy."` /
  `"...route parse error."` — a stage line does not parse cleanly; these
  fire on hand-edited or corrupted blueprint text, which is why
  `save_blueprint`'s own page recommends treating the format as opaque.

## Practical Examples

### Example 1

Loading a valid blueprint discards prior stages first — starting from a
`Flow` with one stage, loading a blueprint that describes a different one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow source;
  source.add_one_hot_encoder({}, {1});
  const std::string blueprint = source.save_blueprint().value();

  cls::Flow target;
  target.add_standard_scaler();  // a different stage -- discarded on load
  std::cout << "target stage_count before load = " << target.stage_count() << "\n";
  const cls::Status loaded = target.load_blueprint(blueprint);
  std::cout << "load ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "target stage_count after load  = " << target.stage_count() << "\n";
  std::cout << target.flow_summary() << "\n";

  return 0;
}
```

Output

```text
target stage_count before load = 1
load ok = true
target stage_count after load  = 1
task=2 stages=1 trained=0 model=0
[0] stage0 kind=one_hot_encoder route=1 remainder=pass
```

### Example 2

Malformed blueprint text — an empty string and an unrecognized header.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  const cls::Status empty = flow.load_blueprint("");
  std::cout << "empty:  ok = " << std::boolalpha << empty.ok()
            << ", message = \"" << empty.message << "\"\n";

  const cls::Status bad_header = flow.load_blueprint("not_a_real_blueprint\n");
  std::cout << "header: ok = " << bad_header.ok()
            << ", message = \"" << bad_header.message << "\"\n";

  return 0;
}
```

Output

```text
empty:  ok = false, message = "Flow::load_blueprint requires a non-empty blueprint."
header: ok = false, message = "Flow::load_blueprint invalid blueprint header."
```

### Example 3

The older `pluss_flow_blueprint_v1` header (no per-stage options segment)
still loads — options fall back to that kind's defaults.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const std::string v1_text = "pluss_flow_blueprint_v1\nstandard_scaler|-|pass|\n";
  cls::Flow flow;
  const cls::Status loaded = flow.load_blueprint(v1_text);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << flow.flow_summary() << "\n";

  return 0;
}
```

Output

```text
ok = true
task=2 stages=1 trained=0 model=0
[0] stage0 kind=standard_scaler route=0 remainder=pass
```

## See Also

- [`save_blueprint`](save_blueprint.md) — the producing call.
- [`clear_transforms`](clear_transforms.md) — what `load_blueprint` calls
  internally before parsing.
- [`load_transform_states`](load_transform_states.md) — restoring fitted
  parameters onto the stages this rebuilds.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn_map.html">
    <span class="label">Previous</span>
    <span class="title">learn_map</span>
  </a>
  <a class="next-link" href="load_model_state.html">
    <span class="label">Next</span>
    <span class="title">load_model_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
