<!-- pluss-hand-authored -->

# Nexus::to_string

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::to_string(const display::DisplayOptions& options = {}) const -> std::string
```

## What It Does

Renders a short, human-readable text summary of the workspace: a header, and one line per
table naming its key and shape (`ncases() x nfeatures()`). This is what
`operator<<(std::ostream&, const Nexus&)` uses, and what [`show()`](show.md) prints.

```{note} options is accepted but currently unused
The `display::DisplayOptions` parameter exists for signature consistency with
`to_string()` methods elsewhere in `PlussMic`, but `Nexus::to_string()` doesn't currently
read anything from it — the output is the same regardless of what's passed.
```

## Parameters

**options** (`const display::DisplayOptions&`, default `{}`)
    Accepted but currently unused — see the note above.

## Returns

`std::string`
    A multi-line summary: a two-line header (`"Nexus\n-----\n"`), a `"tables: N"` line,
    then one `"<key> -> [<rows> cs x <cols> feat]"` line per table.

## Examples

Example 1. A two-table workspace.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 2.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

std::cout << biz.to_string();
```

### Output

```text
Nexus
-----
tables: 2
customers -> [3 cs x 2 feat]
orders -> [3 cs x 2 feat]
```

## Notes

- `options` is currently a no-op — see the note above.
- Summarizes shape only — no cell values, no declared metadata, no relationships. See
  [`schema()`](schema.md)/[`graph()`](graph.md) for those.
- `std::cout << nexus` (via `operator<<`) and [`show()`](show.md) both produce exactly
  this same text.
- `pluss::bundle::output_render(std::ostream& os, const Nexus& nexus)` is a third,
  free-function spelling of the exact same one-line forward — `output_render(os, nexus)`,
  `os << nexus`, and `nexus.show(os)` are all interchangeable. `output_render` exists to
  satisfy generic/templated printing code that looks for an `output_render(stream, value)`
  customization point rather than assuming `operator<<` is available.

## See Also

- [`Nexus::show`](show.md): print this same text directly to a stream.
- [`Nexus::schema`](schema.md) / [`graph`](graph.md): far more detailed structured
  summaries, as `DataFrame`s.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_atlas.html">
    <span class="label">Previous</span>
    <span class="title">to_atlas</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
