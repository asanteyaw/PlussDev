<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0983f4ad2dab -->

# DataFrameFeatureAxisAccessor::timeline

[Back to DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)

## Signature

```cpp
DataFrameFeatureAxisAccessor::timeline() const -> DataFrame
```

## What It Does

Selects every feature whose `Slate` kind is `Timeline` **or** `TimeIndex` —
dropping numeric (`Series`) and text (`Note`) features. Case labels and row
order are preserved unchanged.

## Returns

`DataFrame`
    A new frame with only the `Timeline`/`TimeIndex`-kind features; same case
    labels and rows as the source.

## Examples

Example 1. Keep only the temporal feature out of a mixed frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales_units", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
     {"city",        Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
     {"visit_date",  Slate(pluss::make_timeline({
          "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"}))}});
std::cout << orders;

auto out = orders.feataxis().timeline();
std::cout << out;
```

### Output (before)

```text
======================================
     | sales_units  city    visit_date
--------------------------------------
0    | 100          Accra   2024-01-01
1    | 120          Tema    2024-01-02
2    | 90           Kumasi  2024-01-03
--------------------------------------
[3 cs x 3 feat]
```

### Output (after)

```text
=================
     | visit_date
-----------------
0    | 2024-01-01
1    | 2024-01-02
2    | 2024-01-03
-----------------
[3 cs x 1 feat]
```

## Notes

- Includes both `Timeline` and `TimeIndex` kinds — it does not distinguish
  between them.
- If no feature is temporal, the result has zero features but the same case
  labels and row count.

## See Also

- [`DataFrameFeatureAxisAccessor::series`](series.md),
  [`DataFrameFeatureAxisAccessor::note`](note.md): the analogous filters for
  the other two payload kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="suffix.html">
    <span class="label">Previous</span>
    <span class="title">suffix</span>
  </a>
</div>
