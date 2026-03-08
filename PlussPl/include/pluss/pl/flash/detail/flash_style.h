// This header is intentionally included from flash.h inside namespace pluss::pl::flash.

enum class FlashContext : std::uint8_t {
  Paper,
  Notebook,
  Talk,
  Poster,
};

enum class FlashStyle : std::uint8_t {
  Darkgrid,
  Whitegrid,
  Dark,
  White,
  Ticks,
};

enum class FlashPalette : std::uint8_t {
  Deep,
  Muted,
  Pastel,
  Bright,
  Dark,
  Colorblind,
};

struct FlashThemeOptions {
  FlashStyle style{FlashStyle::Whitegrid};
  FlashContext context{FlashContext::Notebook};
  FlashPalette palette{FlashPalette::Deep};
  std::optional<double> width{std::nullopt};
  std::optional<double> height{std::nullopt};
};

[[nodiscard]] inline FlashContext flash_context(std::string_view name) {
  if (name == "paper") return FlashContext::Paper;
  if (name == "talk") return FlashContext::Talk;
  if (name == "poster") return FlashContext::Poster;
  return FlashContext::Notebook;
}

[[nodiscard]] inline FlashStyle flash_style(std::string_view name) {
  if (name == "darkgrid") return FlashStyle::Darkgrid;
  if (name == "dark") return FlashStyle::Dark;
  if (name == "white") return FlashStyle::White;
  if (name == "ticks") return FlashStyle::Ticks;
  return FlashStyle::Whitegrid;
}

[[nodiscard]] inline FlashPalette flash_palette_kind(std::string_view name) {
  if (name == "muted") return FlashPalette::Muted;
  if (name == "pastel") return FlashPalette::Pastel;
  if (name == "bright") return FlashPalette::Bright;
  if (name == "dark") return FlashPalette::Dark;
  if (name == "colorblind") return FlashPalette::Colorblind;
  return FlashPalette::Deep;
}

[[nodiscard]] inline std::vector<Rgba> flash_palette_colors(FlashPalette palette) {
  switch (palette) {
    case FlashPalette::Muted:
      return {
          Rgba::from_u8(72, 120, 208),
          Rgba::from_u8(238, 133, 74),
          Rgba::from_u8(106, 204, 100),
          Rgba::from_u8(214, 95, 95),
          Rgba::from_u8(149, 108, 180),
          Rgba::from_u8(140, 109, 49),
          Rgba::from_u8(218, 94, 155),
          Rgba::from_u8(140, 140, 140),
      };
    case FlashPalette::Pastel:
      return {
          Rgba::from_u8(161, 201, 244),
          Rgba::from_u8(255, 180, 130),
          Rgba::from_u8(141, 229, 161),
          Rgba::from_u8(255, 159, 155),
          Rgba::from_u8(208, 187, 255),
          Rgba::from_u8(222, 187, 155),
          Rgba::from_u8(250, 176, 228),
          Rgba::from_u8(207, 207, 207),
      };
    case FlashPalette::Bright:
      return {
          Rgba::from_u8(2, 62, 255),
          Rgba::from_u8(255, 124, 0),
          Rgba::from_u8(26, 201, 56),
          Rgba::from_u8(232, 0, 11),
          Rgba::from_u8(142, 43, 226),
          Rgba::from_u8(159, 72, 0),
          Rgba::from_u8(241, 76, 193),
          Rgba::from_u8(160, 160, 160),
      };
    case FlashPalette::Dark:
      return {
          Rgba::from_u8(0, 28, 127),
          Rgba::from_u8(177, 64, 13),
          Rgba::from_u8(18, 113, 28),
          Rgba::from_u8(140, 8, 0),
          Rgba::from_u8(89, 30, 113),
          Rgba::from_u8(114, 57, 6),
          Rgba::from_u8(154, 26, 116),
          Rgba::from_u8(90, 90, 90),
      };
    case FlashPalette::Colorblind:
      return {
          Rgba::from_u8(1, 115, 178),
          Rgba::from_u8(222, 143, 5),
          Rgba::from_u8(2, 158, 115),
          Rgba::from_u8(213, 94, 0),
          Rgba::from_u8(204, 120, 188),
          Rgba::from_u8(202, 145, 97),
          Rgba::from_u8(251, 175, 228),
          Rgba::from_u8(148, 148, 148),
      };
    case FlashPalette::Deep:
    default:
      return {
          Rgba::from_u8(76, 114, 176),
          Rgba::from_u8(221, 132, 82),
          Rgba::from_u8(85, 168, 104),
          Rgba::from_u8(196, 78, 82),
          Rgba::from_u8(129, 114, 179),
          Rgba::from_u8(147, 120, 96),
          Rgba::from_u8(218, 139, 195),
          Rgba::from_u8(140, 140, 140),
      };
  }
}

[[nodiscard]] inline PaletteRamp flash_palette_ramp(FlashPalette palette) {
  std::string name = "deep";
  switch (palette) {
    case FlashPalette::Muted:
      name = "muted";
      break;
    case FlashPalette::Pastel:
      name = "pastel";
      break;
    case FlashPalette::Bright:
      name = "bright";
      break;
    case FlashPalette::Dark:
      name = "dark";
      break;
    case FlashPalette::Colorblind:
      name = "colorblind";
      break;
    case FlashPalette::Deep:
    default:
      name = "deep";
      break;
  }
  return PaletteRamp(std::move(name), flash_palette_colors(palette));
}

[[nodiscard]] inline PaletteRamp flash_palette(FlashPalette palette = FlashPalette::Deep) {
  return flash_palette_ramp(palette);
}

[[nodiscard]] inline PaletteRamp flash_palette(std::string_view name) {
  return flash_palette(flash_palette_kind(name));
}

[[nodiscard]] inline Theme flash_theme(FlashStyle style = FlashStyle::Whitegrid,
                                         FlashPalette palette = FlashPalette::Deep) {
  Rgba background = Rgba::from_u8(255, 255, 255);
  Rgba foreground = Rgba::from_u8(34, 34, 34);
  std::string theme_name = "flash_whitegrid";
  switch (style) {
    case FlashStyle::Darkgrid:
      // Darkgrid keeps a neutral figure background; the renderer shades only the plot face.
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_darkgrid";
      break;
    case FlashStyle::Whitegrid:
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_whitegrid";
      break;
    case FlashStyle::Dark:
      background = Rgba::from_u8(236, 240, 245);
      foreground = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_dark";
      break;
    case FlashStyle::White:
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(26, 26, 26);
      theme_name = "flash_white";
      break;
    case FlashStyle::Ticks:
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(26, 26, 26);
      theme_name = "flash_ticks";
      break;
  }
  return Theme{
      std::move(theme_name),
      flash_palette_colors(palette),
      background,
      foreground,
      flash_palette_ramp(palette),
  };
}

[[nodiscard]] inline std::tuple<double, double, double, double> flash_context_values(
    FlashContext context) {
  switch (context) {
    case FlashContext::Paper:
      return {920.0, 640.0, 0.92, 0.94};
    case FlashContext::Talk:
      return {1440.0, 900.0, 1.18, 1.14};
    case FlashContext::Poster:
      return {1840.0, 1160.0, 1.36, 1.3};
    case FlashContext::Notebook:
    default:
      return {1200.0, 780.0, 1.0, 1.0};
  }
}

inline Studio& set(Studio& studio, Theme theme = Theme::paper()) {
  studio.theme(std::move(theme));
  return studio;
}

inline Studio& set_theme(Studio& studio, Theme theme = Theme::paper()) {
  return set(studio, std::move(theme));
}

inline Pane& set_style(Pane& pane, GridStyle style = GridStyle::Major) {
  return pane.grid(style);
}

inline Pane& set_style(Pane& pane, FlashStyle style) {
  const Theme base = pane.theme_tokens();
  Rgba bg = base.background();
  Rgba fg = base.foreground();
  std::string theme_name = "flash_style_whitegrid";
  switch (style) {
    case FlashStyle::Darkgrid:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_style_darkgrid";
      break;
    case FlashStyle::Whitegrid:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_style_whitegrid";
      break;
    case FlashStyle::Dark:
      bg = Rgba::from_u8(236, 240, 245);
      fg = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_style_dark";
      break;
    case FlashStyle::White:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(26, 26, 26);
      theme_name = "flash_style_white";
      break;
    case FlashStyle::Ticks:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(26, 26, 26);
      theme_name = "flash_style_ticks";
      break;
  }
  pane.theme_tokens() = Theme{std::move(theme_name), base.accents(), bg, fg, base.surface_palette()};
  switch (style) {
    case FlashStyle::Darkgrid:
    case FlashStyle::Whitegrid:
      pane.axes_visible(true)
          .grid(GridStyle::Major)
          .x_major_grid(true)
          .y_major_grid(true)
          .z_major_grid(true);
      break;
    case FlashStyle::Dark:
    case FlashStyle::White:
    case FlashStyle::Ticks:
      pane.axes_visible(true)
          .grid(GridStyle::None)
          .x_major_grid(false)
          .y_major_grid(false)
          .z_major_grid(false)
          .x_minor_grid(false)
          .y_minor_grid(false)
          .z_minor_grid(false);
      break;
  }
  return pane;
}

inline Pane& set_style(Pane& pane, std::string_view style) {
  return set_style(pane, flash_style(style));
}

inline Pane& axes_style(Pane& pane, GridStyle style = GridStyle::Major) {
  return set_style(pane, style);
}

inline Pane& axes_style(Pane& pane, FlashStyle style) {
  return set_style(pane, style);
}

inline Pane& axes_style(Pane& pane, std::string_view style) {
  return set_style(pane, flash_style(style));
}

inline Studio& set_context(
    Studio& studio,
    double width,
    double height,
    double tick_font_scale = 1.0,
    double axis_label_font_scale = 1.0) {
  studio.size(width, height);
  for (const Pane& pane_const : studio.panes()) {
    auto& pane = const_cast<Pane&>(pane_const);
    detail::apply_tick_font_scale(pane, tick_font_scale, axis_label_font_scale);
  }
  return studio;
}

inline Studio& plotting_context(
    Studio& studio,
    double width,
    double height,
    double tick_font_scale = 1.0,
    double axis_label_font_scale = 1.0) {
  return set_context(studio, width, height, tick_font_scale, axis_label_font_scale);
}

inline Studio& set_context(
    Studio& studio,
    FlashContext context,
    std::optional<double> width = std::nullopt,
    std::optional<double> height = std::nullopt) {
  const auto [base_w, base_h, tick_scale, axis_scale] = flash_context_values(context);
  return set_context(
      studio,
      width.value_or(base_w),
      height.value_or(base_h),
      tick_scale,
      axis_scale);
}

inline Studio& set_context(
    Studio& studio,
    std::string_view context,
    std::optional<double> width = std::nullopt,
    std::optional<double> height = std::nullopt) {
  return set_context(studio, flash_context(context), width, height);
}

inline Studio& set_theme(Studio& studio, const FlashThemeOptions& options) {
  const Theme theme_tokens = flash_theme(options.style, options.palette);
  studio.theme(theme_tokens);
  set_context(studio, options.context, options.width, options.height);
  for (const Pane& pane_const : studio.panes()) {
    auto& pane = const_cast<Pane&>(pane_const);
    pane.theme_tokens() = theme_tokens;
    set_style(pane, options.style);
  }
  return studio;
}

inline Studio& set_theme(
    Studio& studio,
    std::string_view style,
    std::string_view context = "notebook",
    std::string_view palette = "deep") {
  FlashThemeOptions options;
  options.style = flash_style(style);
  options.context = flash_context(context);
  options.palette = flash_palette_kind(palette);
  return set_theme(studio, options);
}

inline Studio& tight_layout(Studio& studio, double padding = 6.0) {
  return studio.tight_layout(true, padding);
}

inline Pane& despine(Pane& pane, bool = true) noexcept {
  return pane;
}

inline Pane& tickless(Pane& pane, bool enable = true) {
  if (!enable) {
    pane.x_tick_labels(true).y_tick_labels(true).z_tick_labels(true).axes_visible(true);
    return pane;
  }
  pane.axes_visible(false).x_tick_labels(true).y_tick_labels(true).z_tick_labels(true);
  return pane;
}

inline Pane& horizontal_grid(Pane& pane, bool enable = true) noexcept {
  pane.grid(enable ? GridStyle::Major : GridStyle::None);
  pane.x_major_grid(false).y_major_grid(enable).z_major_grid(false);
  return pane;
}

inline Pane& move_legend(Pane& pane, KeyPlace place) noexcept {
  return pane.legend(place);
}

inline Studio& move_legend(Studio& studio, KeyDock dock) noexcept {
  return studio.legend(dock);
}

inline Pane& legend(Pane& pane, KeyPlace place) noexcept {
  return pane.legend(place);
}

inline Studio& legend(Studio& studio, KeyDock dock) noexcept {
  return studio.legend(dock);
}

inline Pane& legend_frame(Pane& pane, KeyFrame frame) noexcept {
  return pane.legend_frame(frame);
}

inline Studio& legend_frame(Studio& studio, KeyFrame frame) noexcept {
  return studio.legend_frame(frame);
}

inline bool show(Studio& studio,
                 std::string_view title = {},
                 std::int64_t width = 1280,
                 std::int64_t height = 820,
                 bool launch = true) {
  return studio.show(title, width, height, launch);
}

inline void save_png(Studio& studio, std::string_view path, std::int64_t dpi = 144) {
  studio.save_png(path, dpi);
}
