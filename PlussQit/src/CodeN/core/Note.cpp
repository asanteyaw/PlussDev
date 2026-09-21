#include "CodeN/core/Note.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <utility>

#include "CodeN/core/OpsInternal.h"
#include "CodeN/native/cpu/TextKernels.h"

namespace pluss::coden {

namespace {

static ::std::string escape_for_debug(::std::string_view s,
                                      bool escape_non_ascii) {
  ::std::string out;
  out.reserve(s.size() + 8);
  for (unsigned char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c >= 0x20 && c != 0x7F && (!escape_non_ascii || c < 0x80)) {
          out.push_back(static_cast<char>(c));
        } else {
          out += "\\x";
          static constexpr char kHex[] = "0123456789ABCDEF";
          out.push_back(kHex[(c >> 4) & 0x0F]);
          out.push_back(kHex[c & 0x0F]);
        }
        break;
    }
  }
  return out;
}

enum class NoteCmpOp : ::std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

::std::vector<::std::int64_t> note_shape(const Note& n) {
  return n.sizes();
}

void validate_note_pair_for_text_ops(const Note& lhs, const Note& rhs, ::std::string_view where) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), ::std::string(where) + ": operands must be defined");
  PLUSS_CHECK((lhs.is_text() || lhs.is_bytes()) &&
                  (rhs.is_text() || rhs.is_bytes()),
              ::std::string(where) + ": both operands must be Text or Bytes Notes");
  const bool lhs_bytes = static_cast<const NoteBase&>(lhs).is_bytes();
  const bool rhs_bytes = static_cast<const NoteBase&>(rhs).is_bytes();
  PLUSS_CHECK(lhs_bytes == rhs_bytes, ::std::string(where) + ": mixed bytes/utf8 operands are not supported");
  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  PLUSS_CHECK(bx || by || lhs.sizes() == rhs.sizes(),
              ::std::string(where) +
                  ": expected equal shapes or scalar broadcast");
}

Note note_concat_pair(const Note& lhs, const Note& rhs) {
  validate_note_pair_for_text_ops(lhs, rhs, "Note::operator+");
  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  const auto n = (nx == ny) ? nx : (bx ? ny : nx);

  ::std::vector<::std::string> out;
  out.reserve(static_cast<::std::size_t>(n));
  for (::std::int64_t i = 0; i < n; ++i) {
    const auto x = lhs.at(bx ? 0 : i);
    const auto y = rhs.at(by ? 0 : i);
    ::std::string z;
    z.reserve(x.size() + y.size());
    z.append(x.data(), x.size());
    z.append(y.data(), y.size());
    out.push_back(::std::move(z));
  }

  ::std::vector<::std::string_view> flat;
  flat.reserve(out.size());
  for (const auto& v : out) flat.push_back(v);

  const Note& shape_src = (nx == ny) ? lhs : (bx ? rhs : lhs);
  const auto shape = note_shape(shape_src);
  if (static_cast<const NoteBase&>(lhs).is_bytes()) {
    return ops_internal::note_from_bytes(
        ::std::span<const ::std::string_view>(flat.data(), flat.size()),
        ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
  }
  return ops_internal::note_from_utf8(
      ::std::span<const ::std::string_view>(flat.data(), flat.size()),
      ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
}

Note note_repeat_scalar(const Note& n, ::std::int64_t times) {
  PLUSS_CHECK(n.defined(), "Note::operator*: operand must be defined");
  PLUSS_CHECK(n.is_text() || n.is_bytes(),
              "Note::operator*: operand must be a Text or Bytes Note");
  PLUSS_CHECK(times >= 0, "Note::operator*: repeat count must be >= 0");
  ::std::vector<::std::string> out;
  out.reserve(static_cast<::std::size_t>(n.numel()));
  for (::std::int64_t i = 0; i < n.numel(); ++i) {
    const auto s = n.at(i);
    ::std::string z;
    PLUSS_CHECK(times == 0 ||
                    s.size() <= z.max_size() / static_cast<::std::size_t>(times),
                "Note::operator*: repeated cell exceeds maximum string size");
    const auto reserve = static_cast<::std::size_t>(times) * s.size();
    z.reserve(reserve);
    for (::std::int64_t k = 0; k < times; ++k) {
      z.append(s.data(), s.size());
    }
    out.push_back(::std::move(z));
  }
  ::std::vector<::std::string_view> flat;
  flat.reserve(out.size());
  for (const auto& v : out) flat.push_back(v);
  const auto shape = note_shape(n);
  if (static_cast<const NoteBase&>(n).is_bytes()) {
    return ops_internal::note_from_bytes(
        ::std::span<const ::std::string_view>(flat.data(), flat.size()),
        ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
  }
  return ops_internal::note_from_utf8(
      ::std::span<const ::std::string_view>(flat.data(), flat.size()),
      ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
}

// Dtype-generic value comparison, used for Bool/Int64/Float64 Note-vs-Note
// compares. Text/Bytes keeps its own lexical path below (native SIMD batch
// compare over spans of string_view, distinct from this scalar template).
template <class T>
::std::uint8_t apply_numeric_cmp_(T a, T b, NoteCmpOp op) {
  switch (op) {
    case NoteCmpOp::Eq: return static_cast<::std::uint8_t>(a == b);
    case NoteCmpOp::Ne: return static_cast<::std::uint8_t>(a != b);
    case NoteCmpOp::Lt: return static_cast<::std::uint8_t>(a < b);
    case NoteCmpOp::Le: return static_cast<::std::uint8_t>(a <= b);
    case NoteCmpOp::Gt: return static_cast<::std::uint8_t>(a > b);
    case NoteCmpOp::Ge: return static_cast<::std::uint8_t>(a >= b);
  }
  return 0;
}

void validate_note_pair_for_compare_(const Note& lhs, const Note& rhs, ::std::string_view where) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), ::std::string(where) + ": operands must be defined");
  PLUSS_CHECK(lhs.dtype() == rhs.dtype(), ::std::string(where) + ": mixed dtype operands are not supported");
  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  PLUSS_CHECK(bx || by || lhs.sizes() == rhs.sizes(),
              ::std::string(where) +
                  ": expected equal shapes or scalar broadcast");
}

template <class Span>
Note note_compare_numeric_(const Note& lhs, const Note& rhs, NoteCmpOp op,
                           Span lhs_values, Span rhs_values) {
  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  const auto n = (nx == ny) ? nx : (bx ? ny : nx);
  NoteImpl::BoolStorage out(static_cast<::std::size_t>(n));
  for (::std::int64_t i = 0; i < n; ++i) {
    out[static_cast<::std::size_t>(i)] = apply_numeric_cmp_(
        lhs_values[static_cast<::std::size_t>(bx ? 0 : i)],
        rhs_values[static_cast<::std::size_t>(by ? 0 : i)], op);
  }
  const Note& shape_src = (nx == ny) ? lhs : (bx ? rhs : lhs);
  const auto shape = note_shape(shape_src);
  return ops_internal::note_from_bool_storage(
      ::std::move(out),
      ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
}

bool note_compare_strings(::std::string_view lhs, ::std::string_view rhs, NoteCmpOp op) {
  using native_op = ::pluss::coden::native::cpu::LexCompareOp;
  const auto map_op = [&]() -> native_op {
    switch (op) {
      case NoteCmpOp::Eq:
        return native_op::Eq;
      case NoteCmpOp::Ne:
        return native_op::Ne;
      case NoteCmpOp::Lt:
        return native_op::Lt;
      case NoteCmpOp::Le:
        return native_op::Le;
      case NoteCmpOp::Gt:
        return native_op::Gt;
      case NoteCmpOp::Ge:
        return native_op::Ge;
      default:
        return native_op::Eq;
    }
  }();
  return ::pluss::coden::native::cpu::compare_lex_bytes(lhs, rhs, map_op);
}

Note note_compare_pair(const Note& lhs, const Note& rhs, NoteCmpOp op) {
  validate_note_pair_for_compare_(lhs, rhs, "Note::compare");

  switch (lhs.dtype()) {
    case NoteDType::Bool:
      return note_compare_numeric_(lhs, rhs, op, lhs.bools(), rhs.bools());
    case NoteDType::Int64:
      return note_compare_numeric_(lhs, rhs, op, lhs.integers(), rhs.integers());
    case NoteDType::Float64:
      return note_compare_numeric_(lhs, rhs, op, lhs.reals(), rhs.reals());
    case NoteDType::Text:
    case NoteDType::Bytes:
      break; // Lexical text comparison continues below.
  }

  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  const auto n = (nx == ny) ? nx : (bx ? ny : nx);
  NoteImpl::BoolStorage out(static_cast<::std::size_t>(n), 0);

  const auto wrap = [&]() {
    const Note& shape_src = (nx == ny) ? lhs : (bx ? rhs : lhs);
    const auto shape = note_shape(shape_src);
    return ops_internal::note_from_bool_storage(
        ::std::move(out),
        ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
  };

  if (!bx && !by) {
    ::std::vector<::std::string_view> lhs_flat(static_cast<::std::size_t>(n));
    ::std::vector<::std::string_view> rhs_flat(static_cast<::std::size_t>(n));
    for (::std::int64_t i = 0; i < n; ++i) {
      lhs_flat[static_cast<::std::size_t>(i)] = lhs.at(i);
      rhs_flat[static_cast<::std::size_t>(i)] = rhs.at(i);
    }
    using native_op = ::pluss::coden::native::cpu::LexCompareOp;
    const auto map_op = [&]() -> native_op {
      switch (op) {
        case NoteCmpOp::Eq:
          return native_op::Eq;
        case NoteCmpOp::Ne:
          return native_op::Ne;
        case NoteCmpOp::Lt:
          return native_op::Lt;
        case NoteCmpOp::Le:
          return native_op::Le;
        case NoteCmpOp::Gt:
          return native_op::Gt;
        case NoteCmpOp::Ge:
          return native_op::Ge;
        default:
          return native_op::Eq;
      }
    }();
    ::pluss::coden::native::cpu::compare_lex_bytes_batch(
        ::std::span<const ::std::string_view>(lhs_flat.data(), lhs_flat.size()),
        ::std::span<const ::std::string_view>(rhs_flat.data(), rhs_flat.size()),
        map_op,
        ::std::span<::std::uint8_t>(out.data(), out.size()));
    return wrap();
  }

  for (::std::int64_t i = 0; i < n; ++i) {
    out[static_cast<::std::size_t>(i)] =
        static_cast<::std::uint8_t>(note_compare_strings(lhs.at(bx ? 0 : i), rhs.at(by ? 0 : i), op));
  }
  return wrap();
}

Note note_compare_scalar(const Note& lhs, ::std::string_view rhs, NoteCmpOp op) {
  PLUSS_CHECK(lhs.defined(), "Note::compare: lhs must be defined");
  PLUSS_CHECK(lhs.is_text() || lhs.is_bytes(),
              "Note::compare: comparing against a string requires a Text or Bytes Note");
  NoteImpl::BoolStorage out(static_cast<::std::size_t>(lhs.numel()), 0);
  for (::std::int64_t i = 0; i < lhs.numel(); ++i) {
    out[static_cast<::std::size_t>(i)] =
        static_cast<::std::uint8_t>(note_compare_strings(lhs.at(i), rhs, op));
  }
  const auto shape = note_shape(lhs);
  return ops_internal::note_from_bool_storage(
      ::std::move(out),
      ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
}

} // namespace

NoteBase::value_type NoteBase::flat_at(index_type linear) const {
  PLUSS_CHECK(defined(), "flat_at() on undefined Note");
  PLUSS_CHECK(linear >= 0 && linear < numel(), "flat_at() index out of range");
  return impl_->bytes_at(linear);
}

NoteBase::index_type NoteBase::linear_index(::std::span<const index_type> indices) const {
  PLUSS_CHECK(defined(), "Note indexing requires a defined Note");
  PLUSS_CHECK(indices.size() == static_cast<::std::size_t>(dim()), "Note index rank mismatch");

  if (indices.empty()) return 0;

  const auto s = sizes3();
  index_type linear = 0;
  for (index_type d = 0; d < dim(); ++d) {
    const auto idx = indices[static_cast<::std::size_t>(d)];
    const auto size_d = s[static_cast<::std::size_t>(d)];
    PLUSS_CHECK(idx >= 0 && idx < size_d, "Note index out of bounds");
    linear = linear * size_d + idx;
  }
  return linear;
}

NoteBase::value_type NoteBase::at_indices(::std::span<const index_type> indices) const {
  return flat_at(linear_index(indices));
}

NoteBase::value_type NoteBase::scalar() const {
  PLUSS_CHECK(defined(), "scalar() on undefined Note");
  PLUSS_CHECK(numel() == 1, "scalar() requires exactly one element");
  return impl_->bytes_at(0);
}

::std::vector<NoteBase::value_type> NoteBase::vector() const {
  PLUSS_CHECK(defined(), "vector() on undefined Note");

  ::std::vector<value_type> out;
  out.reserve(static_cast<::std::size_t>(numel()));
  copy_flat_to(::std::back_inserter(out));
  return out;
}

::std::vector<Note> NoteBase::matrix() const {
  PLUSS_CHECK(defined(), "matrix() on undefined Note");
  PLUSS_CHECK(dim() <= 2, "matrix() requires scalar, 1D, or 2D Note");

  if (dim() <= 1) return {static_cast<const Note &>(*this)};

  const auto rows = size(0);
  const auto cols = size(1);
  ::std::vector<Note> out;
  out.reserve(static_cast<::std::size_t>(rows));
  const ::std::array<index_type, 1> row_shape{cols};
  for (index_type r = 0; r < rows; ++r) {
    out.push_back(ops_internal::note_contiguous_view(
        static_cast<const Note &>(*this), r * cols, row_shape));
  }
  return out;
}

::std::vector<Note> NoteBase::slabs() const {
  PLUSS_CHECK(defined(), "slabs() on undefined Note");
  PLUSS_CHECK(dim() == 3, "slabs() requires a rank-3 Note");

  const auto d0 = size(0);
  const auto d1 = size(1);
  const auto d2 = size(2);
  ::std::vector<Note> out;
  out.reserve(static_cast<::std::size_t>(d0));
  const ::std::array<index_type, 2> slab_shape{d1, d2};
  for (index_type i = 0; i < d0; ++i) {
    out.push_back(ops_internal::note_contiguous_view(
        static_cast<const Note &>(*this), i * d1 * d2, slab_shape));
  }
  return out;
}

bool NoteBase::bool_item() const {
  PLUSS_CHECK(defined() && numel() == 1,
              "Note::bool_item() requires one defined element");
  return bools().front() != 0;
}

::std::int64_t NoteBase::int_item() const {
  PLUSS_CHECK(defined() && numel() == 1,
              "Note::int_item() requires one defined element");
  return integers().front();
}

double NoteBase::float_item() const {
  PLUSS_CHECK(defined() && numel() == 1,
              "Note::float_item() requires one defined element");
  return reals().front();
}

::std::span<const ::std::uint8_t> NoteBase::bools() const {
  PLUSS_CHECK(defined(), "Note::bools() on undefined Note");
  return impl_->bool_values();
}

::std::span<const ::std::int64_t> NoteBase::integers() const {
  PLUSS_CHECK(defined(), "Note::integers() on undefined Note");
  return impl_->int64_values();
}

::std::span<const double> NoteBase::reals() const {
  PLUSS_CHECK(defined(), "Note::reals() on undefined Note");
  return impl_->float64_values();
}

NoteBase::HostAny NoteBase::host() const {
  PLUSS_CHECK(defined(), "Note::host(): undefined");

  if (is_scalar()) {
    if (is_text() || is_bytes())
      return HostAny::from_scalar(::std::string(scalar()), dtype());
    if (is_bool()) return HostAny::from_scalar(bool_item(), dtype());
    if (is_int64()) return HostAny::from_scalar(int_item(), dtype());
    return HostAny::from_scalar(float_item(), dtype());
  }

  ::std::vector<::std::string> text_data;
  ::std::vector<::std::uint8_t> bool_data;
  ::std::vector<::std::int64_t> int_data;
  ::std::vector<double> float_data;
  if (is_text() || is_bytes()) {
    text_data.reserve(static_cast<::std::size_t>(numel()));
    for (index_type i = 0; i < numel(); ++i) text_data.emplace_back(flat_at(i));
  } else if (is_bool()) {
    const auto values = bools();
    bool_data.assign(values.begin(), values.end());
  } else if (is_int64()) {
    const auto values = integers();
    int_data.assign(values.begin(), values.end());
  } else {
    const auto values = reals();
    float_data.assign(values.begin(), values.end());
  }

  if (is_vector()) {
    if (is_text() || is_bytes()) return HostAny::from_vector(::std::move(text_data), dtype());
    if (is_bool()) return HostAny::from_vector(::std::move(bool_data), dtype());
    if (is_int64()) return HostAny::from_vector(::std::move(int_data), dtype());
    return HostAny::from_vector(::std::move(float_data), dtype());
  }

  HostArray array;
  array.data = ::std::move(text_data);
  array.bool_data = ::std::move(bool_data);
  array.int_data = ::std::move(int_data);
  array.float_data = ::std::move(float_data);
  array.dtype = dtype();
  const auto extents = impl_->sizes();
  array.sizes.assign(extents.begin(), extents.end());
  return HostAny::from_array(::std::move(array));
}

::std::string NoteBase::join_flat(::std::string_view sep) const {
  PLUSS_CHECK(defined(), "join_flat() on undefined Note");
  if (numel() == 0) return {};

  ::std::string out;
  const auto n = numel();
  for (index_type i = 0; i < n; ++i) {
    if (i != 0) out.append(sep);
    out.append(impl_->bytes_at(i));
  }
  return out;
}

::std::string NoteBase::debug(index_type max_items) const {
  if (!defined()) return "Note(undefined)";
  if (max_items < 0) max_items = 0;

  ::std::ostringstream oss;
  oss << "Note(shape=[";
  const auto shp = impl_->sizes();
  for (::std::size_t i = 0; i < shp.size(); ++i) {
    if (i != 0) oss << ", ";
    oss << shp[i];
  }
  oss << "], dim=" << dim() << ", numel=" << numel();
  if (is_text() || is_bytes())
    oss << ", encoding=" << (is_bytes() ? "bytes" : "utf8");
  else
    oss << ", dtype=" << dtype_name(dtype());
  oss << ", values=[";

  const auto shown = ::std::min<index_type>(numel(), max_items);
  for (index_type i = 0; i < shown; ++i) {
    if (i != 0) oss << ", ";
    if (is_text() || is_bytes()) {
      oss << '"' << escape_for_debug(impl_->bytes_at(i), is_bytes()) << '"';
    } else if (is_bool()) {
      oss << (impl_->bool_values()[static_cast<::std::size_t>(i)] ? "true" : "false");
    } else if (is_int64()) {
      oss << impl_->int64_values()[static_cast<::std::size_t>(i)];
    } else {
      oss << impl_->float64_values()[static_cast<::std::size_t>(i)];
    }
  }
  if (numel() > shown) oss << (shown == 0 ? "..." : ", ...");
  oss << "])";
  return oss.str();
}

::std::vector<::std::int64_t> Note::sizes() const {
  PLUSS_CHECK(defined(), "Note::sizes(): undefined Note");
  const auto extents = impl()->sizes();
  return {extents.begin(), extents.end()};
}

::std::span<const ::std::int64_t> Note::sizes_view() const {
  PLUSS_CHECK(defined(), "Note::sizes_view on undefined Note");
  return impl()->sizes();
}

::std::string_view Note::at(::std::int64_t linear) const {
  PLUSS_CHECK(defined(), "Note::at on undefined Note");
  const auto& impl = this->impl();
  PLUSS_CHECK(linear >= 0 && linear < impl->numel(), "Note::at index out of range");
  return impl->bytes_at(linear);
}

TextRef Note::cell(::std::int64_t linear) const {
  PLUSS_CHECK(defined(), "Note::cell on undefined Note");
  const auto& impl = this->impl();
  PLUSS_CHECK(linear >= 0 && linear < impl->numel(), "Note::cell index out of range");
  const auto& span = impl->spans()[static_cast<::std::size_t>(linear)];
  return TextRef(impl->arena(), span.first, span.second);
}

TextRef Note::cell_at(::std::span<const ::std::int64_t> indices) const {
  return cell(linear_index(indices));
}

TextRef Note::owned() const {
  PLUSS_CHECK(defined(), "Note::owned on undefined Note");
  PLUSS_CHECK(numel() == 1, "Note::owned requires exactly one element");
  return cell(0);
}

Note Note::take(pluss::msharp::RangeView::Int indices) const {
  if (!defined()) return Note::undefined();
  const auto& p = impl();
  PLUSS_CHECK(p, "Note::take on undefined Note");

  // take() gathers over the flattened (row-major) cells regardless of rank and
  // always returns a rank-1 Note of the picked cells.
  const auto n = p->numel();
  const auto normalize = [n](::std::int64_t idx) {
    if (idx < 0) idx += n;
    PLUSS_CHECK(idx >= 0 && idx < n, "Note::take index out of range");
    return idx;
  };
  for (const auto idx_raw : indices) {
    ::std::int64_t idx = idx_raw;
    (void)normalize(idx);
  }

  ::std::array<::std::int64_t, 3> shape{0, 1, 1};
  shape[0] = static_cast<::std::int64_t>(indices.size());
  switch (p->dtype()) {
    case NoteDType::Text:
    case NoteDType::Bytes: {
      const auto& src = p->spans();
      ::std::vector<NoteImpl::OffLen> out;
      out.reserve(indices.size());
      for (const auto raw : indices) {
        const auto idx = normalize(raw);
        out.push_back(src[static_cast<::std::size_t>(idx)]);
      }
      return Note(::pluss::flame::make_intrusive<NoteImpl>(
          p->arena(), ::std::move(out), shape, 1, p->dtype()));
    }
    case NoteDType::Bool: {
      const auto src = p->bool_values();
      NoteImpl::BoolStorage out(indices.size());
      ::std::size_t i = 0;
      for (const auto raw : indices) {
        out[i++] = src[static_cast<::std::size_t>(normalize(raw))];
      }
      return Note(::pluss::flame::make_intrusive<NoteImpl>(
          ::std::move(out), shape, 1));
    }
    case NoteDType::Int64: {
      const auto src = p->int64_values();
      NoteImpl::Int64Storage out(indices.size());
      ::std::size_t i = 0;
      for (const auto raw : indices) {
        out[i++] = src[static_cast<::std::size_t>(normalize(raw))];
      }
      return Note(::pluss::flame::make_intrusive<NoteImpl>(
          ::std::move(out), shape, 1));
    }
    case NoteDType::Float64: {
      const auto src = p->float64_values();
      NoteImpl::Float64Storage out(indices.size());
      ::std::size_t i = 0;
      for (const auto raw : indices) {
        out[i++] = src[static_cast<::std::size_t>(normalize(raw))];
      }
      return Note(::pluss::flame::make_intrusive<NoteImpl>(
          ::std::move(out), shape, 1));
    }
  }
  PLUSS_CHECK(false, "Note::take encountered an invalid dtype");
}

Note Note::take(const Note& indices) const {
  PLUSS_CHECK(indices.defined(), "Note::take indices must be defined");
  PLUSS_CHECK(indices.is_int64(), "Note::take indices must be an Int64 Note");
  return take(indices.integers());
}

Note Note::operator+(const Note& other) const { return note_concat_pair(*this, other); }
Note Note::operator+(::std::string_view suffix) const {
  const ::std::array<::std::string_view, 1> flat{suffix};
  auto scalar = static_cast<const NoteBase&>(*this).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return note_concat_pair(*this, scalar);
}
Note& Note::operator+=(const Note& other) {
  *this = *this + other;
  return *this;
}
Note& Note::operator+=(::std::string_view suffix) {
  *this = *this + suffix;
  return *this;
}
Note Note::operator*(::std::int64_t times) const { return note_repeat_scalar(*this, times); }
Note& Note::operator*=(::std::int64_t times) {
  *this = *this * times;
  return *this;
}

Note Note::operator==(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Eq);
}
Note Note::operator!=(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Ne);
}
Note Note::operator<(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Lt);
}
Note Note::operator<=(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Le);
}
Note Note::operator>(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Gt);
}
Note Note::operator>=(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Ge);
}

Note Note::operator==(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Eq);
}
Note Note::operator!=(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Ne);
}
Note Note::operator<(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Lt);
}
Note Note::operator<=(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Le);
}
Note Note::operator>(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Gt);
}
Note Note::operator>=(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Ge);
}

namespace {

enum class NoteBoolOp : ::std::uint8_t { And = 0, Or = 1, Xor = 2 };

Note note_bool_pair(const Note& lhs, const Note& rhs, NoteBoolOp op, const char* where) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), ::std::string(where) + ": operands must be defined");
  PLUSS_CHECK(lhs.is_bool() && rhs.is_bool(), ::std::string(where) + ": both operands must be Bool Notes");
  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  PLUSS_CHECK(bx || by || lhs.sizes() == rhs.sizes(),
              ::std::string(where) +
                  ": expected equal shapes or scalar broadcast");
  const auto n = (nx == ny) ? nx : (bx ? ny : nx);

  const auto lhs_bools = lhs.bools();
  const auto rhs_bools = rhs.bools();
  NoteImpl::BoolStorage out(static_cast<::std::size_t>(n));
  auto write = [&](auto combine) {
    if (bx) {
      const auto scalar = lhs_bools.front();
      for (::std::int64_t i = 0; i < n; ++i)
        out[static_cast<::std::size_t>(i)] =
            combine(scalar, rhs_bools[static_cast<::std::size_t>(i)]);
    } else if (by) {
      const auto scalar = rhs_bools.front();
      for (::std::int64_t i = 0; i < n; ++i)
        out[static_cast<::std::size_t>(i)] =
            combine(lhs_bools[static_cast<::std::size_t>(i)], scalar);
    } else {
      for (::std::int64_t i = 0; i < n; ++i)
        out[static_cast<::std::size_t>(i)] = combine(
            lhs_bools[static_cast<::std::size_t>(i)],
            rhs_bools[static_cast<::std::size_t>(i)]);
    }
  };
  switch (op) {
    case NoteBoolOp::And:
      write([](auto a, auto b) { return static_cast<::std::uint8_t>(a & b); });
      break;
    case NoteBoolOp::Or:
      write([](auto a, auto b) { return static_cast<::std::uint8_t>(a | b); });
      break;
    case NoteBoolOp::Xor:
      write([](auto a, auto b) { return static_cast<::std::uint8_t>(a ^ b); });
      break;
  }
  const Note& shape_src = (nx == ny) ? lhs : (bx ? rhs : lhs);
  const auto shape = note_shape(shape_src);
  return ops_internal::note_from_bool_storage(
      ::std::move(out),
      ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
}

} // namespace

Note Note::operator&(const Note& other) const {
  return note_bool_pair(*this, other, NoteBoolOp::And, "Note::operator&");
}
Note Note::operator|(const Note& other) const {
  return note_bool_pair(*this, other, NoteBoolOp::Or, "Note::operator|");
}
Note Note::operator^(const Note& other) const {
  return note_bool_pair(*this, other, NoteBoolOp::Xor, "Note::operator^");
}
Note Note::operator~() const {
  PLUSS_CHECK(defined(), "Note::operator~: operand must be defined");
  PLUSS_CHECK(is_bool(), "Note::operator~: operand must be a Bool Note");
  const auto lanes = bools();
  NoteImpl::BoolStorage out(lanes.size());
  for (::std::size_t i = 0; i < lanes.size(); ++i) {
    out[i] = static_cast<::std::uint8_t>(lanes[i] == 0);
  }
  const auto shape = sizes();
  return ops_internal::note_from_bool_storage(
      ::std::move(out),
      ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
}

Note Note::any() const {
  PLUSS_CHECK(defined(), "Note::any: operand must be defined");
  PLUSS_CHECK(is_bool(), "Note::any: operand must be a Bool Note");
  const auto lanes = bools();
  NoteImpl::BoolStorage out{static_cast<::std::uint8_t>(
      ::std::any_of(lanes.begin(), lanes.end(),
                    [](::std::uint8_t b) { return b != 0; }))};
  return ops_internal::note_from_bool_storage(::std::move(out), {});
}

Note Note::all() const {
  PLUSS_CHECK(defined(), "Note::all: operand must be defined");
  PLUSS_CHECK(is_bool(), "Note::all: operand must be a Bool Note");
  const auto lanes = bools();
  NoteImpl::BoolStorage out{static_cast<::std::uint8_t>(
      ::std::all_of(lanes.begin(), lanes.end(),
                    [](::std::uint8_t b) { return b != 0; }))};
  return ops_internal::note_from_bool_storage(::std::move(out), {});
}

Note Note::argsort(bool descending, bool stable) const {
  PLUSS_CHECK(defined(), "Note::argsort on undefined Note");
  auto order = ops_internal::note_argsort_indices_1d(*this, descending, stable);
  const ::std::array<::std::int64_t, 1> shape{static_cast<::std::int64_t>(order.size())};
  return ops_internal::note_from_int64_storage(::std::move(order), shape);
}

Note operator+(::std::string_view prefix, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{prefix};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar + rhs;
}

Note operator*(::std::int64_t times, const Note& rhs) { return rhs * times; }

Note operator==(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar == rhs;
}
Note operator!=(::std::string_view lhs, const Note& rhs) {
  return ~(lhs == rhs);
}
Note operator<(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar < rhs;
}
Note operator<=(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar <= rhs;
}
Note operator>(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar > rhs;
}
Note operator>=(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar >= rhs;
}

} // namespace pluss::coden
