#include "CodeN/core/OpsInternal.h"

#include <array>
#include <string>
#include <utility>
#include <regex>

#include "CodeN/core/TextArena.h"
#include "CodeN/core/TokenBatchImpl.h"
#include "CodeN/native/cpu/TextKernels.h"

namespace pluss::coden::ops_internal {

namespace {

static ::std::array<::std::int64_t,3> strides3(const ::std::array<::std::int64_t,3>& sizes, ::std::int64_t dim) {
  ::std::array<::std::int64_t,3> st{1,1,1};
  ::std::int64_t acc = 1;
  for (::std::int64_t i = dim - 1; i >= 0; --i) {
    st[static_cast<::std::size_t>(i)] = acc;
    acc *= sizes[static_cast<::std::size_t>(i)];
  }
  return st;
}

static void unravel(::std::int64_t linear,
                    const ::std::array<::std::int64_t,3>& sizes,
                    ::std::int64_t dim,
                    ::std::array<::std::int64_t,3>& out_coords) {
  out_coords = {0,0,0};
  for (::std::int64_t i = dim - 1; i >= 0; --i) {
    const auto s = sizes[static_cast<::std::size_t>(i)];
    out_coords[static_cast<::std::size_t>(i)] = (s == 0) ? 0 : (linear % s);
    linear = (s == 0) ? 0 : (linear / s);
  }
}

static ::std::int64_t ravel(const ::std::array<::std::int64_t,3>& coords,
                          const ::std::array<::std::int64_t,3>& strides,
                          ::std::int64_t dim) {
  ::std::int64_t linear = 0;
  for (::std::int64_t i = 0; i < dim; ++i) {
    linear += coords[static_cast<::std::size_t>(i)] * strides[static_cast<::std::size_t>(i)];
  }
  return linear;
}

static void check_dim(const Note& x, ::std::int64_t d) {
  PLUSS_CHECK(x.defined(), "Note op on undefined Note");
  const auto dim = x.dim();
  PLUSS_CHECK(d >= 0 && d < dim, "dim out of range");
}

} // namespace

// Internal construction hooks.
struct ChordFactoryAccess {
  static Chord wrap(::pluss::flame::intrusive_ptr<ChordImpl> impl) {
    return Chord(::std::move(impl));
  }
};

struct NoteFactoryAccess {
  static Note wrap(::pluss::flame::intrusive_ptr<NoteImpl> impl) {
    return Note(::std::move(impl));
  }
};

Chord chord_from_utf8(::std::string_view utf8) {
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  auto [off, len] = arena->append(utf8);
  auto impl = ::pluss::flame::make_intrusive<ChordImpl>(arena, off, len);
  return ChordFactoryAccess::wrap(::std::move(impl));
}

Note note_from_utf8(::std::span<const ::std::string_view> utf8_flat,
                    ::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(sizes.size() >= 0 && sizes.size() <= 3, "Note supports up to 3D");
  ::std::array<::std::int64_t, 3> sz{1,1,1};
  for (::std::size_t i = 0; i < sizes.size(); ++i) sz[i] = sizes[i];

  ::std::int64_t dim = static_cast<::std::int64_t>(sizes.size());
  ::std::int64_t n = 1;
  for (int i = 0; i < dim; ++i) n *= sz[i];
  PLUSS_CHECK(static_cast<::std::int64_t>(utf8_flat.size()) == n,
              "note_from_utf8 expects flat size == numel");

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(utf8_flat.size());
  for (auto s : utf8_flat) {
    spans.push_back(arena->append(s));
  }
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), sz, dim, NoteImpl::Encoding::Utf8Text);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

Note note_from_bytes(::std::span<const ::std::string_view> bytes_flat,
                     ::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(sizes.size() >= 0 && sizes.size() <= 3, "Note supports up to 3D");
  ::std::array<::std::int64_t, 3> sz{1,1,1};
  for (::std::size_t i = 0; i < sizes.size(); ++i) sz[i] = sizes[i];

  ::std::int64_t dim = static_cast<::std::int64_t>(sizes.size());
  ::std::int64_t n = 1;
  for (int i = 0; i < dim; ++i) n *= sz[i];
  PLUSS_CHECK(static_cast<::std::int64_t>(bytes_flat.size()) == n,
              "note_from_bytes expects flat size == numel");

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(bytes_flat.size());
  for (auto b : bytes_flat) {
    spans.push_back(arena->append(b));
  }
  auto impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), sz, dim, NoteImpl::Encoding::Bytes);
  return NoteFactoryAccess::wrap(::std::move(impl));
}

static void require_text_note(const NoteImpl& impl, const char* op) {
  PLUSS_CHECK(!impl.is_bytes(), ::std::string(op) + " requires a UTF-8 text Note (not bytes)");
}

Chord lower(const Chord& x) {
  if (!x.defined()) return Chord{};
  ::std::string out = ::pluss::coden::native::cpu::lower_ascii(x.utf8());
  return chord_from_utf8(out);
}

Chord strip(const Chord& x) {
  if (!x.defined()) return Chord{};
  ::std::string out = ::pluss::coden::native::cpu::strip_ascii(x.utf8());
  return chord_from_utf8(out);
}

bool contains(const Chord& x, ::std::string_view needle) {
  if (!x.defined()) return false;
  return ::pluss::coden::native::cpu::contains_bytes(x.utf8(), needle);
}

Chord replace(const Chord& x, ::std::string_view from, ::std::string_view to) {
  if (!x.defined()) return Chord{};
  ::std::string out = ::pluss::coden::native::cpu::replace_bytes(x.utf8(), from, to);
  return chord_from_utf8(out);
}

Note lower(const Note& x) {
  if (!x.defined()) return Note{};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::lower");
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    ::std::string out = ::pluss::coden::native::cpu::lower_ascii(impl->bytes_at(i));
    spans.push_back(arena->append(out));
  }
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note strip(const Note& x) {
  if (!x.defined()) return Note{};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::strip");
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    ::std::string out = ::pluss::coden::native::cpu::strip_ascii(impl->bytes_at(i));
    spans.push_back(arena->append(out));
  }
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note replace(const Note& x, ::std::string_view from, ::std::string_view to) {
  if (!x.defined()) return Note{};
  const auto& impl = NoteAccess::impl(x);
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    ::std::string out = ::pluss::coden::native::cpu::replace_bytes(impl->bytes_at(i), from, to);
    spans.push_back(arena->append(out));
  }
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

::std::vector<::std::uint8_t> contains(const Note& x, ::std::string_view needle) {
  ::std::vector<::std::uint8_t> out;
  if (!x.defined()) return out;
  const auto& impl = NoteAccess::impl(x);
  out.resize(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    out[static_cast<::std::size_t>(i)] = ::pluss::coden::native::cpu::contains_bytes(impl->bytes_at(i), needle) ? 1 : 0;
  }
  return out;
}

::std::vector<::std::int64_t> lengths_bytes(const Note& x) {
  ::std::vector<::std::int64_t> out;
  if (!x.defined()) return out;
  const auto& impl = NoteAccess::impl(x);
  out.resize(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i = 0; i < impl->numel(); ++i) {
    out[static_cast<::std::size_t>(i)] = static_cast<::std::int64_t>(impl->bytes_at(i).size());
  }
  return out;
}

Note reshape(const Note& x, ::std::span<const ::std::int64_t> sizes) {
  PLUSS_CHECK(x.defined(), "reshape on undefined Note");
  PLUSS_CHECK(sizes.size() >= 0 && sizes.size() <= 3, "Note supports up to 3D");
  const auto& impl = NoteAccess::impl(x);
  ::std::array<::std::int64_t,3> sz{1,1,1};
  ::std::int64_t dim = static_cast<::std::int64_t>(sizes.size());
  ::std::int64_t n = 1;
  for (int i = 0; i < dim; ++i) {
    sz[static_cast<::std::size_t>(i)] = sizes[static_cast<::std::size_t>(i)];
    n *= sizes[static_cast<::std::size_t>(i)];
  }
  PLUSS_CHECK(n == impl->numel(), "reshape numel mismatch");
  // share arena/spans: view-only reshape
  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->arena(), ::std::vector<NoteImpl::OffLen>(impl->spans()), sz, dim, impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note permute(const Note& x, ::std::span<const ::std::int64_t> dims) {
  PLUSS_CHECK(x.defined(), "permute on undefined Note");
  const auto& impl = NoteAccess::impl(x);
  const auto dim = impl->dim();
  PLUSS_CHECK(static_cast<::std::int64_t>(dims.size()) == dim, "permute expects dims.size() == dim");

  // Validate permutation.
  ::std::array<bool,3> seen{false,false,false};
  for (::std::int64_t i = 0; i < dim; ++i) {
    const auto d = dims[static_cast<::std::size_t>(i)];
    PLUSS_CHECK(d >= 0 && d < dim, "permute dim out of range");
    PLUSS_CHECK(!seen[static_cast<::std::size_t>(d)], "permute dims must be a permutation");
    seen[static_cast<::std::size_t>(d)] = true;
  }

  const auto old_sizes = impl->sizes3();
  const auto old_strides = strides3(old_sizes, dim);

  ::std::array<::std::int64_t,3> new_sizes{1,1,1};
  for (::std::int64_t i = 0; i < dim; ++i) {
    new_sizes[static_cast<::std::size_t>(i)] = old_sizes[static_cast<::std::size_t>(dims[static_cast<::std::size_t>(i)])];
  }
  const auto new_strides = strides3(new_sizes, dim);

  ::std::vector<NoteImpl::OffLen> new_spans;
  new_spans.resize(static_cast<::std::size_t>(impl->numel()));

  ::std::array<::std::int64_t,3> new_c{0,0,0};
  ::std::array<::std::int64_t,3> old_c{0,0,0};
  for (::std::int64_t new_lin = 0; new_lin < impl->numel(); ++new_lin) {
    unravel(new_lin, new_sizes, dim, new_c);
    old_c = {0,0,0};
    for (::std::int64_t i = 0; i < dim; ++i) {
      const auto old_d = dims[static_cast<::std::size_t>(i)];
      old_c[static_cast<::std::size_t>(old_d)] = new_c[static_cast<::std::size_t>(i)];
    }
    const auto old_lin = ravel(old_c, old_strides, dim);
    new_spans[static_cast<::std::size_t>(new_lin)] = impl->spans().at(static_cast<::std::size_t>(old_lin));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->arena(), ::std::move(new_spans), new_sizes, dim, impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note transpose(const Note& x, ::std::int64_t dim0, ::std::int64_t dim1) {
  PLUSS_CHECK(x.defined(), "transpose on undefined Note");
  const auto d = x.dim();
  PLUSS_CHECK(dim0 >= 0 && dim0 < d && dim1 >= 0 && dim1 < d, "transpose dim out of range");
  ::std::array<::std::int64_t,3> order{0,1,2};
  if (d > 0) {
    ::std::swap(order[static_cast<::std::size_t>(dim0)], order[static_cast<::std::size_t>(dim1)]);
  }
  ::std::vector<::std::int64_t> v;
  v.reserve(static_cast<::std::size_t>(d));
  for (::std::int64_t i = 0; i < d; ++i) v.push_back(order[static_cast<::std::size_t>(i)]);
  return permute(x, v);
}

Note select(const Note& x, ::std::int64_t dim, ::std::int64_t index) {
  check_dim(x, dim);
  const auto& impl = NoteAccess::impl(x);
  const auto old_sizes = impl->sizes3();
  const auto old_dim = impl->dim();
  PLUSS_CHECK(old_dim > 0, "select expects dim() > 0");
  const auto size_d = old_sizes[static_cast<::std::size_t>(dim)];
  PLUSS_CHECK(index >= 0 && index < size_d, "select index out of range");

  // Build new sizes (remove dim).
  ::std::array<::std::int64_t,3> new_sizes{1,1,1};
  ::std::int64_t new_dim = old_dim - 1;
  for (::std::int64_t i = 0, j = 0; i < old_dim; ++i) {
    if (i == dim) continue;
    new_sizes[static_cast<::std::size_t>(j++)] = old_sizes[static_cast<::std::size_t>(i)];
  }

  const auto old_strides = strides3(old_sizes, old_dim);
  ::std::vector<NoteImpl::OffLen> new_spans;
  const ::std::int64_t new_numel = (new_dim == 0) ? 1 : (new_sizes[0] * (new_dim > 1 ? new_sizes[1] : 1) * (new_dim > 2 ? new_sizes[2] : 1));
  new_spans.resize(static_cast<::std::size_t>(new_numel));

  ::std::array<::std::int64_t,3> new_c{0,0,0};
  ::std::array<::std::int64_t,3> old_c{0,0,0};
  for (::std::int64_t new_lin = 0; new_lin < new_numel; ++new_lin) {
    unravel(new_lin, new_sizes, new_dim, new_c);
    old_c = {0,0,0};
    for (::std::int64_t i = 0, j = 0; i < old_dim; ++i) {
      if (i == dim) {
        old_c[static_cast<::std::size_t>(i)] = index;
      } else {
        old_c[static_cast<::std::size_t>(i)] = new_c[static_cast<::std::size_t>(j++)];
      }
    }
    const auto old_lin = ravel(old_c, old_strides, old_dim);
    new_spans[static_cast<::std::size_t>(new_lin)] = impl->spans().at(static_cast<::std::size_t>(old_lin));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->arena(), ::std::move(new_spans), new_sizes, new_dim, impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

Note narrow(const Note& x, ::std::int64_t dim, ::std::int64_t start, ::std::int64_t length) {
  check_dim(x, dim);
  const auto& impl = NoteAccess::impl(x);
  const auto old_sizes = impl->sizes3();
  const auto old_dim = impl->dim();
  const auto size_d = old_sizes[static_cast<::std::size_t>(dim)];
  PLUSS_CHECK(start >= 0 && start <= size_d, "narrow start out of range");
  PLUSS_CHECK(length >= 0, "narrow length must be non-negative");
  PLUSS_CHECK(start + length <= size_d, "narrow range out of bounds");

  ::std::array<::std::int64_t,3> new_sizes = old_sizes;
  new_sizes[static_cast<::std::size_t>(dim)] = length;
  const auto old_strides = strides3(old_sizes, old_dim);

  ::std::vector<NoteImpl::OffLen> new_spans;
  new_spans.resize(static_cast<::std::size_t>(impl->numel() / size_d * length));

  const auto new_dim = old_dim;
  const ::std::int64_t new_numel = (new_dim == 0) ? 1 : (new_sizes[0] * (new_dim > 1 ? new_sizes[1] : 1) * (new_dim > 2 ? new_sizes[2] : 1));
  new_spans.resize(static_cast<::std::size_t>(new_numel));

  ::std::array<::std::int64_t,3> new_c{0,0,0};
  ::std::array<::std::int64_t,3> old_c{0,0,0};
  for (::std::int64_t new_lin = 0; new_lin < new_numel; ++new_lin) {
    unravel(new_lin, new_sizes, new_dim, new_c);
    old_c = new_c;
    old_c[static_cast<::std::size_t>(dim)] += start;
    const auto old_lin = ravel(old_c, old_strides, old_dim);
    new_spans[static_cast<::std::size_t>(new_lin)] = impl->spans().at(static_cast<::std::size_t>(old_lin));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(impl->arena(), ::std::move(new_spans), new_sizes, new_dim, impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

TokenBatch split_whitespace(const Note& x) {
  if (!x.defined()) {
    return TokenBatchFactoryAccess::make(Note{}, {0});
  }

  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::split_whitespace");
  const ::std::int64_t n = impl->numel();

  ::std::vector<::std::int64_t> offsets;
  offsets.resize(static_cast<::std::size_t>(n) + 1);
  offsets[0] = 0;

  // First pass: count tokens per element.
  ::std::vector<::std::int64_t> counts(static_cast<::std::size_t>(n), 0);
  ::std::int64_t total = 0;
  for (::std::int64_t i = 0; i < n; ++i) {
    auto spans = ::pluss::coden::native::cpu::split_whitespace_spans(impl->bytes_at(i));
    const ::std::int64_t c = static_cast<::std::int64_t>(spans.size());
    counts[static_cast<::std::size_t>(i)] = c;
    total += c;
  }
  for (::std::int64_t i = 0; i < n; ++i) {
    offsets[static_cast<::std::size_t>(i) + 1] = offsets[static_cast<::std::size_t>(i)] + counts[static_cast<::std::size_t>(i)];
  }

  // Build a 1D Note of tokens.
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> token_spans;
  token_spans.reserve(static_cast<::std::size_t>(total));

  for (::std::int64_t i = 0; i < n; ++i) {
    const auto s = impl->bytes_at(i);
    auto spans = ::pluss::coden::native::cpu::split_whitespace_spans(s);
    for (auto [off, len] : spans) {
      const auto token = ::std::string_view(s.data() + off, static_cast<::std::size_t>(len));
      const auto token_off = arena->append(token);
      token_spans.push_back(token_off);
    }
  }

  ::std::array<::std::int64_t, 3> sizes{static_cast<::std::int64_t>(token_spans.size()), 0, 0};
  auto token_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      ::std::move(arena), ::std::move(token_spans), sizes, /*dim=*/1, NoteImpl::Encoding::Utf8Text);
  Note tokens = NoteFactoryAccess::wrap(::std::move(token_impl));

  return TokenBatchFactoryAccess::make(::std::move(tokens), ::std::move(offsets));
}



// ---- Patch8: Unicode/regex/split/join hooks ----

Chord normalize(const Chord& x, ::std::int64_t /*form*/) {
  // ICU-optional hook. Without ICU we conservatively no-op.
  return x;
}

Chord casefold(const Chord& x, ::std::string_view /*locale*/) {
  // ICU-optional hook. ASCII fallback = lower.
  return lower(x);
}

bool regex_match(const Chord& x, ::std::string_view pattern) {
  if (!x.defined()) return false;
  const ::std::regex re{::std::string(pattern)};
  return ::std::regex_match(::std::string(x.utf8()), re);
}

bool regex_search(const Chord& x, ::std::string_view pattern) {
  if (!x.defined()) return false;
  const ::std::regex re{::std::string(pattern)};
  return ::std::regex_search(::std::string(x.utf8()), re);
}

Chord regex_replace(const Chord& x, ::std::string_view pattern, ::std::string_view repl) {
  if (!x.defined()) return Chord{};
  const ::std::regex re{::std::string(pattern)};
  ::std::string out = ::std::regex_replace(::std::string(x.utf8()), re, ::std::string(repl));
  return chord_from_utf8(out);
}

TokenBatch split_whitespace(const Chord& x) {
  if (!x.defined()) return TokenBatchFactoryAccess::make(Note{}, {0,0});
  const ::std::string_view s = x.utf8();
  const auto spans = ::pluss::coden::native::cpu::split_whitespace_spans(s);

  ::std::vector<::std::int64_t> offsets = {0, static_cast<::std::int64_t>(spans.size())};
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> token_spans;
  token_spans.reserve(spans.size());

  for (auto [off, len] : spans) {
    const auto token = ::std::string_view(s.data() + off, len);
    auto [toff, tlen] = arena->append(token);
    token_spans.push_back({toff, tlen});
  }

  ::std::array<::std::int64_t, 3> sizes{static_cast<::std::int64_t>(token_spans.size()), 0, 0};
  auto token_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      ::std::move(arena), ::std::move(token_spans), sizes, /*dim=*/1, NoteImpl::Encoding::Utf8Text);
  Note tokens = NoteFactoryAccess::wrap(::std::move(token_impl));

  return TokenBatchFactoryAccess::make(::std::move(tokens), ::std::move(offsets));
}

TokenBatch split(const Chord& x, ::std::string_view delim) {
  if (!x.defined()) return TokenBatchFactoryAccess::make(Note{}, {0,0});
  PLUSS_CHECK(!delim.empty(), "split: delimiter must be non-empty");
  const ::std::string_view s = x.utf8();
  const auto spans = ::pluss::coden::native::cpu::split_delim_spans(s, delim);

  ::std::vector<::std::int64_t> offsets = {0, static_cast<::std::int64_t>(spans.size())};
  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> token_spans;
  token_spans.reserve(spans.size());

  for (auto [off, len] : spans) {
    const auto token = ::std::string_view(s.data() + off, len);
    auto [toff, tlen] = arena->append(token);
    token_spans.push_back({toff, tlen});
  }

  ::std::array<::std::int64_t, 3> sizes{static_cast<::std::int64_t>(token_spans.size()), 0, 0};
  auto token_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      ::std::move(arena), ::std::move(token_spans), sizes, /*dim=*/1, NoteImpl::Encoding::Utf8Text);
  Note tokens = NoteFactoryAccess::wrap(::std::move(token_impl));

  return TokenBatchFactoryAccess::make(::std::move(tokens), ::std::move(offsets));
}

Note normalize(const Note& x, ::std::int64_t form) {
  // no-op normalization for now
  (void)form;
  if (x.defined()) {
    const auto& impl = NoteAccess::impl(x);
    require_text_note(*impl, "Note::normalize");
  }
  return x;
}

Note casefold(const Note& x, ::std::string_view locale) {
  (void)locale;
  return lower(x);
}

::std::vector<::std::uint8_t> regex_match(const Note& x, ::std::string_view pattern) {
  ::std::vector<::std::uint8_t> out;
  if (!x.defined()) return out;
  const ::std::regex re{::std::string(pattern)};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::regex_match");
  out.resize(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i=0;i<impl->numel();++i) {
    out[static_cast<::std::size_t>(i)] = ::std::regex_match(::std::string(impl->bytes_at(i)), re) ? 1 : 0;
  }
  return out;
}

::std::vector<::std::uint8_t> regex_search(const Note& x, ::std::string_view pattern) {
  ::std::vector<::std::uint8_t> out;
  if (!x.defined()) return out;
  const ::std::regex re{::std::string(pattern)};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::regex_search");
  out.resize(static_cast<::std::size_t>(impl->numel()));
  for (::std::int64_t i=0;i<impl->numel();++i) {
    out[static_cast<::std::size_t>(i)] = ::std::regex_search(::std::string(impl->bytes_at(i)), re) ? 1 : 0;
  }
  return out;
}

Note regex_replace(const Note& x, ::std::string_view pattern, ::std::string_view repl) {
  if (!x.defined()) return Note{};
  const ::std::regex re{::std::string(pattern)};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::regex_replace");

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> spans;
  spans.reserve(static_cast<::std::size_t>(impl->numel()));

  for (::std::int64_t i=0;i<impl->numel();++i) {
    ::std::string out = ::std::regex_replace(::std::string(impl->bytes_at(i)), re, ::std::string(repl));
    spans.push_back(arena->append(out));
  }

  auto new_impl = ::pluss::flame::make_intrusive<NoteImpl>(arena, ::std::move(spans), impl->sizes3(), impl->dim(), impl->encoding());
  return NoteFactoryAccess::wrap(::std::move(new_impl));
}

TokenBatch split(const Note& x, ::std::string_view delim) {
  if (!x.defined()) return TokenBatchFactoryAccess::make(Note{}, {0});
  PLUSS_CHECK(!delim.empty(), "split: delimiter must be non-empty");

  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::split");
  const ::std::int64_t n = impl->numel();

  ::std::vector<::std::int64_t> offsets(static_cast<::std::size_t>(n)+1);
  offsets[0]=0;
  ::std::vector<::std::int64_t> counts(static_cast<::std::size_t>(n),0);

  ::std::int64_t total=0;
  for (::std::int64_t i=0;i<n;++i) {
    auto spans = ::pluss::coden::native::cpu::split_delim_spans(impl->bytes_at(i), delim);
    counts[static_cast<::std::size_t>(i)] = static_cast<::std::int64_t>(spans.size());
    total += counts[static_cast<::std::size_t>(i)];
  }
  for (::std::int64_t i=0;i<n;++i) offsets[static_cast<::std::size_t>(i)+1] = offsets[static_cast<::std::size_t>(i)] + counts[static_cast<::std::size_t>(i)];

  auto arena = ::pluss::flame::make_intrusive<TextArena>();
  ::std::vector<NoteImpl::OffLen> token_spans;
  token_spans.reserve(static_cast<::std::size_t>(total));

  for (::std::int64_t i=0;i<n;++i) {
    const auto s = impl->bytes_at(i);
    for (auto [off,len] : ::pluss::coden::native::cpu::split_delim_spans(s, delim)) {
      const auto tok = ::std::string_view(s.data()+off, len);
      auto [toff,tlen] = arena->append(tok);
      token_spans.push_back({toff, tlen});
    }
  }

  ::std::array<::std::int64_t,3> sizes{static_cast<::std::int64_t>(token_spans.size()),0,0};
  auto token_impl = ::pluss::flame::make_intrusive<NoteImpl>(
      ::std::move(arena), ::std::move(token_spans), sizes, /*dim=*/1, NoteImpl::Encoding::Utf8Text);
  Note tokens = NoteFactoryAccess::wrap(::std::move(token_impl));

  return TokenBatchFactoryAccess::make(::std::move(tokens), ::std::move(offsets));
}

Chord join(const Note& x, ::std::string_view sep) {
  if (!x.defined()) return Chord{};
  const auto& impl = NoteAccess::impl(x);
  require_text_note(*impl, "Note::join");
  ::std::string out;
  for (::std::int64_t i=0;i<impl->numel();++i) {
    if (i>0) out.append(sep.data(), sep.size());
    const auto s = impl->bytes_at(i);
    out.append(s.data(), s.size());
  }
  return chord_from_utf8(out);
}
} // namespace pluss::coden::ops_internal

#ifdef CODEN_ENABLE_SERIES_INTEROP
#include "CodeM/core/ScalarType.h"

namespace pluss::coden::ops_internal {

pluss::codem::Series note_contains_series(const Note& a, ::std::string_view needle) {
  auto mask = a.contains(needle); // uint8_t per element
  ::std::vector<double> tmp(mask.size());
  for (::std::size_t i = 0; i < mask.size(); ++i) tmp[i] = static_cast<double>(mask[i] != 0);
  return pluss::codem::Series::make_from_doubles(tmp, a.sizes(), pluss::codem::ScalarType::Bool);
}

pluss::codem::Series note_lengths_bytes_series(const Note& a) {
  auto lens = a.lengths_bytes();
  ::std::vector<double> tmp(lens.size());
  for (::std::size_t i = 0; i < lens.size(); ++i) tmp[i] = static_cast<double>(lens[i]);
  return pluss::codem::Series::make_from_doubles(tmp, a.sizes(), pluss::codem::ScalarType::Int64);
}

} // namespace pluss::coden::ops_internal
#endif // CODEN_ENABLE_SERIES_INTEROP
