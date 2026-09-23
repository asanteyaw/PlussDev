<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 280e981d0a31 -->

# `pluss::ai::nn::Module::snapshot`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
Module::Snapshot nn::Module::snapshot() const;
```

## Parameters

None.

## Returns

A `Module::Snapshot{bool training, vector<SnapshotEntry> parameters, vector<SnapshotEntry> buffers}`,
where each `SnapshotEntry` is `{std::string name, Series value}`. Every
`value` is an independent **deep copy** (`Series::clone()`) of the current
parameter or buffer, taken via [`named_parameters`](named_parameters.md) /
[`named_buffers`](named_buffers.md) — mutating the module afterward never
changes an already-taken snapshot, and mutating the snapshot never changes
the module. An unregistered/undefined `Series` snapshots as a default
`Series{}` rather than being skipped, so `restore` can still find its name.

## Errors

None.

## Practical Examples

Three examples: capturing a single layer's snapshot (the capture half of the
round trip fully walked through on [`restore`](restore.md)), the empty case,
and a multi-stage `Schedule`'s dotted names.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion layer(2, 2, /*bias=*/false, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), /*seed=*/7);

  pluss::ai::nn::Module::Snapshot snap = layer.snapshot();
  std::cout << "snapshot.training = " << std::boolalpha << snap.training << "\n";
  std::cout << "snapshot.parameters.size() = " << snap.parameters.size() << "\n";
  std::cout << "snapshot.parameters[0].name = '" << snap.parameters[0].name << "'\n";

  return 0;
}
```

Output

```text
snapshot.training = true
snapshot.parameters.size() = 1
snapshot.parameters[0].name = 'weight'
```

Only one entry (`"weight"`) — this `Fusion` was constructed with
`bias=false`, so it never registered a bias parameter to snapshot.

### Example 2

A bare `Module`, nothing registered -- `snapshot()` still succeeds, with
empty `parameters`/`buffers` vectors.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  pluss::ai::nn::Module m;
  auto snap = m.snapshot();
  std::cout << "bare Module snapshot: parameters.empty()=" << std::boolalpha << snap.parameters.empty()
            << " buffers.empty()=" << snap.buffers.empty() << "\n";
  return 0;
}
```

Output

```text
bare Module snapshot: parameters.empty()=true buffers.empty()=true
```

### Example 3

A two-stage `Schedule` -- `snapshot()` captures the same dotted names
[`named_parameters`](named_parameters.md) would report, since that is
exactly what it calls internally.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("in", nn::Fusion(3, 4));
  net.then("out", nn::Fusion(4, 2, /*bias=*/false));

  auto snap = net.snapshot();
  for (auto& entry : snap.parameters) {
    std::cout << "'" << entry.name << "'\n";
  }

  return 0;
}
```

Output

```text
'in.weight'
'in.bias'
'out.weight'
```

## See Also

- [`restore`](restore.md) -- consumes the value this method produces, and
  covers the full mutate/restore round trip plus the mismatched-snapshot
  case.
- [`train`](train.md) -- `training` is captured alongside values.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="restore.html">
    <span class="label">Previous</span>
    <span class="title">restore</span>
  </a>
  <a class="next-link" href="to.html">
    <span class="label">Next</span>
    <span class="title">to</span>
  </a>
</div>
<!-- apiref:nav:end -->
