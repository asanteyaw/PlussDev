// This header is intentionally included from flash.h inside namespace pluss::pl::flash.

enum class FlashContext : std::uint8_t {
  Paper,
  Notebook,
  Talk,
  Poster,
};

enum class FlashStyle : std::uint8_t {
  Darkgrid,
  Lightgrid,
  Sandgrid,
  Dark,
  Light,
  Sand,
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
  FlashStyle style{FlashStyle::Lightgrid};
  FlashContext context{FlashContext::Notebook};
  bool borderless{false};
  bool horizontal_grid{false};
  std::optional<double> width{std::nullopt};
  std::optional<double> height{std::nullopt};
};

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
          Rgba::from_u8(140, 97, 60),
          Rgba::from_u8(220, 126, 192),
          Rgba::from_u8(121, 121, 121),
          Rgba::from_u8(213, 187, 103),
          Rgba::from_u8(130, 198, 226),
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
          Rgba::from_u8(255, 254, 163),
          Rgba::from_u8(185, 242, 240),
      };
    case FlashPalette::Bright:
      return {
          Rgba::from_u8(2, 62, 255),
          Rgba::from_u8(255, 124, 0),
          Rgba::from_u8(26, 201, 56),
          Rgba::from_u8(232, 0, 11),
          Rgba::from_u8(139, 43, 226),
          Rgba::from_u8(159, 72, 0),
          Rgba::from_u8(241, 76, 193),
          Rgba::from_u8(163, 163, 163),
          Rgba::from_u8(255, 196, 0),
          Rgba::from_u8(0, 215, 255),
      };
    case FlashPalette::Dark:
      return {
          Rgba::from_u8(0, 44, 127),
          Rgba::from_u8(177, 64, 13),
          Rgba::from_u8(18, 113, 28),
          Rgba::from_u8(140, 8, 1),
          Rgba::from_u8(89, 30, 114),
          Rgba::from_u8(89, 47, 13),
          Rgba::from_u8(162, 53, 131),
          Rgba::from_u8(60, 60, 60),
          Rgba::from_u8(184, 133, 10),
          Rgba::from_u8(0, 99, 117),
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
          Rgba::from_u8(236, 225, 51),
          Rgba::from_u8(86, 180, 233),
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
          Rgba::from_u8(204, 185, 116),
          Rgba::from_u8(100, 181, 205),
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

[[nodiscard]] inline Theme flash_theme(FlashStyle style = FlashStyle::Lightgrid,
                                       FlashPalette palette = FlashPalette::Deep) {
  Rgba background = Rgba::from_u8(255, 255, 255);
  Rgba foreground = Rgba::from_u8(34, 34, 34);
  std::string theme_name = "flash_lightgrid";
  switch (style) {
    case FlashStyle::Darkgrid:
      // Darkgrid keeps a neutral figure background; the renderer shades only the plot face.
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_darkgrid";
      break;
    case FlashStyle::Lightgrid:
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_lightgrid";
      break;
    case FlashStyle::Sandgrid:
      // Sandgrid keeps the neutral panel and paints a warm plot face in the renderer.
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(56, 52, 40);
      theme_name = "flash_sandgrid";
      break;
    case FlashStyle::Dark:
      // Dark style shades only the plot face; keep panel background neutral.
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_dark";
      break;
    case FlashStyle::Light:
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(26, 26, 26);
      theme_name = "flash_light";
      break;
    case FlashStyle::Sand:
      background = Rgba::from_u8(255, 255, 255);
      foreground = Rgba::from_u8(56, 52, 40);
      theme_name = "flash_sand";
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
      return {920.0, 640.0, 0.8, 0.8};
    case FlashContext::Talk:
      return {1440.0, 900.0, 1.3, 1.3};
    case FlashContext::Poster:
      return {1840.0, 1160.0, 1.6, 1.6};
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
  std::string theme_name = "flash_style_lightgrid";
  switch (style) {
    case FlashStyle::Darkgrid:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_style_darkgrid";
      break;
    case FlashStyle::Lightgrid:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_style_lightgrid";
      break;
    case FlashStyle::Sandgrid:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(56, 52, 40);
      theme_name = "flash_style_sandgrid";
      break;
    case FlashStyle::Dark:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(34, 34, 34);
      theme_name = "flash_style_dark";
      break;
    case FlashStyle::Light:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(26, 26, 26);
      theme_name = "flash_style_light";
      break;
    case FlashStyle::Sand:
      bg = Rgba::from_u8(255, 255, 255);
      fg = Rgba::from_u8(56, 52, 40);
      theme_name = "flash_style_sand";
      break;
  }
  pane.theme_tokens() = Theme{std::move(theme_name), base.accents(), bg, fg, base.surface_palette()};
  switch (style) {
    case FlashStyle::Darkgrid:
    case FlashStyle::Lightgrid:
    case FlashStyle::Sandgrid:
      pane.axes_visible(true)
          .grid(GridStyle::Major)
          .x_major_grid(true)
          .y_major_grid(true)
          .z_major_grid(true);
      break;
    case FlashStyle::Dark:
    case FlashStyle::Light:
    case FlashStyle::Sand:
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

inline Pane& axes_style(Pane& pane, GridStyle style = GridStyle::Major) {
  return set_style(pane, style);
}

inline Pane& axes_style(Pane& pane, FlashStyle style) {
  return set_style(pane, style);
}

inline Pane& apply_horizontal_grid_policy(Pane& pane, bool enable) noexcept {
  if (pane.kind() != PaneKind::Planar) {
    return pane;
  }
  const std::string_view theme_name = pane.theme_tokens().name();
  const bool grid_style = theme_name.find("darkgrid") != std::string_view::npos ||
                          theme_name.find("lightgrid") != std::string_view::npos ||
                          theme_name.find("sandgrid") != std::string_view::npos;

  if (!grid_style) {
    // Dark/light styles remain gridless even when horizontal_grid=true.
    pane.grid(GridStyle::None)
        .x_major_grid(false)
        .y_major_grid(false)
        .z_major_grid(false)
        .x_minor_grid(false)
        .y_minor_grid(false)
        .z_minor_grid(false);
    return pane;
  }

  if (!enable) {
    // Restore default grid-style behavior (both axes major grid on).
    pane.grid(GridStyle::Major)
        .x_major_grid(true)
        .y_major_grid(true)
        .z_major_grid(true);
    return pane;
  }

  pane.grid(GridStyle::Major)
      .x_major_grid(false)
      .y_major_grid(true)
      .z_major_grid(false)
      .x_minor_grid(false)
      .z_minor_grid(false);
  return pane;
}

inline Pane& set_theme(Pane& pane, const FlashThemeOptions& options) {
  set_style(pane, options.style);
  const auto [base_w, base_h, tick_scale, axis_scale] = flash_context_values(options.context);
  (void)base_w;
  (void)base_h;
  detail::apply_tick_font_scale(pane, tick_scale, axis_scale);
  if (options.borderless) {
    pane.axes_visible(false);
  }
  apply_horizontal_grid_policy(pane, options.horizontal_grid);
  return pane;
}

inline Pane& set_theme(
    Pane& pane,
    FlashStyle style,
    FlashContext context = FlashContext::Notebook,
    bool borderless = false,
    bool horizontal_grid = false) {
  FlashThemeOptions options;
  options.style = style;
  options.context = context;
  options.borderless = borderless;
  options.horizontal_grid = horizontal_grid;
  return set_theme(pane, options);
}

inline Studio& set_context(
    Studio& studio,
    double width,
    double height,
    double tick_font_scale = 1.0,
    double axis_label_font_scale = 1.0) {
  studio.size(width, height);
  studio.context_scales(tick_font_scale, axis_label_font_scale);
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

inline Studio& set_theme(Studio& studio, const FlashThemeOptions& options) {
  const Theme base_theme = studio.theme_tokens();
  set_context(studio, options.context, options.width, options.height);
  Pane pane_defaults(PaneKind::Planar, base_theme, {});
  set_style(pane_defaults, options.style);
  if (options.borderless) {
    pane_defaults.axes_visible(false);
  }
  apply_horizontal_grid_policy(pane_defaults, options.horizontal_grid);
  studio.pane_style_defaults(pane_defaults);
  for (const Pane& pane_const : studio.panes()) {
    auto& pane = const_cast<Pane&>(pane_const);
    set_style(pane, options.style);
    if (options.borderless) {
      pane.axes_visible(false);
    }
    apply_horizontal_grid_policy(pane, options.horizontal_grid);
  }
  return studio;
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
  return apply_horizontal_grid_policy(pane, enable);
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
