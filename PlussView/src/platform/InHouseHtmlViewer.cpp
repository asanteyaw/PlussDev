#include "platform/PlatformHtmlViewer.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "pluss/view/html_window.h"
#include "pluss/view/window.h"

namespace pluss::view::detail {

namespace {

[[nodiscard]] int clamp_positive(const std::int64_t value, const int fallback) {
  if (value <= 0) {
    return fallback;
  }
  if (value > static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(value);
}

[[nodiscard]] int clamp_nonneg_int64(const std::int64_t value) {
  if (value <= 0) {
    return 0;
  }
  if (value > static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(value);
}

[[nodiscard]] bool node_contains_tag(const window::HtmlNode& node, const std::string_view tag) {
  if (node.tag == tag) {
    return true;
  }
  for (const auto& child : node.children) {
    if (node_contains_tag(child, tag)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool region_contains(const window::HtmlHitRegion& region, const int x, const int y) {
  return x >= region.rect.x && y >= region.rect.y && x < (region.rect.x + region.rect.width) &&
         y < (region.rect.y + region.rect.height);
}

[[nodiscard]] std::optional<std::size_t> find_hit_region_index(const std::vector<window::HtmlHitRegion>& regions,
                                                               const int x,
                                                               const int y) {
  for (std::size_t i = regions.size(); i > 0; --i) {
    if (region_contains(regions[i - 1], x, y)) {
      return i - 1;
    }
  }
  return std::nullopt;
}

void erase_last_utf8_codepoint(std::string& text) {
  if (text.empty()) {
    return;
  }
  std::size_t i = text.size();
  do {
    --i;
  } while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0U) == 0x80U);
  text.erase(i);
}

struct ScrollBinding {
  int* target{nullptr};
  int limit{0};
  int viewport_span{0};
  int content_hint{0};
};

[[nodiscard]] int clamp_scroll_to_limit(const std::int64_t value, const int limit) {
  if (value <= 0 || limit <= 0) {
    return 0;
  }
  const auto bounded = std::min<std::int64_t>(value, static_cast<std::int64_t>(limit));
  return static_cast<int>(bounded);
}

[[nodiscard]] ScrollBinding select_scroll_binding(const bool prefer_table,
                                                  const bool horizontal,
                                                  const window::HtmlRenderStats& stats,
                                                  int& table_scroll_x,
                                                  int& table_scroll_y,
                                                  int& document_scroll_x,
                                                  int& document_scroll_y) {
  ScrollBinding table_binding{};
  table_binding.target = horizontal ? &table_scroll_x : &table_scroll_y;
  table_binding.limit = horizontal ? stats.table_max_scroll_x : stats.table_max_scroll_y;
  table_binding.viewport_span = horizontal ? stats.table_viewport_rect.width : stats.table_viewport_rect.height;
  table_binding.content_hint = horizontal ? stats.table_viewport_rect.width : stats.table_body_row_height_px;

  ScrollBinding doc_binding{};
  doc_binding.target = horizontal ? &document_scroll_x : &document_scroll_y;
  doc_binding.limit = horizontal ? stats.document_max_scroll_x : stats.document_max_scroll_y;
  doc_binding.viewport_span = horizontal ? stats.document_viewport_rect.width : stats.document_viewport_rect.height;
  doc_binding.content_hint = doc_binding.viewport_span;

  if (prefer_table && table_binding.limit > 0) {
    return table_binding;
  }
  if (doc_binding.limit > 0) {
    return doc_binding;
  }
  return prefer_table ? table_binding : doc_binding;
}

[[nodiscard]] int adaptive_key_step(const ScrollBinding& binding) {
  if (binding.limit <= 0) {
    return 0;
  }
  const double span = std::max(1.0, static_cast<double>(binding.viewport_span));
  const double travel_ratio = static_cast<double>(binding.limit) / span;
  double step = std::sqrt(span) * std::log1p(1.0 + travel_ratio);
  if (binding.content_hint > 0) {
    const double hint = std::sqrt(static_cast<double>(binding.content_hint));
    step = std::max(step, hint * std::sqrt(std::max(1.0, std::log1p(span))));
  }
  if (!std::isfinite(step) || step <= 0.0) {
    return 0;
  }
  return std::max(1, static_cast<int>(std::lround(step)));
}

[[nodiscard]] double adaptive_pointer_delta_px(const double raw_delta, const ScrollBinding& binding) {
  if (binding.limit <= 0 || raw_delta == 0.0) {
    return 0.0;
  }
  const double span = std::max(1.0, static_cast<double>(binding.viewport_span));
  const double travel_ratio = static_cast<double>(binding.limit) / std::max(1.0, span);
  const double density = std::max(1.0, std::log1p(1.0 + travel_ratio));
  const double hint = std::max(span, static_cast<double>(binding.content_hint));
  const double hint_factor = std::log1p(1.0 + hint / std::max(1.0, span));
  const double gain = density * hint_factor;
  // Positive wheel/drag deltas should move content in the same direction as native web views.
  return -raw_delta * gain;
}

[[nodiscard]] int consume_residual_pixels(double& residual) {
  if (!std::isfinite(residual)) {
    residual = 0.0;
    return 0;
  }
  const int step = static_cast<int>(std::llround(residual));
  if (step == 0) {
    return 0;
  }
  residual -= static_cast<double>(step);
  return step;
}

[[nodiscard]] bool apply_scroll_delta(const ScrollBinding& binding, const int delta_px) {
  if (binding.target == nullptr || delta_px == 0) {
    return false;
  }
  const int before = *binding.target;
  *binding.target = clamp_scroll_to_limit(static_cast<std::int64_t>(before) + delta_px, binding.limit);
  return *binding.target != before;
}

} // namespace

bool inhouse_html_viewer_available() {
  if (!runtime_gui_environment_available()) {
    return false;
  }
  auto shell = window::make_window_shell(window::ShellSelectionPolicy::NativeOnly);
  return static_cast<bool>(shell);
}

bool show_inhouse_html_window(const std::string& title,
                              const std::string& html,
                              const std::int64_t width,
                              const std::int64_t height,
                              const std::string& snapshot_name,
                              const std::string& svg_payload,
                              const std::string& svg_snapshot_name) {
  (void)svg_snapshot_name;

  if ((html.empty() && svg_payload.empty()) || !inhouse_html_viewer_available()) {
    return false;
  }

  auto shell = window::make_window_shell(window::ShellSelectionPolicy::NativeOnly);
  if (!shell) {
    return false;
  }

  window::SizeI size{
      clamp_positive(width, 1280),
      clamp_positive(height, 760),
  };

  window::RuntimeConfig runtime_cfg{};
  runtime_cfg.canvas_size = size;
  runtime_cfg.background = window::Color::rgba(245, 248, 252, 255);
  window::ViewerRuntime runtime(runtime_cfg);

  const std::string& payload = svg_payload.empty() ? html : svg_payload;
  auto document = window::parse_html_document(payload, title.empty() ? std::string("PlussView") : title);
  window::HtmlRenderConfig render_cfg{};
  window::HtmlRenderStats render_stats{};
  std::vector<window::HtmlHitRegion> hit_regions;
  hit_regions.reserve(512);
  int table_scroll_x = 0;
  int table_scroll_y = 0;
  int document_scroll_x = 0;
  int document_scroll_y = 0;
  int table_sort_column = -1;
  bool table_sort_ascending = true;
  std::string table_sort_target;
  std::string active_input_target;
  std::unordered_map<std::string, std::string> input_values;
  const bool use_table_scroll = node_contains_tag(document.root, "table");
  double wheel_residual_x = 0.0;
  double wheel_residual_y = 0.0;
  double drag_residual_x = 0.0;
  double drag_residual_y = 0.0;

  if (!shell->open(window::ShellConfig{document.title, size})) {
    return false;
  }

  auto render_current = [&](const window::SizeI target_size) -> bool {
    auto cfg = runtime_cfg;
    cfg.canvas_size = target_size;
    if (!runtime.configure(cfg)) {
      return false;
    }
    render_cfg.document_scroll_x_px = document_scroll_x;
    render_cfg.document_scroll_y_px = document_scroll_y;
    render_cfg.table_scroll_x_px = table_scroll_x;
    render_cfg.table_scroll_y_px = table_scroll_y;
    render_cfg.table_sort_column = table_sort_column;
    render_cfg.table_sort_ascending = table_sort_ascending;
    render_cfg.table_sort_target = table_sort_target;
    render_cfg.active_input_target = active_input_target;
    if (!active_input_target.empty()) {
      const auto it = input_values.find(active_input_target);
      render_cfg.active_input_value = (it != input_values.end()) ? it->second : std::string{};
    } else {
      render_cfg.active_input_value.clear();
    }
    auto search_it = input_values.find("table-search");
    if (search_it == input_values.end()) {
      search_it = input_values.find("table_search");
    }
    render_cfg.table_filter_query = (search_it != input_values.end()) ? search_it->second : std::string{};
    render_cfg.table_filter_target.clear();
    hit_regions.clear();
    render_cfg.hit_regions = &hit_regions;
    render_cfg.render_stats = &render_stats;
    const auto scene = window::build_html_document_scene(document, target_size, render_cfg);
    table_scroll_x = clamp_scroll_to_limit(table_scroll_x, render_stats.table_max_scroll_x);
    table_scroll_y = clamp_scroll_to_limit(table_scroll_y, render_stats.table_max_scroll_y);
    document_scroll_x = clamp_scroll_to_limit(document_scroll_x, render_stats.document_max_scroll_x);
    document_scroll_y = clamp_scroll_to_limit(document_scroll_y, render_stats.document_max_scroll_y);
    if (!runtime.submit(scene)) {
      return false;
    }
    return shell->present(runtime.canvas());
  };

  if (!render_current(size)) {
    shell->close();
    return false;
  }

  if (!snapshot_name.empty()) {
    (void)window::save_canvas(runtime.canvas(), snapshot_name);
  }

  bool closed = false;
  while (!closed) {
    bool resized = false;
    bool needs_render = false;
    window::SizeI next_size = size;

    for (const auto& event : shell->poll_events()) {
      if (event.kind == window::ShellEventKind::Closed) {
        closed = true;
      } else if (event.kind == window::ShellEventKind::ResizeRequested &&
                 event.size.width > 0 && event.size.height > 0) {
        next_size = event.size;
        resized = true;
      } else if (event.kind == window::ShellEventKind::ShortcutCommand &&
                 event.note == "save_snapshot" && !snapshot_name.empty()) {
        (void)window::save_canvas(runtime.canvas(), snapshot_name);
      } else if (event.kind == window::ShellEventKind::ShortcutCommand) {
        const ScrollBinding x_binding =
            select_scroll_binding(use_table_scroll, true, render_stats, table_scroll_x, table_scroll_y, document_scroll_x, document_scroll_y);
        const ScrollBinding y_binding =
            select_scroll_binding(use_table_scroll, false, render_stats, table_scroll_x, table_scroll_y, document_scroll_x, document_scroll_y);
        if (event.note == "pan_left") {
          needs_render = apply_scroll_delta(x_binding, -adaptive_key_step(x_binding)) || needs_render;
        } else if (event.note == "pan_right") {
          needs_render = apply_scroll_delta(x_binding, adaptive_key_step(x_binding)) || needs_render;
        } else if (event.note == "pan_up") {
          needs_render = apply_scroll_delta(y_binding, -adaptive_key_step(y_binding)) || needs_render;
        } else if (event.note == "pan_down") {
          needs_render = apply_scroll_delta(y_binding, adaptive_key_step(y_binding)) || needs_render;
        } else if (event.note == "reset_view") {
          table_scroll_x = 0;
          table_scroll_y = 0;
          document_scroll_x = 0;
          document_scroll_y = 0;
          wheel_residual_x = 0.0;
          wheel_residual_y = 0.0;
          drag_residual_x = 0.0;
          drag_residual_y = 0.0;
          needs_render = true;
        }
      } else if (event.kind == window::ShellEventKind::PointerScroll) {
        const double abs_x = std::abs(event.delta_x);
        const double abs_y = std::abs(event.delta_y);
        if (abs_x > 0.0 || abs_y > 0.0) {
          const ScrollBinding x_binding =
              select_scroll_binding(use_table_scroll, true, render_stats, table_scroll_x, table_scroll_y, document_scroll_x, document_scroll_y);
          const ScrollBinding y_binding =
              select_scroll_binding(use_table_scroll, false, render_stats, table_scroll_x, table_scroll_y, document_scroll_x, document_scroll_y);
          if (x_binding.limit <= 0) {
            wheel_residual_x = 0.0;
          }
          if (y_binding.limit <= 0) {
            wheel_residual_y = 0.0;
          }

          wheel_residual_x += adaptive_pointer_delta_px(event.delta_x, x_binding);
          wheel_residual_y += adaptive_pointer_delta_px(event.delta_y, y_binding);

          needs_render = apply_scroll_delta(x_binding, consume_residual_pixels(wheel_residual_x)) || needs_render;
          needs_render = apply_scroll_delta(y_binding, consume_residual_pixels(wheel_residual_y)) || needs_render;
        }
      } else if (event.kind == window::ShellEventKind::PointerDrag) {
        const ScrollBinding x_binding =
            select_scroll_binding(use_table_scroll, true, render_stats, table_scroll_x, table_scroll_y, document_scroll_x, document_scroll_y);
        const ScrollBinding y_binding =
            select_scroll_binding(use_table_scroll, false, render_stats, table_scroll_x, table_scroll_y, document_scroll_x, document_scroll_y);
        if (x_binding.limit <= 0) {
          drag_residual_x = 0.0;
        }
        if (y_binding.limit <= 0) {
          drag_residual_y = 0.0;
        }
        drag_residual_x += -event.delta_x;
        drag_residual_y += event.delta_y;
        needs_render = apply_scroll_delta(x_binding, consume_residual_pixels(drag_residual_x)) || needs_render;
        needs_render = apply_scroll_delta(y_binding, consume_residual_pixels(drag_residual_y)) || needs_render;
      } else if (event.kind == window::ShellEventKind::PointerDown) {
        const int px = clamp_nonneg_int64(static_cast<std::int64_t>(std::llround(event.x)));
        const int py = clamp_nonneg_int64(static_cast<std::int64_t>(std::llround(event.y)));
        bool consumed = false;
        auto hit_idx = find_hit_region_index(hit_regions, px, py);
        if (!hit_idx.has_value() && size.height > 0) {
          const int py_flipped =
              clamp_nonneg_int64(static_cast<std::int64_t>(size.height) - static_cast<std::int64_t>(py));
          hit_idx = find_hit_region_index(hit_regions, px, py_flipped);
        }
        if (hit_idx.has_value()) {
          consumed = true;
          const auto& hit = hit_regions[*hit_idx];
          if (hit.action == "table.sort.column") {
            const int next_col = hit.value;
            if (table_sort_column == next_col && table_sort_target == hit.target) {
              table_sort_ascending = !table_sort_ascending;
            } else {
              table_sort_column = next_col;
              table_sort_target = hit.target;
              table_sort_ascending = true;
            }
            needs_render = true;
          } else if (hit.action == "button.click") {
            if (hit.target == "zoom-reset") {
              table_scroll_x = 0;
              table_scroll_y = 0;
              document_scroll_x = 0;
              document_scroll_y = 0;
              wheel_residual_x = 0.0;
              wheel_residual_y = 0.0;
              drag_residual_x = 0.0;
              drag_residual_y = 0.0;
              needs_render = true;
            } else if (hit.target == "copy-visible" && !snapshot_name.empty()) {
              (void)window::save_canvas(runtime.canvas(), snapshot_name);
            }
          } else if (hit.action == "input.focus") {
            active_input_target = hit.target;
            if (!input_values.contains(active_input_target)) {
              input_values.emplace(active_input_target, std::string{});
            }
            needs_render = true;
          }
        }
        if (!consumed && !active_input_target.empty()) {
          active_input_target.clear();
          needs_render = true;
        }
      } else if (event.kind == window::ShellEventKind::TextInput) {
        if (!active_input_target.empty()) {
          std::string& value = input_values[active_input_target];
          if (event.note == "__backspace__") {
            erase_last_utf8_codepoint(value);
            needs_render = true;
          } else if (event.note == "__clear__") {
            value.clear();
            needs_render = true;
          } else if (!event.note.empty() && value.size() < 512) {
            value += event.note;
            needs_render = true;
          }
        }
      }
    }

    if (closed) {
      break;
    }
    if (resized) {
      size = next_size;
      wheel_residual_x = 0.0;
      wheel_residual_y = 0.0;
      drag_residual_x = 0.0;
      drag_residual_y = 0.0;
      needs_render = true;
    }
    if (needs_render) {
      if (!render_current(size)) {
        shell->close();
        return false;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  shell->close();
  return true;
}

} // namespace pluss::view::detail
