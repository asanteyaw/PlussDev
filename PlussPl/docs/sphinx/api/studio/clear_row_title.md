<!-- pluss-hand-authored -->

# Studio::clear_row_title

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
Studio& Studio::clear_row_title(std::int64_t row) noexcept
```

## What It Does

Removes the title for one specific row, if one is set — an `erase(row)` on the underlying
map. Clearing a `row` that has no title (never set, already cleared, or simply out of
range) is a harmless no-op; unlike [`row_title()`](row_title.md), there's no validation and
nothing to throw.

## Parameters

**row** (`std::int64_t`)
    The 0-indexed row whose title should be removed. Any value is accepted, including
    negative ones — a negative `row` simply never has an entry to erase.

## Returns

`Studio&`
    A reference to the same figure, for chaining.

## Examples

Example 1. Two rows titled, then row `0`'s title cleared — row `1`'s survives untouched.

```cpp
using namespace pluss::pl;

auto fig = studio("row_titles_cleared_one");
fig.mosaic(2, 1);
fig.row_title(0, "Q1 Results").row_title(1, "Q2 Results");
fig.clear_row_title(0);
auto& top = fig.pane2("top");
top.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0}).label("series a");
auto& bottom = fig.pane2("bottom");
bottom.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{3.0, 5.0, 4.0, 6.0, 7.0}).label("series b");
fig.show();

std::cout << fig.row_titles().size() << "\n";
std::cout << fig.row_titles().count(0) << " " << fig.row_titles().count(1) << "\n";
```

### Output

```text
1
0 1
```

![Output of Example 1](img/row_title-3-cleared-one.svg)

Example 2. Clearing a row that was never set — a harmless no-op, no exception.

```cpp
using namespace pluss::pl;

auto fig = studio("clear_never_set");
fig.clear_row_title(5);  // no-op: row 5 was never titled
std::cout << fig.row_titles().empty() << "\n";

auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0}).label("series");
fig.show();
```

### Output

```text
1
```

![Output of Example 2](img/clear_row_title-2-never-set.svg)

## Notes

- `noexcept`.
- No-op (not an error) when `row` has no current title — including negative or
  out-of-range values.
- Leaves every other row's title untouched.

## See Also

- [`Studio::row_title`](row_title.md): sets the titles this clears.
- [`Studio::clear_row_titles`](clear_row_titles.md): clears every row title at once.
- [`Studio::row_titles`](row_titles.md): reads back what remains.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="row_titles.html">
    <span class="label">Previous</span>
    <span class="title">row_titles</span>
  </a>
  <a class="next-link" href="clear_row_titles.html">
    <span class="label">Next</span>
    <span class="title">clear_row_titles</span>
  </a>
</div>
