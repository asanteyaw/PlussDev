#pragma once

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Deliberately minimal JSON helpers for PlussBI's live-filter wire format.
//
// This is not a general JSON library. It supports exactly what the
// dashboard's client-side fetch() calls need: escaping a string for
// embedding in a JSON value, building a flat (single-level, string-valued)
// JSON object as a response, and parsing a flat (single-level,
// string-valued) JSON object out of a request body. Nested objects, arrays,
// numbers, and booleans are intentionally out of scope -- every value the
// live-filter protocol sends is a string (control ids, filter kinds, and
// filter bound text are all strings; the HTML fragments the server sends
// back are strings too). If PlussBI ever needs a richer wire format, reach
// for a real JSON library instead of growing this one.

namespace pluss::bi_tools {

// Escapes `text` for safe embedding inside a JSON string literal (handles
// the mandatory escapes: quote, backslash, and the C0 control characters).
[[nodiscard]] inline std::string json_escape(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (const unsigned char c : text) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          static const char* kHex = "0123456789abcdef";
          out += "\\u00";
          out += kHex[(c >> 4) & 0xF];
          out += kHex[c & 0xF];
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out;
}

// Builds `{"k1":"v1","k2":"v2",...}` from an ordered list of key/value
// pairs, escaping every key and value.
[[nodiscard]] inline std::string json_object(
    const std::vector<std::pair<std::string, std::string>>& fields) {
  std::string out = "{";
  bool first = true;
  for (const auto& [key, value] : fields) {
    if (!first) out += ',';
    first = false;
    out += '"';
    out += json_escape(key);
    out += "\":\"";
    out += json_escape(value);
    out += '"';
  }
  out += '}';
  return out;
}

// Parses a flat JSON object -- `{"k1": "v1", "k2": "v2", ...}`, string keys
// and string values only, no nesting -- into a key/value map. Malformed
// input yields whatever prefix was successfully parsed rather than
// throwing; callers should treat a missing expected key as "absent" the
// same way they would treat malformed input, since both mean the request
// did not carry a usable value.
[[nodiscard]] inline std::map<std::string, std::string> parse_flat_json_object(
    std::string_view text) {
  std::map<std::string, std::string> out;

  auto skip_ws = [&](std::size_t i) {
    while (i < text.size() &&
           (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r')) {
      ++i;
    }
    return i;
  };

  auto parse_string = [&](std::size_t i, std::string* value) -> std::size_t {
    if (i >= text.size() || text[i] != '"') return std::string_view::npos;
    ++i;
    std::string result;
    while (i < text.size() && text[i] != '"') {
      if (text[i] == '\\' && i + 1 < text.size()) {
        const char next = text[i + 1];
        switch (next) {
          case '"': result += '"'; break;
          case '\\': result += '\\'; break;
          case '/': result += '/'; break;
          case 'n': result += '\n'; break;
          case 'r': result += '\r'; break;
          case 't': result += '\t'; break;
          default: result += next; break;
        }
        i += 2;
        continue;
      }
      result += text[i];
      ++i;
    }
    if (i >= text.size()) return std::string_view::npos; // unterminated string
    *value = std::move(result);
    return i + 1;
  };

  std::size_t i = skip_ws(0);
  if (i >= text.size() || text[i] != '{') return out;
  ++i;
  i = skip_ws(i);
  if (i < text.size() && text[i] == '}') return out; // empty object

  while (i < text.size()) {
    i = skip_ws(i);
    std::string key;
    i = parse_string(i, &key);
    if (i == std::string_view::npos) break;
    i = skip_ws(i);
    if (i >= text.size() || text[i] != ':') break;
    ++i;
    i = skip_ws(i);
    std::string value;
    i = parse_string(i, &value);
    if (i == std::string_view::npos) break;
    out[key] = std::move(value);
    i = skip_ws(i);
    if (i < text.size() && text[i] == ',') {
      ++i;
      continue;
    }
    break;
  }
  return out;
}

} // namespace pluss::bi_tools
