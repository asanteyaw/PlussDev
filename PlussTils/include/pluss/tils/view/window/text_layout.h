#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace pluss::view::window {

struct TextGlyph {
  char ch{' '};
  int x{0};
  int y{0};
  int pixel_size{12};
};

struct TextLineDecoration {
  int x0{0};
  int y0{0};
  int x1{0};
  int y1{0};
  int thickness{1};
};

struct TextLayout {
  std::string normalized_text{};
  std::vector<TextGlyph> glyphs{};
  std::vector<TextLineDecoration> lines{};
  std::vector<std::string> warnings{};
  int width{0};
  int ascent{0};
  int descent{0};
};

[[nodiscard]] TextLayout layout_math_text(std::string_view text, int pixel_size);

} // namespace pluss::view::window
