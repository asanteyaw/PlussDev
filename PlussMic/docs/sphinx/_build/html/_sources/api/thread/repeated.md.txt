<!-- pluss-hand-authored -->

# Thread::repeated

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::repeated() const -> Thread
```

## What It Does

Returns a boolean thread, the same length as the source, marking **every**
entry whose value occurs more than once — including the occurrence that
[`duplicated()`](duplicated.md) would leave unmarked as the "survivor." Where
`duplicated()` answers "which entries are extra copies to drop," `repeated()`
answers "which entries belong to a repeated group at all." For values `[A, B,
A, C, B]`, `repeated()` marks `[true, true, true, false, true]` — both `A`s and
both `B`s are `true`, `C` (unique) is `false` — whereas `duplicated()` with the
default policy would mark only the second `A` and second `B`
(`[false, false, true, false, true]`).

## Parameters

`None`

## Returns

`Thread`
    A boolean `Series`, same length as the source, `true` at every position
    whose value occurs more than once anywhere in the thread.

## Examples

Example 1. Mark every member of a repeated value's group.

```cpp
using namespace pluss::table;

auto cities = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"}));
auto out = cities.repeated();
std::cout << out;
```

### Output

```text
0  true
1  true
2  true
3  false
4  true

ptype: Series
```

Both `"Accra"` occurrences (positions `0`, `2`) and both `"Tema"` occurrences
(positions `1`, `4`) are `true`. Only `"Kumasi"` (unique) is `false`.

## Notes

- Grouping is by each value's string form, matching
  [`distinct()`](distinct.md)/[`duplicated()`](duplicated.md)/[`freq()`](freq.md);
  works across every payload kind.
- Marks the whole repeated group, unlike `duplicated()`, which leaves one
  designated survivor per group unmarked.

## See Also

- [`Thread::duplicated`](duplicated.md): mark only the *extra* occurrences
  under a keep-first/keep-last policy, leaving one survivor per group unmarked.
- [`Thread::distinct`](distinct.md): the deduplicated values themselves.
- [`Thread::filter`](filter.md): apply the mask this method returns to
  actually select the repeated rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rdiv.html">
    <span class="label">Previous</span>
    <span class="title">rdiv</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
