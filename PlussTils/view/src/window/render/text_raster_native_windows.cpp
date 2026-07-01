#if defined(_WIN32)

#include "text_raster.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::view::window::detail {

namespace {

[[nodiscard]] std::wstring utf8_to_wide(const std::string_view text) {
  if (text.empty()) {
    return {};
  }
  const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
  if (count <= 0) {
    return {};
  }
  std::wstring output(static_cast<std::size_t>(count), L'\0');
  (void)MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), output.data(), count);
  return output;
}

[[nodiscard]] std::string first_font_family(std::string_view css_value) {
  const std::size_t comma = css_value.find(',');
  std::string family(css_value.substr(0, comma));
  while (!family.empty() && std::isspace(static_cast<unsigned char>(family.front())) != 0) {
    family.erase(family.begin());
  }
  while (!family.empty() && std::isspace(static_cast<unsigned char>(family.back())) != 0) {
    family.pop_back();
  }
  if (family.size() >= 2 && (family.front() == '\'' || family.front() == '"') && family.back() == family.front()) {
    family = family.substr(1, family.size() - 2);
  }
  std::string lower;
  lower.reserve(family.size());
  for (const char ch : family) {
    lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  if (lower.empty() || lower == "system-ui" || lower == "sans-serif" || lower == "ui-sans-serif") {
    return "Segoe UI";
  }
  if (lower == "serif" || lower == "ui-serif") {
    return "Times New Roman";
  }
  if (lower == "monospace" || lower == "ui-monospace") {
    return "Consolas";
  }
  return family;
}

[[nodiscard]] bool inside_clip(const std::optional<RectI>& clip, const PointI point) {
  if (!clip.has_value()) {
    return true;
  }
  const RectI rect = *clip;
  return !rect.empty() && point.x >= rect.x && point.y >= rect.y && point.x < rect.x + rect.width &&
         point.y < rect.y + rect.height;
}

} // namespace

std::optional<int> measure_text_width_native(const TextCommand& command) {
  if (command.text.empty() || command.pixel_size <= 0) {
    return 0;
  }
  const std::wstring text = utf8_to_wide(command.text);
  const std::wstring family =
      utf8_to_wide(first_font_family(command.font_family));
  if (text.empty()) {
    return std::nullopt;
  }
  HDC dc = CreateCompatibleDC(nullptr);
  if (dc == nullptr) {
    return std::nullopt;
  }
  HFONT font = CreateFontW(
      -command.pixel_size, 0, 0, 0,
      std::clamp(command.font_weight, 100, 900), FALSE, FALSE, FALSE,
      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
      family.empty() ? L"Segoe UI" : family.c_str());
  if (font == nullptr) {
    DeleteDC(dc);
    return std::nullopt;
  }
  HGDIOBJ old_font = SelectObject(dc, font);
  SIZE extent{};
  const bool measured =
      GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()),
                            &extent) != FALSE;
  SelectObject(dc, old_font);
  DeleteObject(font);
  DeleteDC(dc);
  return measured ? std::optional<int>(std::max(0L, extent.cx))
                  : std::nullopt;
}

bool render_text_command_native(PixelCanvas& canvas, const TextCommand& command, const std::optional<RectI>& clip,
                                std::vector<RenderEvent>& events) {
  (void)events;
  if (canvas.empty() || command.text.empty() || command.pixel_size <= 0) {
    return false;
  }

  const std::wstring text = utf8_to_wide(command.text);
  const std::wstring family = utf8_to_wide(first_font_family(command.font_family));
  if (text.empty()) {
    return false;
  }

  HDC dc = CreateCompatibleDC(nullptr);
  if (dc == nullptr) {
    return false;
  }
  HFONT font = CreateFontW(-command.pixel_size, 0, 0, 0, std::clamp(command.font_weight, 100, 900), FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                           DEFAULT_PITCH | FF_DONTCARE, family.empty() ? L"Segoe UI" : family.c_str());
  if (font == nullptr) {
    DeleteDC(dc);
    return false;
  }
  HGDIOBJ old_font = SelectObject(dc, font);

  SIZE extent{};
  TEXTMETRICW metrics{};
  if (!GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()), &extent) ||
      !GetTextMetricsW(dc, &metrics)) {
    SelectObject(dc, old_font);
    DeleteObject(font);
    DeleteDC(dc);
    return false;
  }

  const int padding = 2;
  const int bitmap_width = std::max(1, extent.cx + padding * 2);
  const int bitmap_height = std::max(1, metrics.tmHeight + padding * 2);
  BITMAPINFO bitmap_info{};
  bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info.bmiHeader.biWidth = bitmap_width;
  bitmap_info.bmiHeader.biHeight = -bitmap_height;
  bitmap_info.bmiHeader.biPlanes = 1;
  bitmap_info.bmiHeader.biBitCount = 32;
  bitmap_info.bmiHeader.biCompression = BI_RGB;

  void* raw_pixels = nullptr;
  HBITMAP bitmap = CreateDIBSection(dc, &bitmap_info, DIB_RGB_COLORS, &raw_pixels, nullptr, 0);
  if (bitmap == nullptr || raw_pixels == nullptr) {
    SelectObject(dc, old_font);
    DeleteObject(font);
    DeleteDC(dc);
    return false;
  }
  HGDIOBJ old_bitmap = SelectObject(dc, bitmap);
  std::memset(raw_pixels, 0, static_cast<std::size_t>(bitmap_width) * static_cast<std::size_t>(bitmap_height) * 4U);
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, RGB(255, 255, 255));
  (void)TextOutW(dc, padding, padding, text.data(), static_cast<int>(text.size()));
  GdiFlush();

  const auto* pixels = static_cast<const std::uint8_t*>(raw_pixels);
  const int target_left = command.anchor.x - padding;
  const int target_top = command.anchor.y - metrics.tmAscent - padding;
  for (int y = 0; y < bitmap_height; ++y) {
    for (int x = 0; x < bitmap_width; ++x) {
      const std::size_t index =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(bitmap_width) + static_cast<std::size_t>(x)) * 4U;
      const std::uint8_t coverage = std::max(pixels[index], std::max(pixels[index + 1U], pixels[index + 2U]));
      if (coverage == 0) {
        continue;
      }
      const PointI target{target_left + x, target_top + y};
      if (inside_clip(clip, target)) {
        canvas.blend_pixel(target, command.color, coverage);
      }
    }
  }

  SelectObject(dc, old_bitmap);
  SelectObject(dc, old_font);
  DeleteObject(bitmap);
  DeleteObject(font);
  DeleteDC(dc);
  return true;
}

} // namespace pluss::view::window::detail

#endif
