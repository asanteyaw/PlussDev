# Part XV: API Map

This part maps tasks to API areas. It is not a replacement for the API
reference; it tells you where to look.

## DataFrame API areas

| Task | Methods |
| --- | --- |
| Load | `load`, `load_csv`, `load_txt`, `load_excel`, `load_json`, `load_parquet` |
| Save | `save_csv`, `save_txt`, `save_excel`, `save_json`, `save_parquet`, `save_as` |
| Inspect | `defined`, `ncases`, `nfeatures`, `features`, `info`, `describe`, `to_string` |
| Select features | `features`, `feataxis().like`, `feataxis().match`, `feataxis().series`, `feataxis().note`, `feataxis().timeline` |
| Manage features | `insert`, `drop`, `reorder`, `rename`, `feataxis().prefix`, `feataxis().suffix` |
| Manage case labels | `label().values`, `label().set`, `label().reset`, `label().align` |
| Row selection | `head`, `tail`, `view`, `cloc`, `slice`, `take`, `sample` |
| Filter | `where`, `filter`, `query` |
| Derive | `assign`, `set`, `set` |
| Transform | `transform`, `cast`, `astype` |
| Sort/rank | `sort_values`, `label().sort`, `top`, `bottom` |
| Missing data | `isnull`, `notnull`, `fillnull`, `dropnull`, `ffill`, `bfill` |
| Arithmetic | `add`, `sub`, `mul`, `div`, `truediv`, `floordiv`, `mod`, `pow`, reverse variants |
| Comparison | `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `compare` |
| Reductions | `sum`, `mean`, `median`, `min`, `max`, `std`, `var`, `count`, `corr`, `ndistinct` |
| Grouping | `aggregate`, `group`, `pivot`, `pivot_table` |
| Time | `tl`, `ts`, `tl().localize`, `tl().tz`, `tl().utc`, `tl().floor`, `tl().ceil`, `tl().round`, `tl().shift`, `ts().resample`, `ts().rolling`, `ts().window`, `ts().weigh`, `ts().bounded` |
| Combine | `join`, `update`, `combine`, `unify`, `conform` |
| Plot | `plot`, `plot3d` |
| SQL | `sql`, `prepare_sql` |
| Convert | `kindelize`, `tabelize`, `unsqueeze` |

See [DataFrame API Reference](../dataframe_api_reference.md).

## Slate API areas

| Task | Methods |
| --- | --- |
| Inspect | `defined`, `size`, `kind`, `dtype`, `info`, `to_string` |
| Missing | `isnull`, `notnull`, `isnan`, `isfinite`, `fillnull`, `dropnull`, `ffill`, `bfill` |
| Row operations | `head`, `tail`, `view`, `shift`, `slice`, `take`, `filter`, `arrange`, `distinct` |
| Numeric | `abs`, `clip`, `log`, `exp`, reductions |
| Text | `lower`, `replace`, `contains` |
| Time | `tl`, `tl().year`, `tl().month`, `tl().day`, `tl().hour`, `tl().weekday_name`, `ts` |
| Compare | `compare`, `between` |
| Cast | `cast`, `astype` |
| Payload | `as_series`, `as_note`, `as_timeline`, `as_timeindex` |
| Display | `show`, `with_display_labels`, `name`, `named`, `unname` |

See [Slate API Reference](../slate_api_reference.md).

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
| Inspect | `defined`, `empty`, `depth`, `ncases`, `nfeatures`, `column_names` |
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

## Scribble API areas

| Type | Reference |
| --- | --- |
| `Slot` | [Slot API Reference](../slot_api_reference.md) |
| `Registry` | [Registry API Reference](../registry_api_reference.md) |
| `Axel` | [Axel API Reference](../axel_api_reference.md) |

## Bundle API areas

| Type | Reference |
| --- | --- |
| `Locus` | [Locus API Reference](../locus_api_reference.md) |
| `LocusPull` | [LocusPull API Reference](../locus_pull_api_reference.md) |
| `LocusQuery` | [LocusQuery API Reference](../locus_query_api_reference.md) |
| `LocusQueryPrepared` | [LocusQueryPrepared API Reference](../locus_query_prepared_api_reference.md) |
| `LocusTraverse` | [LocusTraverse API Reference](../locus_traverse_api_reference.md) |
| `Nexus` | [Nexus API Reference](../nexus_api_reference.md) |
| `NexusTraverse` | [NexusTraverse API Reference](../nexus_traverse_api_reference.md) |
| `NexusQuery` | [NexusQuery API Reference](../nexus_query_api_reference.md) |

## Example source map

| Example | What it shows |
| --- | --- |
| `dataframe_initial_demo.cpp` | loading, display, `head`, `tail`, `info`, `describe`, Atlas basics |
| `dataframe_versatile_methods_demo.cpp` | query, transform, aggregate, arithmetic, missing data, casts, rolling |
| `dataframe_plot_demo.cpp` | DataFrame plot families |
| `time_series_accessor_demo.cpp` | time-series accessors |
| `atlas_plot_demo.cpp` | Atlas plotting |
| `loader_saver_parity_demo.cpp` | load/save round trips |
| `kindle_model_demo.cpp` | Kindle model |
| `scribble_model_demo.cpp` | Scribble model |
