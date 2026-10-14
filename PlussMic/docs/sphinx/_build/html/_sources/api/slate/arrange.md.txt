<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6672e4b10217 -->

# Slate::arrange

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::arrange(bool ascending = true, bool nulls_last = true) const -> Slate
```

## What It Does

Returns a new slate with every entry reordered by value — the payload kind is
unchanged, only the order changes. The comparison key depends on what the slate
holds:

- **Numeric `Series`**: sorted numerically. If there's no validity mask, this is a
  single stable argsort (`descending = !ascending`) followed by a
  [`take()`](take.md). If there *is* a validity mask, valid and `NULL` positions
  are argsorted separately (only the valid subset is compared), then concatenated
  in the order `nulls_last` dictates.
- **`Timeline`/`TimeIndex`**: sorted by epoch-nanosecond value via a stable sort,
  with the same null-grouping logic applied.
- **`Note`-kind, or a non-numeric `Series`**: sorted lexicographically by string
  key (each entry's string form), with the same null-grouping logic applied.

In every case the sort is **stable** — entries that compare equal keep their
original relative order — and `NULL` entries are grouped to one end according to
`nulls_last` rather than being compared to real values at all. A slate with
`size() <= 1` returns unchanged immediately.

## Parameters

**ascending** (`bool`, default `true`)
    `true` sorts smallest/earliest/alphabetically-first values first; `false`
    reverses the comparison.

**nulls_last** (`bool`, default `true`)
    `true` places every `NULL` entry after all valid entries, regardless of
    `ascending`; `false` places them before all valid entries instead.

## Returns

`Slate`
    A new slate of the same kind and length, reordered.

## Examples

Example 1. Ascending numeric sort, no validity mask.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({30.0, 10.0, 50.0, 20.0}));
auto out = scores.arrange();
std::cout << out;
```

### Output

```text
0  10
1  20
2  30
3  50

ptype: Series
```

Example 2. Descending numeric sort.

```cpp
auto out2 = scores.arrange(false);
std::cout << out2;
```

### Output

```text
0  50
1  30
2  20
3  10

ptype: Series
```

Example 3. With a validity mask, `NULL`s sort last by default regardless of
`ascending`.

```cpp
auto masked = Slate(
    pluss::make_series({30.0, 10.0, 50.0, 20.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out3 = masked.arrange();
std::cout << out3;
```

### Output

```text
0  20
1  30
2  50
3  NULL

ptype: Series
```

Example 4. `nulls_last = false` moves the `NULL` entries to the front instead.

```cpp
auto out4 = masked.arrange(true, false);
std::cout << out4;
```

### Output

```text
0  NULL
1  20
2  30
3  50

ptype: Series
```

Example 5. `Note`-kind slates sort lexicographically.

```cpp
auto words = Slate(pluss::make_note({"banana", "apple", "cherry"}));
auto out5 = words.arrange();
std::cout << out5;
```

### Output

```text
0  apple
1  banana
2  cherry

ptype: Note
```

Example 6. The sort is stable — equal keys keep their original relative order.
Here two entries tie at `20.0`; the one that appeared first in the input
(originally at position 1) still precedes the one that appeared later
(originally at position 3) in the sorted result.

```cpp
auto ties = Slate(pluss::make_series({30.0, 20.0, 10.0, 20.0}));
auto out6 = ties.arrange();
std::cout << out6;
```

### Output

```text
0  10
1  20
2  20
3  30

ptype: Series
```

## Notes

- Always stable, across every payload kind.
- `NULL` entries are never compared against real values — they're grouped
  wholesale to one end per `nulls_last`.
- `size() <= 1` short-circuits to the slate unchanged.
- Backs both [`top()`](top.md) and [`bottom()`](bottom.md), which take a prefix of
  a descending or ascending `arrange()` call respectively.

## See Also

- [`Slate::top`](top.md) / [`Slate::bottom`](bottom.md): value-sorted previews
  built on this method.
- [`Slate::shuffle`](shuffle.md): reorder randomly instead of by value.
- [`Slate::take`](take.md): reorder (or subset) by explicit position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="append.html">
    <span class="label">Previous</span>
    <span class="title">append</span>
  </a>
  <a class="next-link" href="as_note.html">
    <span class="label">Next</span>
    <span class="title">as_note</span>
  </a>
</div>
