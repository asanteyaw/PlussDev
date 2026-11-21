<!-- pluss-hand-authored -->

# flash::TableColumns and flash::ColumnSelection

[Back to flash API Reference](../../flash_api_reference)

## Signatures

```cpp
flash::TableColumns
flash::ColumnSelection
```

## What They Are

Every flash function that plots a `DataFrame`, an `Atlas` or a `Shelf`-backed table takes a
`flash::TableColumns` that says which columns to draw. flash never picks columns from the shape of a
frame: a function called with no column named and no selection throws a `pluss::Error` that lists the
ways to name them.

`TableColumns` names the roles of a plot:

| Field | Type | Role |
|---|---|---|
| `feature` | `std::optional<std::string>` | the single column of a one-column plot (`histplot`, `kdeplot`, ...) |
| `x_feature`, `y_feature`, `z_feature` | `std::optional<std::string>` | the x, y and z columns of 2D and 3D plots |
| `hue_feature`, `label_feature`, `class_feature`, `err_feature` | `std::optional<std::string>` | grouping, labelling and error columns |
| `selection` | `ColumnSelection` | which columns a column-per-trace or column-set plot draws |

`ColumnSelection` picks columns without naming each role:

| Field | Type | Meaning |
|---|---|---|
| `all_columns` | `bool` | every numeric column |
| `include_columns` | `std::vector<std::string>` | exactly these columns, in this order |
| `exclude_columns` | `std::vector<std::string>` | every numeric column except these |

`include_columns` and `exclude_columns` cannot both be set, and `all_columns` cannot be combined with
either. An unknown name, a non-numeric column in `include_columns`, and the x column listed in
`include_columns` are errors; each message says what was wrong and what to do.

## Rules

- **The x-axis is never guessed.** It is `x_feature` when given; otherwise the row positions.
- **Fixed-arity plots take names only.** `scatterplot` pairs, `relplot`, `jointplot`, the 3D plots and
  the like need `x_feature`/`y_feature`(/`z_feature`) and reject a `selection`.
- **One trace per column, one pane.** `lineplot`, `stepplot`, `areaplot` and `scatterplot` with a
  selection and no `y_feature` draw one trace per selected column against `x_feature` or the row
  positions, all on the same pane, labelled by column name.
- **One distribution per column.** `histplot`, `kdeplot`, `ecdfplot` and `rugplot` draw one
  distribution per selected column on the same pane; `boxplot`, `violinplot`, `stripplot` and
  `swarmplot` draw one group per selected column.
- **Column sets.** `heatmap`, `clustermap`, `pairplot`, `triplot`, `summaryplot`, `parallelplot`,
  `andrews_curves` and `radviz` need a selection (or, for a tall heatmap, named x, y and z columns).

## Examples

```cpp
using namespace pluss::pl;

auto frame = DataFrame({
    {"day",   Shelf(pluss::make_series({1.0, 2.0, 3.0}))},
    {"sales", Shelf(pluss::make_series({10.0, 12.5, 11.0}))},
    {"cost",  Shelf(pluss::make_series({6.0, 7.5, 7.0}))},
});

Studio fig("columns");
auto& pane = fig.pane2("sales");
flash::lineplot(pane, frame, {.x_feature = "day", .y_feature = "sales"});       // one named pair
flash::lineplot(pane, frame, {.x_feature = "day", .selection = {.all_columns = true}});   // sales and cost against day
flash::lineplot(pane, frame, {});   // throws: no columns named
```

Misuse examples and their messages are shown on
[`DataFramePlotAccessor::line`](../../../../PlussMic/docs/sphinx/api/dataframe-plot/line.md), which speaks the
same rules in PlussMic's vocabulary (a column is a feature there).
