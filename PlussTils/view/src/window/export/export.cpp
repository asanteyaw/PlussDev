#include "pluss/tils/view/window/export.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace pluss::view::window {

namespace {

[[nodiscard]] bool has_extension(std::string_view path, std::string_view ext) {
  if (path.size() < ext.size()) {
    return false;
  }
  const std::size_t start = path.size() - ext.size();
  for (std::size_t i = 0; i < ext.size(); ++i) {
    const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(path[start + i])));
    const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));
    if (a != b) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::optional<CanvasExportFormat> detect_export_format_(const std::string_view path) {
  if (has_extension(path, ".png")) {
    return CanvasExportFormat::Png;
  }
  if (has_extension(path, ".ppm")) {
    return CanvasExportFormat::Ppm;
  }
  return std::nullopt;
}

void write_u32_be(std::vector<std::uint8_t>& out, const std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

[[nodiscard]] std::uint32_t crc32(std::string_view tag, const std::vector<std::uint8_t>& data) {
  static const auto table = [] {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t c = i;
      for (int k = 0; k < 8; ++k) {
        c = (c & 1U) ? (0xEDB88320U ^ (c >> 1U)) : (c >> 1U);
      }
      t[i] = c;
    }
    return t;
  }();

  auto update = [](std::uint32_t c, const std::uint8_t b) {
    return table[(c ^ b) & 0xFFU] ^ (c >> 8U);
  };

  std::uint32_t c = 0xFFFFFFFFU;
  for (const char ch : tag) {
    c = update(c, static_cast<std::uint8_t>(ch));
  }
  for (const auto b : data) {
    c = update(c, b);
  }
  return c ^ 0xFFFFFFFFU;
}

[[nodiscard]] std::uint32_t adler32(const std::vector<std::uint8_t>& data) {
  constexpr std::uint32_t mod = 65521U;
  std::uint32_t s1 = 1U;
  std::uint32_t s2 = 0U;
  for (const std::uint8_t b : data) {
    s1 = (s1 + b) % mod;
    s2 = (s2 + s1) % mod;
  }
  return (s2 << 16U) | s1;
}

void append_chunk(std::vector<std::uint8_t>& out,
                  std::string_view tag,
                  const std::vector<std::uint8_t>& data) {
  write_u32_be(out, static_cast<std::uint32_t>(data.size()));
  for (const char ch : tag) {
    out.push_back(static_cast<std::uint8_t>(ch));
  }
  out.insert(out.end(), data.begin(), data.end());
  write_u32_be(out, crc32(tag, data));
}

[[nodiscard]] bool write_png_rgba(const PixelCanvas& canvas, std::string_view path) {
  const SizeI size = canvas.size();
  if (size.width <= 0 || size.height <= 0 || canvas.empty()) {
    return false;
  }
  if (!has_extension(path, ".png")) {
    return false;
  }

  const auto bytes = canvas.bytes();
  const std::size_t row_bytes = static_cast<std::size_t>(size.width) * 4U;
  std::vector<std::uint8_t> filtered;
  filtered.reserve((row_bytes + 1U) * static_cast<std::size_t>(size.height));
  for (int y = 0; y < size.height; ++y) {
    filtered.push_back(0U); // filter type: None
    const std::size_t offset = static_cast<std::size_t>(y) * row_bytes;
    filtered.insert(filtered.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                    bytes.begin() + static_cast<std::ptrdiff_t>(offset + row_bytes));
  }

  std::vector<std::uint8_t> zlib;
  zlib.push_back(0x78U);
  zlib.push_back(0x01U);

  std::size_t pos = 0;
  while (pos < filtered.size()) {
    const std::size_t remaining = filtered.size() - pos;
    const std::size_t len = std::min<std::size_t>(remaining, 65535U);
    const bool final_block = (pos + len == filtered.size());
    zlib.push_back(final_block ? 0x01U : 0x00U);
    zlib.push_back(static_cast<std::uint8_t>(len & 0xFFU));
    zlib.push_back(static_cast<std::uint8_t>((len >> 8U) & 0xFFU));
    const std::uint16_t nlen = static_cast<std::uint16_t>(~static_cast<std::uint16_t>(len));
    zlib.push_back(static_cast<std::uint8_t>(nlen & 0xFFU));
    zlib.push_back(static_cast<std::uint8_t>((nlen >> 8U) & 0xFFU));
    zlib.insert(zlib.end(), filtered.begin() + static_cast<std::ptrdiff_t>(pos),
                filtered.begin() + static_cast<std::ptrdiff_t>(pos + len));
    pos += len;
  }
  write_u32_be(zlib, adler32(filtered));

  std::vector<std::uint8_t> png;
  png.reserve(8U + 64U + zlib.size());
  static constexpr std::array<std::uint8_t, 8> signature{
      0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU};
  png.insert(png.end(), signature.begin(), signature.end());

  std::vector<std::uint8_t> ihdr;
  ihdr.reserve(13U);
  write_u32_be(ihdr, static_cast<std::uint32_t>(size.width));
  write_u32_be(ihdr, static_cast<std::uint32_t>(size.height));
  ihdr.push_back(8U);  // bit depth
  ihdr.push_back(6U);  // color type: RGBA
  ihdr.push_back(0U);  // compression
  ihdr.push_back(0U);  // filter
  ihdr.push_back(0U);  // interlace
  append_chunk(png, "IHDR", ihdr);
  append_chunk(png, "IDAT", zlib);
  append_chunk(png, "IEND", {});

  std::ofstream out(std::string(path), std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    return false;
  }
  out.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
  return static_cast<bool>(out);
}

[[nodiscard]] bool write_ppm_rgb(const PixelCanvas& canvas, std::string_view path) {
  const SizeI size = canvas.size();
  if (size.width <= 0 || size.height <= 0 || canvas.empty()) {
    return false;
  }
  if (!has_extension(path, ".ppm")) {
    return false;
  }

  std::ofstream out(std::string(path), std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    return false;
  }

  out << "P6\n" << size.width << ' ' << size.height << "\n255\n";
  const auto bytes = canvas.bytes();
  for (std::size_t i = 0; i + 3U < bytes.size(); i += 4U) {
    out.put(static_cast<char>(bytes[i]));
    out.put(static_cast<char>(bytes[i + 1U]));
    out.put(static_cast<char>(bytes[i + 2U]));
  }
  return static_cast<bool>(out);
}

} // namespace

std::optional<CanvasExportFormat> detect_canvas_export_format(const std::string_view path) {
  return detect_export_format_(path);
}

bool save_canvas_ppm(const PixelCanvas& canvas, const std::string_view path) {
  return write_ppm_rgb(canvas, path);
}

bool save_canvas_png(const PixelCanvas& canvas, const std::string_view path) {
  return write_png_rgba(canvas, path);
}

bool save_canvas(const PixelCanvas& canvas,
                 const std::string_view path,
                 const CanvasExportFormat format) {
  switch (format) {
    case CanvasExportFormat::Ppm:
      return save_canvas_ppm(canvas, path);
    case CanvasExportFormat::Png:
      return save_canvas_png(canvas, path);
  }
  return false;
}

bool save_canvas(const PixelCanvas& canvas, const std::string_view path) {
  const auto format = detect_export_format_(path);
  return format.has_value() && save_canvas(canvas, path, *format);
}

} // namespace pluss::view::window
