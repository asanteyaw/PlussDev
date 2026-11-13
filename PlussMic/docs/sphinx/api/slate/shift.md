<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f15f0fed7f8d -->

# Slate::shift

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::shift(std::int64_t periods = 1) const
```

## What It Does

Shifts every value forward (`periods > 0`) or backward (`periods < 0`) by
`periods` positions, keeping the slate the same length — positions that shift in
from beyond either edge become `NULL`, and values shifted past the opposite edge
are dropped entirely (not wrapped around). `periods = 0` (or an empty slate) is a
no-op, returning `*this` unchanged.

## Parameters

**periods** (`std::int64_t`, default `1`)
    Number of positions to shift. Positive shifts values to later positions
    (introducing `NULL`s at the start); negative shifts values to earlier positions
    (introducing `NULL`s at the end).

## Returns

`Slate`
    A new slate, same length as `*this`, with values shifted and the vacated
    positions set to `NULL`.

## Examples

Example 1. A positive shift moves values later; the leading positions become
`NULL`.

```cpp
using namespace pluss::table;

auto readings = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto out = readings.shift(1);
std::cout << out;
```

### Output

```text
0  NULL
1  10
2  20
3  30

ptype: Series
```

`40.0` (the last value) is dropped entirely — shifted past the end, not wrapped
back to the front.

Example 2. A negative shift moves values earlier; the trailing positions become
`NULL`.

```cpp
auto out2 = readings.shift(-2);
std::cout << out2;
```

### Output

```text
0  30
1  40
2  NULL
3  NULL

ptype: Series
```

Example 3. `periods = 0` is a no-op.

```cpp
auto out3 = readings.shift(0);
std::cout << out3;
```

### Output

```text
0  10
1  20
2  30
3  40

ptype: Series
```

Example 4. A cell that was already `NULL` before the shift stays `NULL` after
moving to its new position.

```cpp
auto with_null = Slate(pluss::make_series({1.0, 2.0, 3.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
auto out4 = with_null.shift(1);
std::cout << out4;
```

### Output

```text
0  NULL
1  1
2  NULL

ptype: Series
```

Row `2`'s `NULL` comes from row `1`'s already-`NULL` value moving into that
position, not from the shift itself vacating it.

## Notes

- Values shifted past either edge are dropped, not wrapped around.
- Vacated positions (and any already-`NULL` positions that moved) are `NULL` in
  the result.
- `periods = 0`, or shifting an empty slate, returns `*this` unchanged.
- Works on every payload kind, not just `Series`.

## See Also

- [`Slate::ffill`](ffill.md) / [`Slate::bfill`](bfill.md): fill the `NULL`s a
  shift introduces, if that's the goal instead.
- [`Slate::slice`](slice.md) / [`Slate::take`](take.md): drop or reorder entries
  without introducing `NULL`s.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
