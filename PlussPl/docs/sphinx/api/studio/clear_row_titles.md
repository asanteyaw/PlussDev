<!-- pluss-hand-authored -->

# Studio::clear_row_titles

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
Studio& Studio::clear_row_titles() noexcept
```

## What It Does

Removes **every** row title at once — a `clear()` on the underlying map. Calling it when
no row titles are set at all is a harmless no-op.

## Parameters

*(none)*

## Returns

`Studio&`
    A reference to the same figure, for chaining.

## Examples

Example 1. Two rows titled, then all of them cleared in one call.

```cpp
using namespace pluss::pl;

auto fig = studio("row_titles_cleared_all");
fig.mosaic(2, 1);
fig.row_title(0, "Q1 Results").row_title(1, "Q2 Results");
fig.clear_row_titles();
auto& top = fig.pane2("top");
flash::lineplot(top, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0}).label("series a");
auto& bottom = fig.pane2("bottom");
flash::lineplot(bottom, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{3.0, 5.0, 4.0, 6.0, 7.0}).label("series b");
fig.show();

std::cout << fig.row_titles().empty() << "\n";
```

### Output

```text
1
```

![Output of Example 1](img/row_title-4-cleared-all.svg)

## Notes

- `noexcept`.
- No-op when no titles are currently set.
- Compare against [`clear_row_title(row)`](clear_row_title.md), which removes only one
  row's title.

## See Also

- [`Studio::row_title`](row_title.md): sets the titles this clears.
- [`Studio::clear_row_title`](clear_row_title.md): clears a single row's title only.
- [`Studio::row_titles`](row_titles.md): reads back what remains (always empty right after
  this call).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_row_title.html">
    <span class="label">Previous</span>
    <span class="title">clear_row_title</span>
  </a>
  <a class="next-link" href="pane2.html">
    <span class="label">Next</span>
    <span class="title">pane2</span>
  </a>
</div>
