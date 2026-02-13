# CodeM patches

- Patch 01: `PlussQit_CodeM_next.zip` (TensorIterator coalescing + parallel_for, masked_select, matmul)
- Patch 02: `PlussQit_CodeM_patch_02.zip` (max 7D, gather/scatter)
- Patch 03: `PlussQit_CodeM_patch_03.zip` (full broadcasted masked_select, where/masked_fill/nonzero/take/put, clone)
- Patch 04: `PlussQit_CodeM_patch_04.zip` (reshape/view/expand, cat/stack, mean/amax/argmax, unary math pack)
- Patch 05: `PlussQit_CodeM_patch_05.zip` (promotion lattice v1 + scatter_add accumulate)
- Patch 06: `PlussQit_CodeM_patch_06.zip` (promotion v2 + index/index_put + put accumulate)
- Patch 07: `PlussQit_CodeM_patch_07.zip` (advanced indexing v2: partial indexing + values broadcasting; amin/argmin)
- Patch 08: `PlussQit_CodeM_patch_08.zip` (view ops: permute/transpose/squeeze/unsqueeze; comparisons; minimum/maximum/clamp)
- Patch 09: `PlussQit_CodeM_patch_09.zip` (Float16/BFloat16 storage + casts + factories + min/max/compare support)
- Patch 10: `PlussQit_CodeM_patch_10.zip` (arithmetic pack add/sub/mul/div/pow/abs/sign + scalar reductions sum/all/any)
- Patch 11: `PlussQit_CodeM_patch_11.zip` (uint16/32/64 + expanded casts + wider int ops + in-place add_/sub_/mul_/div_/clamp_/index_put_)
- Patch 12: `PlussQit_CodeM_patch_12.zip` (factory suite: arange/linspace/eye/rand/randn + CMake fix for TensorIterator.cpp + migrated core copies to TensorIterator)

- Patch 13: `PlussQit_CodeM_patch_13.zip` (Torch-like aliasing in-place across views + delete redundant iterator and rename TensorIterator→SeriesIterator + true in-place index_put_)

- Patch 14: `PlussQit_CodeM_patch_14.zip` (production longevity: storage external handle + overlap detection for in-place + LibTorch interop + pluss::Series alias)

- Patch 16: `PlussQit_CodeM_patch_16.zip` (MSharp SnapPtr intrusive refcount; debug view invariants; stricter interop invariants; index_put_ accumulate parity expansion; docs)

  - Added docs: `API_Philosophy.md`, `MSharp_SnapPtr.md`, `View_Invariants.md`.

- Patch 17: `PlussQit_CodeM_patch_17.zip` (accelerated surface area: unary math methods + _ inplace; cat/stack as methods; gather/scatter/scatter_add methods + inplace; fixed debug invariants to not reject general views; improved unary kernels for strides + float16/bfloat16)

- Patch 18: `PlussQit_CodeM_patch_18.zip` (major acceleration: operator overloads + compound assigns + stream printing; method expansion: matmul/where/masked/index_select/arg* reductions + index_put non-inplace; inplace arithmetic pairs add_/sub_/mul_/div_/neg_)

- Patch 19: `PlussQit_CodeM_patch_19.zip` (acceleration: scalar overloads + scalar operators; add nonzero/take/index/put_ methods; broaden operator set; docs updated)

- Patch 20: `PlussQit_CodeM_patch_20.zip` (acceleration: 28 new unary methods with inplace variants; renamed cat/stack -> unite/pile with deprecated aliases; added UnaryMore kernels with Float16/BFloat16; docs + tests)

- Patch 21: `PlussQit_CodeM_patch_21.zip` (Config options for factories + overloads; docs)

- Patch 22: `PlussQit_CodeM_patch_22.zip` (Config ergonomics: cfg tags + string-map config builder; added value/make_series/from_blob factories; PLUSS_API/MSHARP_API export macros; error handling upgraded with source_location + internal stack trace)

- Patch 23: `PlussQit_CodeM_patch_23.zip` (200+ method declarations added via SeriesMethods.inc; deduction overloads for make_series; new out-of-place put(); added guarded stubs for dim-reductions and many forward-compatible ops; docs updated)

- Patch 24: `PlussQit_CodeM_patch_24.zip` (generated 2000+ Torch native_functions.yaml-derived Series method signatures + inline stubs; method-first API preserved; prep for implementing kernels gradually)

- Patch 25: `PlussQit_CodeM_patch_25.zip` (renamed Torch alignment method files to remove 'Aten'; implemented log1p/expm1/erf/erfc/lgamma/asinh/acosh/atanh + isnan/isinf/isfinite kernels + Series methods; added tests)

- Patch 26: `PlussQit_CodeM_patch_26.zip` (added YAML→generated method surface tool; added ReduceKernels + implemented dim reductions for sum/mean/amax/amin/all/any; tests)

- Patch 27: `PlussQit_CodeM_patch_27.zip` (moved TorchAligned stubs out of headers; added ternary SeriesIterator; implemented where/clamp_min/clamp_max/nan_to_num/cumsum/cumprod; added kernels + tests + docs)

- Patch 28: `PlussQit_CodeM_patch_28.zip` (where broadcasting + scalar where; clamp_min/max Series overloads; clamp alias; moved stub defs fully out-of-line; added KernelTable registry and used it for log1p; tests)

- Patch 29: `PlussQit_CodeM_patch_29.zip` (upgraded SnapPtr/RefCounted to production-grade atomic intrusive ownership; adopt semantics; casts; docs)

- Patch 30: `PlussQit_CodeM_patch_30.zip` (production kernel-table extended to binary ops; fixed missing ops_internal by adding production OpsInternal; added compare/pow/unary-basic kernels; expanded elementwise dtype coverage; docs for broadcasting/type promotion; NaN predicates; tests)

- Patch 31: `PlussQit_CodeM_patch_31.zip` (SIMD-friendly fast unary ops relu/exp/log; Float8 storage + conversions + compute policy; scatter_add basic semantics; div-by-zero checks for integral; nan/inf factories; tests + docs)

- Patch 32: `PlussQit_CodeM_patch_32.zip` (scatter_add broadcasting; scatter_reduce sum/amax/amin; index_put_ overlap/accumulate tightened; contiguous fast-paths for relu/exp/log/sqrt; Complex64/Complex128)

- Patch 33: `PlussQit_CodeM_patch_33.zip` (advanced indexing: non-leading + mixed bool/int; scatter_reduce include_self/initial; docs + tests)

- Patch 34: `PlussQit_CodeM_patch_34.zip` (index_put parity: multi-index broadcast mixed shapes; multi-dim bool masks in indices; contiguous-advanced-dims full-values order; more failure tests; hardened stack trace)

- Patch 35: `PlussQit_CodeM_patch_35.zip` (scatter_reduce dtype expansion + complex policy; broadcastable full bool mask indexing; improved printing; contract test suite)

- Patch 36: `PlussQit_CodeM_patch_36.zip` (type promotion parity incl float8+complex; contiguous SIMD-friendly fast paths for unary/binary hot ops; scatter_reduce stays; benchmarks harness; docs + tests)

- Patch 37: `PlussQit_CodeM_patch_37.zip` (method surface: pile/unite naming deviations; factories accept unordered Config kv list; fixed Series.h declarations; more indexing failure-contract tests; docs)

- Patch 38: `PlussQit_CodeM_patch_38.zip` (large method surface expansion via scalar templates; new factories linspace/eye/*_like; public rand/randn/randint wrappers; printing polish; tests + docs)

- Patch 39: `PlussQit_CodeM_patch_39.zip` (offline/repro build: optional FetchContent; production linspace/eye direct kernels; *_like dtype defaulting; correctness tests; deps docs)

- Patch 40: `PlussQit_CodeM_patch_40.zip` (fix missing ops_internal reductions; add reduction methods & dim variants; add cond.where and masked_fill(_); extend reduction kernels for more dtypes incl half/bf16/ints/complex sum; add inplace masked_fill kernel; tests+docs; safety/build hygiene)

- Patch 41: `PlussQit_CodeM_patch_41.zip` (complex dtype parity: add/sub/mul/div/pow; unary math exp/log/sqrt/etc; conj/real/imag/angle; complex eq/ne; tests+docs)

- Patch 42: `PlussQit_CodeM_patch_42.zip` (complex mean/prod; clamp/clip + relu methods and inplace kernels; float32/64 contiguous fast-path add/mul; tests+docs)

- Patch 43: `PlussQit_CodeM_patch_43.zip` (activation kernels: sigmoid/softplus/silu/gelu/leaky_relu + inplace; Series method API; docs+tests)

- Patch 44: `PlussQit_CodeM_patch_44.zip` (softmax/log_softmax/logsumexp methods + CPU kernels; docs+tests)

- Patch 45: `PlussQit_CodeM_patch_45.zip` (permute/unsqueeze/squeeze/flatten view ops; norm methods; docs+tests)

- Patch 46: `PlussQit_CodeM_patch_46.zip` (UnaryMath contiguous fast paths for exp/log/sqrt; Softmax contiguous specialization for f32/f64; var/std methods; docs+tests)

- Patch 47: `PlussQit_CodeM_patch_47.zip` (TypePromotion module + wired into binary ops; Welford global var; complex norms; docs+tests)

  - Patch 47 note: CMake updated to glob CodeM core/native cpu .cpp sources (CONFIGURE_DEPENDS) so new kernels compile without manual list edits.

- Patch 48: `PlussQit_CodeM_patch_48.zip` (scatter_add + scatter_reduce(sum/amax/amin) CPU-only; Series method API; docs+tests)

- Patch 49: `PlussQit_CodeM_patch_49.zip` (scatter: relaxed dim-size rules + src broadcasting + Float16/BFloat16 support; docs+tests)

- Patch 50: `PlussQit_CodeM_patch_50.zip` (advanced indexing parity: non-leading advanced dims; mixed bool+int; multi-d masks; index_put parity via permute-to-front; docs+tests)

- Patch 51: `PlussQit_CodeM_patch_51.zip` (advanced index broadcast parity; index_put_ accumulate dtypes expanded; type promotion expanded; SIMD fast paths relu/add/mul; where/clamp/maximum/minimum methods; docs+tests)

- Patch 52: `PlussQit_CodeM_patch_52.zip` (index_put_ accumulate expanded for advanced path; tanh/softplus SIMD fast paths; more tests+docs)

- Patch 53: `PlussQit_CodeM_patch_53.zip` (codegen: auto-generate Series methods from OpsInternal.h; stricter indexing broadcast failure tests; docs)

- Patch 54: `PlussQit_CodeM_patch_54.zip` (codegen duplicate suppression; add unary-math family to ops_internal + kernels; new method-surface test; docs)

- Patch 55: `PlussQit_CodeM_patch_55.zip` (contiguous-advanced indexing parity; logical_and/or/xor; masked_select/masked_fill(_); tests+docs)

- Patch 56: `PlussQit_CodeM_patch_56.zip` (scatter_reduce include_self+initial; remove scatter_reduce placeholders so codegen provides methods; fix non-contig advanced index permute bug; tests+docs)

- Patch 57: `PlussQit_CodeM_patch_57.zip` (remove deprecated series_factory.h; move value/make_series/from_blob into pluss.h via codem implementations; fix inline placement; dedupe from_torch; nan/inf factories call codem; tests updated)

- Patch 58: `PlussQit_CodeM_patch_58.zip` (mixed bool+int advanced indices for index_put_; index_put_ determinism guard for duplicate indices; overlap-safe value materialization; type promotion fix float16+bfloat16=>float32; docs factories)

- Patch 59: `PlussQit_CodeM_patch_59.zip` (import Torch YAML method names; generate Series method wrappers; add torch_compat invoke backstop; doc coverage report)

- Patch 60: `PlussQit_CodeM_patch_60.zip` (torch_compat invoke_any variadic; broader mapping to ops_internal/Series; factory cleanup; tests+docs)

- Patch 61: `PlussQit_CodeM_patch_61.zip` (expand torch_compat mapping to many more ops; add inplace variants; tests+docs)

- Patch 62: `PlussQit_CodeM_patch_62.zip` (add extra unary/binary math kernels; add ops_internal maximum/minimum/atan2/fmod/remainder/rsqrt/log2/log10/exp2/cbrt/square/nan_to_num/clamp_min/clamp_max; tests+docs)

- Patch 63: `PlussQit_CodeM_patch_63.zip` (fix where brace; add native cpu SortingKernels; add Series sort/argsort/topk; extend invoke routing; tests+docs)

- Patch 64: `PlussQit_CodeM_patch_64.zip` (expand pluss::value overloads incl nested init_list up to 7D; add Series make_from_doubles; add unique/bincount/histc/masked_scatter; rebuild torch_compat Invoke.cpp; tests+docs)

- Patch 65: `PlussQit_CodeM_patch_65.zip` (rewrite pluss.h for namespace hygiene + remove duplication; add shared factory dtype deduction policy; masked_select broadcast; add where method + nonzero; gather/scatter/scatter_add invoke routing; tests+docs)

- Patch 66: `PlussQit_CodeM_patch_66.zip` (lower cmake_minimum_required to 3.25; add warn(); make Invoke fall back instead of throwing NotImplemented; add tests+docs)
