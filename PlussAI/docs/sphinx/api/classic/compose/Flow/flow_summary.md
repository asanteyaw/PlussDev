<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8ff442e9ff91 -->

# `pluss::ai::classic::Flow::flow_summary`

`flow_summary` renders the same structural state
[`graph_info()`](graph_info.md) exposes as a value type, as one
human-readable multi-line string — a header line (task, stage count,
trained, model) followed by one line per stage (its key, kind, route width,
remainder policy). Reach for it when you want to print or log a flow's shape
(a debugging session, a training log); reach for
[`graph_info()`](graph_info.md) when you need the values themselves.

## Signature

```cpp
[[nodiscard]] std::string classic::Flow::flow_summary() const;
```

## Parameters

None.

## Returns

A `std::string`, never empty (even a bare `Flow{}` produces a header line
with `stages=0`). The exact format is a debugging convenience, not a
persistence format — use [`save_blueprint`](save_blueprint.md) /
[`load_blueprint`](load_blueprint.md) if you need to reconstruct a `Flow`
from text.

## Errors

None — like `graph_info()`, this only reads existing state.

## Practical Examples

### Example 1

A two-stage classification flow's summary, printed directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler({}, {0});
  flow.add_one_hot_encoder({}, {1});
  flow.set_stage_key(0, "scale");
  flow.set_stage_key(1, "encode");

  std::cout << flow.flow_summary() << "\n";

  return 0;
}
```

Output

```text
task=0 stages=2 trained=0 model=1
[0] scale kind=standard_scaler route=1 remainder=pass
[1] encode kind=one_hot_encoder route=1 remainder=pass
```

### Example 2

An empty `Flow{}`'s summary — a header line, no stage lines.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  std::cout << flow.flow_summary() << "\n";

  return 0;
}
```

Output

```text
task=2 stages=0 trained=0 model=0
```

### Example 3

`set_nested_param`'s `kind` write swaps a stage's transform in place —
reflected immediately in the next `flow_summary()` call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler();
  flow.set_stage_key(0, "prep");
  std::cout << "before: " << flow.flow_summary() << "\n";

  flow.set_nested_param("prep::kind", static_cast<double>(static_cast<int>(cls::TransformKind::MinMaxScaler)));
  std::cout << "after:  " << flow.flow_summary() << "\n";

  return 0;
}
```

Output

```text
before: task=2 stages=1 trained=0 model=0
[0] prep kind=standard_scaler route=0 remainder=pass
after:  task=2 stages=1 trained=0 model=0
[0] prep kind=minmax_scaler route=0 remainder=pass
```

## See Also

- [`graph_info`](graph_info.md) — the same state as a structured value
  instead of a string.
- [`save_blueprint`](save_blueprint.md) — the actual reconstructible
  serialization format.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dump_transform_states.html">
    <span class="label">Previous</span>
    <span class="title">dump_transform_states</span>
  </a>
  <a class="next-link" href="graph_info.html">
    <span class="label">Next</span>
    <span class="title">graph_info</span>
  </a>
</div>
<!-- apiref:nav:end -->
