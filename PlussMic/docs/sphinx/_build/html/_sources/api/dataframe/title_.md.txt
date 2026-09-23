<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d8f61f0089f0 -->

# DataFrame::title_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame& DataFrame::title_(std::string title)
```

## What It Does

The in-place counterpart to [`title(title)`](title.md): sets the frame's title
directly, mutating `*this` and returning a reference to it, instead of returning a
new titled frame. As with [`title()`](title.md)'s one-argument overload, this never
touches any value, dtype, feature order, or case label — only the title metadata
changes.

## Parameters

**title** (`std::string`)

The title text to attach to the current frame, in place.

## Returns

`DataFrame&`
    A reference to `*this`, now carrying `title` as its title.

## Examples

Example 1. Set a title in place, then read it back.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});

orders.title_("sales_report");
std::cout << orders.title() << "\n";
```

### Output

```text
sales_report
```

`orders` itself now carries the title — unlike [`title("sales_report")`](title.md),
which would have left `orders` untitled and returned a separate titled frame
instead.

## Notes

- `title_()` mutates and returns `*this` by reference; it never allocates a new
  `DataFrame` the way `title()`'s one-argument overload does.
- The title has no effect on the frame's own plain-text box-table rendering — see
  [`title()`](title.md)'s Example 2 for a direct demonstration.

## See Also

- [`DataFrame::title`](title.md): the non-mutating counterpart, plus the
  no-argument overload for reading the current title back.
- [`DataFrame::show`](show.md): where a frame's title becomes visible, as a
  GUI/HTML viewer window title.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="title.html">
    <span class="label">Previous</span>
    <span class="title">title</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
