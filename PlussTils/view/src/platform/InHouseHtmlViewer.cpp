#include "platform/PlatformHtmlViewer.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "pluss/tils/view/html_window.h"
#include "pluss/tils/view/window.h"

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

[[nodiscard]] std::string lowercase(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char ch : value) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
}

[[nodiscard]] const std::string* attribute_value(const window::HtmlNode& node,
                                                 const std::string_view name) {
  for (const auto& attribute : node.attributes) {
    if (attribute.name == name) {
      return &attribute.value;
    }
  }
  return nullptr;
}

[[nodiscard]] std::vector<std::string> class_tokens(const window::HtmlNode& node) {
  std::vector<std::string> tokens;
  const auto* value = attribute_value(node, "class");
  if (value == nullptr) {
    return tokens;
  }
  std::istringstream stream(*value);
  for (std::string token; stream >> token;) {
    tokens.push_back(std::move(token));
  }
  return tokens;
}

[[nodiscard]] bool has_class(const window::HtmlNode& node, const std::string_view class_name) {
  const auto classes = class_tokens(node);
  return std::find(classes.begin(), classes.end(), class_name) != classes.end();
}

void set_class(window::HtmlNode& node, const std::string_view class_name, const bool enabled) {
  auto classes = class_tokens(node);
  const auto found = std::find(classes.begin(), classes.end(), class_name);
  if (enabled && found == classes.end()) {
    classes.emplace_back(class_name);
  } else if (!enabled && found != classes.end()) {
    classes.erase(found);
  }

  auto attribute = std::find_if(node.attributes.begin(), node.attributes.end(), [](const auto& item) {
    return item.name == "class";
  });
  if (classes.empty()) {
    if (attribute != node.attributes.end()) {
      node.attributes.erase(attribute);
    }
    return;
  }

  std::string value;
  for (const auto& token : classes) {
    if (!value.empty()) {
      value.push_back(' ');
    }
    value += token;
  }
  if (attribute == node.attributes.end()) {
    node.attributes.push_back(window::HtmlAttribute{"class", std::move(value)});
  } else {
    attribute->value = std::move(value);
  }
}

void set_atlas_frame_visibility(window::HtmlNode& node, const std::optional<int> active_frame) {
  if (has_class(node, "atlas-panel")) {
    const auto* index = attribute_value(node, "data-frame-index");
    const int frame = index != nullptr ? std::atoi(index->c_str()) : -1;
    set_class(node, "active", !active_frame.has_value() || frame == *active_frame);
  } else if (has_class(node, "atlas-tab")) {
    const auto* index = attribute_value(node, "data-frame-index");
    const int frame = index != nullptr ? std::atoi(index->c_str()) : -1;
    set_class(node, "active", active_frame.has_value() && frame == *active_frame);
  }
  for (auto& child : node.children) {
    set_atlas_frame_visibility(child, active_frame);
  }
}

bool toggle_body_class(window::HtmlNode& node, const std::string_view class_name) {
  if (node.tag == "body") {
    const bool enabled = !has_class(node, class_name);
    set_class(node, class_name, enabled);
    return true;
  }
  for (auto& child : node.children) {
    if (toggle_body_class(child, class_name)) {
      return true;
    }
  }
  return false;
}

bool cycle_body_theme(window::HtmlNode& node) {
  if (node.tag == "body") {
    constexpr std::string_view themes[] = {
        "theme-midnight",
        "theme-ocean",
        "theme-violet",
    };
    int active = -1;
    for (int index = 0; index < 3; ++index) {
      if (has_class(node, themes[index])) {
        active = index;
      }
      set_class(node, themes[index], false);
    }
    if (active + 1 < 3) {
      set_class(node, themes[active + 1], true);
    }
    return true;
  }
  for (auto& child : node.children) {
    if (cycle_body_theme(child)) {
      return true;
    }
  }
  return false;
}

void collect_text(const window::HtmlNode& node, std::string& out) {
  if (node.kind == window::HtmlNodeKind::Text) {
    if (!out.empty() && !std::isspace(static_cast<unsigned char>(out.back()))) {
      out.push_back(' ');
    }
    out += node.text;
    return;
  }
  for (const auto& child : node.children) {
    collect_text(child, out);
  }
}

[[nodiscard]] std::string node_text(const window::HtmlNode& node) {
  std::string out;
  collect_text(node, out);
  while (!out.empty() && std::isspace(static_cast<unsigned char>(out.front()))) {
    out.erase(out.begin());
  }
  while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) {
    out.pop_back();
  }
  return out;
}

void collect_table_rows(const window::HtmlNode& node,
                        std::vector<std::vector<std::string>>& rows,
                        std::vector<std::string>& headers) {
  if (node.tag == "tr") {
    std::vector<std::string> cells;
    bool header = false;
    for (const auto& child : node.children) {
      if (child.tag != "th" && child.tag != "td") {
        continue;
      }
      header = header || child.tag == "th";
      cells.push_back(node_text(child));
    }
    if (!cells.empty()) {
      if (header && headers.empty()) {
        headers = std::move(cells);
      } else {
        rows.push_back(std::move(cells));
      }
    }
    return;
  }
  for (const auto& child : node.children) {
    collect_table_rows(child, rows, headers);
  }
}

[[nodiscard]] std::string table_target(const window::HtmlNode& node) {
  if (const auto* id = attribute_value(node, "id"); id != nullptr && !id->empty()) {
    return *id;
  }
  if (const auto* name = attribute_value(node, "name"); name != nullptr && !name->empty()) {
    return *name;
  }
  return "table@" + std::to_string(reinterpret_cast<std::uintptr_t>(&node));
}

[[nodiscard]] std::optional<double> numeric_value(const std::string& value) {
  char* end = nullptr;
  const double parsed = std::strtod(value.c_str(), &end);
  if (end == value.c_str() || *end != '\0' || !std::isfinite(parsed)) {
    return std::nullopt;
  }
  return parsed;
}

void append_visible_tables(const window::HtmlNode& node,
                           const bool visible,
                           const std::string& filter,
                           const std::string& sort_target,
                           const int sort_column,
                           const bool sort_ascending,
                           std::vector<std::string>& blocks) {
  const bool node_visible =
      visible && !has_class(node, "hidden") &&
      (!has_class(node, "atlas-panel") || has_class(node, "active"));
  if (!node_visible) {
    return;
  }

  if (node.tag == "table") {
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    collect_table_rows(node, rows, headers);
    const std::string needle = lowercase(filter);
    if (!needle.empty()) {
      rows.erase(std::remove_if(rows.begin(), rows.end(), [&](const auto& row) {
                   std::string haystack;
                   for (const auto& cell : row) {
                     haystack += lowercase(cell);
                     haystack.push_back(' ');
                   }
                   return haystack.find(needle) == std::string::npos;
                 }),
                 rows.end());
    }

    if (sort_column >= 0 && table_target(node) == sort_target) {
      std::stable_sort(rows.begin(), rows.end(), [&](const auto& lhs, const auto& rhs) {
        const std::string left =
            static_cast<std::size_t>(sort_column) < lhs.size() ? lhs[static_cast<std::size_t>(sort_column)] : "";
        const std::string right =
            static_cast<std::size_t>(sort_column) < rhs.size() ? rhs[static_cast<std::size_t>(sort_column)] : "";
        const auto left_number = numeric_value(left);
        const auto right_number = numeric_value(right);
        if (left_number.has_value() && right_number.has_value()) {
          if (*left_number == *right_number) {
            return false;
          }
          return sort_ascending ? *left_number < *right_number : *right_number < *left_number;
        }
        const std::string left_text = lowercase(left);
        const std::string right_text = lowercase(right);
        if (left_text == right_text) {
          return false;
        }
        return sort_ascending ? left_text < right_text : right_text < left_text;
      });
    }

    std::string block;
    const auto append_row = [&](const std::vector<std::string>& row) {
      for (std::size_t column = 0; column < row.size(); ++column) {
        if (column > 0) {
          block.push_back('\t');
        }
        block += row[column];
      }
      block.push_back('\n');
    };
    if (!headers.empty()) {
      append_row(headers);
    }
    for (const auto& row : rows) {
      append_row(row);
    }
    if (!block.empty()) {
      blocks.push_back(std::move(block));
    }
    return;
  }

  for (const auto& child : node.children) {
    append_visible_tables(
        child, node_visible, filter, sort_target, sort_column, sort_ascending, blocks);
  }
}

[[nodiscard]] std::string visible_table_text(const window::HtmlDocument& document,
                                             const std::string& filter,
                                             const std::string& sort_target,
                                             const int sort_column,
                                             const bool sort_ascending) {
  std::vector<std::string> blocks;
  append_visible_tables(
      document.root, true, filter, sort_target, sort_column, sort_ascending, blocks);
  std::string out;
  for (const auto& block : blocks) {
    if (!out.empty()) {
      out.push_back('\n');
    }
    out += block;
  }
  return out;
}

[[nodiscard]] bool write_text_file(const std::string& path, const std::string& content) {
  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  out.write(content.data(), static_cast<std::streamsize>(content.size()));
  return out.good();
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

  const std::string& payload = html.empty() ? svg_payload : html;
  auto document = window::parse_html_document(payload, title.empty() ? std::string("PlussTilsView") : title);
  window::HtmlRenderConfig render_cfg{};
  window::HtmlRenderStats render_stats{};
  window::HtmlScrollModel scroll_model;
  std::vector<window::HtmlHitRegion> hit_regions;
  hit_regions.reserve(512);
  int table_sort_column = -1;
  bool table_sort_ascending = true;
  std::string table_sort_target;
  std::string active_input_target;
  std::unordered_map<std::string, std::string> input_values;
  double widget_scale = 1.0;

  if (!shell->open(window::ShellConfig{document.title, size})) {
    return false;
  }

  auto render_current = [&](const window::SizeI target_size) -> bool {
    auto cfg = runtime_cfg;
    cfg.canvas_size = target_size;
    if (!runtime.configure(cfg)) {
      return false;
    }
    const auto document_offset = scroll_model.document_offset_px();
    const auto table_offsets = scroll_model.table_offsets_px();
    render_cfg.document_scroll_x_px = document_offset.x;
    render_cfg.document_scroll_y_px = document_offset.y;
    render_cfg.table_scroll_x_px = 0;
    render_cfg.table_scroll_y_px = 0;
    render_cfg.table_scroll_offsets = &table_offsets;
    render_cfg.widget_scale = widget_scale;
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
    scroll_model.update_metrics(render_stats);
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
  const auto save_png = [&]() {
    const std::string suggested =
        snapshot_name.empty() ? std::string("pluss-view.png")
                              : std::filesystem::path(snapshot_name).filename().string();
    const std::string path = shell->choose_save_path(suggested, "png");
    return !path.empty() && window::save_canvas(runtime.canvas(), path);
  };
  const auto save_svg = [&]() {
    if (svg_payload.empty()) {
      return false;
    }
    const std::string suggested =
        svg_snapshot_name.empty() ? std::string("pluss-plot.svg")
                                  : std::filesystem::path(svg_snapshot_name).filename().string();
    const std::string path = shell->choose_save_path(suggested, "svg");
    return !path.empty() && write_text_file(path, svg_payload);
  };

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
                 event.note == "save_snapshot") {
        (void)save_png();
      } else if (event.kind == window::ShellEventKind::ShortcutCommand) {
        if (event.note == "zoom_in") {
          widget_scale = std::min(3.0, widget_scale + 0.1);
          needs_render = true;
        } else if (event.note == "zoom_out") {
          widget_scale = std::max(0.35, widget_scale - 0.1);
          needs_render = true;
        } else if (event.note == "reset_view") {
          widget_scale = 1.0;
          scroll_model.reset();
          needs_render = true;
        } else {
          needs_render = scroll_model.command(event.note) || needs_render;
        }
      } else if (event.kind == window::ShellEventKind::PointerScroll) {
        needs_render =
            scroll_model.scroll_wheel(
                event.delta_x,
                event.delta_y,
                window::PointI{
                    static_cast<int>(std::lround(event.x)),
                    static_cast<int>(std::lround(event.y)),
                },
                event.precise_delta,
                event.shift_modifier) ||
            needs_render;
      } else if (event.kind == window::ShellEventKind::PointerDrag) {
        needs_render =
            scroll_model.pointer_drag(
                event.delta_x,
                event.delta_y,
                window::PointI{
                    static_cast<int>(std::lround(event.x)),
                    static_cast<int>(std::lround(event.y)),
                }) ||
            needs_render;
      } else if (event.kind == window::ShellEventKind::PointerDown) {
        const int px = clamp_nonneg_int64(static_cast<std::int64_t>(std::llround(event.x)));
        const int py = clamp_nonneg_int64(static_cast<std::int64_t>(std::llround(event.y)));
        bool consumed = scroll_model.pointer_down(window::PointI{px, py});
        needs_render = consumed || needs_render;
        auto hit_idx = find_hit_region_index(hit_regions, px, py);
        if (!consumed && !hit_idx.has_value() && size.height > 0) {
          const int py_flipped =
              clamp_nonneg_int64(static_cast<std::int64_t>(size.height) - static_cast<std::int64_t>(py));
          hit_idx = find_hit_region_index(hit_regions, px, py_flipped);
        }
        if (!consumed && hit_idx.has_value()) {
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
            if (hit.target == "zoom-in") {
              widget_scale = std::min(3.0, widget_scale + 0.1);
              needs_render = true;
            } else if (hit.target == "zoom-out") {
              widget_scale = std::max(0.35, widget_scale - 0.1);
              needs_render = true;
            } else if (hit.target == "zoom-reset") {
              widget_scale = 1.0;
              scroll_model.reset();
              needs_render = true;
            } else if (hit.target == "toggle-bg") {
              needs_render = toggle_body_class(document.root, "dark") || needs_render;
            } else if (hit.target == "toggle-theme") {
              needs_render = cycle_body_theme(document.root) || needs_render;
            } else if (hit.target == "save-png") {
              (void)save_png();
            } else if (hit.target == "download-svg") {
              (void)save_svg();
            } else if (hit.target == "copy-visible") {
              const std::string query = input_values.contains("table-search")
                                            ? input_values.at("table-search")
                                            : std::string{};
              const std::string text = visible_table_text(document,
                                                          query,
                                                          table_sort_target,
                                                          table_sort_column,
                                                          table_sort_ascending);
              if (!text.empty()) {
                (void)shell->set_clipboard_text(text);
              }
            } else if (hit.target == "reset-table-view") {
              input_values["table-search"].clear();
              active_input_target.clear();
              table_sort_column = -1;
              table_sort_target.clear();
              table_sort_ascending = true;
              scroll_model.reset();
              needs_render = true;
            } else if (hit.target == "atlas-show-all") {
              set_atlas_frame_visibility(document.root, std::nullopt);
              scroll_model.reset();
              needs_render = true;
            } else if (hit.target.rfind("atlas-tab-", 0) == 0) {
              set_atlas_frame_visibility(document.root, hit.value);
              scroll_model.reset();
              needs_render = true;
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
      } else if (event.kind == window::ShellEventKind::PointerUp) {
        scroll_model.pointer_up();
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
