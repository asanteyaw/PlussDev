<!-- pluss-hand-authored -->

# Studio::legend_dock

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
KeyDock Studio::legend_dock() const noexcept
```

## What It Does

Returns the figure's current consolidated-legend dock — whatever
[`legend(KeyDock)`](legend.md) last set it to, or `KeyDock::None` (the constructor
default, meaning no docked legend) if `legend()` was never called.

## Parameters

*(none)*

## Returns

`KeyDock`
    The current dock: `None`, `East`, or `South`.

## Examples

Example 1. The default, and after switching it on.

```cpp
using namespace pluss::pl;

auto fig = studio("legend_dock_check");
std::cout << static_cast<int>(fig.legend_dock()) << "\n";  // None == 0

fig.legend(KeyDock::East);
std::cout << static_cast<int>(fig.legend_dock()) << "\n";  // East == 1

fig.legend(KeyDock::South);
std::cout << static_cast<int>(fig.legend_dock()) << "\n";  // South == 2

auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0}).label("series");
fig.show();
```

### Output

```text
0
1
2
```

## Notes

- `noexcept`.
- Purely a readback of the value set by `legend(KeyDock)`; has no side effect on rendering
  itself.
- `KeyDock::None` is `0`, `East` is `1`, `South` is `2` when compared or printed via
  `static_cast<int>`.

## See Also

- [`Studio::legend`](legend.md): the only way to change this.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="legend.html">
    <span class="label">Previous</span>
    <span class="title">legend</span>
  </a>
  <a class="next-link" href="legend_frame.html">
    <span class="label">Next</span>
    <span class="title">legend_frame</span>
  </a>
</div>
