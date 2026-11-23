#include "pluss/tils/doc.h"

#include "pluss/detail/inflate.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <array>
#include <map>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

namespace pluss::tils {
namespace {

struct Row {
  const char* title;
  const char* slug;
  const char* source;
};

// One row per page of the documentation, written by tools/gen_doc_index.py when the library is configured.
const Row kRows[] = {
#include "doc_index.inc"
    {"", "", ""},  // keeps the array non-empty when no documentation was found at configure time
};
constexpr std::size_t kRowCount = sizeof(kRows) / sizeof(kRows[0]) - 1;

constexpr const char* kSite = "https://plussible.org";

std::string lower(std::string_view text) {
  std::string out(text);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

// What a name is compared as: lower case, "." read as "::", spaces collapsed.
std::string normal(std::string_view text) {
  std::string out;
  bool space = true;
  for (const char raw : text) {
    const char c = raw == '.' ? ':' : raw;
    if (std::isspace(static_cast<unsigned char>(c)) != 0) {
      if (!space) out += ' ';
      space = true;
      continue;
    }
    space = false;
    out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  while (!out.empty() && out.back() == ' ') out.pop_back();
  // "series.sum" became "series:sum": make every run of colons a pair.
  std::string fixed;
  for (std::size_t i = 0; i < out.size(); ++i) {
    if (out[i] == ':') {
      std::size_t run = 1;
      while (i + run < out.size() && out[i + run] == ':') ++run;
      fixed += "::";
      i += run - 1;
    } else {
      fixed += out[i];
    }
  }
  return fixed;
}

DocPage page_of(std::size_t i) {
  const Row& row = kRows[i];
  return DocPage{row.title, row.slug, row.source, std::string(kSite) + "/docs/" + row.slug};
}

struct Index {
  std::unordered_multimap<std::string, std::size_t> keys;  // normalised name -> row
  std::vector<std::string> titles;                         // normalised titles, by row

  Index() {
    titles.reserve(kRowCount);
    for (std::size_t i = 0; i < kRowCount; ++i) {
      const std::string title = normal(kRows[i].title);
      titles.push_back(title);
      keys.emplace(title, i);
      // "X API Reference" is the page of the type X.
      constexpr std::string_view suffix = " api reference";
      if (title.size() > suffix.size() && title.compare(title.size() - suffix.size(), suffix.size(), suffix) == 0) {
        const std::string bare = title.substr(0, title.size() - suffix.size());
        keys.emplace(bare, i);
        if (const auto at = bare.rfind("::"); at != std::string::npos) keys.emplace(bare.substr(at + 2), i);
      }
      // "pluss::bi::Studio" is also "bi::Studio" and, for a type, "Studio": every ending of two or
      // more names, and the last name alone when it is written as a type (upper case).
      std::size_t at = title.find("::");
      while (at != std::string::npos) {
        const std::string tail = title.substr(at + 2);
        if (tail.find("::") != std::string::npos) keys.emplace(tail, i);
        at = title.find("::", at + 2);
      }
      const auto last = kRows[i].title;
      const std::string_view whole(last);
      const auto cut = whole.rfind("::");
      if (cut != std::string_view::npos && cut + 2 < whole.size() && std::isupper(static_cast<unsigned char>(whole[cut + 2])) != 0) {
        keys.emplace(normal(whole.substr(cut + 2)), i);
      }
    }
  }
};

const Index& index() {
  static const Index instance;
  return instance;
}

std::size_t edit_distance(std::string_view a, std::string_view b) {
  std::vector<std::size_t> row(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j) row[j] = j;
  for (std::size_t i = 1; i <= a.size(); ++i) {
    std::size_t diagonal = row[0];
    row[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const std::size_t above = row[j];
      row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a[i - 1] == b[j - 1] ? 0 : 1)});
      diagonal = above;
    }
  }
  return row[b.size()];
}

fs::path cache_folder() {
#if defined(_WIN32)
  if (const char* base = std::getenv("LOCALAPPDATA")) return fs::path(base) / "pluss" / "cache" / "docs";
#elif defined(__APPLE__)
  if (const char* home = std::getenv("HOME")) return fs::path(home) / "Library" / "Caches" / "pluss" / "docs";
#else
  if (const char* base = std::getenv("XDG_CACHE_HOME"); base != nullptr && *base != '\0') return fs::path(base) / "pluss" / "docs";
  if (const char* home = std::getenv("HOME")) return fs::path(home) / ".cache" / "pluss" / "docs";
#endif
  return fs::temp_directory_path() / "pluss" / "docs";
}

// Source paths come from the generated index, so they are plain; this keeps them out of a shell
// command should that ever change.
bool plain_path(std::string_view text) {
  return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) {
    return std::isalnum(c) != 0 || c == '_' || c == '-' || c == '.' || c == '/';
  });
}

// The documentation bundled in the download: one file, written by tools/bundle_docs.py.
struct Packed {
  std::uint64_t offset{0};
  std::uint32_t packed{0};
  std::uint32_t size{0};
};

struct Pack {
  fs::path file;
  std::unordered_map<std::string, Packed> pages;
};

std::string prefix_of(std::string_view prefix) {
  if (!prefix.empty()) return std::string(prefix);
  if (const char* env = std::getenv("PLUSS_PREFIX")) return env;
  return {};
}

const Pack* pack_in(std::string_view prefix_text) {
  static std::mutex lock;
  static std::map<std::string, Pack> loaded;
  const std::string prefix = prefix_of(prefix_text);
  if (prefix.empty()) return nullptr;
  const std::lock_guard<std::mutex> guard(lock);
  const auto found = loaded.find(prefix);
  if (found != loaded.end()) return found->second.file.empty() ? nullptr : &found->second;
  Pack pack;
  const fs::path file = fs::path(prefix) / "share" / "pluss" / "docs" / "docs.pack";
  std::ifstream in(file, std::ios::binary);
  char magic[8];
  std::uint32_t count = 0;
  if (in && in.read(magic, 8) && std::string_view(magic, 8) == "PLDOCPK1" && in.read(reinterpret_cast<char*>(&count), 4)) {
    pack.file = file;
    for (std::uint32_t i = 0; i < count; ++i) {
      std::uint16_t length = 0;
      in.read(reinterpret_cast<char*>(&length), 2);
      std::string path(length, '\0');
      in.read(path.data(), length);
      Packed entry;
      in.read(reinterpret_cast<char*>(&entry.offset), 8);
      in.read(reinterpret_cast<char*>(&entry.packed), 4);
      in.read(reinterpret_cast<char*>(&entry.size), 4);
      if (!in) {
        pack.file.clear();
        break;
      }
      pack.pages.emplace(std::move(path), entry);
    }
  }
  const auto stored = loaded.emplace(prefix, std::move(pack)).first;
  return stored->second.file.empty() ? nullptr : &stored->second;
}

// A page from the bundle, or nothing.
std::optional<std::string> bundled(const DocPage& page, std::string_view prefix) {
  const Pack* pack = pack_in(prefix);
  if (pack == nullptr) return std::nullopt;
  const auto entry = pack->pages.find(page.source);
  if (entry == pack->pages.end()) return std::nullopt;
  std::ifstream in(pack->file, std::ios::binary);
  in.seekg(static_cast<std::streamoff>(entry->second.offset));
  std::string packed(entry->second.packed, '\0');
  if (!in.read(packed.data(), static_cast<std::streamsize>(packed.size()))) return std::nullopt;
  try {
    return pluss::detail::Inflater(packed, entry->second.size).run();
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

std::optional<std::string> read_text(const fs::path& file) {
  std::error_code ignored;
  if (!fs::is_regular_file(file, ignored)) return std::nullopt;
  std::ifstream in(file, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// A page downloaded from the website into the cache; the cached file, or nothing.
fs::path download(const DocPage& page, std::string* why) {
  std::error_code ignored;
  const fs::path cached = cache_folder() / page.source;
  if (!plain_path(page.source)) {
    if (why != nullptr) *why = "the page's path is not usable";
    return {};
  }
  fs::create_directories(cached.parent_path(), ignored);
  const fs::path part = cached.string() + ".part";
  const std::string url = std::string(kSite) + "/docs-content/" + page.source;
#if defined(_WIN32)
  const std::string command = "curl -fsSL --max-time 20 -o \"" + part.string() + "\" \"" + url + "\" >nul 2>nul";
#else
  const std::string command = "curl -fsSL --max-time 20 -o '" + part.string() + "' '" + url + "' >/dev/null 2>&1";
#endif
  if (std::system(command.c_str()) != 0 || !fs::is_regular_file(part, ignored)) {
    fs::remove(part, ignored);
    if (why != nullptr) *why = "it is not in PLUSS_DOCS_DIR, the download's bundled documentation or the cache, and downloading " + url + " failed (is curl installed and the network up?)";
    return {};
  }
  fs::rename(part, cached, ignored);
  return cached;
}

// The page's text from wherever it is found first: PLUSS_DOCS_DIR, the bundle, the cache, the website.
std::optional<std::string> page_text(const DocPage& page, std::string_view prefix, std::string* why) {
  if (const char* folder = std::getenv("PLUSS_DOCS_DIR"); folder != nullptr && *folder != '\0') {
    if (auto text = read_text(fs::path(folder) / page.source)) return text;
  }
  if (auto text = bundled(page, prefix)) return text;
  if (auto text = read_text(cache_folder() / page.source)) return text;
  const fs::path file = download(page, why);
  if (file.empty()) return std::nullopt;
  return read_text(file);
}

// A file holding the page, for an editor to open: a source folder's file, the cache's copy, one
// written from the bundle into the cache, or a download.
fs::path page_file(const DocPage& page, std::string_view prefix, std::string* why) {
  std::error_code ignored;
  if (const char* folder = std::getenv("PLUSS_DOCS_DIR"); folder != nullptr && *folder != '\0') {
    const fs::path local = fs::path(folder) / page.source;
    if (fs::is_regular_file(local, ignored)) return local;
  }
  const fs::path cached = cache_folder() / page.source;
  if (fs::is_regular_file(cached, ignored)) return cached;
  if (auto text = bundled(page, prefix)) {
    fs::create_directories(cached.parent_path(), ignored);
    std::ofstream(cached, std::ios::binary | std::ios::trunc) << *text;
    return cached;
  }
  return download(page, why);
}

// The page as a terminal shows it best: the site's navigation and raw HTML blocks removed, runs of
// blank lines collapsed.
std::string terminal_text(const std::string& markdown) {
  std::istringstream in(markdown);
  std::ostringstream out;
  std::string line;
  bool skipping_raw = false;
  bool in_code = false;
  int blanks = 0;
  bool first_content = true;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.rfind("```{raw}", 0) == 0) {
      skipping_raw = true;
      continue;
    }
    if (skipping_raw) {
      if (line.rfind("```", 0) == 0) skipping_raw = false;
      continue;
    }
    if (line.rfind("```", 0) == 0) in_code = !in_code;
    if (!in_code) {
      const auto first = line.find_first_not_of(" \t");
      if (first != std::string::npos && line[first] == '<' && line.back() == '>') continue;  // an HTML line of the site's layout
    }
    if (line.rfind("<!--", 0) == 0 || line.rfind("<meta", 0) == 0) continue;
    if (line.rfind("[Back to", 0) == 0 || line.rfind("[Back ", 0) == 0) continue;
    if (line.empty()) {
      if (first_content || ++blanks > 1) continue;
    } else {
      blanks = 0;
      first_content = false;
    }
    out << line << '\n';
  }
  return out.str();
}

}  // namespace

namespace doc_detail {

std::string last_name(std::string_view type) {
  std::string text(type);
  // Remove template arguments, leaving the template's own name.
  std::string flat;
  int depth = 0;
  for (const char c : text) {
    if (c == '<') {
      ++depth;
    } else if (c == '>') {
      if (depth > 0) --depth;
    } else if (depth == 0) {
      flat += c;
    }
  }
  // Qualifiers, references and pointers are not part of the name.
  for (const char* word : {"const", "volatile", "struct", "class", "enum"}) {
    std::size_t at;
    const std::string w = word;
    while ((at = flat.find(w)) != std::string::npos) {
      const bool left = at == 0 || !(std::isalnum(static_cast<unsigned char>(flat[at - 1])) != 0 || flat[at - 1] == '_');
      const bool right = at + w.size() >= flat.size() || !(std::isalnum(static_cast<unsigned char>(flat[at + w.size()])) != 0 || flat[at + w.size()] == '_');
      if (left && right) flat.erase(at, w.size());
      else break;
    }
  }
  std::string name;
  for (const char c : flat) {
    if (c == '&' || c == '*' || std::isspace(static_cast<unsigned char>(c)) != 0) continue;
    name += c;
  }
  const auto at = name.rfind("::");
  return at == std::string::npos ? name : name.substr(at + 2);
}

}  // namespace doc_detail

std::optional<DocPage> doc_find(std::string_view name) {
  const std::string wanted = normal(name);
  if (wanted.empty()) return std::nullopt;
  const auto range = index().keys.equal_range(wanted);
  std::vector<std::size_t> hits;
  for (auto it = range.first; it != range.second; ++it) hits.push_back(it->second);
  if (hits.empty()) return std::nullopt;
  std::sort(hits.begin(), hits.end());
  hits.erase(std::unique(hits.begin(), hits.end()), hits.end());
  // The page whose whole title is the name, then the shortest address (the type's own page).
  std::size_t best = hits.front();
  for (const std::size_t i : hits) {
    const bool exact_i = index().titles[i] == wanted;
    const bool exact_best = index().titles[best] == wanted;
    if (exact_i != exact_best ? exact_i : std::string_view(kRows[i].slug).size() < std::string_view(kRows[best].slug).size()) best = i;
  }
  return page_of(best);
}

std::vector<DocPage> doc_search(std::string_view name, std::size_t limit) {
  const std::string wanted = normal(name);
  std::vector<std::pair<std::size_t, std::size_t>> scored;  // (score, row): lower is closer
  for (std::size_t i = 0; i < kRowCount; ++i) {
    const std::string& title = index().titles[i];
    std::size_t score;
    if (title == wanted) score = 0;
    else if (title.find(wanted) != std::string::npos) score = 1 + (title.size() - wanted.size());
    else {
      const auto at = title.rfind("::");
      const std::string tail = at == std::string::npos ? title : title.substr(at + 2);
      const std::string head = at == std::string::npos ? std::string() : title.substr(0, at);
      const auto wat = wanted.rfind("::");
      const std::string wanted_tail = wat == std::string::npos ? wanted : wanted.substr(wat + 2);
      const std::string wanted_head = wat == std::string::npos ? std::string() : wanted.substr(0, wat);
      // "Series::sun" means a page of Series: the part before the last "::" must match the page's own.
      if (!wanted_head.empty() && !(head == wanted_head || (head.size() > wanted_head.size() && head.compare(head.size() - wanted_head.size(), wanted_head.size(), wanted_head) == 0 &&
                                                                  head[head.size() - wanted_head.size() - 1] == ':'))) {
        continue;
      }
      const std::size_t distance = edit_distance(wanted_tail, tail);
      if (distance > std::max<std::size_t>(2, wanted_tail.size() / 3)) continue;
      score = 1000 + distance * 10 + tail.size();
    }
    scored.emplace_back(score, i);
  }
  std::sort(scored.begin(), scored.end());
  std::vector<DocPage> out;
  for (std::size_t k = 0; k < scored.size() && out.size() < limit; ++k) out.push_back(page_of(scored[k].second));
  return out;
}

namespace doc_detail {

std::string text_in(const DocPage& page, std::string* why, std::string_view prefix) {
  const auto raw = page_text(page, prefix, why);
  return raw ? terminal_text(*raw) : std::string();
}

std::string file_in(const DocPage& page, std::string* why, std::string_view prefix) {
  const fs::path file = page_file(page, prefix, why);
  return file.empty() ? std::string() : fs::absolute(file).string() + ":1";
}

}  // namespace doc_detail

namespace {

void not_found(std::string_view name, std::ostream& out) {
  out << "No documentation page is called '" << name << "'.\n";
  const auto near = doc_search(name, 6);
  if (near.empty()) {
    out << "Try a type or a member, for example doc(\"Series::sum\") or doc(\"DataFrame\").\n";
    return;
  }
  out << "The closest pages:\n";
  for (const auto& page : near) out << "  " << page.title << "\n";
}

}  // namespace

namespace doc_detail {

void print_in(std::string_view name, std::ostream& out, std::string_view prefix) {
  const auto page = doc_find(name);
  if (!page) {
    not_found(name, out);
    return;
  }
  out << page->title << "  -  " << page->url << "\n\n";
  std::string why;
  const std::string text = text_in(*page, &why, prefix);
  if (text.empty()) {
    out << "(The page could not be read: " << why << ".)\nOpen " << page->url << " instead.\n";
    return;
  }
  out << text;
}

void link_in(std::string_view name, std::ostream& out, std::string_view prefix) {
  const auto page = doc_find(name);
  if (!page) {
    not_found(name, out);
    return;
  }
  std::string why;
  const std::string file = file_in(*page, &why, prefix);
  out << page->title << "\n";
  if (!file.empty()) out << "  file: " << file << "\n";
  else out << "  file: (not available: " << why << ")\n";
  out << "  web:  " << page->url << "\n";
}

bool open_in(std::string_view name, std::ostream& out, std::string_view prefix) {
  const auto page = doc_find(name);
  if (!page) {
    not_found(name, out);
    return false;
  }
  std::string why;
  std::string file = file_in(*page, &why, prefix);
  out << page->title << "\n";
  if (file.empty()) {
    out << "  web:  " << page->url << "\n  (the file could not be fetched: " << why << ")\n";
    return false;
  }
  out << "  file: " << file << "\n  web:  " << page->url << "\n";
  file.resize(file.size() - 2);  // the ":1"
  if (file.find('\'') != std::string::npos || file.find('"') != std::string::npos) return false;  // cannot be quoted safely
  const char* terminal = std::getenv("TERM_PROGRAM");
  std::string command;
#if defined(_WIN32)
  command = (terminal != nullptr && std::string_view(terminal) == "vscode") ? "code -r \"" + file + "\"" : "start \"\" \"" + file + "\"";
#elif defined(__APPLE__)
  command = (terminal != nullptr && std::string_view(terminal) == "vscode") ? "code -r '" + file + "' >/dev/null 2>&1 || open '" + file + "'" : "open '" + file + "'";
#else
  command = (terminal != nullptr && std::string_view(terminal) == "vscode") ? "code -r '" + file + "' >/dev/null 2>&1 || xdg-open '" + file + "' >/dev/null 2>&1" : "xdg-open '" + file + "' >/dev/null 2>&1";
#endif
  return std::system(command.c_str()) == 0;
}

}  // namespace doc_detail

}  // namespace pluss::tils
