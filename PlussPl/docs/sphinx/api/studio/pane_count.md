<!-- pluss-hand-authored -->

# Studio::pane_count

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
[[nodiscard]] std::size_t Studio::pane_count() const noexcept
```

## What It Does

Returns the number of panes currently in the figure — equivalent to `panes().size()`, but
without needing to touch the pane list itself. Includes subpanes created via
[`subpane2()`](subpane2.md)/[`subpane3()`](subpane3.md), which are appended to the same
flat list as top-level [`pane2()`](pane2.md)/[`pane3()`](pane3.md) panes.

## Parameters

*(none)*

## Returns

`std::size_t`
    The number of panes in the figure. `0` for a freshly constructed `Studio`.

## Examples

Example 1. Tracking `pane_count()` as panes and subpanes are added.

```cpp
using namespace pluss::pl;

auto fig = studio("pane_count_check");
std::cout << fig.pane_count() << "\n";

fig.pane2("a");
std::cout << fig.pane_count() << "\n";

fig.pane3("b");
std::cout << fig.pane_count() << "\n";

fig.subpane2(0, 1, 2, 0, 0);
std::cout << fig.pane_count() << "\n";

std::cout << (fig.pane_count() == fig.panes().size()) << "\n";
```

### Output

```text
0
1
2
3
1
```

## Notes

- `noexcept`, `const`.
- Always equal to `panes().size()` — a cheaper way to ask "how many?" without needing the
  elements.
- Counts subpanes as ordinary entries in the same flat list; there's no separate count for
  "top-level panes only."
- There is no public API to reduce this count once panes are created — it only ever grows
  across a figure's lifetime.

## See Also

- [`Studio::panes`](panes.md): the full list this counts.
- [`Studio::pane_index`](pane_index.md): a pane's position, bounded by `[0, pane_count())`.
- [`Studio::subpane2`](subpane2.md) / [`subpane3`](subpane3.md): whose `parent_index`
  parameter must be `< pane_count()` at call time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="panes.html">
    <span class="label">Previous</span>
    <span class="title">panes</span>
  </a>
</div>
