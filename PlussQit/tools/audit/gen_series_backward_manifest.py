#!/usr/bin/env python3
"""Generate tests/CodeM/test_series_backward_manifest.cpp.

The sibling of gen_series_routing_manifest.py, but for the *other* half of
"does calling this Series method work inside a reverse-mode graph": routing
proves the forward call reaches a real kernel; this proves the *output*
actually carries a reverse-mode node back to a tracked input, the same way
every differentiable ATen op in the local PyTorch checkout registers a
grad_fn (see tools/autograd/derivatives.yaml there) so `.backward()` works
through arbitrary user compositions -- not just the curated op list
`tests/CodeM/test_autodiff_sweep.cpp` hand-checks for exact-formula
correctness.

Every generated `Series` wrapper in SeriesMethodsGen.h is probed once with
the receiver (and any `Series` argument) tracked. The outcome:

  TRACKED       output carries a reverse-mode node (source-link or the
                tracks_slope flag) back to the input -- `.reverse()` can flow
                through this call.
  NO_NODE       output is float/complex-valued (i.e. a real differentiable
                quantity by dtype) but carries no node -- .reverse() through
                this call alone would silently disconnect. Candidate gap.
  NONDIFF_DTYPE output dtype is integer/bool/etc -- not a differentiable
                quantity by construction (comparisons, casts, argmax, bit
                ops, index/count queries...); expected, not a gap.
  SKIPPED_BY_DESIGN  on the curated allowlist below: real gradients exist in
                torch for some of these (sign, random fill, detach, ...) but
                Pluss deliberately forms no node (documented divergence,
                already accepted for sign/floor/ceil/round/trunc in the
                autodiff-sweep gate) or the method is Pluss-only bookkeeping
                with no ATen counterpart (tracks_slope_ itself, output_nr,
                retain_grad, ...).
  THREW         forward call raised (bad synthetic args for this probe).

Only NO_NODE rows are gaps; the gate fails on any NO_NODE row not in
known_backward_gaps() below (populated as each is triaged: fixed, or found
to be an accepted divergence and moved to the allowlist with a reason).

Run from the PlussQit source root:
    python3 tools/audit/gen_series_backward_manifest.py
"""
from __future__ import annotations

import pathlib
import re
import sys

SRC = pathlib.Path("src/CodeM/generated/SeriesMethodsGen.h")
OUT = pathlib.Path("tests/CodeM/test_series_backward_manifest.cpp")

# (name+sig) rows already triaged as an accepted "no node" outcome, with a
# reason. Ground truth checked directly against the local PyTorch checkout's
# tools/autograd/derivatives.yaml, and empirically against its own built
# libtorch (the exact v2.8.0 this repo links) via
# `<pytorch-build conda env>/bin/python3` -- `x.grad_fn` after each op, not
# guessed. Three categories:
#
#   torch-stub     torch's own derivatives.yaml formula for this op is
#                  `not_implemented(...)` (raises if backward is actually
#                  exercised) -- forming no node at all, like Pluss does, is
#                  at least as safe as torch's stub, arguably safer (fails at
#                  the op call in spirit rather than an unpredictable later
#                  `.backward()`).
#   torch-nondiff  torch's derivatives.yaml marks the input
#                  `non_differentiable` outright (airy/bessel family) --
#                  Pluss matches torch exactly.
#   zeros-grad     torch registers a node whose backward formula is
#                  `zeros_like(grad)` (sign/trunc/fix family) -- the same
#                  divergence already accepted and gated for
#                  sign/floor/ceil/round/trunc in test_autodiff_sweep.cpp:
#                  Pluss forms no node instead of a zero-gradient node.
#   factory        torch's own new_empty/new_zeros/.../resize_ family are
#                  factory or storage-reallocation calls -- the receiver is
#                  only a dtype/device/layout template, never a
#                  differentiable operand, in torch either.
#   metadata       non-tensor-valued-in-spirit accessor (quantization scale,
#                  real/imag on a non-complex Series) with no meaningful
#                  gradient in torch either (torch outright rejects `.imag`
#                  on a non-complex tensor; d(imag)/d(real x) is 0 regardless).
KNOWN_NO_NODE: dict[str, str] = {
    "airy()": "torch-nondiff: special_airy_ai self is non_differentiable",
    "bernoulli(double)": "zeros-grad: torch's bernoulli self: zeros_like(grad) (a random draw's value has no smooth dependence on the probability template); same family as sign/trunc/fix",
    "besseli()": "torch-nondiff: special_modified_bessel_i0 self is non_differentiable",
    "besseli(double)": "torch-nondiff: special_modified_bessel_i1 self is non_differentiable",
    "besselj()": "torch-nondiff: special_bessel_j0 self is non_differentiable",
    "besselj(double)": "torch-nondiff: special_bessel_j1 self is non_differentiable",
    "besselk()": "torch-nondiff: special_modified_bessel_k0 self is non_differentiable",
    "besselk(double)": "torch-nondiff: special_modified_bessel_k1 self is non_differentiable",
    "bessely()": "torch-nondiff: special_bessel_y0 self is non_differentiable",
    "bessely(double)": "torch-nondiff: special_bessel_y1 self is non_differentiable",
    "fix()": "zeros-grad: torch's trunc (fix's alias) is TruncBackward0 -> zeros_like(grad); same accepted divergence as trunc/sign in test_autodiff_sweep.cpp",
    "floor_divide(const Series&)": "torch-stub: derivatives.yaml has no floor_divide entry; torch's own grad_fn is a NotImplemented stub (checked live against this repo's libtorch 2.8.0)",
    "floor_divide(double)": "torch-stub: see floor_divide(const Series&)",
    "heaviside(const Series&)": "torch-stub: derivatives.yaml has no heaviside entry; torch's own grad_fn is a NotImplemented stub (checked live)",
    "heaviside(double)": "torch-stub: see heaviside(const Series&)",
    "igamma(double)": "torch-stub: derivatives.yaml self: not_implemented(\"igamma: input\"); the Series-Series overload's `other` gradient IS implemented and IS tracked (not on this list)",
    "igammac(double)": "torch-stub: derivatives.yaml self: not_implemented(\"igammac: input\")",
    "imag()": "metadata: torch outright rejects .imag on a non-complex tensor (RuntimeError); d(imag)/d(real x) is 0 regardless, so no node is mathematically harmless",
    "new_empty(pluss::msharp::RangeView::Int)": "factory: template-only receiver, matches torch's own new_empty",
    "new_empty_strided(pluss::msharp::RangeView::Int)": "factory: see new_empty",
    "new_empty_strided(pluss::msharp::RangeView::Int, pluss::msharp::RangeView::Int)": "factory: see new_empty",
    "new_full(pluss::msharp::RangeView::Int, double)": "factory: see new_empty",
    "new_ones(pluss::msharp::RangeView::Int)": "factory: see new_empty",
    "new_zeros(pluss::msharp::RangeView::Int)": "factory: see new_empty",
    "nextafter(const Series&)": "torch-stub: derivatives.yaml self/other: not_implemented(\"nextafter\")",
    "q_per_channel_scales()": "metadata: quantization-parameter accessor, non-differentiable in torch's own quantization model",
    "q_scale()": "metadata: see q_per_channel_scales; torch's own quantize_per_tensor also rejects float64 input outright",
    "resize_(pluss::msharp::RangeView::Int)": "factory: storage reallocation, not a differentiable operand in torch either",
    "sgn()": "zeros-grad: torch's SgnBackward0 -> zeros_like(grad) for real input; same family as sign",
    # -- hand-written Series.h surface (see HAND_WRITTEN_PROBES) --
    "union1d(const Series&)": "no-torch-equivalent: NumPy-style set op, no ATen counterpart to check against. Flat value-level dedup (set_flatten_1d_/unique) -- same category as unique()'s own accepted torch-stub precedent above: which source element 'owns' a deduplicated value has no single correct answer. The _along family (unionize_along etc.) IS differentiable and DOES track -- it operates on whole rows via index_select, so provenance is unambiguous; prefer it when a gradient is needed.",
    "unionize(RangeView::Of<Series>)": "no-torch-equivalent: see union1d",
    "intersect1d(const Series&)": "no-torch-equivalent: see union1d",
    "intersection(RangeView::Of<Series>)": "no-torch-equivalent: see union1d",
    "setdiff1d(const Series&)": "no-torch-equivalent: see union1d",
    "setxor1d(const Series&)": "no-torch-equivalent: see union1d",
    "sign()": "zeros-grad: torch's SignBackward0 -> zeros_like(grad); already gated nondiff=true in test_autodiff_sweep.cpp",
    "unique()": "torch-stub: derivatives.yaml self: not_implemented(\"_unique\") despite output_differentiability: [True, False]",
    "zeta(double)": "torch-stub: derivatives.yaml special_zeta self: not_implemented(\"zeta\"); only `other` has a real formula",
}

WRAPPER = re.compile(
    r'^\s*(Series&?|[\w:<>,\s]+?)\s+([a-z][A-Za-z0-9_]*)\s*\(([^)]*)\)\s*const\s*\{\s*'
    r'return\s+pluss::codem::_compat::(invoke_?)\(\s*"([a-z0-9_]+)"\s*(.*?)\);\s*\}',
    re.M,
)

# Pluss-only bookkeeping / metadata methods the generator happens to match
# because they're routed through the same string-dispatch wrapper macro, but
# which aren't a differentiable ATen op at all -- excluded outright rather
# than probed.
NOT_AN_OP = {
    "tracks_slope_", "retain_grad", "retains_grad", "output_nr", "size",
    "record_stream", "set_data", "data", "detach", "detach_",
}


# The generated-wrapper surface above (SeriesMethodsGen.h, one canonical
# `return _compat::invoke(...)` body shape) is mechanically enumerable. A
# second, smaller surface is hand-written directly in Series.h -- structural/
# indexing/set/grouped/moving-window/linalg-solve ops with heterogeneous
# signatures (ExtractSpec, RangeView::Of<Series>, the `.linearalg` proxy,
# multi-Series-argument reductions) that don't fit one mechanical pattern.
# Enumerated manually here instead, same probe() harness, same gate --
# distinguished in the report only by needing hand-picked synthetic args
# rather than `synth_arg`. (name, sig, extra setup statements, call
# expression). `s`/`mate` are the same tracked probe_input()s the generated
# batch uses; anything else needed is built in `setup`.
HAND_WRITTEN_PROBES: list[tuple[str, str, list[str], str]] = [
    ("extract", "(ExtractSpec)", [],
     "s.extract(pluss::Slicing::SliceAll())"),
    ("extract_put", "(RangeView::Of<ExtractSpec>, const Series&)",
     ["std::vector<pluss::codem::ExtractSpec> specs{pluss::Slicing::SliceAll()};",
      "Series values = probe_input();"],
     "s.extract_put(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(specs), values)"),
    ("index_put", "(std::initializer_list<Series>, const Series&, bool)",
     ["Series idx = make_series(std::vector<std::int64_t>{0, 1}, {2}).to(ScalarType::Int64);",
      "Series values = probe_input();"],
     "s.index_put({idx}, values, false)"),
    ("to", "(ScalarType)", [], "s.to(ScalarType::Float32)"),
    ("movsum", "(int64_t, int64_t)", [], "s.stat.movsum(INT64_C(2), INT64_C(1))"),
    ("movmean", "(int64_t, int64_t)", [], "s.stat.movmean(INT64_C(2), INT64_C(1))"),
    ("movmax", "(int64_t, int64_t)", [], "s.stat.movmax(INT64_C(2), INT64_C(1))"),
    ("movmin", "(int64_t, int64_t)", [], "s.stat.movmin(INT64_C(2), INT64_C(1))"),
    ("movstd", "(int64_t, int64_t)", [], "s.stat.movstd(INT64_C(2), INT64_C(1))"),
    ("movvar", "(int64_t, int64_t)", [], "s.stat.movvar(INT64_C(2), INT64_C(1))"),
    # grouped_* requires 1D values; override the shared 2D `s` locally
    # (Series is copy-assignable, so this just rebinds it before use).
    ("grouped_sum", "(const Series&, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_sum(gid, INT64_C(2))"),
    ("grouped_mean", "(const Series&, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_mean(gid, INT64_C(2))"),
    ("grouped_var", "(const Series&, int64_t, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_var(gid, INT64_C(2))"),
    ("grouped_std", "(const Series&, int64_t, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_std(gid, INT64_C(2))"),
    ("grouped_min", "(const Series&, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_min(gid, INT64_C(2))"),
    ("grouped_max", "(const Series&, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_max(gid, INT64_C(2))"),
    ("grouped_median", "(const Series&, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_median(gid, INT64_C(2))"),
    ("grouped_mode", "(const Series&, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_mode(gid, INT64_C(2))"),
    ("grouped_count", "(const Series&, int64_t)",
     ["s = make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {6}); s.tracks_slope_(true);",
      "Series gid = make_series(std::vector<std::int64_t>{0, 1, 0, 1, 0, 1}, {6}).to(ScalarType::Int64);"],
     "s.grouped_count(gid, INT64_C(2))"),
    ("scatter_reduce_sum", "(int64_t, const Series&, const Series&, bool)",
     ["Series idx2 = make_series(std::vector<std::int64_t>{0, 1, 2, 0, 1, 2}, {2, 3}).to(ScalarType::Int64);",
      "Series src = probe_input();"],
     "s.scatter_reduce_sum(INT64_C(1), idx2, src, true)"),
    ("scatter_reduce_amax", "(int64_t, const Series&, const Series&, bool)",
     ["Series idx2 = make_series(std::vector<std::int64_t>{0, 1, 2, 0, 1, 2}, {2, 3}).to(ScalarType::Int64);",
      "Series src = probe_input();"],
     "s.scatter_reduce_amax(INT64_C(1), idx2, src, true)"),
    ("scatter_reduce_amin", "(int64_t, const Series&, const Series&, bool)",
     ["Series idx2 = make_series(std::vector<std::int64_t>{0, 1, 2, 0, 1, 2}, {2, 3}).to(ScalarType::Int64);",
      "Series src = probe_input();"],
     "s.scatter_reduce_amin(INT64_C(1), idx2, src, true)"),
    # Contraction composites (permute/reshape/bmm/diagonal/sum): TRACKED by
    # construction; probed so a future native rewrite cannot silently drop the node.
    ("tensordot", "(const Series&, int64_t)", [], "s.tensordot(mate, INT64_C(2))"),
    ("tensordot", "(const Series&, vector<int64_t>, vector<int64_t>)", [],
     "s.tensordot(mate, std::vector<std::int64_t>{1}, std::vector<std::int64_t>{1})"),
    ("einsum", "(string_view, vector<Series>)", [], 'Series::einsum("ij,kj->ik", {s, mate})'),
    ("isnan", "()", [], "s.isnan()"),
    ("isinf", "()", [], "s.isinf()"),
    ("isfinite", "()", [], "s.isfinite()"),
    ("isin", "(const Series&)", [], "s.isin(mate)"),
    ("nonzero", "()", [], "s.nonzero()"),
    # `mate` is bit-identical to `s` (both plain probe_input()), which makes
    # every one of these ops degenerate: setdiff/setxor against an identical
    # array is empty, and an empty result can legitimately fall back to a
    # fresh untracked placeholder without that meaning the *populated* case
    # is broken. Use a partially-overlapping second array (shares half its
    # values with `s`, differs on the rest) so union/intersect/diff/xor all
    # produce real, non-empty, mixed-provenance results.
    ("union1d", "(const Series&)",
     ["Series set_b = make_series(std::vector<double>{0.3, -0.7, 5.0, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.union1d(set_b)"),
    ("unionize", "(RangeView::Of<Series>)",
     ["Series set_b = make_series(std::vector<double>{0.3, -0.7, 5.0, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.unionize(pluss::msharp::RangeView::Series{&set_b, 1})"),
    ("intersect1d", "(const Series&)",
     ["Series set_b = make_series(std::vector<double>{0.3, -0.7, 5.0, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.intersect1d(set_b)"),
    ("intersection", "(RangeView::Of<Series>)",
     ["Series set_b = make_series(std::vector<double>{0.3, -0.7, 5.0, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.intersection(pluss::msharp::RangeView::Series{&set_b, 1})"),
    ("setdiff1d", "(const Series&)",
     ["Series set_b = make_series(std::vector<double>{0.3, -0.7, 5.0, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.setdiff1d(set_b)"),
    ("setxor1d", "(const Series&)",
     ["Series set_b = make_series(std::vector<double>{0.3, -0.7, 5.0, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.setxor1d(set_b)"),
    # The `_along` family treats each row as an atomic set element (row-wise
    # dedup/matching, not scalar-wise), so a meaningful (non-degenerate) probe
    # needs at least one row shared verbatim between `s` and the other array
    # AND at least one row unique to each -- the flat family's scalar-overlap
    # dataset above makes every one of these empty (no whole row matches).
    ("unionize_along", "(const Series&, int64_t)",
     ["Series set_b_rows = make_series(std::vector<double>{0.3, -0.7, 1.1, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.unionize_along(set_b_rows, INT64_C(0))"),
    ("intersection_along", "(const Series&, int64_t)",
     ["Series set_b_rows = make_series(std::vector<double>{0.3, -0.7, 1.1, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.intersection_along(set_b_rows, INT64_C(0))"),
    ("setdiff_along", "(const Series&, int64_t)",
     ["Series set_b_rows = make_series(std::vector<double>{0.3, -0.7, 1.1, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.setdiff_along(set_b_rows, INT64_C(0))"),
    ("setxor_along", "(const Series&, int64_t)",
     ["Series set_b_rows = make_series(std::vector<double>{0.3, -0.7, 1.1, 4.0, -0.2, 9.0}, {2, 3});"],
     "s.setxor_along(set_b_rows, INT64_C(0))"),
    ("traverse_slice", "(RangeView::Of<ExtractSpec>)",
     ["std::vector<pluss::codem::ExtractSpec> tspecs{pluss::Slicing::SliceAll()};"],
     "s.traverse_slice(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(tspecs))"),
    ("traverse_put", "(RangeView::Of<ExtractSpec>, const Series&)",
     ["std::vector<pluss::codem::ExtractSpec> tspecs2{pluss::Slicing::SliceAll()};",
      "Series tvalues = probe_input();"],
     "s.traverse_put(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(tspecs2), tvalues)"),
    # cholesky_inverse/cholesky_solve/lu_solve require a square (SPD, for
    # cholesky) receiver; override the shared 2x3 `s` locally with a 2x2 SPD
    # matrix, and give a compatible RHS instead of reusing the 2x3 `mate`.
    ("cholesky_inverse", "(bool) [.linearalg]",
     ["s = make_series(std::vector<double>{2.0, 0.5, 0.5, 1.0}, {2, 2}); s.tracks_slope_(true);"],
     "s.linearalg.cholesky_inverse(false)"),
    ("cholesky_solve", "(const Series&, bool) [.linearalg]",
     ["s = make_series(std::vector<double>{2.0, 0.5, 0.5, 1.0}, {2, 2}); s.tracks_slope_(true);",
      "Series rhs = make_series(std::vector<double>{1.0, 0.5}, {2, 1});"],
     "s.linearalg.cholesky_solve(rhs, false)"),
    ("lu_solve", "(const Series&) [.linearalg]",
     ["s = make_series(std::vector<double>{2.0, 0.5, 0.5, 1.0}, {2, 2}); s.tracks_slope_(true);",
      "Series rhs = make_series(std::vector<double>{1.0, 0.5}, {2, 1});"],
     "s.linearalg.lu_solve(rhs)"),
]


def base_type(param: str) -> str:
    t = re.sub(r'=.*$', '', param).strip()
    t = re.sub(r'\s+[A-Za-z_]\w*$', '', t).strip()
    return t


FFT_FAMILY = {"fft", "ifft", "rfft", "irfft"}


def synth_arg(ptype: str, idx: int) -> tuple[str, str | None]:
    t = base_type(ptype)
    if t == "const Series&":
        return ("mate", None)
    if t == "std::int64_t":
        return ("INT64_C(0)", None)
    if t == "double":
        return ("0.5", None)
    if t == "bool":
        return ("false", None)
    if t == "const std::string&":
        return ('std::string("sum")', None)
    if t == "pluss::msharp::RangeView::Int":
        var = f"il{idx}"
        return (f"pluss::msharp::RangeView::Int({var})",
                f"std::vector<std::int64_t> {var}{{1, 1}};")
    return ("", None)


def main() -> int:
    if not SRC.exists():
        print(f"cannot find {SRC}; run from the PlussQit source root", file=sys.stderr)
        return 2
    text = SRC.read_text()

    probes: list[tuple[str, str, list[str], str, bool]] = []
    skipped: list[str] = []
    for m in WRAPPER.finditer(text):
        _ret, name, plist, _kind, op, _tail = m.groups()
        if name in NOT_AN_OP:
            continue
        params = [p.strip() for p in plist.split(",") if p.strip()]
        pres: list[str] = []
        args: list[str] = []
        ok = True
        uses_mate = False
        for i, p in enumerate(params):
            expr, pre = synth_arg(p, i)
            if not expr:
                ok = False
                break
            if expr == "mate":
                uses_mate = True
            if pre:
                pres.append(pre)
            args.append(expr)
        sig = "(" + ", ".join(base_type(p) for p in params) + ")"
        if not ok:
            skipped.append(f"{name}{sig}")
            continue
        if name in FFT_FAMILY and params:
            # n=0 is rejected ("transform length must be greater than zero");
            # probe with the documented defaults (n=-1 -> input length).
            args = ["INT64_C(-1)", "INT64_C(-1)", 'std::string("backward")']
        if name in {"diagonal", "trace", "diag_embed"} and len(params) == 3:
            # (offset, dim1, dim2): all-zero args name the same axis twice.
            args = ["INT64_C(0)", "INT64_C(0)", "INT64_C(1)"]
        call = f"s.{name}(" + ", ".join(args) + ")"
        probes.append((name, sig, pres, call, uses_mate))

    lines: list[str] = []
    W = lines.append
    W("// GENERATED by tools/audit/gen_series_backward_manifest.py -- do not edit.")
    W("//")
    W("// Probes every generated Series wrapper with the receiver (and any Series")
    W("// argument) tracked, and classifies whether the output carries a reverse-mode")
    W("// node back to the input -- the Series-engine analogue of \"does this ATen op")
    W("// register a grad_fn\" (see the local PyTorch checkout's")
    W("// tools/autograd/derivatives.yaml). See the module docstring in")
    W("// tools/audit/gen_series_backward_manifest.py for the full classification.")
    W("// Writes the full manifest to series_backward_manifest.txt in the run")
    W("// directory.")
    W("")
    W("#include <catch2/catch_test_macros.hpp>")
    W("")
    W("#include <pluss/codem/codem.h>")
    W("#include <pluss/codem/series_factory.h>")
    W("")
    W('#include "CodeM/compat/Invoke.h"')
    W('#include "CodeM/Indexing/ExtractSpec.h"')
    W('#include "MSharp/RangeView.h"')
    W('#include "pluss/slicing.h"')
    W("")
    W("#include <cstdio>")
    W("#include <fstream>")
    W("#include <set>")
    W("#include <string>")
    W("#include <vector>")
    W("")
    W("namespace {")
    W("")
    W("using pluss::ScalarType;")
    W("using pluss::Series;")
    W("using pluss::make_series;")
    W("namespace tape = pluss::codem::series_tape;")
    W("")
    W("enum class R { TRACKED, NO_NODE, NONDIFF_DTYPE, SKIPPED_BY_DESIGN, THREW };")
    W("")
    W("struct Row { std::string key; R r; };")
    W("std::vector<Row>& rows() { static std::vector<Row> v; return v; }")
    W("")
    W('const char* kManifestPath = "series_backward_manifest.txt";')
    W("std::ofstream& sink() {")
    W("  static std::ofstream s(kManifestPath, std::ios::trunc);")
    W("  return s;")
    W("}")
    W("")
    W("Series probe_input() {")
    W("  return make_series(std::vector<double>{0.3, -0.7, 1.1, 0.4, -0.2, 0.9}, {2, 3})")
    W("      .to(ScalarType::Float64);")
    W("}")
    W("")
    W("// Rows already triaged as an accepted \"no node\" outcome -- see")
    W("// KNOWN_NO_NODE in the generator for the reason each was added.")
    W("const std::set<std::string>& known_no_node() {")
    W("  static const std::set<std::string> s{")
    for key in KNOWN_NO_NODE:
        W(f'      "{key}",')
    W("  };")
    W("  return s;")
    W("}")
    W("")
    W("template <class F>")
    W("void probe(const std::string& name, const std::string& sig, F&& f) {")
    W("  const std::string key = name + sig;")
    W("  R r;")
    W("  std::string detail;")
    W("  try {")
    W("    Series y = f();")
    W("    const bool has_node = tape::source_info(y).has_value() || tape::tracks_slope(y);")
    W("    const bool is_diff_dtype = y.is_floating_point() || y.is_complex();")
    W("    if (!is_diff_dtype) {")
    W("      r = R::NONDIFF_DTYPE;")
    W("    } else if (has_node) {")
    W("      r = R::TRACKED;")
    W("    } else if (known_no_node().count(key)) {")
    W("      r = R::SKIPPED_BY_DESIGN;")
    W("    } else {")
    W("      r = R::NO_NODE;")
    W("    }")
    W("  } catch (const std::exception& e) {")
    W("    r = R::THREW;")
    W("    detail = e.what();")
    W("  } catch (...) {")
    W('    r = R::THREW; detail = "non-std exception";')
    W("  }")
    W("  rows().push_back({key, r});")
    W("  static const char* names[] = ")
    W('      {"TRACKED", "NO_NODE", "NONDIFF_DTYPE", "SKIPPED_BY_DESIGN", "THREW"};')
    W('  sink() << names[static_cast<int>(r)] << "\\t" << key;')
    W('  if (!detail.empty()) sink() << "\\t" << detail.substr(0, 160);')
    W('  sink() << "\\n";')
    W("  sink().flush();")
    W("}")
    W("")
    W("}  // namespace")
    W("")
    W('TEST_CASE("series backward manifest", "[backward-manifest][audit]") {')
    for name, sig, pres, call, uses_mate in probes:
        stmt = f'  probe("{name}", "{sig}", [&]{{ Series s = probe_input(); s.tracks_slope_(true); '
        if uses_mate:
            stmt += "Series mate = probe_input(); mate.tracks_slope_(true); "
        for pre in pres:
            stmt += pre + " "
        stmt += f'return {call}; }});'
        W(stmt)
    W("")
    W("  // Hand-written Series.h surface (heterogeneous signatures, manually")
    W("  // enumerated -- see HAND_WRITTEN_PROBES in the generator).")
    for name, sig, pres, call in HAND_WRITTEN_PROBES:
        uses_mate = "mate" in call or any("mate" in p for p in pres)
        stmt = f'  probe("{name}", "{sig}", [&]{{ Series s = probe_input(); s.tracks_slope_(true); '
        if uses_mate:
            stmt += "Series mate = probe_input(); mate.tracks_slope_(true); "
        for pre in pres:
            stmt += pre + " "
        stmt += f'return {call}; }});'
        W(stmt)
    W("")
    W("  int tracked = 0, no_node = 0, nondiff = 0, skipped_n = 0, threw = 0;")
    W("  for (const auto& row : rows()) {")
    W("    switch (row.r) {")
    W("      case R::TRACKED: ++tracked; break;")
    W("      case R::NO_NODE: ++no_node; break;")
    W("      case R::NONDIFF_DTYPE: ++nondiff; break;")
    W("      case R::SKIPPED_BY_DESIGN: ++skipped_n; break;")
    W("      case R::THREW: ++threw; break;")
    W("    }")
    W("  }")
    W('  sink() << "# " << tracked << " tracked, " << no_node << " no_node, "')
    W('         << nondiff << " nondiff_dtype, " << skipped_n << " skipped_by_design, "')
    W('         << threw << " threw\\n";')
    W("  sink().flush();")
    W('  WARN("backward manifest: " << tracked << " tracked, " << no_node << " no_node, "')
    W('       << nondiff << " nondiff_dtype, " << skipped_n << " skipped_by_design, "')
    W('       << threw << " threw (see " << kManifestPath << ")");')
    W("")
    W("  std::vector<std::string> gaps;")
    W("  for (const auto& row : rows()) {")
    W("    if (row.r == R::NO_NODE) gaps.push_back(row.key);")
    W("  }")
    W('  for (const auto& k : gaps) UNSCOPED_INFO("candidate backward gap: " << k);')
    W("  CHECK(gaps.empty());")
    W("}")
    W("")

    OUT.write_text("\n".join(lines))
    print(f"wrote {OUT} with {len(probes)} probes")
    if skipped:
        print(f"skipped {len(skipped)} wrappers with unmapped param types:")
        for s in skipped[:40]:
            print("  ", s)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
