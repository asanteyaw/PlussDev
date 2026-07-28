<!-- pluss-hand-authored -->

# DataFrame::transform

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
// Whole-frame, thread-wise
DataFrame::transform(const std::function<Thread(const Thread&)>& fn) const -> DataFrame
DataFrame::transform(const std::function<Thread(const Thread&)>& fn, Axis axis) const -> DataFrame  // axis must be Axis::Feature

// One or several features via structured specs
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
- **One or several features via `TransformSpec`** (`transform(specs)`): apply a
  different `Thread`-callback to each of one or several named features in one call —
  `transform({{"sales", fn}})` targets just `"sales"`; every feature not named in
  `specs` is left untouched.
- **Row-wise** (`Axis::Case` only): the two least common overloads. The
  `ThreadValue`-callback form calls `fn` once per case (each case observed as a
  one-row `DataFrame`) and expects a single scalar back, concatenating the results
  into one `Thread` — every returned value must share the same `ThreadKind`. The
  `DataFrame`-callback form is the same per-case iteration but expects a `DataFrame`
  back each time, controlled by `shape`: `ResultShape::Infer` (the default) imposes
  no consistency check at all; `Reduce`/`Preserve` require exactly one output row per
  case; `Expand` requires every case's callback result to share the same case count
  and feature schema.

To *derive a brand-new feature* from the whole frame (rather than rewrite an existing
one from its own values), reach for [`assign()`](assign.md) instead — `transform()`'s
`TransformSpec` overload only ever sees one feature's own `Thread` at a time, so it
can rewrite an existing feature but never create one that doesn't already exist.

## Parameters

**fn**

The transform function. Its signature (`Thread(const Thread&)`, `ThreadValue(const
DataFrame&)`, or `DataFrame(const DataFrame&)`) selects which overload is called. A
returned `Thread` must match the frame's case count.

**axis** (`Axis`)

For the whole-frame overload, must be `Axis::Feature` (throws otherwise). For the
row-wise overloads, must be `Axis::Case` (throws otherwise) — `axis` exists on these
signatures mainly to make the row-wise-vs-feature-wise choice explicit at the call
site.

**specs** (`pluss::RangeView<TransformSpec>`)

A list of `{feature, expr}` pairs, each naming one existing feature and the
`Thread`-callback to apply to it. A single-element list targets just one feature.

**shape** (`ResultShape`, default `Infer`, row-wise `DataFrame`-callback overload
only)

Controls what consistency is required across every case's callback result:
`Infer` (no check), `Reduce`/`Preserve` (each result must be exactly one row;
`Preserve` additionally requires the same feature schema as the input),
`Expand` (every result must share the same case count and feature schema as each
other).

## Returns

`DataFrame`
    (Most overloads) A new frame with the targeted feature(s) rewritten.

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

Example 2. Rewrite just one named feature with a `TransformSpec`, leaving the rest of
the frame alone by construction rather than by the callback's own logic.

```cpp
auto sales_clipped = orders.transform({
    {"sales", [](const Thread& t) { return t.clip(0.0, 110.0); }},
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

Example 3. Rewrite two named features with different callbacks in one call.

```cpp
auto both = orders.transform({
    {"id",    [](const Thread& t) { return t.add(100.0); }},
    {"sales", [](const Thread& t) { return t.clip(0.0, 110.0); }},
});
std::cout << both;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 101   100    Accra 
1    | 102   110    Tema  
2    | 103   90     Kumasi
--------------------------
[3 cs x 3 feat]
```

`city` is untouched since it wasn't named in `specs`.

## Notes

- `transform(fn)` (no `specs`) always applies `fn` to every feature; use the
  `TransformSpec` overload to target specific, already-existing features instead.
- `transform()` can only **rewrite** features that already exist on the frame —
  passing a name not already present throws. To **create** a new feature, use
  [`assign()`](assign.md) instead.
- Every returned `Thread` (whole-frame and `TransformSpec` overloads) must have the
  same length as the frame's case count, or the call throws.
- For the row-wise `DataFrame`-callback overload, `ResultShape::Infer` (the
  default) performs **no** shape consistency check across cases at all — reach for
  `Reduce`, `Preserve`, or `Expand` explicitly when you need that guarantee enforced.

## See Also

- [`DataFrame::assign`](assign.md): derive one or more **new** features from
  whole-frame expressions, including creating a feature that doesn't already exist —
  the tool to reach for when `transform()`'s existing-feature-only `TransformSpec`
  overload isn't enough.
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
