#include "pluss/view/window/html_document.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::view::window {

namespace {

[[nodiscard]] std::string to_lower(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char ch : value) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
}

[[nodiscard]] std::string trim_copy(std::string_view value) {
  std::size_t begin = 0;
  while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
    ++begin;
  }

  std::size_t end = value.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    --end;
  }

  return std::string(value.substr(begin, end - begin));
}

[[nodiscard]] std::string collapse_html_whitespace(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  bool in_space = false;
  for (const char ch : value) {
    const bool space = std::isspace(static_cast<unsigned char>(ch)) != 0;
    if (space) {
      if (!in_space) {
        out.push_back(' ');
      }
      in_space = true;
      continue;
    }
    in_space = false;
    out.push_back(ch);
  }
  return trim_copy(out);
}

[[nodiscard]] std::string decode_basic_entities(std::string_view text) {
  std::string out;
  out.reserve(text.size());

  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '&') {
      out.push_back(text[i]);
      continue;
    }

    if (text.substr(i, 4) == "&lt;") {
      out.push_back('<');
      i += 3;
      continue;
    }
    if (text.substr(i, 4) == "&gt;") {
      out.push_back('>');
      i += 3;
      continue;
    }
    if (text.substr(i, 5) == "&amp;") {
      out.push_back('&');
      i += 4;
      continue;
    }
    if (text.substr(i, 6) == "&quot;") {
      out.push_back('"');
      i += 5;
      continue;
    }
    if (text.substr(i, 6) == "&apos;") {
      out.push_back('\'');
      i += 5;
      continue;
    }
    if (text.substr(i, 6) == "&nbsp;") {
      out.push_back(' ');
      i += 5;
      continue;
    }

    out.push_back(text[i]);
  }

  return out;
}

[[nodiscard]] bool is_void_tag(std::string_view tag) {
  return tag == "area" || tag == "base" || tag == "br" || tag == "col" || tag == "embed" ||
         tag == "hr" || tag == "img" || tag == "input" || tag == "link" || tag == "meta" ||
         tag == "param" || tag == "source" || tag == "track" || tag == "wbr";
}

[[nodiscard]] bool is_inline_tag(std::string_view tag) {
  return tag == "span" || tag == "a" || tag == "strong" || tag == "em" || tag == "b" ||
         tag == "i" || tag == "code" || tag == "label";
}

[[nodiscard]] HtmlNode make_element_node(std::string tag) {
  HtmlNode node{};
  node.kind = HtmlNodeKind::Element;
  node.tag = std::move(tag);
  node.style.display = is_inline_tag(node.tag) ? "inline" : "block";

  if (node.tag == "h1") {
    node.style.font_size_px = 24;
    node.style.line_height_px = 30;
    node.style.margin_px = 12;
  } else if (node.tag == "h2") {
    node.style.font_size_px = 20;
    node.style.line_height_px = 26;
    node.style.margin_px = 10;
  } else if (node.tag == "h3") {
    node.style.font_size_px = 18;
    node.style.line_height_px = 24;
    node.style.margin_px = 8;
  } else if (node.tag == "p") {
    node.style.font_size_px = 13;
    node.style.line_height_px = 20;
    node.style.margin_px = 6;
  } else if (node.tag == "button") {
    node.style.font_size_px = 13;
    node.style.line_height_px = 18;
    node.style.display = "inline-block";
    node.style.padding_px = 8;
    node.style.margin_px = 6;
    node.style.border_px = 1;
    node.style.border_color = "#8c959f";
    node.style.background = "#f6f8fa";
    node.style.border_radius_px = 8;
  } else if (node.tag == "input") {
    node.style.font_size_px = 13;
    node.style.line_height_px = 18;
    node.style.display = "inline-block";
    node.style.padding_px = 8;
    node.style.margin_px = 4;
    node.style.border_px = 1;
    node.style.border_color = "#c4ccd5";
    node.style.background = "#ffffff";
    node.style.border_radius_px = 8;
    node.style.width_px = 260;
    node.style.height_px = 36;
  } else if (node.tag == "table") {
    node.style.margin_px = 8;
    node.style.border_px = 1;
    node.style.border_color = "#d0d7de";
    node.style.background = "#ffffff";
    node.style.border_radius_px = 10;
  } else if (node.tag == "th" || node.tag == "td") {
    node.style.font_size_px = 13;
    node.style.line_height_px = 20;
    node.style.padding_px = 6;
    node.style.border_px = 1;
    node.style.border_color = "#d8dee4";
    node.style.display = "table-cell";
  } else if (node.tag == "tr") {
    node.style.display = "table-row";
  } else if (node.tag == "canvas") {
    node.kind = HtmlNodeKind::Widget;
    node.widget = HtmlWidgetKind::Canvas;
    node.style.height_px = 240;
    node.style.border_px = 1;
    node.style.border_color = "#d0d7de";
  } else if (node.tag == "svg") {
    node.kind = HtmlNodeKind::Widget;
    node.widget = HtmlWidgetKind::Svg;
    node.style.height_px = 240;
    node.style.border_px = 1;
    node.style.border_color = "#d0d7de";
  } else if (node.tag == "line-plot" || node.tag == "chart") {
    node.kind = HtmlNodeKind::Widget;
    node.widget = HtmlWidgetKind::LinePlot;
    node.style.height_px = 260;
    node.style.border_px = 1;
    node.style.border_color = "#d0d7de";
  }

  return node;
}

[[nodiscard]] std::vector<HtmlAttribute> parse_attributes(std::string_view raw) {
  std::vector<HtmlAttribute> attributes;
  std::size_t i = 0;

  auto skip_space = [&]() {
    while (i < raw.size() && std::isspace(static_cast<unsigned char>(raw[i])) != 0) {
      ++i;
    }
  };

  while (i < raw.size()) {
    skip_space();
    if (i >= raw.size()) {
      break;
    }
    if (raw[i] == '/' || raw[i] == '>') {
      ++i;
      continue;
    }

    const std::size_t name_begin = i;
    while (i < raw.size()) {
      const char ch = raw[i];
      if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == '=' || ch == '/' || ch == '>') {
        break;
      }
      ++i;
    }
    if (i <= name_begin) {
      ++i;
      continue;
    }

    std::string name = to_lower(raw.substr(name_begin, i - name_begin));
    skip_space();

    std::string value;
    if (i < raw.size() && raw[i] == '=') {
      ++i;
      skip_space();

      if (i < raw.size() && (raw[i] == '"' || raw[i] == '\'')) {
        const char quote = raw[i++];
        const std::size_t value_begin = i;
        while (i < raw.size() && raw[i] != quote) {
          ++i;
        }
        value = std::string(raw.substr(value_begin, i - value_begin));
        if (i < raw.size() && raw[i] == quote) {
          ++i;
        }
      } else {
        const std::size_t value_begin = i;
        while (i < raw.size()) {
          const char ch = raw[i];
          if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == '/' || ch == '>') {
            break;
          }
          ++i;
        }
        value = std::string(raw.substr(value_begin, i - value_begin));
      }
    }

    attributes.push_back(HtmlAttribute{std::move(name), decode_basic_entities(value)});
  }

  return attributes;
}

struct ParsedTag {
  std::string name{};
  std::vector<HtmlAttribute> attributes{};
  bool is_closing{false};
  bool is_self_closing{false};
  bool is_ignored{false};
};

[[nodiscard]] ParsedTag parse_tag(std::string_view token) {
  ParsedTag out{};
  std::string raw = trim_copy(token);
  if (raw.empty()) {
    out.is_ignored = true;
    return out;
  }

  if (raw.front() == '!' || raw.front() == '?') {
    out.is_ignored = true;
    return out;
  }

  if (raw.front() == '/') {
    out.is_closing = true;
    raw.erase(raw.begin());
    raw = trim_copy(raw);
  }

  if (!raw.empty() && raw.back() == '/') {
    out.is_self_closing = true;
    raw.pop_back();
    raw = trim_copy(raw);
  }

  std::size_t pos = 0;
  while (pos < raw.size()) {
    const char ch = raw[pos];
    if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == '/' || ch == '>') {
      break;
    }
    ++pos;
  }
  out.name = to_lower(raw.substr(0, pos));
  if (out.name.empty()) {
    out.is_ignored = true;
    return out;
  }

  if (!out.is_closing && pos < raw.size()) {
    out.attributes = parse_attributes(raw.substr(pos));
  }
  return out;
}

void append_text_node(std::string_view text, HtmlNode& parent) {
  const std::string decoded = decode_basic_entities(text);
  const std::string cleaned = collapse_html_whitespace(decoded);
  if (cleaned.empty()) {
    return;
  }

  HtmlNode node{};
  node.kind = HtmlNodeKind::Text;
  node.text = cleaned;
  parent.children.push_back(std::move(node));
}

[[nodiscard]] std::vector<HtmlAttribute> parse_css_declarations(std::string_view block) {
  std::vector<HtmlAttribute> out;
  std::size_t start = 0;
  while (start < block.size()) {
    std::size_t end = block.find(';', start);
    if (end == std::string_view::npos) {
      end = block.size();
    }
    const std::string decl = trim_copy(block.substr(start, end - start));
    start = end + 1;
    if (decl.empty()) {
      continue;
    }

    const std::size_t colon = decl.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    const std::string key = to_lower(trim_copy(std::string_view(decl.data(), colon)));
    const std::string value = trim_copy(std::string_view(decl.data() + colon + 1, decl.size() - colon - 1));
    if (key.empty() || value.empty()) {
      continue;
    }
    out.push_back(HtmlAttribute{key, value});
  }
  return out;
}

[[nodiscard]] std::string strip_css_comments(std::string_view css) {
  std::string out;
  out.reserve(css.size());
  std::size_t i = 0;
  while (i < css.size()) {
    if (i + 1 < css.size() && css[i] == '/' && css[i + 1] == '*') {
      i += 2;
      while (i + 1 < css.size() && !(css[i] == '*' && css[i + 1] == '/')) {
        ++i;
      }
      if (i + 1 < css.size()) {
        i += 2;
      }
      continue;
    }
    out.push_back(css[i]);
    ++i;
  }
  return out;
}

[[nodiscard]] std::size_t find_matching_brace(std::string_view text, const std::size_t open_brace) {
  if (open_brace >= text.size() || text[open_brace] != '{') {
    return std::string_view::npos;
  }

  int depth = 1;
  for (std::size_t i = open_brace + 1; i < text.size(); ++i) {
    if (text[i] == '{') {
      ++depth;
    } else if (text[i] == '}') {
      --depth;
      if (depth == 0) {
        return i;
      }
    }
  }
  return std::string_view::npos;
}

struct MediaConstraint {
  int min_width_px{0};
  int max_width_px{0};
};

[[nodiscard]] int parse_css_width_px(std::string_view token) {
  const std::string t = to_lower(trim_copy(token));
  if (t.empty()) {
    return 0;
  }

  char* end = nullptr;
  const long parsed = std::strtol(t.c_str(), &end, 10);
  if (end == t.c_str()) {
    return 0;
  }
  if (end != nullptr && *end != '\0') {
    const std::string unit(end);
    if (unit != "px") {
      return 0;
    }
  }
  return (parsed > 0) ? static_cast<int>(parsed) : 0;
}

[[nodiscard]] MediaConstraint merge_media_constraints(const MediaConstraint base,
                                                      const int min_width_px,
                                                      const int max_width_px) {
  MediaConstraint merged = base;
  if (min_width_px > 0) {
    merged.min_width_px = std::max(merged.min_width_px, min_width_px);
  }
  if (max_width_px > 0) {
    merged.max_width_px =
        (merged.max_width_px > 0) ? std::min(merged.max_width_px, max_width_px) : max_width_px;
  }
  if (merged.max_width_px > 0 && merged.min_width_px > merged.max_width_px) {
    merged.max_width_px = merged.min_width_px;
  }
  return merged;
}

[[nodiscard]] MediaConstraint parse_media_query_constraint(std::string_view query,
                                                           const MediaConstraint base) {
  const std::string lower = to_lower(trim_copy(query));
  if (lower.rfind("@media", 0) != 0) {
    return base;
  }

  int min_width_px = 0;
  int max_width_px = 0;

  auto parse_width_clause = [&](const std::string_view key, int& out) {
    std::size_t pos = 0;
    while (true) {
      pos = lower.find(std::string(key), pos);
      if (pos == std::string::npos) {
        break;
      }
      const std::size_t colon = lower.find(':', pos + key.size());
      if (colon == std::string::npos) {
        break;
      }
      std::size_t end = lower.find(')', colon + 1);
      if (end == std::string::npos) {
        end = lower.size();
      }
      const int parsed = parse_css_width_px(std::string_view(lower.data() + colon + 1, end - colon - 1));
      if (parsed > 0) {
        out = parsed;
      }
      pos = end;
    }
  };

  parse_width_clause("min-width", min_width_px);
  parse_width_clause("max-width", max_width_px);
  return merge_media_constraints(base, min_width_px, max_width_px);
}

void parse_css_rules(std::string_view css, HtmlDocument& doc, const MediaConstraint active_constraint) {
  const std::string cleaned = strip_css_comments(css);
  std::size_t pos = 0;
  while (pos < cleaned.size()) {
    while (pos < cleaned.size() && std::isspace(static_cast<unsigned char>(cleaned[pos])) != 0) {
      ++pos;
    }
    if (pos >= cleaned.size()) {
      break;
    }

    const std::size_t lbrace = cleaned.find('{', pos);
    if (lbrace == std::string_view::npos) {
      break;
    }
    const std::size_t rbrace = find_matching_brace(cleaned, lbrace);
    if (rbrace == std::string_view::npos) {
      break;
    }

    const std::string selectors = trim_copy(std::string_view(cleaned.data() + pos, lbrace - pos));
    const std::string_view block(cleaned.data() + lbrace + 1, rbrace - lbrace - 1);

    if (!selectors.empty()) {
      if (selectors[0] == '@') {
        // Handle grouped at-rules (e.g. @media) by parsing nested rules.
        parse_css_rules(block, doc, parse_media_query_constraint(selectors, active_constraint));
      } else {
        const auto declarations = parse_css_declarations(block);
        if (!declarations.empty()) {
          std::size_t s = 0;
          while (s < selectors.size()) {
            std::size_t e = selectors.find(',', s);
            if (e == std::string::npos) {
              e = selectors.size();
            }
            const std::string selector = to_lower(trim_copy(std::string_view(selectors.data() + s, e - s)));
            if (!selector.empty()) {
              if (selector == ":root" || selector == "html" || selector == "body") {
                for (const auto& decl : declarations) {
                  if (decl.name.size() > 2 && decl.name[0] == '-' && decl.name[1] == '-') {
                    doc.css_variables[to_lower(decl.name)] = decl.value;
                  }
                }
              }
              doc.style_rules.push_back(HtmlCssRule{
                  .selector = selector,
                  .declarations = declarations,
                  .media_min_width_px = active_constraint.min_width_px,
                  .media_max_width_px = active_constraint.max_width_px,
              });
            }
            s = e + 1;
          }
        }
      }
    }

    pos = rbrace + 1;
  }
}

void collect_text_recursive(const HtmlNode& node, std::string& out) {
  if (node.kind == HtmlNodeKind::Text && !node.text.empty()) {
    if (!out.empty()) {
      out.push_back('\n');
    }
    out += node.text;
  } else if (node.kind == HtmlNodeKind::Widget && !node.tag.empty()) {
    if (!out.empty()) {
      out.push_back('\n');
    }
    out += "[widget:";
    out += node.tag;
    out += "]";
  }

  for (const auto& child : node.children) {
    collect_text_recursive(child, out);
  }
}

} // namespace

HtmlDocument parse_html_document(const std::string_view html, std::string title) {
  HtmlDocument doc{};
  if (!title.empty()) {
    doc.title = std::move(title);
  }

  std::vector<HtmlNode*> stack;
  stack.reserve(64);
  stack.push_back(&doc.root);

  std::size_t cursor = 0;
  while (cursor < html.size()) {
    const std::size_t lt = html.find('<', cursor);
    if (lt == std::string_view::npos) {
      append_text_node(html.substr(cursor), *stack.back());
      break;
    }

    if (lt > cursor) {
      append_text_node(html.substr(cursor, lt - cursor), *stack.back());
    }

    if (html.substr(lt, 4) == "<!--") {
      const std::size_t comment_end = html.find("-->", lt + 4);
      if (comment_end == std::string_view::npos) {
        break;
      }
      cursor = comment_end + 3;
      continue;
    }

    const std::size_t gt = html.find('>', lt + 1);
    if (gt == std::string_view::npos) {
      break;
    }

    const ParsedTag tag = parse_tag(html.substr(lt + 1, gt - lt - 1));
    cursor = gt + 1;
    if (tag.is_ignored || tag.name.empty()) {
      continue;
    }

    if (tag.is_closing) {
      while (stack.size() > 1 && stack.back()->tag != tag.name) {
        stack.pop_back();
      }
      if (stack.size() > 1 && stack.back()->tag == tag.name) {
        stack.pop_back();
      }
      continue;
    }

    if (tag.name == "title") {
      const std::size_t close_pos = html.find("</title>", cursor);
      if (close_pos != std::string_view::npos) {
        const std::string decoded = decode_basic_entities(html.substr(cursor, close_pos - cursor));
        const std::string cleaned = collapse_html_whitespace(decoded);
        if (!cleaned.empty()) {
          doc.title = cleaned;
        }
        const std::size_t close_gt = html.find('>', close_pos);
        cursor = (close_gt == std::string_view::npos) ? close_pos : close_gt + 1;
      }
      continue;
    }

    if (tag.name == "script" || tag.name == "style") {
      const std::string close_tag = "</" + tag.name + ">";
      const std::size_t close_pos = html.find(close_tag, cursor);
      if (close_pos == std::string_view::npos) {
        break;
      }

      if (tag.name == "style" && close_pos > cursor) {
        parse_css_rules(html.substr(cursor, close_pos - cursor), doc, MediaConstraint{});
      }
      if (tag.name == "script" && close_pos > cursor) {
        doc.scripts.push_back(HtmlScript{
            .attributes = tag.attributes,
            .content = std::string(html.substr(cursor, close_pos - cursor)),
        });
      }

      const std::size_t close_gt = html.find('>', close_pos);
      cursor = (close_gt == std::string_view::npos) ? close_pos : close_gt + 1;
      continue;
    }

    HtmlNode node = make_element_node(tag.name);
    node.attributes = tag.attributes;
    stack.back()->children.push_back(std::move(node));
    HtmlNode* inserted = &stack.back()->children.back();

    if (!tag.is_self_closing && !is_void_tag(tag.name)) {
      stack.push_back(inserted);
    }
  }

  return doc;
}

std::string flatten_text_content(const HtmlDocument& document) {
  std::string out;
  out.reserve(1024);
  collect_text_recursive(document.root, out);
  return out;
}

const HtmlAttribute* find_attribute(const HtmlNode& node, const std::string_view name) noexcept {
  for (const auto& attr : node.attributes) {
    if (attr.name == name) {
      return &attr;
    }
  }
  return nullptr;
}

} // namespace pluss::view::window
