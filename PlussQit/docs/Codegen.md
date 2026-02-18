# Code generation

`SeriesMethodsGen.h` is generated (during CMake build) from:

- `src/CodeM/native/native_functions.yaml`
- `src/CodeM/core/OpsInternal.h` (extra method names)
- `tools/codegen/series_methods_seed.txt` (compatibility seed list)

Generator:

```bash
python3 tools/codegen/generate_series_methods_gen_h.py \
  --yaml src/CodeM/native/native_functions.yaml \
  --opsinternal src/CodeM/core/OpsInternal.h \
  --seed-list tools/codegen/series_methods_seed.txt \
  --out src/CodeM/generated/SeriesMethodsGen.h
```

How to add methods:

1. Add method schema/name in `native_functions.yaml` (preferred).
2. If it is a non-YAML extension method, add the name to the seed list or generator extras.
3. Route/implement it in `src/CodeM/compat/Invoke.cpp` (and CPU kernels where needed).
