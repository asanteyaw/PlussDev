#include "signal/display/Display.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "signal/Archive.h"
#include "signal/Stream.h"
#include "signal/catalog.h"
#include "table/display/Display.h"

namespace pluss::signal::display {

namespace {

struct EffectiveOptions {
  std::int64_t terminal_width{120};
  std::int64_t terminal_height{40};
  std::int64_t preview_values{4};
  std::int64_t edge_items{4};
  std::int64_t tail_items{2};
  bool show_meta{true};
};

EffectiveOptions resolve_options(const DisplayOptions& in) {
  const auto term = pluss::table::display::detect_terminal();
  EffectiveOptions out;
  out.terminal_width = in.terminal_width > 0 ? in.terminal_width : std::max<std::int64_t>(40, term.columns);
  out.terminal_height = in.terminal_height > 0 ? in.terminal_height : std::max<std::int64_t>(20, term.rows);
  out.preview_values = std::max<std::int64_t>(1, in.preview_values);
  out.edge_items = std::max<std::int64_t>(1, in.edge_items);
  out.tail_items = std::max<std::int64_t>(1, in.tail_items);
  out.show_meta = in.show_meta;
  return out;
}

std::string json_escape(std::string_view in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (char c : in) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

std::string clip_text(std::string_view in, std::int64_t max_len) {
  if (max_len <= 4 || static_cast<std::int64_t>(in.size()) <= max_len) return std::string(in);
  return std::string(in.substr(0, static_cast<std::size_t>(max_len - 3))) + "...";
}

std::vector<std::int64_t> visible_indices(std::int64_t total,
                                          std::int64_t head,
                                          std::int64_t tail,
                                          bool& ellipsis) {
  std::vector<std::int64_t> out;
  ellipsis = false;
  if (total <= 0) return out;
  const auto cap = std::max<std::int64_t>(2, head + tail);
  if (total <= cap) {
    out.reserve(static_cast<std::size_t>(total));
    for (std::int64_t i = 0; i < total; ++i) out.push_back(i);
    return out;
  }
  ellipsis = true;
  out.reserve(static_cast<std::size_t>(head + tail));
  for (std::int64_t i = 0; i < head; ++i) out.push_back(i);
  for (std::int64_t i = total - tail; i < total; ++i) out.push_back(i);
  return out;
}

std::string kind_name(StreamKind kind) {
  switch (kind) {
    case StreamKind::Series: return "Series";
    case StreamKind::Note: return "Note";
    case StreamKind::Timeline: return "Timeline";
    case StreamKind::TimeIndex: return "TimeIndex";
  }
  return "Unknown";
}

std::string thread_value_json(const pluss::table::Thread& thread,
                              std::int64_t row,
                              std::int64_t clip_width) {
  const auto note = thread.cast(pluss::table::ThreadCastTarget::Note).as_note();
  return fmt::format("\"{}\"", json_escape(clip_text(note.at(row), clip_width)));
}

std::string stream_preview_json(const Stream& stream, const EffectiveOptions& options) {
  const auto n = stream.size();
  if (n <= 0) return "[]";

  bool ellipsis = false;
  const auto idx = visible_indices(
      n,
      std::max<std::int64_t>(1, options.preview_values - 1),
      1,
      ellipsis);

  const auto clip_w = std::max<std::int64_t>(10, options.terminal_width / 8);
  std::vector<std::string> parts;
  parts.reserve(static_cast<std::size_t>(idx.size()) + (ellipsis ? 1u : 0u));
  const auto values = stream.to_thread();
  for (std::size_t i = 0; i < idx.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(std::max<std::int64_t>(1, options.preview_values - 1))) {
      parts.push_back("\"...\"");
    }
    parts.push_back(thread_value_json(values, idx[i], clip_w));
  }
  return fmt::format("[{}]", fmt::join(parts, ", "));
}

void append_stream_json(std::ostringstream& oss,
                        const Stream& stream,
                        const EffectiveOptions& options,
                        std::int64_t indent) {
  const auto i0 = std::string(static_cast<std::size_t>(indent), ' ');
  const auto i1 = std::string(static_cast<std::size_t>(indent + 2), ' ');
  const auto i2 = std::string(static_cast<std::size_t>(indent + 4), ' ');
  oss << i0 << "{\n";
  oss << i1 << "\"kind\": \"" << kind_name(stream.kind()) << "\",\n";
  oss << i1 << "\"size\": " << stream.size() << ",\n";
  oss << i1 << "\"dtype\": \"" << json_escape(stream.dtype_name()) << "\",\n";
  oss << i1 << "\"cases\": " << (stream.has_cases() ? "true" : "false") << ",\n";
  oss << i1 << "\"mask\": " << (stream.has_validity_mask() ? "true" : "false") << ",\n";
  oss << i1 << "\"preview\": " << stream_preview_json(stream, options);
  if (options.show_meta) {
    std::vector<std::pair<std::string, std::string>> meta_items(stream.meta().begin(), stream.meta().end());
    std::sort(meta_items.begin(), meta_items.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    oss << ",\n";
    oss << i1 << "\"meta\": {";
    if (!meta_items.empty()) oss << "\n";
    for (std::size_t i = 0; i < meta_items.size(); ++i) {
      oss << i2 << "\"" << json_escape(meta_items[i].first) << "\": \"" << json_escape(meta_items[i].second) << "\"";
      if (i + 1 < meta_items.size()) oss << ",";
      oss << "\n";
    }
    if (!meta_items.empty()) oss << i1;
    oss << "}";
  }
  oss << "\n" << i0 << "}";
}

std::string key_preview_json(const Catalog& catalog, const EffectiveOptions& options) {
  const auto total = static_cast<std::int64_t>(catalog.keys().size());
  if (total <= 0) return "[]";
  bool ellipsis = false;
  const auto idx = visible_indices(total, options.edge_items, options.tail_items, ellipsis);
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(idx.size()) + (ellipsis ? 1u : 0u));
  for (std::size_t i = 0; i < idx.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(options.edge_items)) out.push_back("\"...\"");
    out.push_back(fmt::format("\"{}\"", json_escape(catalog.keys()[static_cast<std::size_t>(idx[i])])));
  }
  return fmt::format("[{}]", fmt::join(out, ", "));
}

} // namespace

std::string render(const Stream& stream, const DisplayOptions& options) {
  if (!stream.defined()) {
    return "{\n  \"_type\": \"Stream\",\n  \"defined\": false\n}";
  }
  const auto effective = resolve_options(options);
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Stream\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"data\": ";
  append_stream_json(oss, stream, effective, 2);
  oss << "\n}";
  return oss.str();
}

std::string render(const Catalog& catalog, const DisplayOptions& options) {
  if (!catalog.defined()) {
    return "{\n  \"_type\": \"Catalog\",\n  \"defined\": false\n}";
  }
  const auto effective = resolve_options(options);
  bool ellipsis = false;
  const auto total = catalog.nstreams();
  const auto visible = visible_indices(total, effective.edge_items, effective.tail_items, ellipsis);

  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Catalog\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"nstreams\": " << total << ",\n";
  oss << "  \"keys\": " << key_preview_json(catalog, effective) << ",\n";
  oss << "  \"streams\": {\n";

  bool first = true;
  for (std::size_t vi = 0; vi < visible.size(); ++vi) {
    if (ellipsis && vi == static_cast<std::size_t>(effective.edge_items)) {
      if (!first) oss << ",\n";
      const auto omitted = total - static_cast<std::int64_t>(visible.size());
      oss << "    \"...\": {\"omitted\": " << std::max<std::int64_t>(0, omitted) << "}";
      first = false;
    }
    const auto idx = static_cast<std::size_t>(visible[vi]);
    if (!first) oss << ",\n";
    oss << "    \"" << json_escape(catalog.keys()[idx]) << "\": ";
    append_stream_json(oss, catalog.streams()[idx], effective, 4);
    first = false;
  }
  oss << "\n  }\n";
  oss << "}";
  return oss.str();
}

std::string render(const Archive& archive, const DisplayOptions& options) {
  if (!archive.defined()) {
    return "{\n  \"_type\": \"Archive\",\n  \"defined\": false\n}";
  }
  const auto effective = resolve_options(options);
  const auto total = archive.depth();
  bool ellipsis = false;
  const auto visible = visible_indices(total, effective.edge_items, effective.tail_items, ellipsis);

  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Archive\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"depth\": " << total << ",\n";
  oss << "  \"has_depth_labels\": " << (archive.depth_labels().has_value() ? "true" : "false") << ",\n";
  oss << "  \"catalogs\": [\n";

  bool first = true;
  for (std::size_t vi = 0; vi < visible.size(); ++vi) {
    if (ellipsis && vi == static_cast<std::size_t>(effective.edge_items)) {
      if (!first) oss << ",\n";
      const auto omitted = total - static_cast<std::int64_t>(visible.size());
      oss << "    {\"index\": \"...\", \"omitted\": " << std::max<std::int64_t>(0, omitted) << "}";
      first = false;
    }
    const auto idx = visible[vi];
    const auto cat = archive.catalog(idx);
    if (!first) oss << ",\n";
    oss << "    {\n";
    oss << "      \"index\": " << idx << ",\n";
    if (archive.depth_labels().has_value()) {
      oss << "      \"label\": " << thread_value_json(archive.depth_labels()->to_thread(), idx,
                                                     std::max<std::int64_t>(12, effective.terminal_width / 8))
          << ",\n";
    }
    oss << "      \"nstreams\": " << cat.nstreams() << ",\n";
    oss << "      \"keys\": " << key_preview_json(cat, effective) << "\n";
    oss << "    }";
    first = false;
  }
  oss << "\n  ]\n";
  oss << "}";
  return oss.str();
}

void print(std::ostream& os, const Stream& stream, const DisplayOptions& options) {
  os << render(stream, options) << '\n';
}

void print(std::ostream& os, const Catalog& catalog, const DisplayOptions& options) {
  os << render(catalog, options) << '\n';
}

void print(std::ostream& os, const Archive& archive, const DisplayOptions& options) {
  os << render(archive, options) << '\n';
}

} // namespace pluss::signal::display
