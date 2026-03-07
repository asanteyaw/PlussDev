#include "platform/PlatformHtmlViewer.h"

#include <chrono>
#include <cstdint>
#include <cmath>
#include <limits>
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
    const auto scene = window::build_html_document_scene(document, target_size, render_cfg);
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
        constexpr int kKeyStep = 96;
        int& target_x = use_table_scroll ? table_scroll_x : document_scroll_x;
        int& target_y = use_table_scroll ? table_scroll_y : document_scroll_y;
        if (event.note == "pan_left") {
          target_x = clamp_nonneg_int64(static_cast<std::int64_t>(target_x) - kKeyStep);
          needs_render = true;
        } else if (event.note == "pan_right") {
          target_x = clamp_nonneg_int64(static_cast<std::int64_t>(target_x) + kKeyStep);
          needs_render = true;
        } else if (event.note == "pan_up") {
          target_y = clamp_nonneg_int64(static_cast<std::int64_t>(target_y) - kKeyStep);
          needs_render = true;
        } else if (event.note == "pan_down") {
          target_y = clamp_nonneg_int64(static_cast<std::int64_t>(target_y) + kKeyStep);
          needs_render = true;
        } else if (event.note == "reset_view") {
          table_scroll_x = 0;
          table_scroll_y = 0;
          document_scroll_x = 0;
          document_scroll_y = 0;
          needs_render = true;
        }
      } else if (event.kind == window::ShellEventKind::PointerScroll) {
        const double abs_x = std::abs(event.delta_x);
        const double abs_y = std::abs(event.delta_y);
        if (abs_x > 0.0 || abs_y > 0.0) {
          int& target_x = use_table_scroll ? table_scroll_x : document_scroll_x;
          int& target_y = use_table_scroll ? table_scroll_y : document_scroll_y;
          if (abs_x >= abs_y) {
            const std::int64_t delta = static_cast<std::int64_t>(std::llround(event.delta_x * 22.0));
            if (delta != 0) {
              target_x = clamp_nonneg_int64(static_cast<std::int64_t>(target_x) + delta);
              needs_render = true;
            }
          } else {
            const std::int64_t delta = static_cast<std::int64_t>(std::llround(-event.delta_y * 18.0));
            if (delta != 0) {
              target_y = clamp_nonneg_int64(static_cast<std::int64_t>(target_y) + delta);
              needs_render = true;
            }
          }
        }
      } else if (event.kind == window::ShellEventKind::PointerDrag) {
        const std::int64_t dx = static_cast<std::int64_t>(std::llround(-event.delta_x * 16.0));
        const std::int64_t dy = static_cast<std::int64_t>(std::llround(event.delta_y * 16.0));
        if (dx != 0 || dy != 0) {
          int& target_x = use_table_scroll ? table_scroll_x : document_scroll_x;
          int& target_y = use_table_scroll ? table_scroll_y : document_scroll_y;
          target_x = clamp_nonneg_int64(static_cast<std::int64_t>(target_x) + dx);
          target_y = clamp_nonneg_int64(static_cast<std::int64_t>(target_y) + dy);
          needs_render = true;
        }
      } else if (event.kind == window::ShellEventKind::PointerDown) {
        const int px = clamp_nonneg_int64(static_cast<std::int64_t>(std::llround(event.x)));
        const int py = clamp_nonneg_int64(static_cast<std::int64_t>(std::llround(event.y)));
        bool consumed = false;
        for (auto it = hit_regions.rbegin(); it != hit_regions.rend(); ++it) {
          if (!region_contains(*it, px, py)) {
            continue;
          }
          consumed = true;
          if (it->action == "table.sort.column") {
            const int next_col = it->value;
            if (table_sort_column == next_col && table_sort_target == it->target) {
              table_sort_ascending = !table_sort_ascending;
            } else {
              table_sort_column = next_col;
              table_sort_target = it->target;
              table_sort_ascending = true;
            }
            needs_render = true;
          } else if (it->action == "button.click") {
            if (it->target == "zoom-reset") {
              table_scroll_x = 0;
              table_scroll_y = 0;
              document_scroll_x = 0;
              document_scroll_y = 0;
              needs_render = true;
            } else if (it->target == "copy-visible" && !snapshot_name.empty()) {
              (void)window::save_canvas(runtime.canvas(), snapshot_name);
            }
          } else if (it->action == "input.focus") {
            active_input_target = it->target;
            if (!input_values.contains(active_input_target)) {
              input_values.emplace(active_input_target, std::string{});
            }
            needs_render = true;
          }
          break;
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
