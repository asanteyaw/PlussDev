# PlussII 0.0.6a — Transient

The latest validated development build. It carries fixes and features that are not yet in a Steady release, so expect it to change from one update to the next. The next Steady release, 0.0.6, will collect what proves stable here.

## Changes since 0.0.5

- The LibTorch bridge (`pluss::series::from_torch` and `Series::tensor()`) now compiles with any include order. `<pluss/pluss.h>` no longer has to come after `<torch/torch.h>`.
- Apple Silicon is published twice: a CPU-only archive (`Darwin-arm64`) and one with the Apple GPU (MPS) backend (`Darwin-arm64-mps`). Intel Mac, Linux and Windows archives are CPU-only.
- Training errors from the tagger and entity-recognition stages no longer name an internal function.
- The PlussView test hook `PLUSSTILS_VIEW_NSEVENTS` posts real mouse events so the hover, drag and double-click paths can be tested without a person at the mouse.

## Renamed methods

Public method and function names in PlussMic now have at most two words. Update call sites as follows.

| Type | 0.0.5 | 0.0.6a |
| --- | --- | --- |
| `pluss::series` | `select(conditions, choices, default)` | `choose(conditions, choices, default)` |
| `pluss::note` | `select(conditions, choices, default)` | `choose(conditions, choices, default)` |
| `pluss::timeline` | `select(conditions, choices, default)` | `choose(conditions, choices, default)` |
| `Shelf`, `Stream` | `has_validity_mask`, `with_validity_mask`, `clear_validity_mask` | `has_mask`, `with_mask`, `clear_mask` |
| `Shelf` | `has_display_labels`, `with_display_labels`, `clear_display_labels` | `has_labels`, `with_labels`, `clear_labels` |
| `Nexus`, `Locus` | `has_table_meta` | `has_meta` |
| `Nexus`, `Locus` | `set_table_meta`, `set_table_meta_` | `meta(key, value)`, `meta_(key, value)` |
| `Locus` | `alignment_cache_stats` | `cache_stats` |
| `Slot`, `Registry` | `is_stream_compatible`, `ensure_stream_compatible` | `streamable`, `ensure_streamable` |
| `ConfigValue` | `is_string_list`, `as_string_list` | `is_list`, `as_list` |
| `LoadResult` | `sheet_by_index` | `sheet` |
| `pluss::table::display` | `set_default_backend`, `set_default_theme`, `gui_backend_available`, `render_info_html` | `default_backend(backend)`, `default_theme(theme)`, `gui_available`, `info_html` |

## Compatibility

Unchanged from 0.0.5. The distribution requires CMake 3.28 or newer and a C++20 compiler. See the download page for the archive for your system.
