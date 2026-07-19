<!-- pluss-hand-authored -->

# DataFrame::fold

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::fold(const std::function<Thread(const DataFrame&)>& fn, ResultShape shape = ResultShape::Reduce) const -> Thread
```

## What It Does

A plain escape hatch for a custom reduction that isn't already one of the built-in
aggregates ([`sum()`](sum.md), [`mean()`](mean.md), and so on). `fold()` calls `fn`
with the current frame and returns whatever `Thread` `fn` produces, optionally checking
that the result's size matches what `shape` expects. [`flow()`](flow.md) is the sibling
hook for a callback that returns a whole `DataFrame` instead of a `Thread`.

## Parameters

**fn** (`const std::function<Thread(const DataFrame&)>&`)

The callback to run. It receives the current frame by `const` reference and must
return a defined `Thread`. Throws `"DataFrame::fold: callback is empty"` if `fn` holds
no target, and `"DataFrame::fold: callback returned undefined"` if it returns an
undefined thread.

**shape** (`ResultShape`, default `ResultShape::Reduce`)

Governs what size the returned `Thread` is checked against:

- **`Reduce`** (the default): the result must have exactly one entry — a single
  aggregate value, the same shape [`sum()`](sum.md) or [`mean()`](mean.md) produce for
  one feature.
- **`Preserve`**: the result must have exactly `ncases()` entries — one value per row,
  the same shape a row-wise computed column would have.
- **`Infer`** / **`Expand`**: no size check is performed; `fn`'s result is returned
  exactly as computed.

## Returns

`Thread`
    Exactly whatever `fn` returned, after the `shape` check (if any) passes.

## Examples

Example 1. `ResultShape::Reduce` — compute a single custom aggregate value.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"units",      Thread(pluss::make_series({2, 5, 1}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0}))},
}, Thread(pluss::make_note({"o1", "o2", "o3"})));

auto avg_price = orders.fold([](const DataFrame& f) {
  return f.features({"unit_price"}).mean();
}, ResultShape::Reduce);
std::cout << avg_price;
```

### Output

```text
unit_price  798.5
ptype: Series
```

`f.features({"unit_price"})` projects down to a one-column frame first, so `.mean()`
on it produces exactly one value — satisfying `Reduce`'s "exactly one entry"
requirement. `(1200 + 45.5 + 1150) / 3 = 798.5`.

Example 2. `ResultShape::Preserve` — compute a custom per-row value using two
features together.

```cpp
auto revenue = orders.fold([](const DataFrame& f) {
  return f.features("units") * f.features("unit_price");
}, ResultShape::Preserve);
std::cout << revenue;
```

### Output

```text
o1  2400
o2  227.5
o3  1150
ptype: Series
```

The callback multiplies two feature threads elementwise, producing one value per case
— `2 * 1200 = 2400`, `5 * 45.5 = 227.5`, `1 * 1150 = 1150` — which satisfies
`Preserve`'s "exactly `ncases()` entries" requirement (`3` rows in, `3` values out).
Each case keeps the case label carried by `features("units")` (`o1`, `o2`, `o3`).

## Notes

- `fold()` does not compute anything itself — all of the actual logic lives in `fn`.
  It exists purely to standardize "run a callback against this frame and validate the
  result's size" so ad hoc reductions can slot into a pipeline the same way a built-in
  aggregate would.
- `Reduce` and `Preserve` are the two shapes with an enforced size check; `Infer` and
  `Expand` accept whatever size `fn` returns without validation — reach for them only
  when neither "exactly one value" nor "exactly one value per case" describes what
  your callback computes.
- Because `fn` receives the frame by `const` reference and `fold()` itself is `const`,
  the original frame is never modified by a `fold()` call.

## See Also

- [`DataFrame::flow`](flow.md): the sibling hook for a callback that returns a whole
  `DataFrame` instead of a `Thread`.
- [`DataFrame::sum`](sum.md), [`DataFrame::mean`](mean.md): built-in reductions to
  reach for first, before writing a custom `fold()` callback for something they
  already compute.
- [`DataFrame::transform`](transform.md): apply a function per-feature across the
  frame and get a `DataFrame` back, rather than reducing to a single `Thread`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flow.html">
    <span class="label">Previous</span>
    <span class="title">flow</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
