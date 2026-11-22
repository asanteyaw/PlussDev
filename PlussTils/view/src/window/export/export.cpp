#include "pluss/tils/view/window/export.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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


namespace {

// PDF RunLengthDecode: a length byte n in 0..127 is followed by n + 1 literal bytes; n in
// 129..255 is followed by one byte repeated 257 - n times; 128 ends the data.
void run_length_encode(const std::vector<std::uint8_t>& in, std::vector<std::uint8_t>& out) {
  constexpr std::size_t kMaxRun = 128;
  constexpr std::size_t kMinRepeat = 3;  // shorter repeats cost more than they save
  std::size_t i = 0;
  while (i < in.size()) {
    std::size_t run = 1;
    while (i + run < in.size() && run < kMaxRun && in[i + run] == in[i]) {
      ++run;
    }
    if (run >= kMinRepeat) {
      out.push_back(static_cast<std::uint8_t>(257U - run));
      out.push_back(in[i]);
      i += run;
      continue;
    }
    std::size_t end = i;
    while (end < in.size() && end - i < kMaxRun) {
      std::size_t ahead = 1;
      while (end + ahead < in.size() && ahead < kMinRepeat && in[end + ahead] == in[end]) {
        ++ahead;
      }
      if (ahead >= kMinRepeat) {
        break;
      }
      ++end;
    }
    if (end == i) {
      end = i + 1;
    }
    out.push_back(static_cast<std::uint8_t>(end - i - 1U));
    out.insert(out.end(), in.begin() + static_cast<std::ptrdiff_t>(i), in.begin() + static_cast<std::ptrdiff_t>(end));
    i = end;
  }
  out.push_back(128);
}

}  // namespace

bool save_canvas_pdf(const PixelCanvas& canvas, const std::string_view path) {
  if (canvas.empty() || path.empty()) {
    return false;
  }
  const auto size = canvas.size();
  const auto rgba = canvas.bytes();
  std::vector<std::uint8_t> rgb;
  rgb.reserve(static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height) * 3U);
  for (std::size_t i = 0; i + 3U < rgba.size() + 0U; i += 4U) {
    rgb.push_back(rgba[i]);
    rgb.push_back(rgba[i + 1U]);
    rgb.push_back(rgba[i + 2U]);
  }
  std::vector<std::uint8_t> encoded;
  run_length_encode(rgb, encoded);

  std::string body;
  std::vector<std::size_t> offsets;
  const auto begin_object = [&](const int number) {
    offsets.push_back(body.size());
    body += std::to_string(number) + " 0 obj\n";
  };
  body += "%PDF-1.4\n";
  const std::string w = std::to_string(size.width);
  const std::string h = std::to_string(size.height);
  begin_object(1);
  body += "<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
  begin_object(2);
  body += "<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
  begin_object(3);
  body += "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " + w + " " + h +
          "] /Resources << /XObject << /Im0 5 0 R >> >> /Contents 4 0 R >>\nendobj\n";
  const std::string content = "q " + w + " 0 0 " + h + " 0 0 cm /Im0 Do Q";
  begin_object(4);
  body += "<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content + "\nendstream\nendobj\n";
  begin_object(5);
  body += "<< /Type /XObject /Subtype /Image /Width " + w + " /Height " + h +
          " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /RunLengthDecode /Length " +
          std::to_string(encoded.size()) + " >>\nstream\n";
  std::ofstream file{std::string(path), std::ios::binary | std::ios::trunc};
  if (!file) {
    return false;
  }
  file.write(body.data(), static_cast<std::streamsize>(body.size()));
  file.write(reinterpret_cast<const char*>(encoded.data()), static_cast<std::streamsize>(encoded.size()));
  const std::size_t after_image = body.size() + encoded.size();
  const std::string tail_objects = "\nendstream\nendobj\n";
  file.write(tail_objects.data(), static_cast<std::streamsize>(tail_objects.size()));
  const std::size_t xref_at = after_image + tail_objects.size();
  std::string xref = "xref\n0 6\n0000000000 65535 f \n";
  for (const std::size_t offset : offsets) {
    char line[24];
    std::snprintf(line, sizeof(line), "%010zu 00000 n \n", offset);
    xref += line;
  }
  xref += "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n" + std::to_string(xref_at) + "\n%%EOF\n";
  file.write(xref.data(), static_cast<std::streamsize>(xref.size()));
  return static_cast<bool>(file);
}

} // namespace pluss::view::window
