#include <pluss/tils/doc.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int failures = 0;

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "core doc test failed: " << message << '\n';
    ++failures;
  }
}

void write(const std::filesystem::path& path, const std::string& text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << text;
}

// A type whose name is the name of a documented type.
namespace fake {
struct Series {};
template <class T>
struct DataFrame {};
}  // namespace fake

}  // namespace

int main() {
  namespace fs = std::filesystem;
  using namespace pluss::tils;

  // Pages are read from a folder laid out like the sources, so the test needs no network.
  const fs::path docs = fs::temp_directory_path() / "pluss_doc_test";
  std::error_code ignored;
  fs::remove_all(docs, ignored);
  write(docs / "PlussQit/docs/sphinx/api/series/sum.md",
        "# Series::sum\n\n[Back to Series API Reference](../../series_api_reference.md)\n\n```{raw} html\n<meta name=\"x\" content=\"y\">\n```\n\n"
        "Adds up the elements.\n\n\n\n```cpp\nauto s = x.sum();\n```\n");
  write(docs / "PlussQit/docs/sphinx/series_api_reference.md", "# Series API Reference\n\nThe tensor type.\n");
  write(docs / "PlussMic/docs/sphinx/api/dataframe/show.md", "# DataFrame::show\n\nShows a frame.\n");
#if defined(_WIN32)
  _putenv_s("PLUSS_DOCS_DIR", docs.string().c_str());
#else
  setenv("PLUSS_DOCS_DIR", docs.c_str(), 1);
#endif

  // Finding pages by name: exact, case-insensitive, dotted, and a type by its bare name.
  const auto sum = doc_find("Series::sum");
  require(sum && sum->title == "Series::sum" && sum->slug == "qit/api/series/sum", "a method page is found by its title");
  require(sum && sum->url == "https://plussible.org/docs/qit/api/series/sum", "the website address is the page's slug");
  require(doc_find("series.SUM") && doc_find("series.SUM")->slug == sum->slug, "case and '.' do not matter");
  const auto type = doc_find("Series");
  require(type && type->title == "Series API Reference", "a bare type name finds the type's own page");
  require(!doc_find("Series::nonexistentmember"), "an unknown name finds nothing");
  require(!doc_find(""), "an empty name finds nothing");

  // Reading a page: the site's navigation and raw HTML are removed, blank runs collapsed.
  std::ostringstream printed;
  doc("Series::sum", printed);
  const std::string text = printed.str();
  require(text.find("Series::sum  -  https://plussible.org/docs/qit/api/series/sum") == 0, "the page starts with its title and address");
  require(text.find("Adds up the elements.") != std::string::npos && text.find("auto s = x.sum();") != std::string::npos, "the body and its code are printed");
  require(text.find("Back to") == std::string::npos && text.find("<meta") == std::string::npos, "navigation and raw HTML are removed");
  require(text.find("\n\n\n") == std::string::npos, "runs of blank lines are collapsed");

  // From a value or a type: the name is the type's last name.
  std::ostringstream from_value;
  doc(fake::Series{}, "sum", from_value);
  require(from_value.str().find("Adds up the elements.") != std::string::npos, "doc(value, member) reads Type::member");
  std::ostringstream from_type;
  doc<fake::Series>("sum", from_type);
  require(from_type.str() == from_value.str(), "doc<Type>(member) is the same page");
  std::ostringstream whole;
  doc(fake::Series{}, {}, whole);
  require(whole.str().find("The tensor type.") != std::string::npos, "doc(value) reads the type's page");
  std::ostringstream templated;
  doc(fake::DataFrame<int>{}, "show", templated);
  require(templated.str().find("Shows a frame.") != std::string::npos, "template arguments are not part of the name");
  require(doc_detail::last_name("const pluss::table::DataFrame &") == "DataFrame", "qualifiers and references are dropped");
  require(doc_detail::last_name("pluss::codem::Series") == "Series", "namespaces are dropped");

  // A link: the file with :1, and the web address.
  std::ostringstream link;
  doc_link("Series::sum", link);
  require(link.str().find("sum.md:1") != std::string::npos && link.str().find("web:  https://plussible.org/docs/qit/api/series/sum") != std::string::npos,
          "doc_link prints the file with a line and the web address");

  // A name that matches nothing lists the closest pages.
  std::ostringstream near;
  doc("Series::sun", near);
  require(near.str().find("No documentation page is called 'Series::sun'") != std::string::npos && near.str().find("Series::sum") != std::string::npos,
          "a near miss lists the page that was meant");
  require(!doc_search("sum", 5).empty() && doc_search("sum", 3).size() <= 3, "doc_search returns at most the limit");

  fs::remove_all(docs, ignored);
  if (failures == 0) std::cout << "core doc tests passed\n";
  return failures == 0 ? 0 : 1;
}
