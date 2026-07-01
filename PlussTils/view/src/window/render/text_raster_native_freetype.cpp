#if defined(PLUSSTILS_VIEW_HAVE_FREETYPE) && PLUSSTILS_VIEW_HAVE_FREETYPE

#include <ft2build.h>

#include "text_raster.h"
#include FT_FREETYPE_H

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::view::window::detail {

namespace {

struct FreeTypeState {
  FT_Library library{nullptr};
  std::unordered_map<std::string, FT_Face> faces{};
  std::mutex mutex{};

  FreeTypeState() {
    if (FT_Init_FreeType(&library) != 0) {
      library = nullptr;
    }
  }

  ~FreeTypeState() {
    for (auto& [path, face] : faces) {
      (void)path;
      if (face != nullptr) {
        FT_Done_Face(face);
      }
    }
    if (library != nullptr) {
      FT_Done_FreeType(library);
    }
  }
};

[[nodiscard]] FreeTypeState& free_type_state() {
  static FreeTypeState state;
  return state;
}

[[nodiscard]] std::string lowercase(std::string_view text) {
  std::string output;
  output.reserve(text.size());
  for (const char ch : text) {
    output.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
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
  return lowercase(family);
}

[[nodiscard]] std::string select_font_path(const std::string_view css_family, const int font_weight) {
  if (const char* configured = std::getenv("PLUSSTILS_VIEW_FONT");
      configured != nullptr && configured[0] != '\0' && std::filesystem::exists(configured)) {
    return configured;
  }

  const std::string family = first_font_family(css_family);
  const bool monospace = family.find("mono") != std::string::npos || family.find("code") != std::string::npos;
  const bool serif =
      !monospace && family.find("serif") != std::string::npos && family.find("sans") == std::string::npos;
  const bool bold = font_weight >= 600;

  std::vector<std::string> candidates;
  if (monospace) {
    candidates = bold
                     ? std::vector<std::string>{
                           "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
                           "/usr/share/fonts/truetype/liberation2/LiberationMono-Bold.ttf",
                           "/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf",
                       }
                     : std::vector<std::string>{
                           "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                           "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf",
                           "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
                       };
  } else if (serif) {
    candidates = bold
                     ? std::vector<std::string>{
                           "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf",
                           "/usr/share/fonts/truetype/liberation2/LiberationSerif-Bold.ttf",
                           "/usr/share/fonts/TTF/DejaVuSerif-Bold.ttf",
                       }
                     : std::vector<std::string>{
                           "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf",
                           "/usr/share/fonts/truetype/liberation2/LiberationSerif-Regular.ttf",
                           "/usr/share/fonts/TTF/DejaVuSerif.ttf",
                       };
  } else {
    candidates = bold
                     ? std::vector<std::string>{
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                           "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf",
                           "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
                       }
                     : std::vector<std::string>{
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
                           "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans.ttf",
                       };
  }

  const auto found = std::find_if(candidates.begin(), candidates.end(), [](const std::string& path) {
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
  });
  return found != candidates.end() ? *found : std::string{};
}

[[nodiscard]] FT_Face load_face(FreeTypeState& state, const std::string& path) {
  const auto cached = state.faces.find(path);
  if (cached != state.faces.end()) {
    return cached->second;
  }
  FT_Face face = nullptr;
  if (state.library == nullptr || FT_New_Face(state.library, path.c_str(), 0, &face) != 0 || face == nullptr) {
    return nullptr;
  }
  state.faces.emplace(path, face);
  return face;
}

[[nodiscard]] std::vector<std::uint32_t> decode_utf8(const std::string_view text) {
  std::vector<std::uint32_t> output;
  output.reserve(text.size());
  for (std::size_t index = 0; index < text.size();) {
    const auto first = static_cast<unsigned char>(text[index]);
    std::uint32_t scalar = 0xFFFDU;
    std::size_t count = 1;
    if (first < 0x80U) {
      scalar = first;
    } else if ((first & 0xE0U) == 0xC0U && index + 1U < text.size()) {
      scalar = first & 0x1FU;
      count = 2;
    } else if ((first & 0xF0U) == 0xE0U && index + 2U < text.size()) {
      scalar = first & 0x0FU;
      count = 3;
    } else if ((first & 0xF8U) == 0xF0U && index + 3U < text.size()) {
      scalar = first & 0x07U;
      count = 4;
    }
    bool valid = count > 1;
    for (std::size_t offset = 1; offset < count; ++offset) {
      const auto continuation = static_cast<unsigned char>(text[index + offset]);
      if ((continuation & 0xC0U) != 0x80U) {
        valid = false;
        count = 1;
        scalar = 0xFFFDU;
        break;
      }
      scalar = (scalar << 6U) | (continuation & 0x3FU);
    }
    output.push_back(valid || first < 0x80U ? scalar : 0xFFFDU);
    index += count;
  }
  return output;
}

[[nodiscard]] bool inside_clip(const std::optional<RectI>& clip, const PointI point) {
  if (!clip.has_value()) {
    return true;
  }
  const RectI rect = *clip;
  return !rect.empty() && point.x >= rect.x && point.y >= rect.y && point.x < rect.x + rect.width &&
         point.y < rect.y + rect.height;
}

void draw_bitmap(PixelCanvas& canvas, const FT_Bitmap& bitmap, const PointI origin, const Color color,
                 const std::optional<RectI>& clip) {
  for (unsigned int row = 0; row < bitmap.rows; ++row) {
    const auto* source = bitmap.buffer + static_cast<std::ptrdiff_t>(row) * bitmap.pitch;
    for (unsigned int column = 0; column < bitmap.width; ++column) {
      std::uint8_t coverage = 0;
      if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
        coverage = source[column];
      } else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
        coverage = (source[column / 8U] & (0x80U >> (column % 8U))) != 0U ? 255U : 0U;
      }
      if (coverage == 0) {
        continue;
      }
      const PointI target{
          origin.x + static_cast<int>(column),
          origin.y + static_cast<int>(row),
      };
      if (inside_clip(clip, target)) {
        canvas.blend_pixel(target, color, coverage);
      }
    }
  }
}

} // namespace

bool render_text_command_native(PixelCanvas& canvas, const TextCommand& command, const std::optional<RectI>& clip,
                                std::vector<RenderEvent>& events) {
  (void)events;
  if (canvas.empty() || command.text.empty() || command.pixel_size <= 0) {
    return false;
  }

  const std::string path = select_font_path(command.font_family, command.font_weight);
  if (path.empty()) {
    return false;
  }

  FreeTypeState& state = free_type_state();
  std::scoped_lock lock(state.mutex);
  FT_Face face = load_face(state, path);
  if (face == nullptr || FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(command.pixel_size)) != 0) {
    return false;
  }

  int pen_x = command.anchor.x;
  FT_UInt previous = 0;
  const bool has_kerning = FT_HAS_KERNING(face) != 0;
  for (const std::uint32_t scalar : decode_utf8(command.text)) {
    const FT_UInt glyph_index = FT_Get_Char_Index(face, scalar);
    if (has_kerning && previous != 0 && glyph_index != 0) {
      FT_Vector kerning{};
      if (FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_DEFAULT, &kerning) == 0) {
        pen_x += static_cast<int>(kerning.x >> 6);
      }
    }
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT) != 0 ||
        FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
      previous = glyph_index;
      continue;
    }

    const FT_GlyphSlot glyph = face->glyph;
    draw_bitmap(canvas, glyph->bitmap,
                PointI{
                    pen_x + glyph->bitmap_left,
                    command.anchor.y - glyph->bitmap_top,
                },
                command.color, clip);
    pen_x += static_cast<int>(glyph->advance.x >> 6);
    previous = glyph_index;
  }
  return true;
}

} // namespace pluss::view::window::detail

#endif
