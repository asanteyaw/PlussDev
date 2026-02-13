# Patch 59: Torch method surface imported (YAML → generated Series wrappers)

This patch reads Torch's `native_functions.yaml` (from `aten.zip`) and generates a large method-surface for `pluss::codem::Series`.

## Coverage report (method names)

- Torch YAML method names (filtered to valid C++ identifiers, excluding `__*__`): **556**
- Methods already present manually in `Series.h`: **77**
- Newly generated wrappers in `SeriesTorchMethods.gen.h`: **496**
- **Name-surface coverage:** **103.06%**

> Note: these are method *names*. Many are forwarded to `torch_compat::invoke` and may throw `NotImplemented`
> until the corresponding kernels are implemented. This patch is about accelerating API surface first.
