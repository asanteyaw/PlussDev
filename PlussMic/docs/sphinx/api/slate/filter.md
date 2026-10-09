<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f41a11c1d4d5 -->

# Slate::filter

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::filter(const pluss::Series& bool_mask) const -> Slate
Slate::filter(const Slate& bool_mask) const -> Slate
```

## What It Does

Keeps only the entries where `bool_mask` is `true`, dropping the rest — the classic
boolean-mask row filter, built on top of [`take()`](take.md) (`filter()` converts
the mask to the list of `true` positions, then takes them in order). `bool_mask`
must be 1D, `Bool`-scalar-typed, and exactly `size()` elements long.

Two overloads, same behaviour:

- **`filter(const pluss::Series&)`** takes a raw `Bool` `Series`.
- **`filter(const Slate&)`** takes a `Bool`-kind `Slate` directly — the shape the
  comparison methods ([`gt()`](gt.md), [`eq()`](eq.md), …) and predicates
  ([`isnull()`](isnull.md), [`duplicated()`](duplicated.md), [`between()`](between.md),
  …) already return, so you no longer need a trailing `.as_series()` to feed one
  back in. It simply unwraps the slate's payload and forwards to the `Series`
  overload; a non-`Series`-kind slate throws
  `"Slate::filter: bool mask must be a Bool-typed Series slate"`.

This mirrors [`Slate::put`](put.md), [`DataFrame::filter`](../dataframe/filter.md),
and `SlateGrouped::filter`, which all already accept a bare `Slate` mask.

## Parameters

**bool_mask** (`const pluss::Series&` or `const Slate&`)
    A `Bool`-typed, 1D mask the same length as `*this`. Each `true` position is
    kept, in original order; every `false` position is dropped. The `Slate`
    overload additionally requires the slate's payload kind to be `Series`.

## Returns

`Slate`
    A new slate holding only the entries where `bool_mask` was `true`, in their
    original relative order.

## Examples

Example 1. Pass a comparison result straight in — no `.as_series()` needed.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 25.0, 30.0, 15.0}));
auto out = scores.filter(scores.gt(20.0));
std::cout << out;
```

### Output

```text
0  25
1  30

ptype: Series
```

Example 2. The `Series` overload is still there for a mask you already hold as a
`Series` (here via an explicit `.as_series()`).

```cpp
auto mask = scores.gt(20.0).as_series();
auto out2 = scores.filter(mask);
std::cout << out2;
```

### Output

```text
0  25
1  30

ptype: Series
```

Example 3. A hand-built mask works the same way.

```cpp
auto mask3 = pluss::make_series(std::vector<bool>{true, false, true, false});
auto out3 = scores.filter(mask3);
std::cout << out3;
```

### Output

```text
0  10
1  30

ptype: Series
```

Example 4. Drop duplicate case-label rows by feeding [`duplicated()`](duplicated.md)
straight in — the `value_counts()`-style pattern.

```cpp
auto ids = Slate(pluss::make_note({"a", "b", "a", "c", "b"}));
auto counts = ids.freq();                       // case labels: a, b, c
auto repeats = counts.filter(counts.gt(1.0));   // keep counts > 1
std::cout << repeats;
```

### Output

```text
a  2
b  2

ptype: Series
```

Example 5. A length-mismatched mask throws.

```cpp
try {
  auto bad_mask = pluss::make_series(std::vector<bool>{true, false});
  auto bad = scores.filter(bad_mask);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::filter: bool mask length mismatch
```

## Notes

- `bool_mask` must be exactly `size()` elements long, 1D, and `Bool`-typed, or the
  call throws (`"Slate::filter: bool mask length mismatch"`,
  `"...bool mask must be 1D"`, or `"...bool mask dtype must be Bool"`, depending on
  which check fails).
- The `Slate` overload additionally requires `kind() == SlateKind::Series`;
  a `Note`/`Timeline`/`TimeIndex`-kind slate throws
  `"Slate::filter: bool mask must be a Bool-typed Series slate"`.
- Built on [`take()`](take.md) — the true-position indices are gathered and passed
  straight through.
- Preserves relative order of the kept entries.

## See Also

- [`Slate::take`](take.md): the general gather this method is built on.
- [`Slate::gt`](gt.md) / [`Slate::eq`](eq.md) / other comparisons: the usual
  source of the boolean mask passed here.
- [`Slate::dropnull`](dropnull.md): a specialized filter for `NULL` removal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull_.html">
    <span class="label">Previous</span>
    <span class="title">fillnull_</span>
  </a>
  <a class="next-link" href="floordiv.html">
    <span class="label">Next</span>
    <span class="title">floordiv</span>
  </a>
</div>
