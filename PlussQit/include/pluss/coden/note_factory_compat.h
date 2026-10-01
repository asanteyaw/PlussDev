#pragma once

// `pluss::note::*` (note_factory.h) is the primary Note factory surface.
// `pluss::make_note` is the one flat spelling kept as a permanent shortcut
// (mirroring the Series, Timeline, TimeIndex, and Duration factory families)
// for the common construct-from-data case -- every other flat spelling
// (pluss::full_note, pluss::bytes_content, ...) was a transitional
// compatibility alias and has been removed now that all internal call sites
// across the codebase go through pluss::note::* directly.
//
// Each overload mirrors pluss::note::make's parameter list exactly (not a
// generic forwarding template): a template can't deduce a type from a bare
// {...} braced-list argument, so e.g. pluss::make_note({"a","b"}) needs the
// parameter to already be the concrete type for that call to resolve at all.

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

#include "MSharp/RangeView.h"
#include "pluss/coden/coden.h"
#include "pluss/detail/nested_note_literal.h"
#include "pluss/coden/note_factory.h"

namespace pluss {

inline pluss::Note make_note(std::string_view utf8) {
  return pluss::note::make(utf8);
}
inline pluss::Note make_note(std::u8string_view utf8) {
  return pluss::note::make(utf8);
}
inline pluss::Note make_note(const char* utf8) {
  return pluss::note::make(utf8);
}
inline pluss::Note make_note(std::initializer_list<const char*> utf8_1d) {
  return pluss::note::make(utf8_1d);
}
inline pluss::Note make_note(char c) {
  return pluss::note::make(c);
}
inline pluss::Note make_note(pluss::msharp::RangeView::Of<std::string_view> utf8_flat,
                                    pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return pluss::note::make(utf8_flat, sizes);
}
inline pluss::Note make_note(pluss::msharp::RangeView::Of<std::string> utf8_flat,
                                    pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return pluss::note::make(utf8_flat, sizes);
}
inline pluss::Note make_note(pluss::msharp::RangeView::Of<std::string_view> utf8_1d) {
  return pluss::note::make(utf8_1d);
}
inline pluss::Note make_note(pluss::msharp::RangeView::Of<std::string> utf8_1d) {
  return pluss::note::make(utf8_1d);
}
inline pluss::Note make_note(std::initializer_list<pluss::detail::NestedNoteLiteral> utf8_nd) {
  return pluss::note::make(utf8_nd);
}
inline pluss::Note make_note(std::string&& utf8) {
  return pluss::note::make(utf8);
}
inline pluss::Note make_note(std::u8string&& utf8) {
  return pluss::note::make(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
}

} // namespace pluss
