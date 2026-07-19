<!-- pluss-hand-authored -->

# DataFrame::transform

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
// Whole-frame, thread-wise
DataFrame::transform(const std::function<Thread(const Thread&)>& fn) const -> DataFrame
DataFrame::transform(const std::function<Thread(const Thread&)>& fn, Axis axis) const -> DataFrame  // axis must be Axis::Feature

// Single feature
DataFrame::transform(std::string_view feature, const std::function<Thread(const Thread&)>& fn) const -> DataFrame
DataFrame::transform(std::string_view feature, const std::function<Thread(const DataFrame&)>& fn) const -> DataFrame

// Several features via structured specs
DataFrame::transform(pluss::RangeView<TransformSpec> specs) const -> DataFrame

// Row-wise (Axis::Case only)
DataFrame::transform(const std::function<ThreadValue(const DataFrame&)>& fn, Axis axis) const -> Thread
DataFrame::transform(const std::function<DataFrame(const DataFrame&)>& fn, Axis axis, ResultShape shape = ResultShape::Infer) const -> DataFrame
```

## What It Does

Rewrites feature values by applying a user-supplied function, in several shapes
depending on which overload you reach for:

- **Whole-frame, thread-wise** (`transform(fn)`): applies `fn` to *every* feature's
  `Thread` independently, replacing each with `fn`'s result. The `Axis` overload is
  the same thing spelled out explicitly — it requires `Axis::Feature` and throws
  otherwise, pointing you at the row-wise `DataFrame`-callback overload for
  `Axis::Case` instead.
- **Single feature** (`transform(feature, fn)`): rewrites just `feature`. The
  `Thread`-callback form receives that feature's own values; the `DataFrame`-callback
  form receives the *whole frame* so `fn` can derive the new column from other
  features too — this form can also **create** a brand-new feature named `feature`
  if it doesn't already exist (it's implemented via [`assign()`](assign.md)).
- **Several features via `TransformSpec`** (`transform(specs)`): apply a different
  `Thread`-callback to each of several named features in one call.
- **Row-wise** (`Axis::Case` only): the two least common overloads. The
  `ThreadValue`-callback form calls `fn` once per case (each case observed as a
  one-row `DataFrame`) and expects a single scalar back, concatenating the results
  into one `Thread` — every returned value must share the same `ThreadKind`. The
  `DataFrame`-callback form is the same per-case iteration but expects a `DataFrame`
  back each time, controlled by `shape`: `ResultShape::Infer` (the default) imposes
  no consistency check at all; `Reduce`/`Preserve` require exactly one output row per
  case; `Expand` requires every case's callback result to share the same case count
  and feature schema.

## Parameters

**fn**

The transform function. Its signature (`Thread(const Thread&)`, `Thread(const
DataFrame&)`, `ThreadValue(const DataFrame&)`, or `DataFrame(const DataFrame&)`)
selects which overload is called. A returned `Thread` must match the frame's case
count.

**axis** (`Axis`)

For the whole-frame overload, must be `Axis::Feature` (throws otherwise). For the
row-wise overloads, must be `Axis::Case` (throws otherwise) — `axis` exists on these
signatures mainly to make the row-wise-vs-feature-wise choice explicit at the call
site.

**feature** (`std::string_view`)

The feature to rewrite (or create, for the `DataFrame`-callback single-feature
overload).

**specs** (`pluss::RangeView<TransformSpec>`)

A list of `{feature, expr}` pairs, each naming one feature and the `Thread`-callback
to apply to it.

**shape** (`ResultShape`, default `Infer`, row-wise `DataFrame`-callback overload
only)

Controls what consistency is required across every case's callback result:
`Infer` (no check), `Reduce`/`Preserve` (each result must be exactly one row;
`Preserve` additionally requires the same feature schema as the input),
`Expand` (every result must share the same case count and feature schema as each
other).

## Returns

`DataFrame`
    (Most overloads) A new frame with the targeted feature(s) rewritten (or, for the
    `DataFrame`-callback single-feature and row-wise overloads, potentially added).

`Thread`
    (Row-wise `ThreadValue`-callback overload) One entry per case, concatenated from
    each case's scalar result.

## Examples

Example 1. Rewrite every feature with the same thread-level function.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
std::cout << orders;
```

### Output (before)

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     120    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

```cpp
auto clipped = orders.transform([](const Thread& t) {
  return t.kind() == ThreadKind::Series ? t.clip(0.0, 110.0) : t;
});
std::cout << clipped;
```

### Output (after)

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     110    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

`sales`'s `120` is clipped down to `110`; `id` (already within `[0, 110]`) is
unaffected; `city` is untouched because the callback returns it unchanged for any
non-`Series` kind. `transform(fn)` applies `fn` to **every** feature — there is no
way to skip a feature except by having `fn` itself return it unchanged, as done
here.

Example 2. Rewrite just one named feature, leaving the rest of the frame alone by
construction rather than by the callback's own logic.

```cpp
auto sales_clipped = orders.transform("sales", [](const Thread& t) {
  return t.clip(0.0, 110.0);
});
std::cout << sales_clipped;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     110    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

Example 3. Derive a brand-new feature from the whole frame.

```cpp
auto with_profit = orders.transform("profit", [](const DataFrame& frame) {
  return Thread(frame.features("sales").as_series() * 0.3);
});
std::cout << with_profit;
```

### Output

```text
==================================
     | id    sales  city    profit
----------------------------------
0    | 1     100    Accra   30    
1    | 2     120    Tema    36    
2    | 3     90     Kumasi  27    
----------------------------------
[3 cs x 4 feat]
```

`"profit"` didn't exist before, so this `DataFrame`-callback single-feature overload
creates it, computed here as `30%` of each row's `sales`.

## Notes

- `transform(fn)` (no feature name) always applies `fn` to every feature; use the
  single-feature or `TransformSpec` overloads to target specific features instead.
- The single-feature `DataFrame`-callback overload can both **rewrite** an existing
  feature and **create** a brand-new one, depending on whether `feature` already
  exists — it's implemented directly via [`assign()`](assign.md).
- Every returned `Thread` (whole-frame and single-feature overloads) must have the
  same length as the frame's case count, or the call throws.
- For the row-wise `DataFrame`-callback overload, `ResultShape::Infer` (the
  default) performs **no** shape consistency check across cases at all — reach for
  `Reduce`, `Preserve`, or `Expand` explicitly when you need that guarantee enforced.

## See Also

- [`DataFrame::assign`](assign.md): the more direct way to derive one or more new
  features from frame-wide expressions, which this method's single-feature
  `DataFrame`-callback overload is built on.
- [`DataFrame::map`](map.md): a scalar-per-cell transform (`ThreadValue -> ThreadValue`,
  skipping nulls automatically), a narrower tool than this method's `Thread`-wide
  callbacks.
- [`DataFrame::fold`](fold.md), [`DataFrame::flow`](flow.md): other whole-frame
  callback-driven reductions and transformations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="top.html">
    <span class="label">Previous</span>
    <span class="title">top</span>
  </a>
  <a class="next-link" href="transpose.html">
    <span class="label">Next</span>
    <span class="title">transpose</span>
  </a>
</div>
