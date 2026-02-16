#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/coden/coden.h"

namespace pluss {

// ---- Public factories (preferred construction path) ----
//
// content("hi")                     -> scalar Note (0D)
// content({"a","b"})                -> 1D Note
// make_note(flat, sizes)             -> N-D Note (up to 3D)
//
// bytes_content(...)                 -> bytes-mode Note (raw bytes, no UTF-8 guarantees)

inline pluss::coden::Note content(std::string_view utf8) {
  const std::array<std::string_view, 1> flat{utf8};
  return pluss::coden::ops_internal::note_from_utf8(flat, {});
}

inline pluss::coden::Note content(std::u8string_view utf8) {
  return content(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
}

inline pluss::coden::Note content(const char* utf8) {
  return content(std::string_view{utf8 ? utf8 : ""});
}

inline pluss::coden::Note content(char c) {
  std::string s(1, c);
  return content(std::string_view{s});
}

inline pluss::coden::Note content(std::span<const std::string_view> utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return pluss::coden::ops_internal::note_from_utf8(utf8_1d, sizes);
}

inline pluss::coden::Note content(const std::vector<std::string_view>& utf8_1d) {
  return content(std::span<const std::string_view>(utf8_1d.data(), utf8_1d.size()));
}

inline pluss::coden::Note content(std::initializer_list<const char*> utf8_1d) {
  std::vector<std::string_view> flat;
  flat.reserve(utf8_1d.size());
  for (auto s : utf8_1d) flat.emplace_back(s ? s : "");
  return content(flat);
}

inline pluss::coden::Note make_note(std::span<const std::string_view> utf8_flat,
                                    std::span<const std::int64_t> sizes) {
  return pluss::coden::ops_internal::note_from_utf8(utf8_flat, sizes);
}

inline pluss::coden::Note make_note(const std::vector<std::string_view>& flat,
                                    std::span<const std::int64_t> sizes) {
  return pluss::coden::ops_internal::note_from_utf8(flat, sizes);
}

// ---- Bytes-mode factories ----

inline pluss::coden::Note bytes_content(std::string_view bytes) {
  const std::array<std::string_view, 1> flat{bytes};
  return pluss::coden::ops_internal::note_from_bytes(flat, {});
}

inline pluss::coden::Note bytes_content(const char* bytes) {
  return bytes_content(std::string_view{bytes ? bytes : ""});
}

inline pluss::coden::Note bytes_content(std::u8string_view bytes) {
  return bytes_content(std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()});
}

inline pluss::coden::Note bytes_content(std::span<const std::byte> bytes) {
  return bytes_content(std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()});
}

inline pluss::coden::Note bytes_content(std::span<const std::string_view> bytes_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(bytes_1d.size())};
  return pluss::coden::ops_internal::note_from_bytes(bytes_1d, sizes);
}

inline pluss::coden::Note bytes_content(const std::vector<std::string_view>& bytes_1d) {
  return bytes_content(std::span<const std::string_view>(bytes_1d.data(), bytes_1d.size()));
}

inline pluss::coden::Note bytes_content(std::initializer_list<const char*> bytes_1d) {
  std::vector<std::string_view> flat;
  flat.reserve(bytes_1d.size());
  for (auto s : bytes_1d) flat.emplace_back(s ? s : "");
  return bytes_content(flat);
}

inline pluss::coden::Note make_bytes_note(std::span<const std::string_view> bytes_flat,
                                          std::span<const std::int64_t> sizes) {
  return pluss::coden::ops_internal::note_from_bytes(bytes_flat, sizes);
}

inline pluss::coden::Note make_bytes_note(const std::vector<std::string_view>& flat,
                                          std::span<const std::int64_t> sizes) {
  return pluss::coden::ops_internal::note_from_bytes(flat, sizes);
}

} // namespace pluss
