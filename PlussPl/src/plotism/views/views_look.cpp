// Views that change how a figure looks without touching its data: theme, colours, legend, grid, shape, text size.

#include "views_internal.h"

namespace pluss::pl::views::internal {

namespace {

std::vector<Rgba> accent_list(std::string_view id) {
  if (id == "ocean") {
    return Theme::ocean().accents();
  }
  if (id == "gray") {
    return {Rgba::from_u8(35, 35, 35), Rgba::from_u8(120, 120, 120), Rgba::from_u8(70, 70, 70),
            Rgba::from_u8(165, 165, 165), Rgba::from_u8(95, 95, 95), Rgba::from_u8(145, 145, 145)};
  }
  return flash::flash_palette(std::string(id)).stops();
}

void recolor(Studio& studio, const std::vector<Rgba>& accents) {
  if (accents.empty()) {
    return;
  }
  for (Pane& pane : Access::panes(studio)) {
    std::size_t index = 0;
    for (Trace& trace : Access::traces(pane)) {
      auto color = accents[index % accents.size()];
      color.alpha = trace.stroke_color().alpha;  // keep the plot's own opacity
      trace.stroke(color);
      ++index;
    }
  }
}

struct ThemeSpec {
  const char* id;
  const char* label;
};

constexpr ThemeSpec kThemes[] = {
    {"lightgrid", "Light grid"}, {"darkgrid", "Dark grid"}, {"sand", "Sand"},   {"midnight", "Midnight"},
    {"ocean", "Ocean"},          {"contrast", "High contrast"}, {"print", "Print (gray)"},
};

Theme custom_theme(std::string_view id) {
  if (id == "midnight") {
    return Theme{"midnight",
                 {Rgba::from_u8(110, 168, 255), Rgba::from_u8(255, 170, 90), Rgba::from_u8(110, 220, 170),
                  Rgba::from_u8(240, 120, 150), Rgba::from_u8(200, 160, 255), Rgba::from_u8(240, 220, 110)},
                 Rgba::from_u8(17, 21, 29), Rgba::from_u8(226, 232, 240), PaletteRamp::lagoon()};
  }
  if (id == "contrast") {
    return Theme{"contrast",
                 {Rgba::from_u8(0, 84, 180), Rgba::from_u8(214, 40, 20), Rgba::from_u8(0, 0, 0),
                  Rgba::from_u8(235, 140, 0), Rgba::from_u8(0, 140, 70), Rgba::from_u8(120, 0, 160)},
                 Rgba::from_u8(255, 255, 255), Rgba::from_u8(0, 0, 0), PaletteRamp::thermal()};
  }
  if (id == "print") {
    return Theme{"print",
                 {Rgba::from_u8(35, 35, 35), Rgba::from_u8(120, 120, 120), Rgba::from_u8(70, 70, 70),
                  Rgba::from_u8(165, 165, 165), Rgba::from_u8(95, 95, 95), Rgba::from_u8(145, 145, 145)},
                 Rgba::from_u8(255, 255, 255), Rgba::from_u8(20, 20, 20), PaletteRamp::neutral()};
  }
  return Theme::ocean();
}

void apply_theme(Studio& studio, std::string_view id, const Choices&) {
  if (id == "lightgrid" || id == "darkgrid" || id == "sand") {
    const auto style = id == "lightgrid" ? flash::FlashStyle::Lightgrid
                       : id == "darkgrid" ? flash::FlashStyle::Darkgrid
                                          : flash::FlashStyle::Sandgrid;
    studio.theme_tokens() = flash::flash_theme(style);
    for (Pane& pane : Access::panes(studio)) {
      flash::set_style(pane, style);
    }
    return;
  }
  const Theme theme = custom_theme(id);
  studio.theme_tokens() = theme;
  for (Pane& pane : Access::panes(studio)) {
    pane.theme_tokens() = theme;
    if (id == "contrast") {
      pane.grid(GridStyle::Major);
    }
  }
  recolor(studio, theme.accents());
  if (id == "contrast") {
    for (Pane& pane : Access::panes(studio)) {
      for (Trace& trace : Access::traces(pane)) {
        if (trace.kind() == TraceKind::Trend || trace.kind() == TraceKind::Spike) {
          trace.width(std::max(trace.stroke_width() * 1.5F, 2.4F));
        }
      }
    }
  }
}

// A pane with x and y axes to put a grid behind (a pie or a dial has none).
bool has_axes(const Pane& pane) {
  return pane.kind() == PaneKind::Planar &&
         std::any_of(pane.traces().begin(), pane.traces().end(), [](const Trace& trace) {
           return trace.kind() != TraceKind::Wedge && trace.kind() != TraceKind::Radial;
         });
}

bool has_planar_pane(const Studio& studio) {
  return std::any_of(studio.panes().begin(), studio.panes().end(), has_axes);
}

} // namespace

void register_look_views(std::vector<Def>& defs) {
  defs.push_back(Def{
      10, "theme", "Theme", "",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (studio.panes().empty()) {
          return out;
        }
        out.push_back({"default", "Original"});
        for (const auto& theme : kThemes) {
          out.push_back({theme.id, theme.label});
        }
        return out;
      },
      apply_theme});

  defs.push_back(Def{
      20, "colors", "Colors", "",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (studio.panes().empty()) {
          return out;
        }
        out = {{"default", "Default"}, {"deep", "Deep"},   {"muted", "Muted"},   {"pastel", "Pastel"},
               {"bright", "Bright"},   {"dark", "Dark"},   {"colorblind", "Colorblind"}, {"ocean", "Ocean"},
               {"gray", "Gray"}};
        return out;
      },
      [](Studio& studio, std::string_view id, const Choices&) { recolor(studio, accent_list(id)); }});

  defs.push_back(Def{
      900, "legend", "Legend", "Axes",
      [](const Studio& studio) {
        std::vector<Option> out;
        bool worthwhile = false;
        for (const Pane& pane : studio.panes()) {
          std::size_t labeled = 0;
          for (const Trace& trace : pane.traces()) {
            labeled += trace.label_text().empty() ? 0 : 1;
          }
          worthwhile = worthwhile || labeled >= 2 || pane.legend_place() != KeyPlace::None;
        }
        if (worthwhile || studio.legend_dock() != KeyDock::None) {
          out = {{"default", "As drawn"},     {"ne", "Top right"},      {"nw", "Top left"}, {"se", "Bottom right"},
                 {"sw", "Bottom left"}, {"outside", "Outside"}, {"off", "Hidden"}};
        }
        return out;
      },
      [](Studio& studio, std::string_view id, const Choices&) {
        const KeyPlace place = id == "ne"        ? KeyPlace::NorthEast
                               : id == "nw"      ? KeyPlace::NorthWest
                               : id == "se"      ? KeyPlace::SouthEast
                               : id == "sw"      ? KeyPlace::SouthWest
                               : id == "outside" ? KeyPlace::OutsideEast
                                                 : KeyPlace::None;
        for (Pane& pane : Access::panes(studio)) {
          pane.legend(place);
        }
        if (id == "off") {
          studio.legend(KeyDock::None);
        }
      }});

  defs.push_back(Def{
      910, "grid", "Grid", "Axes",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (has_planar_pane(studio)) {
          out = {{"default", "As drawn"}, {"off", "None"}, {"major", "Major lines"}, {"fine", "Major and minor"}};
        }
        return out;
      },
      [](Studio& studio, std::string_view id, const Choices&) {
        const GridStyle style = id == "off" ? GridStyle::None : id == "major" ? GridStyle::Major : GridStyle::MajorMinor;
        for (Pane& pane : Access::panes(studio)) {
          if (has_axes(pane)) {
            pane.grid(style);
          }
        }
      }});

  defs.push_back(Def{
      920, "aspect", "Aspect", "Axes",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (!studio.panes().empty()) {
          out = {{"default", "As drawn"}, {"wide", "Wide (16:9)"}, {"classic", "Classic (4:3)"},
                 {"square", "Square"},    {"tall", "Tall (3:4)"}};
        }
        return out;
      },
      [](Studio& studio, std::string_view id, const Choices&) {
        const double ratio = id == "wide" ? 16.0 / 9.0 : id == "classic" ? 4.0 / 3.0 : id == "square" ? 1.0 : 0.75;
        const double area = studio.extent().width * studio.extent().height;
        studio.size(std::round(std::sqrt(area * ratio)), std::round(std::sqrt(area / ratio)));
      }});

  defs.push_back(Def{
      930, "text", "Axis text", "Axes",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (!studio.panes().empty()) {
          out = {{"default", "As drawn"}, {"small", "Small"}, {"large", "Large"}, {"huge", "Huge"}};
        }
        return out;
      },
      [](Studio& studio, std::string_view id, const Choices&) {
        const double factor = id == "small" ? 0.85 : id == "large" ? 1.25 : 1.6;
        for (Pane& pane : Access::panes(studio)) {
          pane.tick_font_scale(pane.tick_font_scale() * factor);
          pane.axis_label_font_scale(pane.axis_label_font_scale() * factor);
        }
      }});
}

} // namespace pluss::pl::views::internal
