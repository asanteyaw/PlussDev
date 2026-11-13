<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ba92cf6a7ed3 -->

# Slate::prod

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
SlateValue Slate::prod() const
```

## What It Does

Returns the product of every observed (non-missing) value in a numeric `Series`
slate, as a scalar `SlateValue` — the multiplicative counterpart to
[`sum()`](sum.md). Missing entries are excluded before the product is computed,
not treated as `1`. Restricted to `Series` payloads; every other kind throws. A
slate whose every entry is missing (or that is empty) also throws.

See [`Slate::sum`'s note on printing a `SlateValue`](sum.md) — streaming the
result directly prints its diagnostic form, not a bare number; use `.to_dbl()`
(as every example below does) for a clean scalar.

## Parameters

`None`

## Returns

`SlateValue`
    A scalar wrapping the product, in the source `Series`'s own numeric dtype
    (or its promoted equivalent).

## Examples

Example 1. Multiply every observed value together.

```cpp
using namespace pluss::table;

auto factors = Slate(pluss::make_series({2.0, 3.0, 4.0}));
auto out = factors.prod();
std::cout << out.to_dbl();
```

### Output

```text
24
```

Example 2. Missing entries are excluded before multiplying, not treated as `1`.

```cpp
auto with_gap = Slate(
    pluss::make_series({2.0, 3.0, 4.0}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out2 = with_gap.prod();
std::cout << out2.to_dbl();
```

### Output

```text
8
```

Only `2.0` and `4.0` (the two observed entries) participate — `2 * 4 = 8`, not
`2 * 3 * 4 = 24`.

Example 3. A slate with no observed values throws.

```cpp
auto all_missing = Slate(
    pluss::make_series({2.0, 3.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.prod();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::prod on an empty (all-null) selection
```

Example 4. Any non-`Series` payload throws.

```cpp
auto words = Slate(pluss::make_note({"a", "b"}));
try {
  auto bad = words.prod();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::prod is only valid for Series payload
```

## Notes

- Missing entries are excluded before multiplying — never treated as `1`.
- An empty slate, or one where every entry is missing, throws `"Slate::prod on
  an empty (all-null) selection"`.
- Restricted to `Series` payloads; anything else throws `"Slate::prod is only
  valid for Series payload"`.

## See Also

- [`Slate::sum`](sum.md): the additive counterpart, including the note on how
  `SlateValue` prints.
- [`Slate::cumprod`](cumprod.md): the running product at every position,
  instead of one final total.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pow.html">
    <span class="label">Previous</span>
    <span class="title">pow</span>
  </a>
  <a class="next-link" href="ptype.html">
    <span class="label">Next</span>
    <span class="title">ptype</span>
  </a>
</div>
