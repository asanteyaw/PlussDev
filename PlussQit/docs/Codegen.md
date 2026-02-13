# Code generation

PlussQit keeps the "Torch-aligned" method surface in checked-in `.inc` files.

To refresh from upstream Torch YAML:

```bash
python3 tools/codegen/generate_series_methods.py \
  --yaml path/to/native_functions.yaml \
  --out-methods src/CodeM/core/SeriesTorchAlignedMethods.inc \
  --out-stubs   src/CodeM/core/SeriesTorchAlignedStubs.inc
```

Stubs are compiled from `src/CodeM/core/SeriesTorchAlignedStubs.cpp`.
When you implement an op, remove it from the stub file (or add a generation filter).
