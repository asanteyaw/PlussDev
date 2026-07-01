#include "platform/PlatformHtmlViewer.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cmath>
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
#include "window/render/async_compositor.h"

#if defined(PLUSSTILS_VIEW_HAVE_LIBZIP) && PLUSSTILS_VIEW_HAVE_LIBZIP
#include <zip.h>
#endif

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

void collect_plain_text(const window::HtmlNode& node, std::string& out);

[[nodiscard]] std::optional<int> active_atlas_frame(const window::HtmlNode& node) {
  if (has_class(node, "atlas-panel") && has_class(node, "active")) {
    const auto* index = attribute_value(node, "data-frame-index");
    if (index != nullptr) {
      return std::atoi(index->c_str());
    }
  }
  for (const auto& child : node.children) {
    if (const auto active = active_atlas_frame(child); active.has_value()) {
      return active;
    }
  }
  return std::nullopt;
}

[[nodiscard]] int atlas_frame_count(const window::HtmlNode& node) {
  int count = has_class(node, "atlas-panel") ? 1 : 0;
  for (const auto& child : node.children) {
    count += atlas_frame_count(child);
  }
  return count;
}

[[nodiscard]] std::string atlas_frame_label(const window::HtmlNode& node, const int frame) {
  if (has_class(node, "atlas-panel")) {
    const auto* index = attribute_value(node, "data-frame-index");
    if (index != nullptr && std::atoi(index->c_str()) == frame) {
      for (const auto& child : node.children) {
        if (has_class(child, "atlas-frame-head")) {
          for (const auto& heading : child.children) {
            if (has_class(heading, "atlas-frame-title")) {
              std::string label;
              collect_plain_text(heading, label);
              return label;
            }
          }
        }
      }
    }
  }
  for (const auto& child : node.children) {
    const std::string label = atlas_frame_label(child, frame);
    if (!label.empty()) {
      return label;
    }
  }
  return {};
}

bool set_element_text(window::HtmlNode& node,
                      const std::string_view element_id,
                      const std::string_view text) {
  const auto* id = attribute_value(node, "id");
  if (id != nullptr && *id == element_id) {
    window::HtmlNode text_node{};
    text_node.kind = window::HtmlNodeKind::Text;
    text_node.tag = "#text";
    text_node.text = std::string(text);
    node.children = {std::move(text_node)};
    return true;
  }
  for (auto& child : node.children) {
    if (set_element_text(child, element_id, text)) {
      return true;
    }
  }
  return false;
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

void update_atlas_status(window::HtmlNode& root, const std::optional<int> active_frame) {
  if (!active_frame.has_value()) {
    (void)set_element_text(root, "atlas-current-frame", "All frames");
    return;
  }
  const std::string label = atlas_frame_label(root, *active_frame);
  (void)set_element_text(
      root,
      "atlas-current-frame",
      label.empty() ? "Frame " + std::to_string(*active_frame) : label);
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

using ExportTable = std::vector<std::vector<std::string>>;

[[nodiscard]] bool contains_tag(const window::HtmlNode& node, const std::string_view tag) {
  if (node.tag == tag) {
    return true;
  }
  return std::any_of(node.children.begin(), node.children.end(), [&](const auto& child) {
    return contains_tag(child, tag);
  });
}

void collect_plain_text(const window::HtmlNode& node, std::string& out) {
  if (node.kind == window::HtmlNodeKind::Text) {
    out += node.text;
    return;
  }
  for (const auto& child : node.children) {
    collect_plain_text(child, out);
  }
}

void collect_export_rows(const window::HtmlNode& node, ExportTable& table) {
  if (node.tag == "tr") {
    std::vector<std::string> row;
    for (const auto& child : node.children) {
      if (child.tag != "th" && child.tag != "td") {
        continue;
      }
      std::string value;
      collect_plain_text(child, value);
      row.push_back(std::move(value));
    }
    if (!row.empty()) {
      table.push_back(std::move(row));
    }
    return;
  }
  for (const auto& child : node.children) {
    collect_export_rows(child, table);
  }
}

void collect_export_tables(const window::HtmlNode& node,
                           const bool ancestors_visible,
                           std::vector<ExportTable>& tables) {
  const bool visible =
      ancestors_visible && !has_class(node, "hidden") &&
      (!has_class(node, "atlas-panel") || has_class(node, "active"));
  if (!visible) {
    return;
  }
  if (node.tag == "table") {
    ExportTable table;
    collect_export_rows(node, table);
    if (!table.empty()) {
      tables.push_back(std::move(table));
    }
    return;
  }
  for (const auto& child : node.children) {
    collect_export_tables(child, visible, tables);
  }
}

[[nodiscard]] std::string delimited_export(const window::HtmlDocument& document,
                                           const char delimiter) {
  std::vector<ExportTable> tables;
  collect_export_tables(document.root, true, tables);
  std::string output;
  for (std::size_t table_index = 0; table_index < tables.size(); ++table_index) {
    if (table_index > 0) {
      output += "\n";
    }
    for (const auto& row : tables[table_index]) {
      for (std::size_t column = 0; column < row.size(); ++column) {
        if (column > 0) {
          output.push_back(delimiter);
        }
        std::string value = row[column];
        if (delimiter == ',') {
          const bool quote = value.find_first_of(",\"\r\n") != std::string::npos;
          if (quote) {
            output.push_back('"');
            for (const char ch : value) {
              if (ch == '"') output.push_back('"');
              output.push_back(ch);
            }
            output.push_back('"');
          } else {
            output += value;
          }
        } else {
          std::replace(value.begin(), value.end(), '\t', ' ');
          std::replace(value.begin(), value.end(), '\n', ' ');
          std::replace(value.begin(), value.end(), '\r', ' ');
          output += value;
        }
      }
      output.push_back('\n');
    }
  }
  return output;
}

[[nodiscard]] std::string xml_escape(std::string_view value) {
  std::string output;
  output.reserve(value.size() + 16);
  for (const char ch : value) {
    switch (ch) {
      case '&': output += "&amp;"; break;
      case '<': output += "&lt;"; break;
      case '>': output += "&gt;"; break;
      case '"': output += "&quot;"; break;
      case '\'': output += "&apos;"; break;
      default: output.push_back(ch); break;
    }
  }
  return output;
}

[[nodiscard]] std::string excel_column_name(std::size_t index) {
  std::string name;
  do {
    name.insert(name.begin(), static_cast<char>('A' + (index % 26U)));
    index = index / 26U;
    if (index == 0) {
      break;
    }
    --index;
  } while (true);
  return name;
}

[[nodiscard]] bool save_xlsx(const window::HtmlDocument& document, const std::string& path) {
#if defined(PLUSSTILS_VIEW_HAVE_LIBZIP) && PLUSSTILS_VIEW_HAVE_LIBZIP
  std::vector<ExportTable> tables;
  collect_export_tables(document.root, true, tables);
  if (tables.empty() || path.empty()) {
    return false;
  }

  int error = 0;
  zip_t* archive = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
  if (archive == nullptr) {
    return false;
  }

  std::vector<std::string> payloads;
  payloads.reserve(tables.size() + 4U);
  const auto add_entry = [&](const std::string_view entry_path, std::string payload) {
    payloads.push_back(std::move(payload));
    const auto& stable = payloads.back();
    zip_source_t* source = zip_source_buffer(archive, stable.data(), stable.size(), 0);
    if (source == nullptr) {
      return false;
    }
    if (zip_file_add(
            archive, std::string(entry_path).c_str(), source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
      zip_source_free(source);
      return false;
    }
    return true;
  };

  std::string content_types =
      R"(<?xml version="1.0" encoding="UTF-8"?>)"
      R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">)"
      R"(<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>)"
      R"(<Default Extension="xml" ContentType="application/xml"/>)"
      R"(<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>)";
  std::string workbook =
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
      R"(<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" )"
      R"(xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><sheets>)";
  std::string workbook_relationships =
      R"(<?xml version="1.0" encoding="UTF-8"?>)"
      R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)";

  for (std::size_t index = 0; index < tables.size(); ++index) {
    const std::size_t sheet_number = index + 1U;
    content_types +=
        "<Override PartName=\"/xl/worksheets/sheet" + std::to_string(sheet_number) +
        ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>";
    workbook += "<sheet name=\"Table " + std::to_string(sheet_number) + "\" sheetId=\"" +
                std::to_string(sheet_number) + "\" r:id=\"rId" + std::to_string(sheet_number) + "\"/>";
    workbook_relationships +=
        "<Relationship Id=\"rId" + std::to_string(sheet_number) +
        "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" "
        "Target=\"worksheets/sheet" + std::to_string(sheet_number) + ".xml\"/>";

    std::string worksheet =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
        R"(<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><sheetData>)";
    std::size_t row_number = 1;
    for (const auto& row : tables[index]) {
      worksheet += "<row r=\"" + std::to_string(row_number) + "\">";
      for (std::size_t column = 0; column < row.size(); ++column) {
        const std::string reference = excel_column_name(column) + std::to_string(row_number);
        worksheet += "<c r=\"" + reference + "\" t=\"inlineStr\"><is><t>" +
                     xml_escape(row[column]) + "</t></is></c>";
      }
      worksheet += "</row>";
      ++row_number;
    }
    worksheet += "</sheetData></worksheet>";
    if (!add_entry(
            "xl/worksheets/sheet" + std::to_string(sheet_number) + ".xml", std::move(worksheet))) {
      zip_discard(archive);
      return false;
    }
  }
  content_types += "</Types>";
  workbook += "</sheets></workbook>";
  workbook_relationships += "</Relationships>";

  const std::string package_relationships =
      R"(<?xml version="1.0" encoding="UTF-8"?>)"
      R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
      R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>)"
      R"(</Relationships>)";

  if (!add_entry("[Content_Types].xml", std::move(content_types)) ||
      !add_entry("_rels/.rels", package_relationships) ||
      !add_entry("xl/workbook.xml", std::move(workbook)) ||
      !add_entry("xl/_rels/workbook.xml.rels", std::move(workbook_relationships))) {
    zip_discard(archive);
    return false;
  }
  return zip_close(archive) == 0;
#else
  (void)document;
  (void)path;
  return false;
#endif
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

bool export_inhouse_table_xlsx(const std::string& html, const std::string& path) {
  if (html.empty() || path.empty()) {
    return false;
  }
  const auto document = window::parse_html_document(html, "Pluss table export");
  return contains_tag(document.root, "table") && save_xlsx(document, path);
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

  const std::string& payload = html.empty() ? svg_payload : html;
  auto document = window::parse_html_document(payload, title.empty() ? std::string("PlussTilsView") : title);
  window::HtmlRenderConfig render_cfg{};
  render_cfg.draw_top_bar = false;
  render_cfg.draw_scrollbars = false;
  window::HtmlRenderStats render_stats{};
  window::HtmlLayoutCache layout_cache;
  render_cfg.layout_cache = &layout_cache;
  window::HtmlScrollModel scroll_model;
  std::vector<window::HtmlHitRegion> hit_regions;
  hit_regions.reserve(512);
  int table_sort_column = -1;
  bool table_sort_ascending = true;
  std::string table_sort_target;
  std::string active_input_target;
  std::unordered_map<std::string, std::string> input_values;
  double widget_scale = 1.0;
  const bool has_table = contains_tag(document.root, "table");
  std::vector<window::ShellSaveFormat> save_formats{
      {"PNG image", "png"},
      {"PDF document", "pdf"},
  };
  if (has_table) {
    save_formats.push_back(window::ShellSaveFormat{"CSV data", "csv"});
    save_formats.push_back(window::ShellSaveFormat{"Tab-delimited text", "txt"});
    save_formats.push_back(window::ShellSaveFormat{"Excel workbook", "xlsx"});
  }

  if (!shell->open(window::ShellConfig{document.title, size, std::move(save_formats)})) {
    return false;
  }
  const double backing_scale = shell->backing_scale_factor();
  window::detail::AsyncSoftwareCompositor compositor;
  window::PixelCanvas presented_canvas;
  std::uint64_t last_submitted_frame_id = 0;
  std::uint64_t last_presented_frame_id = 0;
  constexpr auto caret_blink_interval = std::chrono::milliseconds(530);
  bool caret_visible = true;
  auto next_caret_blink =
      std::chrono::steady_clock::now() + caret_blink_interval;

  const auto restart_caret_blink = [&]() {
    caret_visible = true;
    next_caret_blink =
        std::chrono::steady_clock::now() + caret_blink_interval;
  };

  auto enqueue_current = [&](const window::SizeI target_size,
                             const bool table_scroll_damage_only) -> bool {
    (void)table_scroll_damage_only;
    // Every asynchronous request is a self-contained recovery frame. Cached
    // scroll-damage layouts intentionally omit static paint commands and are
    // therefore not valid when rendering into a freshly cleared back buffer.
    layout_cache.clear();
    auto cfg = runtime_cfg;
    cfg.canvas_size = window::SizeI{
        std::max(1, static_cast<int>(std::lround(target_size.width * backing_scale))),
        std::max(1, static_cast<int>(std::lround(target_size.height * backing_scale))),
    };
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
    render_cfg.show_input_caret = caret_visible;
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
    auto scene = window::build_html_document_scene(document, target_size, render_cfg);
    if (has_table) {
      render_stats.document_max_scroll_x = 0;
      render_stats.document_max_scroll_y = 0;
      render_stats.document_horizontal_track_rect = {};
      render_stats.document_horizontal_thumb_rect = {};
      render_stats.document_vertical_track_rect = {};
      render_stats.document_vertical_thumb_rect = {};
    }
    scroll_model.update_metrics(render_stats);

    const std::uint64_t base_frame_id = last_submitted_frame_id;
    const std::uint64_t frame_id = ++last_submitted_frame_id;
    window::detail::AsyncCompositorRequest request{
        .frame_id = frame_id,
        .base_frame_id = base_frame_id,
        .canvas_size = cfg.canvas_size,
        .background = cfg.background,
        .full_scene = window::scale_scene(scene, backing_scale),
    };
    compositor.submit(std::move(request));
    return !compositor.failed();
  };

  const auto present_frame =
      [&](window::detail::AsyncCompositorFrame frame) -> bool {
    last_presented_frame_id = frame.frame_id;
    presented_canvas = std::move(frame.canvas);
    return shell->present(presented_canvas);
  };

  const auto present_latest = [&](const bool wait_for_latest,
                                  const bool presentation_due = true) -> bool {
    std::optional<window::detail::AsyncCompositorFrame> frame;
    if (wait_for_latest) {
      frame = compositor.wait_for_frame(last_submitted_frame_id,
                                        std::chrono::seconds(5));
    } else {
      if (!presentation_due) {
        return !compositor.failed();
      }
      frame = compositor.take_latest();
    }
    if (!frame.has_value()) {
      return !compositor.failed() &&
             (!wait_for_latest ||
              last_presented_frame_id >= last_submitted_frame_id);
    }
    return present_frame(std::move(*frame));
  };

  if (!enqueue_current(size, false) || !present_latest(true)) {
    shell->close();
    return false;
  }

  if (!snapshot_name.empty()) {
    (void)window::save_canvas(presented_canvas, snapshot_name);
  }

  bool closed = false;
  const auto save_as = [&](const std::string_view extension) {
    if (last_presented_frame_id < last_submitted_frame_id &&
        !present_latest(true)) {
      return false;
    }
    std::filesystem::path suggested =
        snapshot_name.empty() ? std::filesystem::path("pluss-view.png")
                              : std::filesystem::path(snapshot_name).filename();
    suggested.replace_extension(std::string(".") + std::string(extension));
    const std::string path = shell->choose_save_path(suggested.string(), extension);
    if (path.empty()) {
      return false;
    }
    if (extension == "png") {
      return window::save_canvas(presented_canvas, path);
    }
    if (extension == "pdf") {
      return shell->save_canvas_pdf(presented_canvas, path);
    }
    if (extension == "csv") {
      return write_text_file(path, delimited_export(document, ','));
    }
    if (extension == "txt") {
      return write_text_file(path, delimited_export(document, '\t'));
    }
    if (extension == "xlsx") {
      return save_xlsx(document, path);
    }
    return false;
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

  auto animation_clock = std::chrono::steady_clock::now();
  bool render_pending = false;
  bool pending_damage_only = true;
  while (!closed) {
    bool resized = false;
    bool needs_render = false;
    bool table_scroll_damage_only = true;
    window::SizeI next_size = size;

    for (const auto& event : shell->poll_events()) {
      if (event.kind == window::ShellEventKind::Closed) {
        closed = true;
      } else if (event.kind == window::ShellEventKind::ResizeRequested &&
                 event.size.width > 0 && event.size.height > 0) {
        next_size = event.size;
        resized = true;
        table_scroll_damage_only = false;
      } else if (event.kind == window::ShellEventKind::ShortcutCommand &&
                 event.note == "save_snapshot") {
        (void)save_as("png");
      } else if (event.kind == window::ShellEventKind::ShortcutCommand) {
        table_scroll_damage_only = false;
        if (event.note.rfind("save_as:", 0) == 0) {
          (void)save_as(event.note.substr(8));
        } else if (event.note == "zoom_in") {
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
                event.shift_modifier,
                !event.precise_delta) ||
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
        table_scroll_damage_only = false;
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
            } else if (hit.target == "save-png") {
              (void)save_as("png");
            } else if (hit.target == "download-svg") {
              (void)save_svg();
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
              update_atlas_status(document.root, std::nullopt);
              scroll_model.reset();
              needs_render = true;
            } else if (hit.target == "atlas-previous" || hit.target == "atlas-next") {
              const int count = atlas_frame_count(document.root);
              if (count > 0) {
                const int current = active_atlas_frame(document.root).value_or(
                    hit.target == "atlas-previous" ? 0 : -1);
                const int delta = hit.target == "atlas-previous" ? -1 : 1;
                const int next = ((current + delta) % count + count) % count;
                set_atlas_frame_visibility(document.root, next);
                update_atlas_status(document.root, next);
                scroll_model.reset();
                needs_render = true;
              }
            }
          } else if (hit.action == "input.focus") {
            active_input_target = hit.target;
            if (!input_values.contains(active_input_target)) {
              input_values.emplace(active_input_target, std::string{});
            }
            restart_caret_blink();
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
        table_scroll_damage_only = false;
        if (!active_input_target.empty()) {
          std::string& value = input_values[active_input_target];
          if (event.note == "__backspace__") {
            erase_last_utf8_codepoint(value);
            restart_caret_blink();
            needs_render = true;
          } else if (event.note == "__clear__") {
            value.clear();
            restart_caret_blink();
            needs_render = true;
          } else if (!event.note.empty() && value.size() < 512) {
            value += event.note;
            restart_caret_blink();
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
    const auto animation_now = std::chrono::steady_clock::now();
    const double animation_elapsed =
        std::chrono::duration<double>(animation_now - animation_clock).count();
    animation_clock = animation_now;
    needs_render = scroll_model.advance_animation(animation_elapsed) || needs_render;
    if (!active_input_target.empty() && animation_now >= next_caret_blink) {
      do {
        next_caret_blink += caret_blink_interval;
      } while (animation_now >= next_caret_blink);
      caret_visible = !caret_visible;
      table_scroll_damage_only = false;
      needs_render = true;
    }
    if (needs_render) {
      pending_damage_only =
          render_pending ? (pending_damage_only && table_scroll_damage_only)
                         : table_scroll_damage_only;
      render_pending = true;
    }

    const bool presentation_due = shell->presentation_frame_due();
    if (render_pending && presentation_due) {
      if (!enqueue_current(size, pending_damage_only)) {
        shell->close();
        return false;
      }
      render_pending = false;
      pending_damage_only = true;
    }
    if (!present_latest(false, presentation_due)) {
      shell->close();
      return false;
    }

    if (!render_pending && !scroll_model.has_pending_animation() &&
        !compositor.has_pending_work()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(4));
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  shell->close();
  return true;
}

} // namespace pluss::view::detail
