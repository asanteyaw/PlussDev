#include "kindle/display/Display.h"

#include <algorithm>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "kindle/Archive.h"
#include "kindle/Stream.h"
#include "kindle/catalog.h"
#include "table/Slate.h"
#include "table/display/Display.h"

namespace pluss::kindle::display {

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
  out.terminal_width = in.terminal_width > 0 ? in.terminal_width : std::max<std::int64_t>(48, term.columns);
  out.terminal_height = in.terminal_height > 0 ? in.terminal_height : std::max<std::int64_t>(20, term.rows);
  out.preview_values = std::clamp<std::int64_t>(in.preview_values, 1, 12);
  out.edge_items = std::clamp<std::int64_t>(in.edge_items, 1, 16);
  out.tail_items = std::clamp<std::int64_t>(in.tail_items, 1, 8);
  out.show_meta = in.show_meta;
  return out;
}

std::string clip_text(std::string_view in, std::int64_t max_len) {
  if (max_len <= 0 || static_cast<std::int64_t>(in.size()) <= max_len) {
    return std::string(in);
  }
  if (max_len <= 1) {
    return std::string(in.substr(0, 1));
  }
  return std::string(in.substr(0, static_cast<std::size_t>(max_len - 1))) + "~";
}

std::string right_pad(std::string_view s, std::int64_t width) {
  if (width <= 0) {
    return std::string();
  }
  if (static_cast<std::int64_t>(s.size()) >= width) {
    return clip_text(s, width);
  }
  return std::string(s) + std::string(static_cast<std::size_t>(width - static_cast<std::int64_t>(s.size())), ' ');
}

std::string fit_line(std::string_view line, std::int64_t width) {
  return clip_text(line, width);
}

std::vector<std::int64_t> visible_indices(std::int64_t total,
                                          std::int64_t head,
                                          std::int64_t tail,
                                          bool& ellipsis) {
  std::vector<std::int64_t> out;
  ellipsis = false;
  if (total <= 0) {
    return out;
  }
  const auto cap = std::max<std::int64_t>(2, head + tail);
  if (total <= cap) {
    out.reserve(static_cast<std::size_t>(total));
    for (std::int64_t i = 0; i < total; ++i) {
      out.push_back(i);
    }
    return out;
  }

  ellipsis = true;
  out.reserve(static_cast<std::size_t>(head + tail));
  for (std::int64_t i = 0; i < head; ++i) {
    out.push_back(i);
  }
  for (std::int64_t i = total - tail; i < total; ++i) {
    out.push_back(i);
  }
  return out;
}

std::string kind_name(StreamKind kind) {
  switch (kind) {
    case StreamKind::Series:
      return "Series";
    case StreamKind::Note:
      return "Note";
    case StreamKind::Timeline:
      return "Timeline";
    case StreamKind::Duration:
      return "Duration";
  }
  return "Unknown";
}

std::string slate_value_at(const pluss::table::Slate& slate, std::int64_t row, std::int64_t max_width) {
  if (pluss::table::detail::has_validity_mask(slate)) {
    const auto valid = pluss::table::detail::validity_mask(slate)->to_vec<bool>();
    if (!valid[static_cast<std::size_t>(row)]) {
      return "NA";
    }
  }
  const auto note = slate.cast().note().as_note();
  return clip_text(note.at(row), max_width);
}

std::string stream_preview(const Stream& stream, const EffectiveOptions& options, std::int64_t max_width) {
  if (stream.size() <= 0) {
    return "[]";
  }

  bool ellipsis = false;
  const auto idx = visible_indices(
      stream.size(),
      std::max<std::int64_t>(1, options.preview_values - 1),
      1,
      ellipsis);

  const auto slate = stream.to_slate();
  std::vector<std::string> parts;
  parts.reserve(static_cast<std::size_t>(idx.size()) + (ellipsis ? 1u : 0u));

  for (std::size_t i = 0; i < idx.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(std::max<std::int64_t>(1, options.preview_values - 1))) {
      parts.emplace_back("...");
    }
    parts.emplace_back(slate_value_at(slate, idx[i], std::max<std::int64_t>(8, max_width / 2)));
  }

  return clip_text(fmt::format("[{}]", fmt::join(parts, ", ")), max_width);
}

std::int64_t frame_rule_width(std::int64_t terminal_width) {
  return std::max<std::int64_t>(36, terminal_width);
}

std::string frame_rule(char ch, std::int64_t width) {
  return std::string(static_cast<std::size_t>(std::max<std::int64_t>(1, width)), ch);
}

std::string catalog_keys_preview(const Catalog& catalog,
                                 const EffectiveOptions& options,
                                 std::int64_t max_width) {
  if (catalog.nkeys() == 0) {
    return "[]";
  }
  bool ellipsis = false;
  const auto idx = visible_indices(catalog.nkeys(), options.edge_items, options.tail_items, ellipsis);
  std::vector<std::string> names;
  names.reserve(static_cast<std::size_t>(idx.size()) + (ellipsis ? 1u : 0u));
  for (std::size_t i = 0; i < idx.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(options.edge_items)) {
      names.emplace_back("...");
    }
    names.emplace_back(catalog.keys()[static_cast<std::size_t>(idx[i])]);
  }
  return clip_text(fmt::format("[{}]", fmt::join(names, ", ")), max_width);
}

std::string render_stream_frame(const Stream& stream, const EffectiveOptions& options) {
  if (!stream.defined()) {
    return "kindle::Stream <undefined>\n";
  }

  const auto width = frame_rule_width(options.terminal_width);
  std::ostringstream oss;
  oss << frame_rule('=', width) << "\n";
  oss << fit_line(
      fmt::format("kindle::Stream  kind={}  dtype={}  records={}",
                  kind_name(stream.kind()),
                  stream.dtype(),
                  stream.size()),
      width)
      << "\n";
  oss << fit_line(
      fmt::format("label={}  mask={}  meta={}",
                  stream.has_label() ? "yes" : "no",
                  detail::has_validity_mask(stream) ? "yes" : "no",
                  stream.meta().size()),
      width)
      << "\n";
  oss << frame_rule('-', width) << "\n";

  const auto slate = stream.to_slate();
  std::int64_t body_budget = std::max<std::int64_t>(3, options.terminal_height - 8);
  const auto head = std::max<std::int64_t>(1, std::min<std::int64_t>(options.edge_items, body_budget - 1));
  const auto tail = std::max<std::int64_t>(1, std::min<std::int64_t>(options.tail_items, body_budget - head - 1));

  bool ellipsis = false;
  const auto idx = visible_indices(stream.size(), head, tail, ellipsis);

  const auto case_w = std::max<std::int64_t>(5, std::min<std::int64_t>(12, width / 5));
  const auto value_w = std::max<std::int64_t>(8, width - case_w - 3);

  oss << right_pad("label", case_w) << " | " << right_pad("value", value_w) << "\n";
  oss << frame_rule('-', width) << "\n";

  for (std::size_t i = 0; i < idx.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(head)) {
      oss << right_pad("...", case_w) << " | " << right_pad("...", value_w) << "\n";
    }
    const auto row = idx[i];
    std::string case_cell;
    if (stream.has_label()) {
      case_cell = slate_value_at(stream.label()->cast().note(), row, case_w);
    } else {
      case_cell = std::to_string(row);
    }
    const auto value_cell = slate_value_at(slate, row, value_w);
    oss << right_pad(case_cell, case_w) << " | " << right_pad(value_cell, value_w) << "\n";
  }

  if (options.show_meta && !stream.meta().empty()) {
    oss << frame_rule('-', width) << "\n";
    std::vector<std::pair<std::string, std::string>> meta_items(stream.meta().begin(), stream.meta().end());
    std::sort(meta_items.begin(), meta_items.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& [k, v] : meta_items) {
      oss << fit_line(fmt::format("{}: {}", k, v), width) << "\n";
    }
  }

  oss << frame_rule('=', width);
  return oss.str();
}

std::string render_catalog_frame(const Catalog& catalog, const EffectiveOptions& options) {
  if (!catalog.defined()) {
    return "kindle::Catalog <undefined>\n";
  }

  const auto width = frame_rule_width(options.terminal_width);
  std::ostringstream oss;
  oss << frame_rule('=', width) << "\n";
  oss << fit_line(fmt::format("kindle::Catalog  keys={}", catalog.nkeys()), width) << "\n";
  oss << fit_line(fmt::format("keys: {}", catalog_keys_preview(catalog, options, width - 6)), width) << "\n";
  oss << frame_rule('-', width) << "\n";

  const bool show_dtype = width >= 96;
  const bool show_cs = width >= 72;

  const auto key_w = std::max<std::int64_t>(10, std::min<std::int64_t>(24, width / 4));
  const auto kind_w = std::max<std::int64_t>(8, std::min<std::int64_t>(12, width / 8));
  const auto dtype_w = show_dtype ? std::max<std::int64_t>(8, std::min<std::int64_t>(16, width / 6)) : 0;
  const auto cs_w = show_cs ? 7 : 0;
  const auto sep_count = show_dtype ? (show_cs ? 4 : 3) : (show_cs ? 3 : 2);
  const auto fixed = key_w + kind_w + dtype_w + cs_w + sep_count * 3;
  const auto preview_w = std::max<std::int64_t>(12, width - fixed);

  std::string header = right_pad("key", key_w) + " | " + right_pad("kind", kind_w);
  if (show_dtype) {
    header += " | " + right_pad("dtype", dtype_w);
  }
  if (show_cs) {
    header += " | " + right_pad("cs", cs_w);
  }
  header += " | " + right_pad("preview", preview_w);
  oss << fit_line(header, width) << "\n";
  oss << frame_rule('-', width) << "\n";

  std::int64_t body_budget = std::max<std::int64_t>(4, options.terminal_height - 8);
  const auto head = std::max<std::int64_t>(1, std::min<std::int64_t>(options.edge_items, body_budget - 1));
  const auto tail = std::max<std::int64_t>(1, std::min<std::int64_t>(options.tail_items, body_budget - head - 1));
  bool ellipsis = false;
  const auto idx = visible_indices(catalog.nkeys(), head, tail, ellipsis);

  for (std::size_t i = 0; i < idx.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(head)) {
      oss << right_pad("...", key_w) << " | " << right_pad("...", kind_w);
      if (show_dtype) {
        oss << " | " << right_pad("...", dtype_w);
      }
      if (show_cs) {
        oss << " | " << right_pad("...", cs_w);
      }
      oss << " | " << right_pad("...", preview_w) << "\n";
    }

    const auto row = static_cast<std::size_t>(idx[i]);
    const auto& key = catalog.keys()[row];
    const auto stream = catalog.keys(key);
    std::string line = right_pad(clip_text(key, key_w), key_w) + " | "
                     + right_pad(kind_name(stream.kind()), kind_w);
    if (show_dtype) {
      line += " | " + right_pad(clip_text(stream.dtype(), dtype_w), dtype_w);
    }
    if (show_cs) {
      line += " | " + right_pad(std::to_string(stream.size()), cs_w);
    }
    line += " | " + right_pad(stream_preview(stream, options, preview_w), preview_w);
    oss << fit_line(line, width) << "\n";
  }

  oss << frame_rule('=', width);
  return oss.str();
}

std::string render_archive_frame(const pluss::kindle::Archive& archive,
                                 const EffectiveOptions& options) {
  if (!archive.defined()) {
    return "kindle::Archive <undefined>\n";
  }

  const auto width = frame_rule_width(options.terminal_width);
  std::ostringstream oss;
  oss << frame_rule('=', width) << "\n";
  oss << fit_line(fmt::format("kindle::Archive  depth={}", archive.depth()), width) << "\n";
  oss << fit_line(fmt::format("depth_labels={}", archive.label().has() ? "yes" : "no"), width) << "\n";
  oss << frame_rule('-', width) << "\n";

  const bool show_label = archive.label().has() && width >= 84;
  const bool show_span = width >= 96;

  const auto frame_w = 6;
  const auto label_w = show_label ? std::max<std::int64_t>(8, std::min<std::int64_t>(22, width / 5)) : 0;
  const auto nkeys_w = 8;
  const auto min_w = show_span ? 6 : 0;
  const auto max_w = show_span ? 6 : 0;
  const auto sep_count = show_label ? (show_span ? 5 : 4) : (show_span ? 4 : 3);
  const auto fixed = frame_w + label_w + nkeys_w + min_w + max_w + sep_count * 3;
  const auto keys_w = std::max<std::int64_t>(12, width - fixed);

  std::string header = right_pad("frame", frame_w);
  if (show_label) {
    header += " | " + right_pad("label", label_w);
  }
  header += " | " + right_pad("keys", nkeys_w);
  if (show_span) {
    header += " | " + right_pad("min_cs", min_w) + " | " + right_pad("max_cs", max_w);
  }
  header += " | " + right_pad("keys", keys_w);
  oss << fit_line(header, width) << "\n";
  oss << frame_rule('-', width) << "\n";

  std::int64_t body_budget = std::max<std::int64_t>(4, options.terminal_height - 8);
  const auto head = std::max<std::int64_t>(1, std::min<std::int64_t>(options.edge_items, body_budget - 1));
  const auto tail = std::max<std::int64_t>(1, std::min<std::int64_t>(options.tail_items, body_budget - head - 1));
  bool ellipsis = false;
  const auto idx = visible_indices(archive.depth(), head, tail, ellipsis);

  std::optional<pluss::table::Slate> label_slate = std::nullopt;
  if (archive.label().has()) {
    label_slate = archive.label().values().to_slate().cast().note();
  }

  for (std::size_t i = 0; i < idx.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(head)) {
      std::string line = right_pad("...", frame_w);
      if (show_label) {
        line += " | " + right_pad("...", label_w);
      }
      line += " | " + right_pad("...", nkeys_w);
      if (show_span) {
        line += " | " + right_pad("...", min_w) + " | " + right_pad("...", max_w);
      }
      line += " | " + right_pad("...", keys_w);
      oss << fit_line(line, width) << "\n";
    }

    const auto row = idx[i];
    const auto cat = archive.catalog(row);

    std::int64_t min_cs = 0;
    std::int64_t max_cs = 0;
    if (cat.nkeys() > 0) {
      min_cs = std::numeric_limits<std::int64_t>::max();
      for (const auto& key : cat.keys()) {
        const auto s = cat.keys(key);
        min_cs = std::min(min_cs, s.size());
        max_cs = std::max(max_cs, s.size());
      }
    }

    std::string line = right_pad(std::to_string(row), frame_w);
    if (show_label) {
      auto lbl = slate_value_at(*label_slate, row, label_w);
      line += " | " + right_pad(lbl, label_w);
    }
    line += " | " + right_pad(std::to_string(cat.nkeys()), nkeys_w);
    if (show_span) {
      line += " | " + right_pad(std::to_string(min_cs), min_w)
           + " | " + right_pad(std::to_string(max_cs), max_w);
    }
    line += " | " + right_pad(catalog_keys_preview(cat, options, keys_w), keys_w);
    oss << fit_line(line, width) << "\n";
  }

  oss << frame_rule('=', width);
  return oss.str();
}

} // namespace

std::string render(const Stream& stream, const DisplayOptions& options) {
  return render_stream_frame(stream, resolve_options(options));
}

std::string render(const Catalog& catalog, const DisplayOptions& options) {
  return render_catalog_frame(catalog, resolve_options(options));
}

std::string render(const pluss::kindle::Archive& archive, const DisplayOptions& options) {
  return render_archive_frame(archive, resolve_options(options));
}

void print(std::ostream& os, const Stream& stream, const DisplayOptions& options) {
  os << render(stream, options) << '\n';
}

void print(std::ostream& os, const Catalog& catalog, const DisplayOptions& options) {
  os << render(catalog, options) << '\n';
}

void print(std::ostream& os,
           const pluss::kindle::Archive& archive,
           const DisplayOptions& options) {
  os << render(archive, options) << '\n';
}

} // namespace pluss::kindle::display
