# Part XV: API Map

This part maps tasks to API areas. It is not a replacement for the API
reference; it tells you where to look.

## DataFrame API areas

| Task | Methods |
| --- | --- |
| Load | `load`, `load_csv`, `load_txt`, `load_excel`, `load_json`, `load_parquet` |
| Save | `save_csv`, `save_txt`, `save_excel`, `save_json`, `save_parquet`, `save_as` |
| Inspect | `defined`, `ncases`, `nfeatures`, `feature_names`, `info`, `describe`, `to_string` |
| Select features | `features`, `select`, `select_like`, `select_regex`, `series_features`, `note_features`, `timeline_features` |
| Manage features | `insert`, `drop`, `reorder`, `rename`, `set_axis`, `add_prefix`, `add_suffix` |
| Manage cases | `cases`, `cases_series`, `set_cases`, `reset_cases`, `align_cases` |
| Row selection | `head`, `tail`, `view`, `cloc`, `slice`, `take`, `sample` |
| Filter | `where`, `filter`, `query` |
| Derive | `assign`, `set_features`, `set` |
| Transform | `transform`, `cast`, `astype` |
| Sort/rank | `sort_values`, `sort_cases`, `top`, `bottom` |
| Missing data | `isnull`, `notnull`, `fillna`, `dropna`, `ffill`, `bfill` |
| Arithmetic | `add`, `sub`, `mul`, `div`, `truediv`, `floordiv`, `mod`, `pow`, reverse variants |
| Comparison | `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `compare` |
| Reductions | `sum`, `mean`, `median`, `min`, `max`, `std`, `var`, `count`, `corr`, `ndistinct` |
| Grouping | `aggregate`, `group_by`, `count_by`, `mean_by`, `pivot`, `pivot_table` |
| Time | `tl`, `ts`, `resample`, `rolling`, `window`, `tz_convert`, `tz_localize`, `to_utc`, `floor_time`, `ceil_time`, `round`, `shift` |
| Combine | `join`, `update`, `combine`, `unify`, `conform_to` |
| Plot | `plot`, `plot3d` |
| SQL | `sql`, `prepare_sql` |
| Convert | `kindelize`, `tabelize`, `unsqueeze` |

See [DataFrame API Reference](../dataframe_api_reference.md).

## Thread API areas

| Task | Methods |
| --- | --- |
| Inspect | `defined`, `size`, `kind`, `dtype_name`, `info`, `to_string` |
| Missing | `isnull`, `notnull`, `isnan`, `isfinite`, `fillna`, `dropna`, `ffill`, `bfill` |
| Row operations | `head`, `tail`, `view`, `shift`, `slice`, `take`, `filter`, `sort_values`, `distinct` |
| Numeric | `abs`, `clip`, `log`, `exp`, reductions |
| Text | `lower`, `replace`, `contains` |
| Time | `year`, `month`, `day`, `hour`, `to_note_weekday`, `ts` |
| Compare | `compare`, `btw`, `compare_rows` |
| Cast | `cast`, `astype` |
| Payload | `as_series`, `as_note`, `as_timeline`, `as_timeindex` |
| Display | `show`, `with_display_labels`, `with_display_name` |

See [Thread API Reference](../thread_api_reference.md).

## Loader API areas

| Task | APIs |
| --- | --- |
| CSV options | `ReadCsvOptions`, `WriteCsvOptions` |
| Excel options | `ReadExcelOptions`, `WriteExcelOptions` |
| Raw ingest | `read_csv_raw_batch`, `read_excel_raw_batch`, `read_json_raw_text` |
| Neutral ingest | `read_csv_batch`, `read_excel_batch` |
| DataFrame load | `load_csv`, `load_txt`, `load_excel`, `load_json`, `load_parquet`, `load` |
| Save | `save_csv`, `save_txt`, `save_excel`, `save_json`, `save_parquet`, `save_as` |
| Workbook | `LoadResult`, `Workbook`, `SheetNames` |

## Atlas API areas

| Task | Methods |
| --- | --- |
| Inspect | `defined`, `empty`, `depth`, `ncases`, `nfeats`, `column_names` |
| Access | `frame`, `dloc`, `loc`, `frames`, `axis0_labels` |
| Slice depth | `head`, `tail`, `slice_frames`, `sort_depth` |
| Align depth | `relabel_depth`, `align_depth` |
| Transform | `map_frames`, `aggregate_over_frames` |
| Convert | `kindelize`, `tabelize`, `squeeze` |
| Plot | `plot` |

See [Atlas API Reference](../atlas_api_reference.md).

## Kindle API areas

| Type | Reference |
| --- | --- |
| `Stream` | [Stream API Reference](../stream_api_reference.md) |
| `Catalog` | [Catalog API Reference](../catalog_api_reference.md) |
| `Archive` | [Archive API Reference](../archive_api_reference.md) |

## Bundle API areas

| Type | Reference |
| --- | --- |
| `Slot` | [Slot API Reference](../slot_api_reference.md) |
| `Registry` | [Registry API Reference](../registry_api_reference.md) |
| `Aisle` | [Aisle API Reference](../aisle_api_reference.md) |

## Example source map

| Example | What it teaches |
| --- | --- |
| `dataframe_initial_demo.cpp` | loading, display, `head`, `tail`, `info`, `describe`, Atlas basics |
| `dataframe_versatile_methods_demo.cpp` | query, transform, aggregate, arithmetic, missing data, casts, rolling |
| `dataframe_plot_demo.cpp` | DataFrame plot families |
| `time_series_accessor_demo.cpp` | time-series accessors |
| `atlas_plot_demo.cpp` | Atlas plotting |
| `loader_saver_parity_demo.cpp` | load/save round trips |
| `kindle_model_demo.cpp` | Kindle model |
| `bundle_model_demo.cpp` | Bundle model |
