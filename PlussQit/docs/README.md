# PlussQit Documentation Sources

End-user API documentation lives under `sphinx/api/` as handwritten,
per-operation chapters. The root-level Markdown files retained here are
internal authoring standards, engineering records, and migration notes; Sphinx
excludes them from the published site.

- Parity roadmap: `ParityRoadmap.md`
- Method surface codegen: `Codegen_MethodSurface.md`
- Patch 54 method surface: `Patch54_MethodSurface.md`
- Patch 55: indexing+mask+logical: `Patch55_IndexingAndMasks.md`
- Patch 56: scatter_reduce initial + indexing fixes: `Patch56_ScatterReduceInitialAndIndexingFixes.md`
- Factories and public surface: `Factories.md`
- Patch 59: Torch method surface generation: `Patch59_TorchMethodSurface_Generated.md`
- Patch 60: Torch-compat invoke_any: `Patch60_TorchCompat_InvokeAny.md`
- Patch 61: mapping expansion: `Patch61_MethodsBackstop_MappingExpansion.md`
- Patch 62: more reference kernels: `Patch62_ReferenceKernels_MoreOps.md`
- Patch 63: sort/topk: `Patch63_SortTopk_WhereFix.md`
- Patch 64: factories + unique/histc/bincount: `Patch64_Factories_Overloads_UniqueHistcBincount.md`
- Patch 65: factory dtype policy + where/nonzero: `Patch65_FactoryPolicy_WhereNonzero_GatherScatter.md`
- Patch 66: no NotImplemented throws + CMake 3.25: `Patch66_NoNotImplemented_Fallbacks_CMake325.md`
