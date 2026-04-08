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
./build/PlussQit/plussqit_queue_demo
./build/PlussQit/plussqit_dispense_demo
./build/PlussQit/plussqit_codem_transmit_demo
./build/PlussQit/plussqit_codem_aux_let_transmit_demo
./build/PlussQit/plussqit_codem_output_engine_demo
./build/PlussQit/plussqit_codem_suppose_demo
./build/PlussQit/plussqit_translate_demo
./build/PlussQit/plussqit_codem_autograd_optim_demo
./build/PlussQit/plussqit_codem_autodiff_derivatives_demo
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
- `Type`: runtime union over `Series` / `Note` / `Timeline` / `TimeIndex`
- `pluss::subsp::math::diff`: gradient/Jacobian/JVP/VJP/HVP transform layer
