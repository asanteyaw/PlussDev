<!-- pluss-hand-authored -->

# DataFrameLabelAccessor::map

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::map(const std::function<ThreadValue(const ThreadValue&)>& fn) const -> DataFrame
```

## What It Does

Applies `fn` to every case label individually and replaces the case labels
with the results. Only works on an **unstructured** frame (no `case_axis_` —
i.e. not one built by promoting a named feature via [`set(feature_name)`](set.md));
calling it on a structured, multi-level frame throws, since there's no single
flat thread to map over. Map an individual level via
[`part(name)`](part.md) and [`set()`](set.md) it back instead.

## Parameters

**fn** (`const std::function<ThreadValue(const ThreadValue&)>&`)

A callback receiving one case label at a time (wrapped in a `ThreadValue`) and
returning its replacement.

## Returns

`DataFrame`
    A new frame with the same features, and case labels replaced by `fn`'s
    output.

## Examples

Example 1. Append a suffix to every case label on an unstructured `Note` axis.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales", Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto tagged = orders.label().set(Thread(pluss::make_note({"r1", "r2", "r3", "r4"})));
std::cout << tagged;

auto out = tagged.label().map([](const ThreadValue& v) {
  return ThreadValue(Thread(pluss::make_note({v.to_str() + "!"})));
});
std::cout << out;
```

### Output (before)

```text
============
     | sales
------------
r1   | 100  
r2   | 200  
r3   | 150  
r4   | 90   
------------
[4 cs x 1 feat]
```

### Output (after)

```text
============
     | sales
------------
r1!  | 100  
r2!  | 200  
r3!  | 150  
r4!  | 90   
------------
[4 cs x 1 feat]
```

`tagged`'s case labels came from `set(const Thread&)`, so `has()` is `false`
and `map()` is reachable; each label is transformed by appending `"!"`.

Example 2. Call `map()` on a structured frame, to see it throw.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");
std::cout << labeled.label().has();

try {
  auto out = labeled.label().map([](const ThreadValue& v) {
    return ThreadValue(Thread(pluss::make_note({v.to_str() + "!"})));
  });
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
true
DataFrame::label().map: structured labels are not supported; map a label part instead
```

`labeled`'s case labels came from `set("region")`, which always creates a
structured `case_axis_`, so `has()` is `true` and `map()` is unreachable — it
throws instead of silently doing nothing.

## Notes

- Throws on a structured (multi-level) frame — check
  [`has()`](has.md) first, or map a single level with
  [`part(name)`](part.md) and reattach it with [`set()`](set.md), if that's
  what you need.
- `fn` must return a `ThreadValue` of a kind consistent with the rest of the
  output (a length-1 `Thread` wrapped in `ThreadValue`, as constructed above).

## See Also

- [`DataFrameLabelAccessor::values`](values.md): pull the raw label thread if you
  need to cast or otherwise transform label payloads outside `map(...)`.
- [`DataFrameLabelAccessor::set`](set.md): the underlying mechanism this method
  applies its result through.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="levels.html">
    <span class="label">Previous</span>
    <span class="title">levels</span>
  </a>
  <a class="next-link" href="monotonic.html">
    <span class="label">Next</span>
    <span class="title">monotonic</span>
  </a>
</div>
