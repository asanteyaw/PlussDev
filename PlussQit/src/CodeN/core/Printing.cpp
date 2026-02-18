#include "CodeN/core/Printing.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <vector>

namespace pluss::coden {
namespace {

static bool show_header(const NotePrintOptions& opt) {
  return opt.content_mode != NotePrintContentMode::ValuesOnly;
}

static bool show_values(const NotePrintOptions& opt) {
  return opt.content_mode != NotePrintContentMode::HeaderOnly;
}

static void append_indent(::std::ostringstream& oss, ::std::int64_t n) {
  for (::std::int64_t i = 0; i < n; ++i) oss.put(' ');
}

static ::std::string maybe_clip(::std::string s, ::std::int64_t max_chars) {
  if (max_chars <= 0) return s;
  const auto m = static_cast<::std::size_t>(max_chars);
  if (s.size() <= m) return s;
  if (m <= 3) return s.substr(0, m);
  return s.substr(0, m - 3) + "...";
}

static ::std::string encode_hex(::std::string_view s) {
  ::std::ostringstream oss;
  oss << ::std::hex << ::std::setfill('0');
  for (::std::size_t i = 0; i < s.size(); ++i) {
    if (i != 0) oss << ' ';
    oss << ::std::setw(2) << static_cast<int>(static_cast<unsigned char>(s[i]));
  }
  return oss.str();
}

static ::std::string escape_text(::std::string_view s, bool escape_controls) {
  ::std::string out;
  out.reserve(s.size() + 8);
  for (unsigned char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      case '\f': out += "\\f"; break;
      case '\b': out += "\\b"; break;
      default:
        if (!escape_controls || (c >= 0x20 && c != 0x7F)) {
          out.push_back(static_cast<char>(c));
        } else {
          static constexpr char kHex[] = "0123456789ABCDEF";
          out += "\\x";
          out.push_back(kHex[(c >> 4) & 0xF]);
          out.push_back(kHex[c & 0xF]);
        }
        break;
    }
  }
  return out;
}

static ::std::string format_cell(::std::string_view s, bool is_bytes, const NotePrintOptions& opt) {
  if (is_bytes && opt.bytes_mode == NotePrintBytesMode::Hex) {
    ::std::string out = "hex[" + encode_hex(s) + "]";
    return maybe_clip(::std::move(out), opt.max_cell_chars);
  }

  ::std::string body = escape_text(s, opt.escape_controls);
  if (opt.quote_strings) {
    body = (is_bytes ? "b\"" : "\"") + body + "\"";
  }
  return maybe_clip(::std::move(body), opt.max_cell_chars);
}

static ::std::vector<::std::int64_t> visible_tokens(::std::int64_t n,
                                                    ::std::int64_t edge_items,
                                                    bool summarize) {
  ::std::vector<::std::int64_t> out;
  if (n <= 0) return out;

  const ::std::int64_t edge = ::std::max<::std::int64_t>(0, edge_items);
  if (!summarize || edge <= 0 || n <= (2 * edge + 1)) {
    out.reserve(static_cast<::std::size_t>(n));
    for (::std::int64_t i = 0; i < n; ++i) out.push_back(i);
    return out;
  }

  out.reserve(static_cast<::std::size_t>(2 * edge + 1));
  for (::std::int64_t i = 0; i < edge; ++i) out.push_back(i);
  out.push_back(-1);
  for (::std::int64_t i = n - edge; i < n; ++i) out.push_back(i);
  return out;
}

static void append_metadata(::std::ostringstream& oss, const Note& n, const NotePrintOptions& opt) {
  if (opt.metadata_mode == NotePrintMetadataMode::None) return;

  const auto sizes = n.sizes();
  const bool compact = opt.metadata_mode == NotePrintMetadataMode::Compact;

  oss << "Note(";
  bool first = true;
  auto add = [&](const char* key, const ::std::string& val) {
    if (!first) oss << ", ";
    first = false;
    oss << key << "=" << val;
  };

  if (opt.show_shape || compact) {
    ::std::ostringstream sh;
    sh << "[";
    for (::std::size_t i = 0; i < sizes.size(); ++i) {
      if (i != 0) sh << ", ";
      sh << sizes[i];
    }
    sh << "]";
    add("shape", sh.str());
  }
  if (opt.show_dim || compact) add("dim", ::std::to_string(n.dim()));
  if (opt.show_numel) add("numel", ::std::to_string(n.numel()));
  if (opt.show_encoding || compact) {
    add("encoding", static_cast<const NoteBase&>(n).is_bytes() ? "bytes" : "utf8");
  }

  oss << ")";
}

static void render_recursive(::std::ostringstream& oss,
                             const Note& n,
                             const ::std::vector<::std::int64_t>& sizes,
                             const ::std::vector<::std::int64_t>& strides,
                             ::std::int64_t dim,
                             ::std::int64_t base,
                             const NotePrintOptions& opt,
                             ::std::int64_t indent,
                             bool summarize) {
  if (dim == static_cast<::std::int64_t>(sizes.size())) {
    oss << format_cell(n.at(base), static_cast<const NoteBase&>(n).is_bytes(), opt);
    return;
  }

  const ::std::int64_t size_d = sizes[static_cast<::std::size_t>(dim)];
  const auto toks = visible_tokens(size_d, opt.edge_items, summarize);

  const bool inline_list = (dim == static_cast<::std::int64_t>(sizes.size()) - 1);
  oss << "[";
  for (::std::size_t i = 0; i < toks.size(); ++i) {
    if (i != 0) oss << ", ";
    const auto t = toks[i];
    if (t < 0) {
      oss << "...";
      continue;
    }

    if (inline_list) {
      render_recursive(oss, n, sizes, strides, dim + 1, base + t * strides[static_cast<::std::size_t>(dim)],
                       opt, indent + 2, summarize);
    } else {
      if (i != 0) {
        oss << "\n";
        append_indent(oss, indent + 2);
      }
      if (i == 0) {
        oss << "\n";
        append_indent(oss, indent + 2);
      }
      render_recursive(oss, n, sizes, strides, dim + 1, base + t * strides[static_cast<::std::size_t>(dim)],
                       opt, indent + 2, summarize);
    }
  }

  if (!inline_list && !toks.empty()) {
    oss << "\n";
    append_indent(oss, indent);
  }
  oss << "]";
}

static ::std::string render_note(const Note& n, const NotePrintOptions& opt) {
  if (!n.defined()) return "Note(undefined)";

  ::std::ostringstream oss;
  if (show_header(opt)) {
    append_metadata(oss, n, opt);
    if (show_values(opt)) oss << "\n";
  }
  if (!show_values(opt)) return oss.str();

  if (n.dim() == 0) {
    oss << format_cell(n.at(0), static_cast<const NoteBase&>(n).is_bytes(), opt);
    return oss.str();
  }

  if (n.numel() == 0) {
    oss << "[]";
    return oss.str();
  }

  const auto sizes = n.sizes();
  ::std::vector<::std::int64_t> strides(sizes.size(), 1);
  for (::std::int64_t i = static_cast<::std::int64_t>(sizes.size()) - 2; i >= 0; --i) {
    strides[static_cast<::std::size_t>(i)] =
        strides[static_cast<::std::size_t>(i + 1)] * sizes[static_cast<::std::size_t>(i + 1)];
  }

  const bool summarize =
      (opt.edge_items > 0) && (opt.max_elements > 0) && (n.numel() > opt.max_elements);
  render_recursive(oss, n, sizes, strides, 0, 0, opt, 0, summarize);
  return oss.str();
}

} // namespace

::std::string to_string(const Note& n, const NotePrintOptions& opt) {
  return render_note(n, opt);
}

::std::string print_note(const Note& n, const NotePrintOptions& opt) {
  return render_note(n, opt);
}

::std::ostream& print_note(::std::ostream& os, const Note& n, const NotePrintOptions& opt) {
  os << print_note(n, opt);
  return os;
}

::std::ostream& operator<<(::std::ostream& os, const Note& n) {
  os << to_string(n);
  return os;
}

} // namespace pluss::coden
