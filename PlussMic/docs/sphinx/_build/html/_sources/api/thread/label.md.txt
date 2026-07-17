# Thread::label

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::label() -> ThreadLabelAccessor
```

Return the Thread label machinery for inspecting and manipulating 1D case labels.

`Thread::label()` is the 1D foundation for label handling. DataFrame label machinery is built above this because a DataFrame case axis is itself a `Thread`.

Use this surface for new code instead of reaching for lower-level case-label helpers directly:

- `thread.label().values()` returns the current label values.
- `thread.label().has()` reports whether the thread is defined and can expose labels.
- `thread.label().set(labels)` returns a copy with explicit labels attached.
- `thread.label().range()` returns a copy with explicit `0..n-1` labels materialized.
- `thread.label().reset()` returns a copy with explicit labels cleared.
- `thread.label().align(target, options)` delegates to Thread alignment.
- `thread.label().unite(...)`, `intersect(...)`, `diff(...)`, and `symdiff(...)` provide Index-style set operations.
- `thread.label().find(targets)` returns exact-match positions and `-1` for missing targets.
- `thread.label().pos(value)` returns all matching positions for one label and throws when absent.
- `thread.label().slice(start, stop)` returns an inclusive label-value slice.
- `thread.label().fillnull(...)` returns a copy whose labels have missing values filled.
- `thread.label().isnull()` and `thread.label().notnull()` inspect label missingness.

## Parameters

`None`

## Returns

`ThreadLabelAccessor`
    Lightweight 1D label accessor bound to the source thread.

## Notes

`label().values()` returns default range labels when the thread has no explicit labels, matching the Pluss hybrid alignment model.

`label().set(...)`, `label().range()`, `label().reset()`, `label().align(...)`, and `label().fillnull(...)` are non-mutating. They return new `Thread` values.

DataFrame `label().values()` returns the frame's case-label thread. Thread `label()` is where 1D relabeling and 1D label alignment live.

Set operations preserve label payload dtype when the two label vectors are compatible: same Series dtype stays unchanged, mixed numeric Series labels upcast to `Float64`, Note labels stay Note, and Timeline/TimeIndex labels stay temporal. Incompatible mixed label payloads fall back to Note labels through comparable string keys.

## Examples

Example 1. Inspect default range labels.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto out = scores.label().values();
```

### Output

```text
out:
0
1
2
```

Example 2. Attach custom labels.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto labeled = scores.label().set(Thread(pluss::make_note({"Ada", "Bo", "Cy"})));
auto out = labeled.label().values();
```

### Output

```text
out:
Ada
Bo
Cy
```

Example 3. Materialize explicit range labels.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto out = scores.label().range();
```

### Output

```text
out.label().values():
0
1
2
```

Example 4. Reset explicit labels back to default range labels.

```cpp
using namespace pluss::table;

auto labeled = Thread(pluss::make_series({10.0, 20.0, 30.0}))
    .label().set(Thread(pluss::make_note({"Ada", "Bo", "Cy"})));

auto out = labeled.label().reset().label().values();
```

### Output

```text
out:
0
1
2
```

Example 5. Align one thread to a target label order.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}))
    .label().set(Thread(pluss::make_note({"Bo", "Ada", "Cy"})));

auto out = scores.label().align(
    Thread(pluss::make_note({"Ada", "Bo", "Missing"})),
    AlignOptions{.align = AlignBy::Caselbl});
```

### Output

```text
out:
20
10
NULL
```

Example 6. Inspect and fill missing labels.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}))
    .label().set(Thread(pluss::make_note({"Ada", "", "Cy"})));

auto missing = scores.label().isnull();
auto filled = scores.label().fillnull("Missing").label().values();
```

### Output

```text
missing:
Ada      false
NULL     true
Cy       false

filled:
Ada
Missing
Cy
```

Example 7. Use Index-style set operations.

```cpp
using namespace pluss::table;

auto left = Thread(pluss::make_series({1.0, 2.0, 3.0}))
    .label().set(Thread(pluss::make_note({"b", "a", "c"})));
auto right = Thread(pluss::make_note({"b", "d", "a"}));

auto united = left.label().unite(right);
auto common = left.label().intersect(right);
auto only_left = left.label().diff(right);
auto exclusive = left.label().symdiff(right);
```

### Output

```text
united:    [b, a, c, d]
common:    [b, a]
only_left: [c]
exclusive: [c, d]
```

Example 8. Find positions and locate duplicate labels.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}))
    .label().set(Thread(pluss::make_note({"x", "y", "x"})));

auto positions = scores.label().find(Thread(pluss::make_note({"y", "missing"})));
auto x_positions = scores.label().pos("x");
```

### Output

```text
positions:
y        1
missing -1

x_positions:
0
2
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kurtosis.html">
    <span class="label">Previous</span>
    <span class="title">kurtosis</span>
  </a>
  <a class="next-link" href="land.html">
    <span class="label">Next</span>
    <span class="title">land</span>
  </a>
</div>
