#pragma once

#include <pluss/exception.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <locale>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::tils {

// Where a setting's value came from, lowest precedence first.
enum class Layer : std::uint8_t { Default = 0, File = 1, Env = 2, Args = 3 };

struct Origin {
  Layer layer{Layer::Default};
  std::string source{};  // the file path, the environment variable, or the argument as typed
};

// Run parameters from four layers. A value set later in the list below wins:
//   defaults (set), then a file, then the environment, then the command line.
// Defaults declare the keys, so a key that is not declared is a mistake: with strict mode (the
// default) a file or an argument that names one is an error that lists the keys that exist.
class Settings {
 public:
  // A default. Declare every key before applying any other layer.
  Settings& set(std::string_view key, std::string_view value) {
    entry_(key, /*declare=*/true).value[layer_index_(Layer::Default)] = Slot{std::string(value), "default"};
    return *this;
  }
  Settings& set(std::string_view key, const char* value) { return set(key, std::string_view(value)); }
  Settings& set(std::string_view key, bool value) { return set(key, std::string_view(value ? "true" : "false")); }
  template <class T, class = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>>>
  Settings& set(std::string_view key, T value) {
    return set(key, number_text_(value));
  }

  // A flat JSON object: {"port": 8080, "debug": true, "name": "run"}. A nested object is read as
  // dotted keys ("db.host"). Arrays and null are errors, because a setting is one value.
  Settings& file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    PLUSS_CHECK(in.good(), "Settings::file: cannot open '" + path.string() + "'");
    std::ostringstream text;
    text << in.rdbuf();
    const std::string content = text.str();
    JsonReader reader{content, path.string()};
    for (auto& [key, value] : reader.object()) {
      apply_(Layer::File, key, value, path.string());
    }
    return *this;
  }

  // PREFIX_KEY for every declared key: with prefix "RUN" the key "max_iter" reads RUN_MAX_ITER.
  Settings& env(std::string_view prefix) {
    for (auto& [key, entry] : entries_) {
      const std::string name = variable_name_(prefix, key);
      if (const char* raw = std::getenv(name.c_str()); raw != nullptr) {
        entry.value[layer_index_(Layer::Env)] = Slot{std::string(raw), name};
      }
    }
    return *this;
  }

  // --key=value, --key value, --flag (true), --no-flag (false). "--" ends the options; anything that
  // does not start with "--" is positional and is returned by positional().
  Settings& args(int argc, const char* const* argv) {
    positional_.clear();
    bool options = true;
    for (int i = 1; i < argc; ++i) {
      const std::string token = argv[i];
      if (!options || token.rfind("--", 0) != 0) {
        positional_.push_back(token);
        continue;
      }
      if (token == "--") {
        options = false;
        continue;
      }
      std::string key = token.substr(2);
      std::string value;
      const auto equals = key.find('=');
      if (equals != std::string::npos) {
        value = key.substr(equals + 1);
        key = key.substr(0, equals);
      } else if (key.rfind("no-", 0) == 0 && has_(key.substr(3))) {
        key = key.substr(3);
        value = "false";
      } else if (i + 1 < argc && std::string_view(argv[i + 1]).rfind("--", 0) != 0 && !is_flag_(key)) {
        value = argv[++i];
      } else {
        value = "true";
      }
      apply_(Layer::Args, key, value, token);
    }
    return *this;
  }

  // Strict (the default): a file or argument naming an undeclared key is an error.
  Settings& strict(bool on = true) {
    strict_ = on;
    return *this;
  }

  [[nodiscard]] bool has(std::string_view key) const { return has_(std::string(key)); }

  template <class T>
  [[nodiscard]] T get(std::string_view key) const {
    const auto [text, origin] = resolved_(key);
    return parse_<T>(text, key, origin);
  }

  // The layer and source of the value in force, for a log or a run record.
  [[nodiscard]] Origin origin(std::string_view key) const { return resolved_(key).second; }

  [[nodiscard]] std::vector<std::string> keys() const {
    std::vector<std::string> out;
    for (const auto& item : entries_) out.push_back(item.first);
    return out;
  }

  [[nodiscard]] const std::vector<std::string>& positional() const noexcept { return positional_; }

 private:
  struct Slot {
    std::string text{};
    std::string source{};
  };
  struct Entry {
    std::optional<Slot> value[4]{};
  };

  static constexpr std::size_t layer_index_(Layer layer) noexcept { return static_cast<std::size_t>(layer); }

  [[nodiscard]] bool has_(const std::string& key) const { return entries_.count(key) != 0; }

  // A key counts as a flag when its default is a boolean word, so "--debug" alone means true
  // and does not swallow the next argument.
  [[nodiscard]] bool is_flag_(const std::string& key) const {
    const auto found = entries_.find(key);
    if (found == entries_.end()) return false;
    const auto& base = found->second.value[layer_index_(Layer::Default)];
    return base && (base->text == "true" || base->text == "false");
  }

  Entry& entry_(std::string_view key, bool declare) {
    PLUSS_CHECK(!key.empty(), "Settings: a key cannot be empty");
    if (declare) return entries_[std::string(key)];
    const auto found = entries_.find(std::string(key));
    PLUSS_CHECK(found != entries_.end(), unknown_message_(key), std::string(key), keys());
    return found->second;
  }

  [[nodiscard]] std::string unknown_message_(std::string_view key) const {
    std::string known;
    for (const auto& item : entries_) known += (known.empty() ? "" : ", ") + item.first;
    return "Settings: unknown key '" + std::string(key) + "' (declared keys: " + (known.empty() ? "none" : known) + ")";
  }

  void apply_(Layer layer, const std::string& key, const std::string& value, const std::string& source) {
    if (!has_(key)) {
      if (strict_) PLUSS_FAIL(unknown_message_(key), key, keys());
      entries_[key];  // lenient: the key is created by the layer that names it
    }
    entries_[key].value[layer_index_(layer)] = Slot{value, source};
  }

  [[nodiscard]] std::pair<std::string, Origin> resolved_(std::string_view key) const {
    const auto found = entries_.find(std::string(key));
    PLUSS_CHECK(found != entries_.end(), unknown_message_(key), std::string(key), keys());
    for (int i = 3; i >= 0; --i) {
      if (const auto& slot = found->second.value[i]) {
        return {slot->text, Origin{static_cast<Layer>(i), slot->source}};
      }
    }
    PLUSS_FAIL("Settings: '" + std::string(key) + "' has no value; give it a default with set()");
  }

  [[nodiscard]] static std::string variable_name_(std::string_view prefix, const std::string& key) {
    std::string name(prefix);
    if (!name.empty()) name += '_';
    for (const char ch : key) {
      name += std::isalnum(static_cast<unsigned char>(ch)) != 0 ? static_cast<char>(std::toupper(static_cast<unsigned char>(ch))) : '_';
    }
    return name;
  }

  template <class T>
  [[nodiscard]] static std::string number_text_(T value) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.precision(17);
    if constexpr (std::is_floating_point_v<T>) {
      out << static_cast<double>(value);
    } else {
      out << +value;
    }
    return out.str();
  }

  [[nodiscard]] static std::string trimmed_(std::string_view text) {
    std::size_t first = 0, last = text.size();
    while (first < last && std::isspace(static_cast<unsigned char>(text[first])) != 0) ++first;
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) --last;
    return std::string(text.substr(first, last - first));
  }

  template <class T>
  [[nodiscard]] static T parse_(const std::string& raw, std::string_view key, const Origin& origin) {
    const auto fail = [&](const char* wanted) {
      PLUSS_FAIL("Settings: '" + std::string(key) + "' is '" + raw + "' (from " + origin.source + "), which is not " + wanted);
    };
    const std::string text = trimmed_(raw);
    if constexpr (std::is_same_v<T, std::string>) {
      return raw;
    } else if constexpr (std::is_same_v<T, bool>) {
      std::string lower = text;
      std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") return true;
      if (lower == "0" || lower == "false" || lower == "no" || lower == "off") return false;
      fail("a yes-or-no value (true, false, yes, no, on, off, 1 or 0)");
    } else if constexpr (std::is_integral_v<T>) {
      T value{};
      const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
      if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size()) fail("a whole number");
      return value;
    } else {
      static_assert(std::is_floating_point_v<T>, "Settings::get supports bool, integers, floating point and std::string");
      std::istringstream stream(text);
      stream.imbue(std::locale::classic());
      stream >> std::noskipws;
      double value = 0.0;
      stream >> value;
      if (text.empty() || !stream || !stream.eof() || !std::isfinite(value)) fail("a number");
      return static_cast<T>(value);
    }
  }

  // The small part of JSON a settings file needs: an object whose values are strings, numbers,
  // booleans or objects of those.
  class JsonReader {
   public:
    JsonReader(const std::string& text, std::string name) : text_(text), name_(std::move(name)) {}

    std::vector<std::pair<std::string, std::string>> object() {
      std::vector<std::pair<std::string, std::string>> out;
      skip_();
      read_object_("", out);
      skip_();
      PLUSS_CHECK(at_ == text_.size(), fail_("unexpected text after the object"));
      return out;
    }

   private:
    [[nodiscard]] std::string fail_(const std::string& what) const {
      return "Settings::file: " + name_ + ": " + what + " (at character " + std::to_string(at_) + ")";
    }
    void skip_() {
      while (at_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[at_])) != 0) ++at_;
    }
    void expect_(char ch) {
      skip_();
      PLUSS_CHECK(at_ < text_.size() && text_[at_] == ch, fail_(std::string("expected '") + ch + "'"));
      ++at_;
    }
    void read_object_(const std::string& prefix, std::vector<std::pair<std::string, std::string>>& out) {
      expect_('{');
      skip_();
      if (at_ < text_.size() && text_[at_] == '}') {
        ++at_;
        return;
      }
      for (;;) {
        skip_();
        const std::string key = read_string_();
        expect_(':');
        skip_();
        const std::string full = prefix.empty() ? key : prefix + "." + key;
        PLUSS_CHECK(at_ < text_.size(), fail_("a value is missing"));
        const char ch = text_[at_];
        if (ch == '{') {
          read_object_(full, out);
        } else if (ch == '"') {
          out.emplace_back(full, read_string_());
        } else if (ch == '[') {
          PLUSS_FAIL(fail_("'" + full + "' is an array; a setting is one value"));
        } else {
          const std::size_t start = at_;
          while (at_ < text_.size() && text_[at_] != ',' && text_[at_] != '}' && std::isspace(static_cast<unsigned char>(text_[at_])) == 0) ++at_;
          const std::string word = text_.substr(start, at_ - start);
          PLUSS_CHECK(!word.empty() && word != "null", fail_("'" + full + "' has no value"));
          out.emplace_back(full, word);
        }
        skip_();
        if (at_ < text_.size() && text_[at_] == ',') {
          ++at_;
          continue;
        }
        expect_('}');
        return;
      }
    }
    std::string read_string_() {
      skip_();
      PLUSS_CHECK(at_ < text_.size() && text_[at_] == '"', fail_("expected a quoted key or value"));
      ++at_;
      std::string out;
      while (at_ < text_.size() && text_[at_] != '"') {
        char ch = text_[at_++];
        if (ch == '\\') {
          PLUSS_CHECK(at_ < text_.size(), fail_("an escape is cut off"));
          const char esc = text_[at_++];
          switch (esc) {
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            case 'r': out += '\r'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case '"': case '\\': case '/': out += esc; break;
            case 'u': {
              PLUSS_CHECK(at_ + 4 <= text_.size(), fail_("a \\u escape is cut off"));
              unsigned code = static_cast<unsigned>(std::strtoul(text_.substr(at_, 4).c_str(), nullptr, 16));
              at_ += 4;
              if (code < 0x80) {
                out += static_cast<char>(code);
              } else if (code < 0x800) {
                out += static_cast<char>(0xC0 | (code >> 6));
                out += static_cast<char>(0x80 | (code & 0x3F));
              } else {
                out += static_cast<char>(0xE0 | (code >> 12));
                out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (code & 0x3F));
              }
              break;
            }
            default: PLUSS_FAIL(fail_(std::string("unknown escape '\\") + esc + "'"));
          }
        } else {
          out += ch;
        }
      }
      PLUSS_CHECK(at_ < text_.size(), fail_("a string is not closed"));
      ++at_;
      return out;
    }

    const std::string& text_;
    std::string name_;
    std::size_t at_{0};
  };

  std::map<std::string, Entry> entries_{};
  std::vector<std::string> positional_{};
  bool strict_{true};
};

}  // namespace pluss::tils
