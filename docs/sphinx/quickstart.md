# Quickstart

## Includes

```cpp
#include <pluss/pluss.h>
```

## Build PlussQit examples

```bash
cmake --preset core-dev
cmake --build --preset core-dev
```

## Smoke run sequence

```bash
./build/PlussQit/plussqit_codem_series_usage_demo
./build/PlussQit/plussqit_codem_stats_demo
./build/PlussQit/plussqit_codem_signal_demo
./build/PlussQit/plussqit_coden_note_factory_demo
./build/PlussQit/plussqit_codeh_timeline_non_generated_and_factory_methods_demo
./build/PlussQit/plussqit_codeh_timeline_dataframe_timeindex_methods_demo
```

## Type map

- `Series`: numeric/bool tensor-style type (CodeM)
- `Note`: text/bytes tensor-style type (CodeN)
- `Timeline`: datetime tensor-style type (CodeH)
- `Duration`: time-delta tensor-style type (CodeH)
- `TimeIndex`: 1D datetime index operations (CodeH)
