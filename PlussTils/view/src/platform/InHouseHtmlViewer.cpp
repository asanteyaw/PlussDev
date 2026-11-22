#include "platform/PlatformHtmlViewer.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <set>
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

class AsyncVirtualTableSearch {
 public:
  struct Result {
    std::uint64_t generation{0};
    std::string query{};
    std::vector<std::int64_t> rows{};
  };

  explicit AsyncVirtualTableSearch(
      std::shared_ptr<const VirtualTableModel> model)
      : model_(std::move(model)) {
    if (model_ != nullptr) {
      worker_ = std::thread([this] { run_(); });
    }
  }

  ~AsyncVirtualTableSearch() {
    stopping_.store(true, std::memory_order_release);
    {
      const std::scoped_lock lock(mutex_);
      ++pending_generation_;
    }
    wake_.notify_all();
    if (worker_.joinable()) {
      worker_.join();
    }
  }

  AsyncVirtualTableSearch(const AsyncVirtualTableSearch&) = delete;
  AsyncVirtualTableSearch& operator=(const AsyncVirtualTableSearch&) = delete;

  void submit(std::string query) {
    if (model_ == nullptr) {
      return;
    }
    for (char& ch : query) {
      ch = static_cast<char>(
          std::tolower(static_cast<unsigned char>(ch)));
    }
    {
      const std::scoped_lock lock(mutex_);
      pending_query_ = std::move(query);
      ++pending_generation_;
      active_generation_.store(pending_generation_,
                               std::memory_order_release);
    }
    wake_.notify_one();
  }

  [[nodiscard]] std::optional<Result> take_completed() {
    const std::scoped_lock lock(mutex_);
    if (!completed_.has_value()) {
      return std::nullopt;
    }
    auto result = std::move(completed_);
    completed_.reset();
    return result;
  }

 private:
  void run_() {
    std::uint64_t processed_generation = 0;
    while (!stopping_.load(std::memory_order_acquire)) {
      std::string query;
      std::uint64_t generation = 0;
      {
        std::unique_lock lock(mutex_);
        wake_.wait(lock, [&] {
          return stopping_.load(std::memory_order_acquire) ||
                 pending_generation_ != processed_generation;
        });
        if (stopping_.load(std::memory_order_acquire)) {
          return;
        }
        generation = pending_generation_;
        query = pending_query_;
        processed_generation = generation;
      }

      std::vector<std::int64_t> rows;
      if (!query.empty()) {
        const std::int64_t count =
            std::max<std::int64_t>(0, model_->row_count());
        rows.reserve(static_cast<std::size_t>(
            std::min<std::int64_t>(count, 65536)));
        for (std::int64_t row = 0; row < count; ++row) {
          if ((row & 63) == 0 &&
              (stopping_.load(std::memory_order_acquire) ||
               active_generation_.load(std::memory_order_acquire) !=
                   generation)) {
            break;
          }
          if (model_->row_matches(row, query)) {
            rows.push_back(row);
          }
        }
      }

      if (!stopping_.load(std::memory_order_acquire) &&
          active_generation_.load(std::memory_order_acquire) == generation) {
        const std::scoped_lock lock(mutex_);
        completed_ = Result{
            .generation = generation,
            .query = std::move(query),
            .rows = std::move(rows),
        };
      }
    }
  }

  std::shared_ptr<const VirtualTableModel> model_{};
  std::atomic<bool> stopping_{false};
  std::atomic<std::uint64_t> active_generation_{0};
  std::mutex mutex_{};
  std::condition_variable wake_{};
  std::uint64_t pending_generation_{0};
  std::string pending_query_{};
  std::optional<Result> completed_{};
  std::thread worker_{};
};

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

[[nodiscard]] bool atlas_show_all_active(const window::HtmlNode& node) {
  const auto* id = attribute_value(node, "id");
  if (id != nullptr && *id == "atlas-show-all") {
    return has_class(node, "active");
  }
  for (const auto& child : node.children) {
    if (atlas_show_all_active(child)) {
      return true;
    }
  }
  return false;
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

[[nodiscard]] const window::HtmlNode* find_node_by_id(const window::HtmlNode& node,
                                                       const std::string_view id) {
  const auto* value = attribute_value(node, "id");
  if (value != nullptr && *value == id) {
    return &node;
  }
  for (const auto& child : node.children) {
    if (const auto* found = find_node_by_id(child, id)) {
      return found;
    }
  }
  return nullptr;
}

[[nodiscard]] window::HtmlNode* find_node_by_id_mut(window::HtmlNode& node, const std::string_view id) {
  const auto* value = attribute_value(node, "id");
  if (value != nullptr && *value == id) {
    return &node;
  }
  for (auto& child : node.children) {
    if (auto* found = find_node_by_id_mut(child, id)) {
      return found;
    }
  }
  return nullptr;
}

// Generic tabs: <button class="tab" data-tab-group=G data-tab-target=PANEL_ID>
// switches the matching <section class="tab-panel" data-tab-group=G id=PANEL_ID>.
void apply_tab_selection(window::HtmlNode& node,
                         const std::string_view group,
                         const std::string_view button_id,
                         const std::string_view panel_id) {
  const auto* node_group = attribute_value(node, "data-tab-group");
  if (node_group != nullptr && *node_group == group) {
    const auto* id = attribute_value(node, "id");
    const std::string_view node_id = id != nullptr ? std::string_view(*id) : std::string_view{};
    if (has_class(node, "tab")) {
      set_class(node, "active", node_id == button_id);
    } else if (has_class(node, "tab-panel")) {
      set_class(node, "active", node_id == panel_id);
    }
  }
  for (auto& child : node.children) {
    apply_tab_selection(child, group, button_id, panel_id);
  }
}

void collect_view_options(const window::HtmlNode& node,
                          std::vector<window::ShellMenuOption>& options) {
  if (has_class(node, "tab") && attribute_value(node, "data-tab-group") != nullptr) {
    const auto* id = attribute_value(node, "id");
    if (id != nullptr) {
      std::string label;
      collect_plain_text(node, label);
      options.push_back(window::ShellMenuOption{std::move(label), *id});
    }
  }
  for (const auto& child : node.children) {
    collect_view_options(child, options);
  }
}

void hide_view_picker(window::HtmlNode& node) {
  if (has_class(node, "view-picker")) {
    set_class(node, "hidden", true);
  }
  for (auto& child : node.children) {
    hide_view_picker(child);
  }
}

// Switch to the panel of tab button `button_id`; mirrors a click on that button.
// Fills a lazy view panel (data-lazy="<id>", still empty) with the HTML `render_panel` returns for
// that id, by parsing it as a fragment and moving its nodes under the panel.
bool fill_lazy_panel(window::HtmlNode& root,
                     const std::string_view panel_id,
                     const std::function<std::string(std::string_view)>& render_panel) {
  if (!render_panel) {
    return false;
  }
  auto* panel = find_node_by_id_mut(root, panel_id);
  if (panel == nullptr || !panel->children.empty()) {
    return false;
  }
  const auto* lazy = attribute_value(*panel, "data-lazy");
  if (lazy == nullptr) {
    return false;
  }
  const std::string html = render_panel(*lazy);
  auto fragment = window::parse_html_document("<html><body>" + html + "</body></html>");
  std::function<window::HtmlNode*(window::HtmlNode&)> find_body = [&](window::HtmlNode& node) -> window::HtmlNode* {
    if (node.tag == "body") {
      return &node;
    }
    for (auto& child : node.children) {
      if (auto* found = find_body(child)) {
        return found;
      }
    }
    return nullptr;
  };
  auto* body = find_body(fragment.root);
  if (body == nullptr) {
    return false;
  }
  for (auto& child : body->children) {
    panel->children.push_back(std::move(child));
  }
  return true;
}

// True when a table painted from the virtual model is currently on screen (its view is the
// active one and nothing above it is hidden).
// Largest page edge, in logical pixels, that an "all rows" export will lay out. Beyond this the
// image would not open in common viewers, so a taller table is cut at this height.
constexpr std::int64_t max_export_dimension_px = 16000;

// Lets tables take the height their rows need instead of a fixed or scrolling viewport, by
// appending rules that come after the page's own stylesheet.
void release_table_heights(window::HtmlDocument& document) {
  document.style_rules.push_back(window::HtmlCssRule{
      "table", {window::HtmlAttribute{"height", "auto"}}});
  document.style_rules.push_back(window::HtmlCssRule{
      ".table-scroll", {window::HtmlAttribute{"max-height", "none"}}});
}

// The visible virtual table's node, or null.
window::HtmlNode* find_virtual_table_node(window::HtmlNode& node) {
  if (has_class(node, "hidden") || (has_class(node, "tab-panel") && !has_class(node, "active"))) {
    return nullptr;
  }
  if (node.tag == "table") {
    const auto* attribute = attribute_value(node, "data-virtual-table");
    return attribute != nullptr && *attribute == "true" ? &node : nullptr;
  }
  for (auto& child : node.children) {
    if (auto* found = find_virtual_table_node(child)) {
      return found;
    }
  }
  return nullptr;
}

bool virtual_table_visible(const window::HtmlNode& node) {
  if (has_class(node, "hidden") || (has_class(node, "tab-panel") && !has_class(node, "active"))) {
    return false;
  }
  if (node.tag == "table") {
    const auto* attribute = attribute_value(node, "data-virtual-table");
    return attribute != nullptr && *attribute == "true";
  }
  for (const auto& child : node.children) {
    if (virtual_table_visible(child)) {
      return true;
    }
  }
  return false;
}

// ---- scripted events, for tests --------------------------------------------------------------
// With PLUSSTILS_VIEW_SCRIPT set (semicolon-separated), the viewer plays commands through its
// real event loop, a few frames apart, and prints what happened to stderr. Commands:
//   note:<text>            a toolbar/menu event, e.g. note:view:tab-summary, note:theme:miami
//   hover-border:<col>     move the pointer onto that column's border
//   drag-border:<col>:<dx> press on the border, drag dx pixels, release
//   dump                   print the active view, its table and the viewer's state
//   close                  close the window
std::deque<std::string> scripted_commands() {
  std::deque<std::string> commands;
  if (const char* raw = std::getenv("PLUSSTILS_VIEW_SCRIPT"); raw != nullptr) {
    std::string text(raw);
    std::size_t start = 0;
    while (start <= text.size()) {
      const auto end = text.find(';', start);
      const auto piece = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
      if (!piece.empty()) {
        commands.push_back(piece);
      }
      if (end == std::string::npos) {
        break;
      }
      start = end + 1;
    }
  }
  return commands;
}

void collect_active_panels(const window::HtmlNode& node, std::vector<std::string>& out) {
  if (has_class(node, "tab-panel") && has_class(node, "active")) {
    if (const auto* id = attribute_value(node, "id")) {
      out.push_back(*id);
    }
  }
  for (const auto& child : node.children) {
    collect_active_panels(child, out);
  }
}

std::size_t count_rows(const window::HtmlNode& node) {
  std::size_t rows = node.tag == "tr" ? 1 : 0;
  for (const auto& child : node.children) {
    rows += count_rows(child);
  }
  return rows;
}

const window::HtmlNode* find_active_panel_node(const window::HtmlNode& node) {
  if (has_class(node, "tab-panel") && has_class(node, "active")) {
    return &node;
  }
  for (const auto& child : node.children) {
    if (const auto* found = find_active_panel_node(child)) {
      return found;
    }
  }
  return nullptr;
}

bool select_view_tab(window::HtmlNode& root, const std::string_view button_id) {
  const auto* tab = find_node_by_id(root, button_id);
  if (tab == nullptr || !has_class(*tab, "tab")) {
    return false;
  }
  const auto* group = attribute_value(*tab, "data-tab-group");
  const auto* panel = attribute_value(*tab, "data-tab-target");
  if (group == nullptr || panel == nullptr) {
    return false;
  }
  apply_tab_selection(root, *group, button_id, *panel);
  if (const auto* label_id = attribute_value(*tab, "data-dropdown-label")) {
    std::string label;
    collect_plain_text(*tab, label);
    (void)set_element_text(root, *label_id, label);
    if (const auto* menu_id = attribute_value(*tab, "data-dropdown-menu")) {
      if (auto* menu = find_node_by_id_mut(root, *menu_id)) {
        set_class(*menu, "open", false);
      }
    }
  }
  return true;
}

void set_atlas_frame_visibility(window::HtmlNode& node, const std::optional<int> active_frame) {
  if (has_class(node, "atlas-panel")) {
    const auto* index = attribute_value(node, "data-frame-index");
    const int frame = index != nullptr ? std::atoi(index->c_str()) : -1;
    set_class(node, "active", !active_frame.has_value() || frame == *active_frame);
  } else if (has_class(node, "atlas-tab")) {
    const auto* id = attribute_value(node, "id");
    if (id != nullptr && *id == "atlas-show-all") {
      set_class(node, "active", !active_frame.has_value());
    } else {
      const auto* index = attribute_value(node, "data-frame-index");
      const int frame = index != nullptr ? std::atoi(index->c_str()) : -1;
      set_class(node, "active", active_frame.has_value() && frame == *active_frame);
    }
  }
  for (auto& child : node.children) {
    set_atlas_frame_visibility(child, active_frame);
  }
}

void update_atlas_status(window::HtmlNode& root, const std::optional<int> active_frame) {
  if (!active_frame.has_value()) {
    (void)set_element_text(root, "atlas-current-frame",
                           "All " + std::to_string(atlas_frame_count(root)) + " frames");
    return;
  }
  const std::string label = atlas_frame_label(root, *active_frame);
  (void)set_element_text(
      root,
      "atlas-current-frame",
      (label.empty() ? "Frame " + std::to_string(*active_frame) : label) +
          " (" + std::to_string(*active_frame + 1) + " of " +
          std::to_string(atlas_frame_count(root)) + ")");
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
using ExportTables = std::vector<ExportTable>;

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
                           ExportTables& tables) {
  const bool visible =
      ancestors_visible && !has_class(node, "hidden") &&
      ((!has_class(node, "atlas-panel") && !has_class(node, "tab-panel")) || has_class(node, "active"));
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

[[nodiscard]] ExportTables export_tables_from_document(const window::HtmlDocument& document) {
  ExportTables tables;
  collect_export_tables(document.root, true, tables);
  return tables;
}

[[nodiscard]] ExportTable export_table_from_virtual_model(const VirtualTableModel& model,
                                                          const std::vector<std::int64_t>* row_order) {
  ExportTable table;
  const auto row_count = row_order != nullptr ? static_cast<std::int64_t>(row_order->size()) : model.row_count();
  const auto column_count = model.column_count();
  table.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, row_count)) + 1U);

  std::vector<std::string> header;
  header.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, column_count)) + 1U);
  header.push_back(model.row_header());
  for (std::int64_t column = 0; column < column_count; ++column) {
    header.push_back(model.column_name(column));
  }
  table.push_back(std::move(header));

  for (std::int64_t display_row = 0; display_row < row_count; ++display_row) {
    const auto source_row = row_order != nullptr ? (*row_order)[static_cast<std::size_t>(display_row)] : display_row;
    std::vector<std::string> row;
    row.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, column_count)) + 1U);
    row.push_back(model.row_label(source_row));
    for (std::int64_t column = 0; column < column_count; ++column) {
      row.push_back(model.cell(source_row, column).text);
    }
    table.push_back(std::move(row));
  }
  return table;
}

[[nodiscard]] std::string delimited_export_tables(const ExportTables& tables,
                                                  const char delimiter) {
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

[[nodiscard]] std::string delimited_export(const window::HtmlDocument& document,
                                           const char delimiter) {
  return delimited_export_tables(export_tables_from_document(document), delimiter);
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

[[nodiscard]] bool save_xlsx_tables(const ExportTables& tables, const std::string& path) {
#if defined(PLUSSTILS_VIEW_HAVE_LIBZIP) && PLUSSTILS_VIEW_HAVE_LIBZIP
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
  (void)tables;
  (void)path;
  return false;
#endif
}

[[nodiscard]] bool save_xlsx(const window::HtmlDocument& document, const std::string& path) {
  return save_xlsx_tables(export_tables_from_document(document), path);
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

bool export_inhouse_table_xlsx(std::shared_ptr<const VirtualTableModel> model,
                               const std::string& path,
                               const std::vector<std::int64_t>* row_order) {
  if (model == nullptr || path.empty()) {
    return false;
  }
  ExportTables tables;
  tables.push_back(export_table_from_virtual_model(*model, row_order));
  return save_xlsx_tables(tables, path);
}

bool show_inhouse_html_window(const std::string& title,
                              const std::string& html,
                              const std::int64_t width,
                              const std::int64_t height,
                              const std::string& snapshot_name,
                              const std::string& svg_payload,
                              const std::string& svg_snapshot_name,
                              std::shared_ptr<const VirtualTableModel>
                                  virtual_table_model,
                              const bool table_viewport_scroll_only,
                              std::vector<HtmlMenu> menus,
                              std::function<std::string(std::string_view)> render_panel) {
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
  render_cfg.draw_scrollbars = true;
  render_cfg.enable_document_scroll = !table_viewport_scroll_only;
  render_cfg.virtual_table_model = virtual_table_model.get();
  window::HtmlRenderStats render_stats{};
  window::HtmlLayoutCache layout_cache;
  render_cfg.layout_cache = &layout_cache;
  window::HtmlScrollModel scroll_model;
  std::vector<window::HtmlHitRegion> hit_regions;
  hit_regions.reserve(512);
  int table_sort_column = -1;
  bool table_sort_ascending = true;
  std::string table_sort_target;
  // Column widths the user has set by dragging header borders (table target -> column -> px),
  // and the drag in progress, if any.
  std::unordered_map<std::string, std::unordered_map<int, int>> table_column_widths;
  struct ResizeDrag {
    bool active{false};
    std::string target{};
    int column{0};
    int start_x{0};
    int start_width{0};
  } resize_drag;
  std::string hover_resize_target;  // column border under the pointer
  int hover_resize_column = -1;
  std::set<std::pair<std::string, int>> fitted_columns;  // columns currently auto-fitted (double-click toggles)
  std::string active_input_target;
  std::unordered_map<std::string, std::string> input_values;
  double widget_scale = 1.0;
  const bool has_table = contains_tag(document.root, "table");
  std::string live_svg = svg_payload;  // follows a re-render (e.g. a colour choice) so Save writes what is shown
  std::vector<window::ShellSaveFormat> save_formats{
      {"PNG", "png"},
      {"PDF", "pdf"},
  };
  if (!live_svg.empty()) {
    save_formats.push_back(window::ShellSaveFormat{"SVG", "svg"});
    // The window's single Save menu replaces the in-page save buttons.
    for (const char* id : {"download-svg", "save-png"}) {
      if (auto* node = find_node_by_id_mut(document.root, id)) {
        set_class(*node, "hidden", true);
      }
    }
  }
  if (has_table || virtual_table_model != nullptr) {
    save_formats.push_back(window::ShellSaveFormat{"CSV data", "csv"});
    save_formats.push_back(window::ShellSaveFormat{"Tab-delimited text", "txt"});
    save_formats.push_back(window::ShellSaveFormat{"Excel workbook", "xlsx"});
  }

  // Toolbar menus, left to right: the window-level ones (Theme, ...) and then "View". View
  // lists the page's own view tabs plus any window menu marked `in_view_menu` (e.g.
  // Format) as a cascading entry.
  // The menu a window-level HtmlMenu becomes: its options, the one in effect checked.
  const auto menu_options = [&](const HtmlMenu& menu) {
    std::string current = menu.selected ? menu.selected() : std::string();
    if (current.empty() && !menu.options.empty()) {
      current = menu.options.front().id;
    }
    std::vector<window::ShellMenuOption> options;
    for (const auto& option : menu.options) {
      options.push_back(window::ShellMenuOption{option.label, option.id, {}, menu.selected && option.id == current});
    }
    return options;
  };
  const auto build_menus = [&]() {
    std::vector<window::ShellMenu> built;
    window::ShellMenu view_menu{"view", "View", {}};
    collect_view_options(document.root, view_menu.options);
    for (const auto& menu : menus) {
      if (!menu.render || menu.options.empty()) {
        continue;
      }
      if (!menu.in_view_menu) {
        built.push_back(window::ShellMenu{menu.id, menu.title, menu_options(menu)});
        continue;
      }
      window::ShellMenuOption cascade{menu.title, menu.id, menu_options(menu)};
      if (menu.section.empty()) {
        view_menu.options.push_back(std::move(cascade));
        continue;
      }
      // A named section groups several cascades into one entry of View.
      auto section = std::find_if(view_menu.options.begin(), view_menu.options.end(),
                                  [&](const window::ShellMenuOption& entry) {
                                    return entry.id == "section:" + menu.section;
                                  });
      if (section == view_menu.options.end()) {
        view_menu.options.push_back(window::ShellMenuOption{menu.section, "section:" + menu.section, {}});
        section = view_menu.options.end() - 1;
      }
      section->children.push_back(std::move(cascade));
    }
    if (!view_menu.options.empty()) {
      built.push_back(std::move(view_menu));
    }
    return built;
  };
  std::vector<window::ShellMenu> shell_menus;
  if (shell->supports_menus()) {
    window::ShellMenu probe{"view", "View", {}};
    collect_view_options(document.root, probe.options);
    if (!probe.options.empty()) {
      hide_view_picker(document.root);  // the native "View" menu replaces the in-page picker
    }
    shell_menus = build_menus();
  }
  std::string last_view_button;  // survives re-renders
  // Selecting a view first fills its panel if that is still empty (views are computed on demand).
  const auto select_view = [&](const std::string_view button_id) {
    if (const auto* tab = find_node_by_id(document.root, button_id)) {
      if (const auto* target = attribute_value(*tab, "data-tab-target")) {
        (void)fill_lazy_panel(document.root, *target, render_panel);
      }
    }
    return select_view_tab(document.root, button_id);
  };

  if (!shell->open(window::ShellConfig{document.title, size, std::move(save_formats),
                                       std::move(shell_menus)})) {
    return false;
  }
  const double backing_scale = shell->backing_scale_factor();
  window::detail::AsyncSoftwareCompositor compositor;
  AsyncVirtualTableSearch virtual_search(virtual_table_model);
  std::shared_ptr<const std::vector<std::int64_t>> virtual_row_order;
  std::string submitted_virtual_query;
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
    // Each request still paints a self-contained frame, but table geometry is
    // immutable across scroll offsets. Reusing measured widths and row heights
    // keeps ordinary HTML tables off the scroll-frame critical path.
    auto cfg = runtime_cfg;
    cfg.canvas_size = window::SizeI{
        std::max(1, static_cast<int>(std::lround(target_size.width * backing_scale))),
        std::max(1, static_cast<int>(std::lround(target_size.height * backing_scale))),
    };
    const auto document_offset = scroll_model.document_offset_px();
    const auto table_offsets = scroll_model.table_offsets_px();
    render_cfg.document_scroll_x_px =
        table_viewport_scroll_only ? 0 : document_offset.x;
    render_cfg.document_scroll_y_px =
        table_viewport_scroll_only ? 0 : document_offset.y;
    render_cfg.table_scroll_x_px = 0;
    render_cfg.table_scroll_y_px = 0;
    render_cfg.table_scroll_offsets = &table_offsets;
    render_cfg.widget_scale = widget_scale;
    render_cfg.table_sort_column = table_sort_column;
    render_cfg.table_sort_ascending = table_sort_ascending;
    render_cfg.table_sort_target = table_sort_target;
    render_cfg.table_column_widths = &table_column_widths;
    render_cfg.hover_resize_target = hover_resize_target;
    render_cfg.hover_resize_column = hover_resize_column;
    render_cfg.active_input_target = active_input_target;
    render_cfg.show_input_caret = caret_visible;
    render_cfg.virtual_row_order =
        virtual_row_order != nullptr ? virtual_row_order.get() : nullptr;
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
    // This used to unconditionally zero every document-level scroll limit and
    // scrollbar rect whenever the page contained a <table> anywhere, on the
    // apparent assumption that a table always owns its own bounded inner
    // scroll viewport (table_scroll_regions) so the surrounding document
    // never needs to scroll. That assumption doesn't hold for a table that
    // isn't confined to a smaller box -- e.g. DataFrame::show()'s own
    // output, or any table taller than the window -- which just extends the
    // page and relies on document-level scrolling to reach the rest of it.
    // has_table had no other reader in this file, so this was silently
    // disabling all document scrolling (wheel, drag, and the scrollbar
    // itself) on every page with a table, independent of whether that
    // table's own inner region could scroll or even existed. Table-owned
    // scroll regions aren't affected by removing this -- HtmlScrollModel
    // checks table_scroll_regions first and only falls through to the
    // document metrics below when a gesture isn't inside one of them.
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
  // PNG/PDF exports contain the content only: a plot is re-rendered from its SVG at its natural
  // size, and a table is re-rendered without scrollbars and cropped to what the layout reports
  // as drawn (column headers plus rows). Nothing of the window (title bar, toolbar, search box,
  // scrollbars) is included. With `all_rows`, a table is laid out at its full height instead of
  // the scrolling viewport, so every row is in the image.
  const auto export_canvas = [&](window::PixelCanvas& out, const bool all_rows = false) -> bool {
    window::HtmlRenderConfig cfg = render_cfg;
    const auto export_offsets = scroll_model.table_offsets_px();
    cfg.table_scroll_offsets = &export_offsets;
    cfg.draw_top_bar = false;
    cfg.draw_scrollbars = false;
    cfg.hit_regions = nullptr;
    cfg.render_stats = nullptr;
    cfg.layout_cache = nullptr;
    window::SizeI logical = size;  // logical (unscaled) page size to lay out
    window::RectI crop{};          // logical rectangle to keep
    window::HtmlDocument export_document;
    const window::HtmlDocument* doc = &document;
    if (!svg_payload.empty()) {
      const auto svg_at = svg_payload.find("<svg");
      const std::string head =
          svg_at == std::string::npos
              ? std::string{}
              : std::string(svg_payload.substr(svg_at, svg_payload.find('>', svg_at) - svg_at));
      const auto number_after = [&](const std::string& key) -> double {
        const auto at = head.find(key);
        return at == std::string::npos ? 0.0 : std::atof(head.c_str() + at + key.size());
      };
      double w = number_after(" width=\"");
      double h = number_after(" height=\"");
      if (w <= 0.0 || h <= 0.0) {
        if (const auto vb = head.find("viewBox=\""); vb != std::string::npos) {
          double x0 = 0.0, y0 = 0.0;
          std::sscanf(head.c_str() + vb + 9, "%lf %lf %lf %lf", &x0, &y0, &w, &h);
        }
      }
      if (w <= 0.0 || h <= 0.0) {
        return false;
      }
      export_document = window::parse_html_document(svg_payload, "plot");
      doc = &export_document;
      logical = window::SizeI{static_cast<int>(std::lround(w)), static_cast<int>(std::lround(h))};
      cfg.root_padding_px = 0;
      cfg.enable_document_scroll = false;
      cfg.document_scroll_x_px = 0;
      cfg.document_scroll_y_px = 0;
      cfg.widget_scale = 1.0;
    } else {
      cfg.enable_document_scroll = false;
      cfg.document_scroll_x_px = 0;
      cfg.document_scroll_y_px = 0;
      window::HtmlRenderStats stats{};
      cfg.render_stats = &stats;
      std::vector<window::HtmlHitRegion> scratch;
      cfg.hit_regions = &scratch;
      if (all_rows && virtual_table_model == nullptr) {
        // An ordinary table: drop the fixed heights so it takes the height its rows need.
        export_document = document;
        release_table_heights(export_document);
        doc = &export_document;
        cfg.enable_document_scroll = true;
        (void)window::build_html_document_scene(*doc, logical, cfg);
        logical.height += std::max(0, stats.document_max_scroll_y);
        cfg.enable_document_scroll = false;
        stats = window::HtmlRenderStats{};
      } else if (all_rows) {
        // Give the table node the height that holds every row, then let the page grow to fit it.
        const std::int64_t rows = virtual_row_order != nullptr
                                      ? static_cast<std::int64_t>(virtual_row_order->size())
                                      : virtual_table_model->row_count();
        const std::int64_t table_h = window::kVirtualTableHeaderHeightPx +
                                     rows * window::kVirtualTableRowHeightPx;
        export_document = document;
        if (auto* table_node = find_virtual_table_node(export_document.root)) {
          table_node->style.height_px = static_cast<int>(std::min<std::int64_t>(table_h, max_export_dimension_px));
        }
        doc = &export_document;
        cfg.enable_document_scroll = true;
        (void)window::build_html_document_scene(*doc, logical, cfg);
        logical.height += std::max(0, stats.document_max_scroll_y);
        cfg.enable_document_scroll = false;
        stats = window::HtmlRenderStats{};
      }
      (void)window::build_html_document_scene(*doc, logical, cfg);
      if (!stats.has_table || stats.table_content_rect.width <= 0 || stats.table_content_rect.height <= 0) {
        return false;
      }
      crop = stats.table_content_rect;
      cfg.render_stats = nullptr;
      cfg.hit_regions = nullptr;
    }
    const auto scaled = [&](const int logical_px) {
      return std::max(1, static_cast<int>(std::lround(logical_px * backing_scale)));
    };
    const window::SizeI canvas_size{scaled(logical.width), scaled(logical.height)};
    const auto scene = window::scale_scene(window::build_html_document_scene(*doc, logical, cfg), backing_scale);
    window::PixelCanvas full(canvas_size);
    full.clear(runtime_cfg.background);
    std::vector<window::RenderEvent> events;
    window::SoftwareRenderer renderer;
    if (!renderer.render(scene, full, events)) {
      return false;
    }
    const auto bytes = full.bytes();
    const std::size_t stride = static_cast<std::size_t>(canvas_size.width) * 4U;
    const auto pixel = [&](const int x, const int y) {
      return bytes.data() + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4U;
    };
    // The kept rectangle, in canvas pixels. A plot keeps the bounding box of everything that is
    // not page background; a table keeps the layout's content rectangle.
    int x0 = 0, y0 = 0, x1 = canvas_size.width, y1 = canvas_size.height;
    if (!svg_payload.empty()) {
      const auto bg = runtime_cfg.background;
      x0 = canvas_size.width;
      y0 = canvas_size.height;
      x1 = 0;
      y1 = 0;
      for (int y = 0; y < canvas_size.height; ++y) {
        for (int x = 0; x < canvas_size.width; ++x) {
          const auto* q = pixel(x, y);
          if (q[0] != bg.r || q[1] != bg.g || q[2] != bg.b) {
            x0 = std::min(x0, x);
            x1 = std::max(x1, x + 1);
            y0 = std::min(y0, y);
            y1 = std::max(y1, y + 1);
          }
        }
      }
      if (x1 <= x0 || y1 <= y0) {
        out = std::move(full);
        return true;
      }
    } else {
      x0 = std::clamp(static_cast<int>(std::lround(crop.x * backing_scale)), 0, canvas_size.width - 1);
      y0 = std::clamp(static_cast<int>(std::lround(crop.y * backing_scale)), 0, canvas_size.height - 1);
      x1 = std::clamp(static_cast<int>(std::lround((crop.x + crop.width) * backing_scale)), x0 + 1, canvas_size.width);
      y1 = std::clamp(static_cast<int>(std::lround((crop.y + crop.height) * backing_scale)), y0 + 1, canvas_size.height);
    }
    window::PixelCanvas cropped(window::SizeI{x1 - x0, y1 - y0});
    for (int y = y0; y < y1; ++y) {
      for (int x = x0; x < x1; ++x) {
        const auto* q = pixel(x, y);
        cropped.set_pixel(window::PointI{x - x0, y - y0}, window::Color::rgba(q[0], q[1], q[2], q[3]));
      }
    }
    out = std::move(cropped);
    return true;
  };
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
    if (extension == "png" || extension == "pdf") {
      window::PixelCanvas exported;
      const window::PixelCanvas& image = export_canvas(exported) ? exported : presented_canvas;
      return extension == "png" ? window::save_canvas(image, path) : shell->save_canvas_pdf(image, path);
    }
    if (extension == "svg") {
      return !live_svg.empty() && write_text_file(path, live_svg);
    }
    const auto export_tables =
        virtual_table_model != nullptr && virtual_table_visible(document.root)
            ? ExportTables{export_table_from_virtual_model(
                  *virtual_table_model,
                  virtual_row_order != nullptr ? virtual_row_order.get() : nullptr)}
            : export_tables_from_document(document);
    if (extension == "csv") {
      return write_text_file(path, delimited_export_tables(export_tables, ','));
    }
    if (extension == "txt") {
      return write_text_file(path, delimited_export_tables(export_tables, '\t'));
    }
    if (extension == "xlsx") {
      return save_xlsx_tables(export_tables, path);
    }
    return false;
  };
  const auto save_svg = [&]() {
    if (live_svg.empty()) {
      return false;
    }
    const std::string suggested =
        svg_snapshot_name.empty() ? std::string("pluss-plot.svg")
                                  : std::filesystem::path(svg_snapshot_name).filename().string();
    const std::string path = shell->choose_save_path(suggested, "svg");
    return !path.empty() && write_text_file(path, live_svg);
  };

  window::ShellCursor last_cursor = window::ShellCursor::Arrow;
  const auto set_cursor_ = [&](const window::ShellCursor cursor) {
    last_cursor = cursor;
    shell->set_cursor(cursor);
  };
  std::deque<std::string> script = scripted_commands();
  const auto script_started = std::chrono::steady_clock::now();
  int script_wait = 0;
  auto script_resume_at = std::chrono::steady_clock::now();

  auto animation_clock = std::chrono::steady_clock::now();
  bool render_pending = false;
  bool pending_damage_only = true;
  while (!closed) {
    bool resized = false;
    bool needs_render = false;
    bool table_scroll_damage_only = true;
    window::SizeI next_size = size;

    std::vector<window::ShellEvent> events = shell->poll_events();
    if (!script.empty() && std::chrono::steady_clock::now() >= script_resume_at && ++script_wait >= 4) {  // let a few frames render between steps
      script_wait = 0;
      const std::string command = script.front();
      script.pop_front();
      const auto find_border = [&](const int column) -> const window::HtmlHitRegion* {
        for (const auto& region : hit_regions) {
          if (region.action == "table.resize.column" && region.value == column) {
            return &region;
          }
        }
        return nullptr;
      };
      const auto centre = [](const window::HtmlHitRegion& region) {
        return std::pair<double, double>{region.rect.x + region.rect.width / 2.0, region.rect.y + region.rect.height / 2.0};
      };
      if (command.rfind("sleep:", 0) == 0) {
        // Pause the script (so a real pointer can be driven from outside meanwhile).
        script_resume_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::atoi(command.c_str() + 6));
      } else if (command.rfind("note:", 0) == 0) {
        window::ShellEvent e{};
        e.kind = window::ShellEventKind::ShortcutCommand;
        e.note = command.substr(5);
        events.push_back(e);
      } else if (command.rfind("hover-border:", 0) == 0) {
        if (const auto* border = find_border(std::atoi(command.c_str() + 13))) {
          const auto [x, y] = centre(*border);
          window::ShellEvent e{};
          e.kind = window::ShellEventKind::PointerMove;
          e.x = x;
          e.y = y;
          events.push_back(e);
        } else {
          std::fprintf(stderr, "[view-script] %s: no such border\n", command.c_str());
        }
      } else if (command.rfind("drag-border:", 0) == 0) {
        const int column = std::atoi(command.c_str() + 12);
        const auto colon = command.rfind(':');
        const double dx = colon != std::string::npos ? std::atof(command.c_str() + colon + 1) : 0.0;
        if (const auto* border = find_border(column)) {
          const auto [x, y] = centre(*border);
          window::ShellEvent down{};
          down.kind = window::ShellEventKind::PointerDown;
          down.x = x;
          down.y = y;
          window::ShellEvent drag{};
          drag.kind = window::ShellEventKind::PointerDrag;
          drag.x = x + dx;
          drag.y = y;
          drag.delta_x = dx;
          window::ShellEvent up{};
          up.kind = window::ShellEventKind::PointerUp;
          up.x = x + dx;
          up.y = y;
          events.push_back(down);
          events.push_back(drag);
          events.push_back(up);
        } else {
          std::fprintf(stderr, "[view-script] %s: no such border\n", command.c_str());
        }
      } else if (command == "menus") {
        // Test hook: the toolbar menus as the shell was last given them, checked options marked with [x].
        std::function<void(const std::vector<window::ShellMenuOption>&, int)> print =
            [&](const std::vector<window::ShellMenuOption>& options, const int depth) {
              for (const auto& option : options) {
                std::fprintf(stderr, "[view-script] menu: %*s%s %s\n", depth * 2, "", option.checked ? "[x]" : "[ ]",
                             option.label.c_str());
                print(option.children, depth + 1);
              }
            };
        for (const auto& menu : build_menus()) {
          std::fprintf(stderr, "[view-script] menu: %s\n", menu.title.c_str());
          print(menu.options, 1);
        }
      } else if (command == "dump") {
        std::vector<std::string> active;
        collect_active_panels(document.root, active);
        const auto* panel = find_active_panel_node(document.root);
        std::fprintf(stderr, "[view-script] t=%lldms dump: active panels=",
                     static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                std::chrono::steady_clock::now() - script_started)
                                                .count()));
        for (const auto& id : active) {
          std::fprintf(stderr, "%s ", id.c_str());
        }
        std::fprintf(stderr, "rows in active panel=%zu cursor=%s hover=%s/%d",
                     panel != nullptr ? count_rows(*panel) : std::size_t{0},
                     last_cursor == window::ShellCursor::ResizeColumn ? "resize-column" : "arrow",
                     hover_resize_target.c_str(), hover_resize_column);
        for (const auto& [table, widths] : table_column_widths) {
          for (const auto& [column, width] : widths) {
            std::fprintf(stderr, " width[%s][%d]=%d", table.c_str(), column, width);
          }
        }
        for (const auto& region : hit_regions) {
          if (region.action == "table.resize.column") {
            std::fprintf(stderr, " border[%d]=%.0f,%.0f", region.value,
                         region.rect.x + region.rect.width / 2.0, region.rect.y + region.rect.height / 2.0);
          }
        }
        std::fprintf(stderr, "\n");
      } else if (command.rfind("dblclick-border:", 0) == 0) {
        if (const auto* border = find_border(std::atoi(command.c_str() + 16))) {
          const auto [x, y] = centre(*border);
          window::ShellEvent e{};
          e.kind = window::ShellEventKind::PointerDown;
          e.x = x;
          e.y = y;
          e.click_count = 2;
          events.push_back(e);
        } else {
          std::fprintf(stderr, "[view-script] %s: no such border\n", command.c_str());
        }
      } else if (command == "close") {
        window::ShellEvent e{};
        e.kind = window::ShellEventKind::Closed;
        events.push_back(e);
      }
    }
    for (const auto& event : events) {
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
      } else if (event.kind == window::ShellEventKind::ShortcutCommand &&
                 event.note.rfind("png-to:", 0) == 0) {
        // Test hook: write the last presented frame to a file without a save dialog.
        (void)window::save_canvas(presented_canvas, event.note.substr(7));
      } else if (event.kind == window::ShellEventKind::ShortcutCommand &&
                 event.note.rfind("export-png-to:", 0) == 0) {
        // Test hook: write the content-only export (what Save As PNG writes) without a dialog.
        window::PixelCanvas exported;
        if (export_canvas(exported)) {
          (void)window::save_canvas(exported, event.note.substr(14));
        }
      } else if (event.kind == window::ShellEventKind::ShortcutCommand &&
                 event.note.rfind("export-all-png-to:", 0) == 0) {
        window::PixelCanvas exported;
        if (export_canvas(exported, true)) {
          (void)window::save_canvas(exported, event.note.substr(18));
        }
      } else if (event.kind == window::ShellEventKind::ShortcutCommand) {
        table_scroll_damage_only = false;
        if (event.note.rfind("view:", 0) == 0) {
          if (select_view(std::string_view(event.note).substr(5))) {
            last_view_button = event.note.substr(5);
            scroll_model.reset();
            needs_render = true;
          }
        } else if (const auto colon = event.note.find(':'); colon != std::string::npos &&
                   std::any_of(menus.begin(), menus.end(), [&](const HtmlMenu& menu) {
                     return menu.id == std::string_view(event.note).substr(0, colon) && menu.render;
                   })) {
          // A window-level menu choice: re-render the page, then restore the current view.
          const auto menu_id = std::string_view(event.note).substr(0, colon);
          const auto menu = std::find_if(menus.begin(), menus.end(),
                                         [&](const HtmlMenu& m) { return m.id == menu_id; });
          const std::string rerendered = menu->render(std::string_view(event.note).substr(colon + 1));
          if (!rerendered.empty()) {
            if (!live_svg.empty()) {
              const auto svg_begin = rerendered.find("<svg");
              const auto svg_end = rerendered.rfind("</svg>");
              if (svg_begin != std::string::npos && svg_end != std::string::npos && svg_end > svg_begin) {
                live_svg = rerendered.substr(svg_begin, svg_end + 6 - svg_begin);
              }
            }
            document = window::parse_html_document(
                rerendered, document.title.empty() ? std::string("PlussTilsView") : document.title);
            layout_cache.clear();
            hide_view_picker(document.root);
            if (!live_svg.empty()) {
              for (const char* id : {"download-svg", "save-png"}) {
                if (auto* node = find_node_by_id_mut(document.root, id)) {
                  set_class(*node, "hidden", true);
                }
              }
            }
            if (!last_view_button.empty()) {
              (void)select_view(last_view_button);
            }
            if (shell->supports_menus()) {
              shell->update_menus(build_menus());  // move the check marks to the choice just made
            }
            needs_render = true;
          }
        } else if (event.note.rfind("save_as:", 0) == 0) {
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
        // HTML layout and scroll metrics are computed in logical window
        // coordinates. The scene is scaled only after layout, during final
        // Retina/HiDPI presentation. Shell events are also reported in logical
        // coordinates, so hit-testing must use them directly. Scaling input by
        // backing_scale makes every table/scrollbar hit miss on Retina displays.
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
      } else if (event.kind == window::ShellEventKind::PointerMove) {
        // Hover: over a column border the pointer becomes the resize cursor (and the border
        // is highlighted), like a spreadsheet.
        const int px = clamp_nonneg_int64(static_cast<std::int64_t>(std::llround(event.x)));
        const int py = clamp_nonneg_int64(static_cast<std::int64_t>(std::llround(event.y)));
        auto hover_idx = find_hit_region_index(hit_regions, px, py);
        if (!hover_idx.has_value() && size.height > 0) {
          const int py_flipped = clamp_nonneg_int64(
              static_cast<std::int64_t>(std::llround(size.height)) - static_cast<std::int64_t>(py));
          hover_idx = find_hit_region_index(hit_regions, px, py_flipped);
        }
        const bool over_border =
            hover_idx.has_value() && hit_regions[*hover_idx].action == "table.resize.column";
        const std::string next_target = over_border ? hit_regions[*hover_idx].target : std::string{};
        const int next_column = over_border ? hit_regions[*hover_idx].value : -1;
        if (next_target != hover_resize_target || next_column != hover_resize_column) {
          hover_resize_target = next_target;
          hover_resize_column = next_column;
          table_scroll_damage_only = false;
          needs_render = true;
        }
        set_cursor_(over_border || resize_drag.active ? window::ShellCursor::ResizeColumn
                                                             : window::ShellCursor::Arrow);
      } else if (event.kind == window::ShellEventKind::PointerDrag && resize_drag.active) {
        // Dragging a header border: the column follows the pointer, wrapping live.
        const int dx = static_cast<int>(std::lround(event.x)) - resize_drag.start_x;
        if (dx != 0) fitted_columns.erase({resize_drag.target, resize_drag.column});
        table_column_widths[resize_drag.target][resize_drag.column] =
            std::clamp(resize_drag.start_width + dx, 48, 4000);
        // No cache flush: the layout notices the changed widths and re-measures only that column.
        table_scroll_damage_only = false;
        needs_render = true;
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
          const int py_flipped = clamp_nonneg_int64(
              static_cast<std::int64_t>(std::llround(size.height)) - static_cast<std::int64_t>(py));
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
          } else if (hit.action == "table.resize.column") {
            if (event.click_count >= 2) {
              // Double-click on a border toggles that column: fit to its widest value (width 0 =
              // auto-fit), then back to the default width, then fit again.
              const std::pair<std::string, int> key{hit.target, hit.value};
              if (fitted_columns.erase(key) > 0) {
                table_column_widths[hit.target].erase(hit.value);
              } else {
                table_column_widths[hit.target][hit.value] = 0;
                fitted_columns.insert(key);
              }
              table_scroll_damage_only = false;
              needs_render = true;
            } else {
              resize_drag = ResizeDrag{true, hit.target, hit.value, px, hit.aux};
            }
          } else if (hit.action == "table.sort.asc" || hit.action == "table.sort.desc") {
            // The up / down arrow on a column header: sort that column that way.
            table_sort_column = hit.value;
            table_sort_target = hit.target;
            table_sort_ascending = hit.action == "table.sort.asc";
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
              table_column_widths.clear();
              fitted_columns.clear();
              layout_cache.clear();
              input_values["table-search"].clear();
              active_input_target.clear();
              table_sort_column = -1;
              table_sort_target.clear();
              table_sort_ascending = true;
              scroll_model.reset();
              needs_render = true;
            } else if (const auto* tab = find_node_by_id(document.root, hit.target);
                       tab != nullptr && has_class(*tab, "view-toggle")) {
              // Dropdown toggle: open/close the menu named by data-dropdown.
              const auto* menu_id = attribute_value(*tab, "data-dropdown");
              if (menu_id != nullptr) {
                if (auto* menu = find_node_by_id_mut(document.root, *menu_id)) {
                  set_class(*menu, "open", !has_class(*menu, "open"));
                  scroll_model.reset();
                  needs_render = true;
                }
              }
            } else if (select_view(hit.target)) {
              last_view_button = hit.target;
              scroll_model.reset();
              needs_render = true;
            } else if (hit.target == "atlas-show-all") {
              set_atlas_frame_visibility(document.root, std::nullopt);
              update_atlas_status(document.root, std::nullopt);
              scroll_model.reset();
              needs_render = true;
            } else if (hit.target == "atlas-first" || hit.target == "atlas-last" ||
                       hit.target == "atlas-previous" || hit.target == "atlas-next" ||
                       std::string_view(hit.target).starts_with("atlas-tab-")) {
              const int count = atlas_frame_count(document.root);
              if (count > 0) {
                int next = hit.value;
                if (hit.target == "atlas-first") {
                  next = 0;
                } else if (hit.target == "atlas-last") {
                  next = count - 1;
                } else if (hit.target == "atlas-previous" || hit.target == "atlas-next") {
                  const bool all = atlas_show_all_active(document.root);
                  const int current = all ? (hit.target == "atlas-previous" ? 0 : -1) :
                      active_atlas_frame(document.root).value_or(
                          hit.target == "atlas-previous" ? 0 : -1);
                  const int delta = hit.target == "atlas-previous" ? -1 : 1;
                  next = ((current + delta) % count + count) % count;
                }
                if (next >= 0 && next < count) {
                  set_atlas_frame_visibility(document.root, next);
                  update_atlas_status(document.root, next);
                  scroll_model.reset();
                  needs_render = true;
                }
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
        if (resize_drag.active) {
          resize_drag.active = false;
          set_cursor_(hover_resize_column >= 0 ? window::ShellCursor::ResizeColumn
                                                      : window::ShellCursor::Arrow);
        }
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

    if (virtual_table_model != nullptr) {
      auto query_it = input_values.find("table-search");
      if (query_it == input_values.end()) {
        query_it = input_values.find("table_search");
      }
      std::string query =
          query_it != input_values.end() ? query_it->second : std::string{};
      for (char& ch : query) {
        ch = static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch)));
      }
      if (query != submitted_virtual_query) {
        submitted_virtual_query = query;
        virtual_row_order.reset();
        virtual_search.submit(query);
        scroll_model.reset();
        table_scroll_damage_only = false;
        needs_render = true;
      }
      if (auto result = virtual_search.take_completed();
          result.has_value() &&
          result->query == submitted_virtual_query) {
        if (submitted_virtual_query.empty()) {
          virtual_row_order.reset();
        } else {
          virtual_row_order =
              std::make_shared<const std::vector<std::int64_t>>(
                  std::move(result->rows));
        }
        scroll_model.reset();
        table_scroll_damage_only = false;
        needs_render = true;
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
