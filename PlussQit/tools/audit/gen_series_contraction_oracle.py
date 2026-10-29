#!/usr/bin/env python3
"""Generate tests/CodeM/test_series_contraction.cpp.

Oracle-driven test for Series::einsum / Series::tensordot: every case is run
through the live libtorch (python torch from the cpp_env conda env, the same
build the repo links) and the forward output plus the reverse-mode gradient of
`loss = sum(out * W)` w.r.t. every operand are embedded as literals. Cases
torch itself rejects are emitted as expect-throw cases for Series.

Run from the PlussQit source root with a python that has torch:
    <cpp_env>/bin/python3 tools/audit/gen_series_contraction_oracle.py
"""
import math
import pathlib
import torch

OUT = pathlib.Path("tests/CodeM/test_series_contraction.cpp")


def data(n, seed):
    return [math.sin(0.7 * i + seed) + 0.3 for i in range(n)]


def lit(v):
    return "{" + ", ".join(repr(float(x)) for x in v) + "}"


def shp(s):
    return "{" + ", ".join(str(x) for x in s) + "}"


CASES = [
    ("ij,jk->ik", [(2, 3), (3, 4)]), ("ij,jk", [(2, 3), (3, 4)]),
    ("ij->ji", [(3, 4)]), ("ij->", [(3, 4)]), ("ij", [(3, 4)]),
    ("i,i->", [(5,), (5,)]), ("i,i", [(5,), (5,)]),
    ("i,j->ij", [(3,), (4,)]), ("i,j", [(3,), (4,)]),
    ("ij,ij->ij", [(3, 4), (3, 4)]), ("ij,kj->ik", [(3, 4), (2, 4)]),
    ("ij,ij", [(3, 4), (3, 4)]),
    ("ijk->kji", [(2, 3, 4)]), ("ijk->ik", [(2, 3, 4)]), ("ijk->jk", [(2, 3, 4)]),
    ("bij,bjk->bik", [(2, 3, 4), (2, 4, 5)]), ("bij,bjk", [(2, 3, 4), (2, 4, 5)]),
    ("ij , jk -> ik", [(2, 3), (3, 4)]),
    ("Ab,bC->AC", [(2, 3), (3, 4)]), ("aB,Bc", [(2, 3), (3, 4)]),
    ("ab,ab->", [(3, 4), (3, 4)]),
    ("ii->i", [(4, 4)]), ("ii", [(4, 4)]), ("ii->", [(4, 4)]),
    ("bii->bi", [(2, 3, 3)]), ("iij->j", [(3, 3, 4)]),
    ("ijij->", [(2, 3, 2, 3)]), ("ijji->ij", [(2, 3, 3, 2)]),
    ("...ij,...jk->...ik", [(2, 3, 4), (2, 4, 5)]),
    ("...ij,jk", [(2, 3, 4), (4, 5)]), ("...ij,jk->...ik", [(2, 3, 4), (4, 5)]),
    ("ij,...jk->...ik", [(3, 4), (2, 4, 5)]),
    ("...i,...i->...", [(2, 3, 4), (4,)]), ("...i,...i->...", [(2, 3, 4), (3, 4)]),
    ("...->...", [(2, 3)]), ("...", [(2, 3)]), ("i...->...i", [(2, 3, 4)]),
    ("...i->i", [(2, 3, 4)]), ("...ij->...", [(2, 3, 4)]),
    ("ij,jk,kl->il", [(2, 3), (3, 4), (4, 2)]), ("ij,jk,kl", [(2, 3), (3, 4), (4, 2)]),
    ("abc,cd,de->abe", [(2, 3, 4), (4, 3), (3, 5)]),
    ("i,ij,j->", [(3,), (3, 4), (4,)]),
    ("ij,jk,kl,lm->im", [(2, 3), (3, 4), (4, 2), (2, 3)]),
    ("ijkl,jk->il", [(2, 3, 4, 5), (3, 4)]),
    ("ij,ik,il->jkl", [(2, 3), (2, 4), (2, 2)]),
    ("ab,bc,ca->", [(2, 3), (3, 4), (4, 2)]),
    ("ij,kl->ijkl", [(2, 3), (3, 2)]), ("ij,jk->ki", [(2, 3), (3, 4)]),
    ("bi,bj->bij", [(3, 2), (3, 4)]),
    ("bhqd,bhkd->bhqk", [(2, 2, 3, 4), (2, 2, 5, 4)]),
    ("bhqk,bhkd->bhqd", [(2, 2, 3, 5), (2, 2, 5, 4)]),
    ("i,i,i->", [(4,), (4,), (4,)]), ("ij,ij,ij->i", [(3, 4), (3, 4), (3, 4)]),
    # size-1 broadcasting of a shared label
    ("ij,jk->ik", [(2, 1), (3, 4)]), ("ij,jk->ik", [(2, 3), (1, 4)]),
    ("bij,bjk->bik", [(1, 3, 4), (2, 4, 2)]), ("ij,ij->ij", [(1, 4), (3, 4)]),
    # scalar operands / zero-dim
    (",->", [(), ()]), ("i,->i", [(3,), ()]), (",i", [(), (3,)]),
]
DIAG = [((3, 3), 0, 0, 1), ((3, 4), 1, 0, 1), ((4, 3), -1, 0, 1), ((3, 4), 0, 1, 0), ((3, 4), 2, 1, 0),
        ((2, 3, 4), 0, 0, 1), ((2, 3, 4), 1, 1, 2), ((2, 3, 4), -1, 1, 2), ((2, 3, 4), 0, 0, 2), ((2, 3, 4), 0, -1, -2),
        ((2, 3, 4, 3), 0, 1, 3), ((2, 3, 4, 3), 1, 3, 1), ((2, 3), 5, 0, 1), ((2, 3, 3, 2), 0, 1, 2)]
TRACE_ND = [((3, 4, 5), 0, 0, 1), ((3, 4, 5), 1, 1, 2), ((3, 4, 5), -1, 0, 2), ((2, 3, 4, 3), 0, 1, 3), ((4, 4), 1, 0, 1), ((3, 5), -2, 1, 0)]
EMBED = [((3,), 0, -2, -1), ((3,), 1, -2, -1), ((3,), -2, 0, 1), ((2, 3), 0, -2, -1), ((2, 3), 1, 0, 1), ((2, 3), -1, 0, 2),
         ((2, 3), 0, 1, 2), ((2, 3, 4), 0, -2, -1), ((2, 3, 4), 2, 0, 3), ((2, 3, 4), -1, 1, 3), ((2, 3, 4), 0, 3, 0), ((1, 1), 0, -2, -1)]
ERR = [("ij,jk->ik", [(2, 3), (4, 5)]), ("ij->k", [(2, 3)]), ("ij,jk", [(2, 3)]),
       ("ii", [(2, 3)]), ("i.j", [(2, 3)]), ("ij->ii", [(2, 3)]), ("i1", [(2,)])]

TD = [  # (a_shape, b_shape, dims) ; dims = int or (list, list)
    ((3, 4), (4, 5), 1), ((2, 3, 4), (3, 4, 5), 2), ((2, 3), (4,), 0), ((3,), (3,), 1),
    ((2, 3, 4), (4, 3, 5), ([1, 2], [1, 0])), ((2, 3, 4), (5, 3), ([1], [1])),
    ((2, 3, 4), (4, 2), ([-1, 0], [0, 1])), ((3, 4), (3, 4), 2), ((2, 3, 4, 5), (4, 5, 6), 2),
    ((2, 3), (3, 4), ([1], [0])), ((2, 1), (3, 4), ([1], [0])), ((2, 3), (1, 4), ([1], [0])),
    ((2, 3, 4), (3, 4, 5), ([1, 2], [0, 1])), ((), (), 0), ((3,), (4,), 0),
]
TDERR = [((2, 3), (4, 5), 1), ((2, 3), (3, 4), ([1, 0], [0])), ((2, 3), (3, 4), 3)]


def run(fn, arrs):
    ts = [torch.tensor(a, dtype=torch.float64, requires_grad=True) for a in arrs]
    out = fn(ts)
    w = torch.tensor(data(out.numel(), 9.0), dtype=torch.float64).reshape(out.shape)
    (out * w).sum().backward()
    return out.detach(), [t.grad for t in ts]


def mk(shapes, seed0=0.0):
    arrs = []
    for k, s in enumerate(shapes):
        n = 1
        for d in s:
            n *= d
        v = torch.tensor(data(n, seed0 + 1.3 * k), dtype=torch.float64).reshape(s)
        arrs.append(v)
    return arrs


def main():
    L = []
    L.append("// @generated by tools/audit/gen_series_contraction_oracle.py. Do not edit.")
    L.append("// Oracle: live libtorch %s (forward + reverse of sum(out*W))." % torch.__version__)
    L.append('#include <catch2/catch_approx.hpp>\n#include <catch2/catch_test_macros.hpp>\n#include <pluss/pluss.h>\n\n#include <string>\n#include <vector>\n')
    L.append("using pluss::Series;\nusing pluss::make_series;\n")
    L.append(r'''namespace {
struct Op { std::vector<double> v; std::vector<std::int64_t> s; };

std::vector<double> weights(std::size_t n) {
  std::vector<double> w(n);
  for (std::size_t i = 0; i < n; ++i) w[i] = std::sin(0.7 * static_cast<double>(i) + 9.0) + 0.3;
  return w;
}

std::vector<Series> tracked(const std::vector<Op>& ops) {
  std::vector<Series> out;
  for (const auto& o : ops) {
    Series s = make_series(o.v, o.s);
    s.tracks_slope_(true);
    out.push_back(s);
  }
  return out;
}

std::vector<double> flat(const Series& s) {
  return s.contiguous().reshape(std::vector<std::int64_t>{s.numel()}).host<double>();
}

void close(const std::vector<double>& got, const std::vector<double>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO("index " << i);
    REQUIRE(got[i] == Catch::Approx(want[i]).epsilon(1e-10).margin(1e-12));
  }
}

void finish(const Series& out, const std::vector<Series>& in, const std::vector<std::int64_t>& oshape,
            const std::vector<double>& want, const std::vector<std::vector<double>>& wgrad) {
  REQUIRE(std::vector<std::int64_t>(out.sizes().begin(), out.sizes().end()) == oshape);
  close(flat(out), want);
  const Series w = make_series(weights(want.size()), oshape);
  out.mul(w).sum().reverse();
  for (std::size_t k = 0; k < in.size(); ++k) {
    INFO("grad of operand " << k);
    REQUIRE(in[k].has_sensitivity());
    close(flat(in[k].sensitivity()), wgrad[k]);
  }
}

void check_einsum(const std::string& eq, const std::vector<Op>& ops, const std::vector<std::int64_t>& oshape,
                  const std::vector<double>& want, const std::vector<std::vector<double>>& wgrad) {
  INFO("einsum " << eq);
  const auto in = tracked(ops);
  finish(Series::einsum(eq, in), in, oshape, want, wgrad);
}

void check_td(const std::vector<Op>& ops, const std::vector<std::int64_t>* da, const std::vector<std::int64_t>* db,
              std::int64_t n, const std::vector<std::int64_t>& oshape, const std::vector<double>& want,
              const std::vector<std::vector<double>>& wgrad) {
  const auto in = tracked(ops);
  const Series out = da ? in[0].tensordot(in[1], *da, *db) : in[0].tensordot(in[1], n);
  finish(out, in, oshape, want, wgrad);
}
}  // namespace
''')
    L.append('TEST_CASE("Series::einsum matches libtorch (forward + reverse)", "[einsum][contraction][oracle]") {')
    for i, (eq, shapes) in enumerate(CASES):
        arrs = mk(shapes)
        try:
            out, grads = run(lambda ts, eq=eq: torch.einsum(eq, *ts), arrs)
        except Exception as e:
            print("torch rejects", eq, shapes, e)
            # keep as expected throw
            ops = ", ".join("{%s, %s}" % (lit(a.flatten().tolist()), shp(a.shape)) for a in arrs)
            L.append('  SECTION("throws: %s %s") { std::vector<Series> v; %s REQUIRE_THROWS(Series::einsum("%s", v)); }' % (
                eq, list(shapes), "".join('v.push_back(make_series(%s, %s));' % (lit(a.flatten().tolist()), shp(a.shape)) for a in arrs), eq))
            continue
        ops = ", ".join("{%s, %s}" % (lit(a.flatten().tolist()), shp(a.shape)) for a in arrs)
        gl = ", ".join(lit(g.flatten().tolist()) for g in grads)
        L.append('  SECTION("%s %s") { check_einsum("%s", {%s}, %s, %s, {%s}); }' % (
            eq, list(shapes), eq, ops, shp(out.shape), lit(out.flatten().tolist()), gl))
    for eq, shapes in ERR:
        try:
            torch.einsum(eq, *mk(shapes))
            print("WARNING torch accepts", eq, shapes)
            continue
        except Exception:
            pass
        arrs = mk(shapes)
        L.append('  SECTION("throws: %s") { std::vector<Series> v; %s REQUIRE_THROWS(Series::einsum("%s", v)); }' % (
            eq, "".join('v.push_back(make_series(%s, %s));' % (lit(a.flatten().tolist()), shp(a.shape)) for a in arrs), eq))
    L.append("}\n")

    L.append('TEST_CASE("Series::diagonal / trace match libtorch on any rank (forward + reverse)", "[diagonal][trace][oracle]") {')
    for shape, off, d1, d2 in DIAG:
        arrs = mk([shape])
        out, grads = run(lambda ts, off=off, d1=d1, d2=d2: torch.diagonal(ts[0], off, d1, d2), arrs)
        L.append('  SECTION("diagonal %s off=%d d=(%d,%d)") { const auto in = tracked({{%s, %s}}); finish(in[0].diagonal(%d, %d, %d), in, %s, %s, {%s}); }' % (
            list(shape), off, d1, d2, lit(arrs[0].flatten().tolist()), shp(shape), off, d1, d2,
            shp(out.shape), lit(out.flatten().tolist()), lit(grads[0].flatten().tolist())))
    # (tall matrices are left out on purpose: torch's trace backward fills flat
    # positions 0, c+1, 2(c+1), ... of the whole buffer, which for rows > cols
    # leaks gradient onto off-diagonal entries; Pluss's exact zeros are correct.)
    for shape in [(4, 4), (3, 5)]:
        arrs = mk([shape])
        out, grads = run(lambda ts: torch.trace(ts[0]), arrs)
        L.append('  SECTION("trace %s") { const auto in = tracked({{%s, %s}}); finish(in[0].trace(), in, %s, %s, {%s}); }' % (
            list(shape), lit(arrs[0].flatten().tolist()), shp(shape), shp(out.shape), lit(out.flatten().tolist()), lit(grads[0].flatten().tolist())))
    for shape, off, d1, d2 in TRACE_ND:
        arrs = mk([shape])
        out, grads = run(lambda ts, off=off, d1=d1, d2=d2: torch.diagonal(ts[0], off, d1, d2).sum(-1), arrs)
        L.append('  SECTION("trace %s off=%d d=(%d,%d)") { const auto in = tracked({{%s, %s}}); finish(in[0].trace(%d, %d, %d), in, %s, %s, {%s}); }' % (
            list(shape), off, d1, d2, lit(arrs[0].flatten().tolist()), shp(shape), off, d1, d2,
            shp(out.shape), lit(out.flatten().tolist()), lit(grads[0].flatten().tolist())))
    for shape, off, d1, d2 in EMBED:
        arrs = mk([shape])
        out, grads = run(lambda ts, off=off, d1=d1, d2=d2: torch.diag_embed(ts[0], off, d1, d2), arrs)
        L.append('  SECTION("diag_embed %s off=%d d=(%d,%d)") { const auto in = tracked({{%s, %s}}); finish(in[0].diag_embed(%d, %d, %d), in, %s, %s, {%s}); }' % (
            list(shape), off, d1, d2, lit(arrs[0].flatten().tolist()), shp(shape), off, d1, d2,
            shp(out.shape), lit(out.flatten().tolist()), lit(grads[0].flatten().tolist())))
    L.append("}\n")

    L.append('TEST_CASE("Series::tensordot matches libtorch (forward + reverse)", "[tensordot][contraction][oracle]") {')
    for sa, sb, dims in TD:
        arrs = mk([sa, sb])
        out, grads = run(lambda ts, dims=dims: torch.tensordot(ts[0], ts[1], dims=dims), arrs)
        ops = ", ".join("{%s, %s}" % (lit(a.flatten().tolist()), shp(a.shape)) for a in arrs)
        gl = ", ".join(lit(g.flatten().tolist()) for g in grads)
        L.append('  SECTION("%s %s %s") { const std::vector<std::int64_t> da%s, db%s; check_td({%s}, %s, %s, %d, %s, %s, {%s}); }' % (
            list(sa), list(sb), dims, "" if isinstance(dims, int) else shp(dims[0]), "" if isinstance(dims, int) else shp(dims[1]),
            ops, "nullptr" if isinstance(dims, int) else "&da", "nullptr" if isinstance(dims, int) else "&db",
            dims if isinstance(dims, int) else 0, shp(out.shape), lit(out.flatten().tolist()), gl))
    for sa, sb, dims in TDERR:
        arrs = mk([sa, sb])
        try:
            torch.tensordot(arrs[0], arrs[1], dims=dims); print("WARNING torch accepts", sa, sb, dims); continue
        except Exception:
            pass
        a, b = arrs
        if isinstance(dims, int):
            c = "a.tensordot(b, %d)" % dims
        else:
            c = "a.tensordot(b, std::vector<std::int64_t>%s, std::vector<std::int64_t>%s)" % (shp(dims[0]), shp(dims[1]))
        L.append('  SECTION("throws: %s %s %s") { const Series a = make_series(%s, %s), b = make_series(%s, %s); REQUIRE_THROWS(%s); }' % (
            list(sa), list(sb), dims, lit(a.flatten().tolist()), shp(a.shape), lit(b.flatten().tolist()), shp(b.shape), c))
    L.append("}")
    OUT.write_text("\n".join(L) + "\n")
    print("wrote", OUT)


main()
