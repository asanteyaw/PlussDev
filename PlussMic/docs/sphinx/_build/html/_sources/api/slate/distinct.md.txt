<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f3b5ffd0411 -->

# Slate::distinct

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::distinct(bool dropnull = true) const -> Slate
```

## What It Does

Returns the distinct values of the slate, one representative per value, in
**first-seen order** — not sorted. Grouping is done by each value's string form
(the same conversion [`freq()`](freq.md) uses), so this works on any payload
kind, not just `Note`. Unlike a sort-based dedup, the surviving representative
for each value is always the earliest occurrence, and the relative order of
distinct values in the output matches the order they first appeared in the
source slate.

## Parameters

**dropnull** (`bool`, default `true`)
    - `true` (the default): missing entries are excluded entirely from the
      result.
    - `false`: at most one missing entry is kept, appearing at the position of
      its first occurrence among the survivors.

## Returns

`Slate`
    A new slate holding one representative of each distinct value, in
    first-seen order. Shorter than (or equal in length to) the source slate.

## Examples

Example 1. Collect distinct values in first-seen order.

```cpp
using namespace pluss::table;

auto cities = Slate(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi"}));
auto out = cities.distinct();
std::cout << out;
```

### Output

```text
0  Accra
1  Tema
2  Kumasi

ptype: Note
```

`"Accra"`'s repeat at position `2` is dropped, but the survivors keep the order
they were first seen in — `"Tema"` before `"Kumasi"`, not alphabetical order.

Example 2. `dropnull = false` keeps a single representative missing entry.

```cpp
auto with_gap = Slate(
    pluss::make_note({"Accra", "", "Tema", ""}),
    pluss::make_series(std::vector<bool>{true, false, true, false})
);
auto out2 = with_gap.distinct(false);
std::cout << out2;
```

### Output

```text
0  Accra
1  NULL
2  Tema

ptype: Note
```

Only one `NULL` survives (from its first occurrence at position `1`), even
though the source had two missing entries.

## Notes

- Order is first-seen, never sorted — see [`arrange()`](arrange.md) if
  you want the distinct values in value order instead.
- `dropnull = false` keeps at most one missing entry, not one per occurrence.
- Grouping is by each value's string form, so this works across every payload
  kind.

## See Also

- [`Slate::ndistinct`](ndistinct.md): the count of distinct values, without
  the values themselves.
- [`Slate::freq`](freq.md): distinct values paired with their occurrence
  counts, sorted by frequency instead of first-seen order.
- [`Slate::duplicated`](duplicated.md) / [`Slate::repeated`](repeated.md):
  boolean masks over the original slate's positions, rather than a
  deduplicated slate.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="describe.html">
    <span class="label">Previous</span>
    <span class="title">describe</span>
  </a>
  <a class="next-link" href="div.html">
    <span class="label">Next</span>
    <span class="title">div</span>
  </a>
</div>
