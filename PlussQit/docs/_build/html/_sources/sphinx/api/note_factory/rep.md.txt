<!-- pluss-hand-authored -->
# pluss::note::rep

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::rep(string_view value, integer_range sizes)
Note pluss::note::rep(string_view value, std::initializer_list<std::int64_t> sizes)
```

## Repeat One String Across A Shape, Value First

`rep()` builds the same result as [`full`](full.md) — a `Note` of shape
`sizes` with `value` in every cell — but takes its arguments in the other
order: **value first, shape second**. It exists for the call sites where the
string is the interesting part of the statement and the shape is a short
literal tail.

The value-first order is what makes the second overload possible:

```cpp
pluss::note::rep("-", {5})          // initializer_list shape, no wrapper
```

Because the shape is the *last* argument, it can be a braced
`initializer_list` and still bind unambiguously. `full`, whose shape comes
first, always needs a `RangeView` wrapper around a container. So the rule of
thumb is:

- **`rep("x", {2, 3})`** — shape is a small literal you are writing inline.
- **`full(shape_range, "x")`** — shape is a variable computed elsewhere.

Both produce a **UTF-8-mode** `Note`; both follow the same shape rules
(non-negative extents, zero extents allowed, rank at most three, validated
before allocation). `rep` with a range argument simply forwards to `full`.

The name mirrors the idea of "repeat this value into that shape". It does
**not** repeat the *characters* of `value` — `rep("ab", {3})` is three cells
each holding `"ab"`, not one cell holding `"ababab"`. Character-level
repetition is `Note::operator*` on an existing `Note`.

## Example 1: A Rule Of Dashes

```cpp
std::cout << pluss::note::print(pluss::note::rep("-", {5})) << "\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["-", "-", "-", "-", "-"]
```

Five cells, each the one-character string `"-"`. The `initializer_list`
shape needs no wrapper.

## Example 2: A Multi-Character Value Across A Matrix

```cpp
const pluss::Note n = pluss::note::rep("ab", {2, 2});
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["ab", "ab"], 
  ["ab", "ab"]
]
```

Each cell holds the whole string `"ab"` — `rep` repeats the value, not its
letters.

## See Also

- [`pluss::note::full`](full.md): the shape-first spelling of the same
  operation.
- [`pluss::note::empty`](empty.md): `rep("", sizes)`.
- [`pluss::note::make`](make.md): build from the data when the shape is
  implied.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand_between.html">
    <span class="label">Previous</span>
    <span class="title">rand_between</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
