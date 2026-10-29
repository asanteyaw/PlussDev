// tensordot() / einsum(): composite contractions.
//
// Both reduce to permute -> reshape -> bmm -> reshape (plus sum/diagonal for
// labels that are not contracted pairwise), so reverse-mode autograd comes
// from the already-verified reverse rules of those ops; nothing here binds
// its own SourceLink.
#include "CodeM/core/Errors.h"
#include "CodeM/core/Series.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::codem {

namespace {

using I64 = std::int64_t;
using Dims = std::vector<I64>;

pluss::msharp::RangeView::Int ints_(const Dims &v) {
  return pluss::msharp::RangeView::Int(std::span<const I64>(v.data(), v.size()));
}

I64 prod_(const Dims &sizes, const Dims &which) {
  I64 p = 1;
  for (I64 d : which) p *= sizes[static_cast<std::size_t>(d)];
  return p;
}

Series sum_dims_(const Series &x, Dims dims) {
  if (dims.empty()) return x;
  return x.sum(ints_(dims), false);
}

// ---------------------------------------------------------------- tensordot

Series tensordot_impl_(const Series &a_in, const Series &b_in, Dims da, Dims db) {
  PLUSS_CHECK(a_in.defined() && b_in.defined(), "tensordot: expected defined Series");
  PLUSS_CHECK(da.size() == db.size(),
              "tensordot: both dimension lists should have same length");
  Series a = a_in, b = b_in;
  const I64 na = a.dim(), nb = b.dim();
  for (std::size_t i = 0; i < da.size(); ++i) {
    I64 x = da[i], y = db[i];
    if (x < 0) x += na;
    if (y < 0) y += nb;
    PLUSS_CHECK(x >= 0 && x < na, "tensordot: dims_self out of range");
    PLUSS_CHECK(y >= 0 && y < nb, "tensordot: dims_other out of range");
    da[i] = x;
    db[i] = y;
  }
  // Mismatched contracted sizes: only a size-1 side is allowed. The size-1
  // side multiplies the other side's sum along that axis, so the longer axis
  // is summed (kept as size 1) and the pair then contracts as 1 x 1.
  for (std::size_t i = 0; i < da.size(); ++i) {
    const I64 sa = a.sizes()[static_cast<std::size_t>(da[i])];
    const I64 sb = b.sizes()[static_cast<std::size_t>(db[i])];
    if (sa == sb) continue;
    if (sa == 1) b = b.sum(db[i], true);
    else if (sb == 1) a = a.sum(da[i], true);
    else PLUSS_CHECK(false, "tensordot: contracted dimension sizes must match (or be 1)");
  }

  const Dims sa(a.sizes().begin(), a.sizes().end());
  const Dims sb(b.sizes().begin(), b.sizes().end());
  Dims free_a, free_b;
  for (I64 i = 0; i < a.dim(); ++i)
    if (std::find(da.begin(), da.end(), i) == da.end()) free_a.push_back(i);
  for (I64 i = 0; i < b.dim(); ++i)
    if (std::find(db.begin(), db.end(), i) == db.end()) free_b.push_back(i);

  Dims perm_a = free_a;
  perm_a.insert(perm_a.end(), da.begin(), da.end());
  Dims perm_b = db;
  perm_b.insert(perm_b.end(), free_b.begin(), free_b.end());

  const I64 m = prod_(sa, free_a), k = prod_(sa, da), n = prod_(sb, free_b);
  Dims out_shape;
  for (I64 d : free_a) out_shape.push_back(sa[static_cast<std::size_t>(d)]);
  for (I64 d : free_b) out_shape.push_back(sb[static_cast<std::size_t>(d)]);

  const Series a2 = a.permute(ints_(perm_a)).reshape(Dims{m, k});
  const Series b2 = b.permute(ints_(perm_b)).reshape(Dims{k, n});
  return a2.mm(b2).reshape(out_shape);
}

// ------------------------------------------------------------------- einsum

// Labels are ints: 0..51 for A-Z/a-z (ASCII order within each case), and
// kEllipsis + i for the i-th broadcast axis of the ellipsis block.
constexpr int kEllipsis = 1000;

int label_of_char_(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return 26 + (c - 'a');
  PLUSS_CHECK(false, std::string("einsum: invalid subscript '") + c +
                         "' (only letters and '...' are allowed)");
  return -1;
}

struct Term {
  std::vector<int> labels;  // explicit labels only
  bool has_ellipsis{false};
  std::size_t ellipsis_at{0};  // position in `labels` where "..." sits
};

Term parse_term_(std::string_view s) {
  Term t;
  for (std::size_t i = 0; i < s.size();) {
    const char c = s[i];
    if (c == ' ') { ++i; continue; }
    if (c == '.') {
      PLUSS_CHECK(i + 2 < s.size() + 0 && s[i + 1] == '.' && s[i + 2] == '.',
                  "einsum: '.' must be part of an ellipsis '...'");
      PLUSS_CHECK(!t.has_ellipsis, "einsum: a term may contain only one ellipsis");
      t.has_ellipsis = true;
      t.ellipsis_at = t.labels.size();
      i += 3;
      continue;
    }
    t.labels.push_back(label_of_char_(c));
    ++i;
  }
  return t;
}

struct Operand {
  Series x;
  std::vector<int> labels;  // unique, one per axis of x
};

// Collapse repeated labels in one operand to a diagonal (appended last).
void collapse_repeats_(Operand &op) {
  for (;;) {
    bool again = false;
    for (std::size_t p = 0; p < op.labels.size() && !again; ++p) {
      for (std::size_t q = p + 1; q < op.labels.size(); ++q) {
        if (op.labels[p] != op.labels[q]) continue;
        const int lab = op.labels[p];
        const I64 sp = op.x.sizes()[p], sq = op.x.sizes()[q];
        PLUSS_CHECK(sp == sq, "einsum: repeated subscript must index axes of equal size");
        op.x = op.x.diagonal(0, static_cast<I64>(p), static_cast<I64>(q));
        op.labels.erase(op.labels.begin() + static_cast<std::ptrdiff_t>(q));
        op.labels.erase(op.labels.begin() + static_cast<std::ptrdiff_t>(p));
        op.labels.push_back(lab);
        again = true;
        break;
      }
    }
    if (!again) return;
  }
}

I64 size_of_(const Operand &op, int label) {
  const auto it = std::find(op.labels.begin(), op.labels.end(), label);
  return op.x.sizes()[static_cast<std::size_t>(it - op.labels.begin())];
}

bool has_(const std::vector<int> &v, int l) {
  return std::find(v.begin(), v.end(), l) != v.end();
}

// Sum out every label of `op` that is not in `keep`.
void reduce_unneeded_(Operand &op, const std::vector<int> &keep) {
  Dims gone;
  std::vector<int> labels;
  for (std::size_t i = 0; i < op.labels.size(); ++i) {
    if (has_(keep, op.labels[i])) labels.push_back(op.labels[i]);
    else gone.push_back(static_cast<I64>(i));
  }
  if (gone.empty()) return;
  op.x = sum_dims_(op.x, gone);
  op.labels = std::move(labels);
}

// Expand size-1 axes of `op` so shared label sizes agree with `target`.
void expand_label_(Operand &op, int label, I64 target) {
  const auto it = std::find(op.labels.begin(), op.labels.end(), label);
  const std::size_t pos = static_cast<std::size_t>(it - op.labels.begin());
  if (op.x.sizes()[pos] == target) return;
  Dims shape(op.x.sizes().begin(), op.x.sizes().end());
  shape[pos] = target;
  op.x = op.x.expand(ints_(shape), true);
}

Operand contract_pair_(Operand a, Operand b, const std::vector<int> &needed) {
  // Labels used only by one side and not needed afterwards: sum them first.
  std::vector<int> keep_a, keep_b;
  for (int l : a.labels)
    if (has_(b.labels, l) || has_(needed, l)) keep_a.push_back(l);
  for (int l : b.labels)
    if (has_(a.labels, l) || has_(needed, l)) keep_b.push_back(l);
  reduce_unneeded_(a, keep_a);
  reduce_unneeded_(b, keep_b);

  std::vector<int> batch, contr, a_only, b_only;
  for (int l : a.labels) {
    if (has_(b.labels, l)) (has_(needed, l) ? batch : contr).push_back(l);
    else a_only.push_back(l);
  }
  for (int l : b.labels)
    if (!has_(a.labels, l)) b_only.push_back(l);

  for (int l : batch) {
    const I64 sa = size_of_(a, l), sb = size_of_(b, l);
    if (sa != sb) {
      PLUSS_CHECK(sa == 1 || sb == 1, "einsum: operands do not broadcast for a shared subscript");
      expand_label_(sa == 1 ? a : b, l, std::max(sa, sb));
    }
  }
  for (int l : contr) {
    const I64 sa = size_of_(a, l), sb = size_of_(b, l);
    if (sa != sb) {
      PLUSS_CHECK(sa == 1 || sb == 1, "einsum: operands do not broadcast for a shared subscript");
      expand_label_(sa == 1 ? a : b, l, std::max(sa, sb));
    }
  }

  auto axis_of = [](const Operand &op, int l) {
    return static_cast<I64>(std::find(op.labels.begin(), op.labels.end(), l) - op.labels.begin());
  };
  Dims perm_a, perm_b, batch_sz, a_sz, b_sz;
  for (int l : batch) { perm_a.push_back(axis_of(a, l)); perm_b.push_back(axis_of(b, l)); batch_sz.push_back(size_of_(a, l)); }
  for (int l : a_only) { perm_a.push_back(axis_of(a, l)); a_sz.push_back(size_of_(a, l)); }
  for (int l : contr) perm_a.push_back(axis_of(a, l));
  for (int l : contr) perm_b.push_back(axis_of(b, l));
  for (int l : b_only) { perm_b.push_back(axis_of(b, l)); b_sz.push_back(size_of_(b, l)); }

  I64 bt = 1, m = 1, k = 1, n = 1;
  for (I64 v : batch_sz) bt *= v;
  for (I64 v : a_sz) m *= v;
  for (int l : contr) k *= size_of_(a, l);
  for (I64 v : b_sz) n *= v;

  const Series a3 = a.x.permute(ints_(perm_a)).reshape(Dims{bt, m, k});
  const Series b3 = b.x.permute(ints_(perm_b)).reshape(Dims{bt, k, n});
  Dims out_shape = batch_sz;
  out_shape.insert(out_shape.end(), a_sz.begin(), a_sz.end());
  out_shape.insert(out_shape.end(), b_sz.begin(), b_sz.end());

  Operand out;
  out.x = a3.bmm(b3).reshape(out_shape);
  out.labels = batch;
  out.labels.insert(out.labels.end(), a_only.begin(), a_only.end());
  out.labels.insert(out.labels.end(), b_only.begin(), b_only.end());
  return out;
}

} // namespace

Series Series::tensordot(const Series &other, std::int64_t dims) const {
  PLUSS_CHECK(dims >= 0, "tensordot: dims must be non-negative");
  PLUSS_CHECK(defined() && other.defined(), "tensordot: expected defined Series");
  PLUSS_CHECK(dims <= dim() && dims <= other.dim(),
              "tensordot: dims exceeds the rank of an operand");
  Dims da, db;
  for (I64 i = 0; i < dims; ++i) {
    da.push_back(dim() - dims + i);
    db.push_back(i);
  }
  return tensordot_impl_(*this, other, std::move(da), std::move(db));
}

Series Series::tensordot(const Series &other, const std::vector<std::int64_t> &dims_self,
                         const std::vector<std::int64_t> &dims_other) const {
  return tensordot_impl_(*this, other, dims_self, dims_other);
}

Series Series::einsum(std::string_view equation, const std::vector<Series> &operands) {
  PLUSS_CHECK(!operands.empty(), "einsum: expected at least one operand");

  // ---- split "lhs->rhs"
  std::string_view lhs = equation, rhs;
  bool explicit_out = false;
  if (const auto arrow = equation.find("->"); arrow != std::string_view::npos) {
    lhs = equation.substr(0, arrow);
    rhs = equation.substr(arrow + 2);
    explicit_out = true;
    PLUSS_CHECK(rhs.find("->") == std::string_view::npos && rhs.find(',') == std::string_view::npos,
                "einsum: malformed output specification");
  }
  std::vector<Term> terms;
  for (std::size_t start = 0;;) {
    const auto comma = lhs.find(',', start);
    terms.push_back(parse_term_(lhs.substr(start, comma == std::string_view::npos ? comma : comma - start)));
    if (comma == std::string_view::npos) break;
    start = comma + 1;
  }
  PLUSS_CHECK(terms.size() == operands.size(),
              "einsum: number of operands does not match the equation");

  // ---- expand ellipses to broadcast labels, right-aligned across operands
  I64 ell = 0;
  std::vector<I64> ell_of(terms.size(), 0);
  for (std::size_t i = 0; i < terms.size(); ++i) {
    PLUSS_CHECK(operands[i].defined(), "einsum: expected defined operands");
    const I64 explicit_n = static_cast<I64>(terms[i].labels.size());
    if (terms[i].has_ellipsis) {
      ell_of[i] = operands[i].dim() - explicit_n;
      PLUSS_CHECK(ell_of[i] >= 0, "einsum: operand has fewer axes than its subscripts");
      ell = std::max(ell, ell_of[i]);
    } else {
      PLUSS_CHECK(operands[i].dim() == explicit_n,
                  "einsum: operand rank does not match its subscripts");
    }
  }
  std::vector<Operand> ops(terms.size());
  for (std::size_t i = 0; i < terms.size(); ++i) {
    ops[i].x = operands[i];
    for (std::size_t p = 0; p <= terms[i].labels.size(); ++p) {
      if (terms[i].has_ellipsis && p == terms[i].ellipsis_at)
        for (I64 e = ell - ell_of[i]; e < ell; ++e) ops[i].labels.push_back(kEllipsis + static_cast<int>(e));
      if (p < terms[i].labels.size()) ops[i].labels.push_back(terms[i].labels[p]);
    }
  }
  std::vector<int> ell_labels;
  for (I64 e = 0; e < ell; ++e) ell_labels.push_back(kEllipsis + static_cast<int>(e));

  // ---- output labels
  std::vector<int> out_labels;
  if (explicit_out) {
    const Term t = parse_term_(rhs);
    for (std::size_t p = 0; p <= t.labels.size(); ++p) {
      if (t.has_ellipsis && p == t.ellipsis_at) out_labels.insert(out_labels.end(), ell_labels.begin(), ell_labels.end());
      if (p < t.labels.size()) {
        PLUSS_CHECK(!has_(out_labels, t.labels[p]), "einsum: output subscript repeated");
        out_labels.push_back(t.labels[p]);
      }
    }
    for (int l : out_labels) {
      bool seen = false;
      for (const auto &op : ops) seen = seen || has_(op.labels, l);
      PLUSS_CHECK(seen, "einsum: output subscript does not appear in any input");
    }
  } else {
    out_labels = ell_labels;
    std::vector<int> counts(52, 0);
    for (const auto &t : terms) for (int l : t.labels) ++counts[static_cast<std::size_t>(l)];
    for (int l = 0; l < 52; ++l) if (counts[static_cast<std::size_t>(l)] == 1) out_labels.push_back(l);
  }

  // ---- diagonals, then pairwise contraction left to right
  for (auto &op : ops) collapse_repeats_(op);
  Operand acc = std::move(ops[0]);
  if (ops.size() == 1) {
    reduce_unneeded_(acc, out_labels);
  }
  for (std::size_t i = 1; i < ops.size(); ++i) {
    std::vector<int> needed = out_labels;
    for (std::size_t j = i + 1; j < ops.size(); ++j)
      needed.insert(needed.end(), ops[j].labels.begin(), ops[j].labels.end());
    acc = contract_pair_(std::move(acc), std::move(ops[i]), needed);
  }
  // Single-operand path or leftovers: sum anything not in the output.
  reduce_unneeded_(acc, out_labels);

  // ---- permute to output order (output axes missing from the result can only
  // happen for an ellipsis label no operand uses, which cannot occur here).
  Dims perm;
  for (int l : out_labels) {
    const auto it = std::find(acc.labels.begin(), acc.labels.end(), l);
    PLUSS_CHECK(it != acc.labels.end(), "einsum: internal error: missing output label");
    perm.push_back(static_cast<I64>(it - acc.labels.begin()));
  }
  bool identity = true;
  for (std::size_t i = 0; i < perm.size(); ++i) identity = identity && perm[i] == static_cast<I64>(i);
  return identity ? acc.x : acc.x.permute(ints_(perm));
}

} // namespace pluss::codem
