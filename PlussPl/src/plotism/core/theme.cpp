#include "pluss/pl/plusspl.h"

namespace pluss::pl {
namespace {

[[nodiscard]] constexpr double clamp_unit(double value) noexcept {
  return value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
}

[[nodiscard]] Rgba lerp(Rgba a, Rgba b, double t) noexcept {
  const double u = clamp_unit(t);
  return Rgba{
      a.red + (b.red - a.red) * u,
      a.green + (b.green - a.green) * u,
      a.blue + (b.blue - a.blue) * u,
      a.alpha + (b.alpha - a.alpha) * u,
  };
}

} // namespace

PaletteRamp::PaletteRamp(std::string name, std::vector<Rgba> stops)
    : name_(std::move(name)), stops_(std::move(stops)) {}

Rgba PaletteRamp::sample(double position) const noexcept {
  if (stops_.empty()) {
    return Rgba::from_u8(0, 0, 0);
  }
  if (stops_.size() == 1) {
    return stops_.front();
  }
  const double t = clamp_unit(position);
  const double scaled = t * static_cast<double>(stops_.size() - 1);
  const auto lower_index = static_cast<std::size_t>(scaled);
  const auto upper_index = std::min(lower_index + 1, stops_.size() - 1);
  return lerp(stops_[lower_index], stops_[upper_index], scaled - static_cast<double>(lower_index));
}

PaletteRamp PaletteRamp::neutral() {
  return PaletteRamp{
      "neutral",
      {
          Rgba::from_u8(27, 30, 38),
          Rgba::from_u8(88, 93, 105),
          Rgba::from_u8(161, 166, 176),
          Rgba::from_u8(229, 232, 238),
      },
  };
}

PaletteRamp PaletteRamp::thermal() {
  return PaletteRamp{
      "thermal",
      {
          Rgba::from_u8(21, 43, 81),
          Rgba::from_u8(38, 93, 171),
          Rgba::from_u8(48, 166, 170),
          Rgba::from_u8(248, 196, 72),
          Rgba::from_u8(214, 88, 52),
      },
  };
}

PaletteRamp PaletteRamp::lagoon() {
  return PaletteRamp{
      "lagoon",
      {
          Rgba::from_u8(44, 52, 125),
          Rgba::from_u8(37, 103, 192),
          Rgba::from_u8(29, 161, 214),
          Rgba::from_u8(68, 198, 163),
          Rgba::from_u8(182, 221, 73),
          Rgba::from_u8(248, 230, 84),
      },
  };
}

PaletteRamp PaletteRamp::ocean() {
  return PaletteRamp{
      "ocean",
      {
          Rgba::from_u8(9, 33, 64),
          Rgba::from_u8(15, 74, 117),
          Rgba::from_u8(26, 126, 171),
          Rgba::from_u8(92, 188, 211),
          Rgba::from_u8(211, 239, 244),
      },
  };
}

PaletteRamp PaletteRamp::meadow() {
  return PaletteRamp{
      "meadow",
      {
          Rgba::from_u8(28, 52, 36),
          Rgba::from_u8(54, 110, 69),
          Rgba::from_u8(104, 156, 94),
          Rgba::from_u8(182, 203, 117),
          Rgba::from_u8(240, 226, 171),
      },
  };
}

Theme::Theme(std::string name,
             std::vector<Rgba> accents,
             Rgba background,
             Rgba foreground,
             PaletteRamp surface)
    : name_(std::move(name)),
      accents_(std::move(accents)),
      background_(background),
      foreground_(foreground),
      surface_palette_(std::move(surface)) {}

Rgba Theme::accent(std::size_t index) const noexcept {
  if (accents_.empty()) {
    return foreground_;
  }
  return accents_[index % accents_.size()];
}

Theme Theme::ocean() {
  return Theme{
      "ocean",
      {
          Rgba::from_u8(24, 104, 171),
          Rgba::from_u8(32, 160, 176),
          Rgba::from_u8(233, 154, 42),
          Rgba::from_u8(196, 75, 59),
          Rgba::from_u8(106, 78, 143),
      },
      Rgba::from_u8(247, 250, 252),
      Rgba::from_u8(19, 30, 43),
      PaletteRamp::ocean(),
  };
}

Theme Theme::paper() {
  return Theme{
      "paper",
      {
          Rgba::from_u8(28, 87, 141),
          Rgba::from_u8(49, 130, 92),
          Rgba::from_u8(197, 97, 51),
          Rgba::from_u8(125, 75, 140),
          Rgba::from_u8(161, 52, 74),
      },
      Rgba::from_u8(255, 252, 248),
      Rgba::from_u8(28, 27, 25),
      PaletteRamp::thermal(),
  };
}

namespace themes {

Theme ocean() {
  return Theme::ocean();
}

Theme paper() {
  return Theme::paper();
}

} // namespace themes
} // namespace pluss::pl
