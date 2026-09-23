<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 870ea23630f1 -->

# DataFrameLabelAccessor::values

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::values() const -> Thread
```

## What It Does

Returns the frame's flat case labels as a `Thread` — the same composite,
displayed value shown in the leftmost column of the frame's own printed
table, whether the frame is structured (multi-level), a plain custom axis, or
just the default integer index.

## Returns

`Thread`
    The case labels, as a bare `Thread`.

## Examples

Example 1. Read the case labels back out after promoting a feature to them.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

auto out = labeled.label().values();
std::cout << out;
```

### Output

```text
0  east
1  west
2  east
3  north
ptype: Note
```

This is exactly the same sequence shown as `labeled`'s own case-label column
when printing `labeled` directly — `values()` just gives it to you as a
standalone `Thread`.

## Notes

- For a structured (multi-level) frame, this returns the **joined composite**
  string, not any individual level — use [`part(name)`](part.md) or
  [`part(position)`](part.md) for a single level's own values.
- Always defined: every `DataFrame` has case labels of some kind, even if
  they're just the default integer index.

## See Also

- [`DataFrameLabelAccessor::part`](part.md): a single structured level's own
  values, rather than the full composite.
- [`DataFrameLabelAccessor::has`](has.md): whether the case labels are
  structured at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unite.html">
    <span class="label">Previous</span>
    <span class="title">unite</span>
  </a>
</div>
