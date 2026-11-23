#pragma once

// Read the documentation of anything in PlussII from the program that uses it, as help() does
// in other languages:
//
//   pluss::tils::doc(frame);               // the page of the type of `frame`, in the terminal
//   pluss::tils::doc(frame, "show");       // the page of DataFrame::show
//   pluss::tils::doc("Series::sum");       // a page by name
//   pluss::tils::doc_link("Series::sum");  // where the page is: a file the editor opens, and the website address
//   pluss::tils::doc_open("Series::sum");  // open that file in the editor
//
// The names are the titles of the pages on the website. "Series.sum" and "series::sum" find the same
// page. A name that matches nothing lists the closest pages.
//
// A page is read from PLUSS_DOCS_DIR (a folder laid out like the documentation sources) when that is
// set, otherwise from a copy kept in the user's cache folder, otherwise it is downloaded from the
// website once with curl and kept.

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pluss::tils {

struct DocPage {
  std::string title;   // "Series::sum"
  std::string slug;    // "qit/api/series/sum"
  std::string source;  // "PlussQit/docs/sphinx/api/series/sum.md"
  std::string url;     // the page on the website
};

// The page called `name`, if there is exactly one best match.
[[nodiscard]] std::optional<DocPage> doc_find(std::string_view name);

// The pages whose titles are closest to `name`, best first.
[[nodiscard]] std::vector<DocPage> doc_search(std::string_view name, std::size_t limit = 8);

// The page as plain text for a terminal. Empty when it cannot be read; `why` then says why.
[[nodiscard]] std::string doc_text(const DocPage& page, std::string* why = nullptr);

// Where the page's file is on this machine (fetching it first if needed): path:1, which most
// editors' terminals turn into a link that opens the file.
[[nodiscard]] std::string doc_file(const DocPage& page, std::string* why = nullptr);

void doc(std::string_view name, std::ostream& out = std::cout);
void doc_link(std::string_view name, std::ostream& out = std::cout);
bool doc_open(std::string_view name, std::ostream& out = std::cout);

namespace doc_detail {

// "pluss::codem::Series const&" -> "Series": the last name of a type, without namespaces,
// template arguments, qualifiers or references.
[[nodiscard]] std::string last_name(std::string_view type);

template <class T>
[[nodiscard]] std::string type_text() {
#if defined(_MSC_VER)
  std::string_view text = __FUNCSIG__;
  const auto begin = text.find("type_text<") + 10;
  const auto end = text.rfind(">(void)");
  return std::string(text.substr(begin, end - begin));
#else
  std::string_view text = __PRETTY_FUNCTION__;
  const auto begin = text.find("T = ") + 4;
  auto end = text.find_first_of(";]", begin);
  return std::string(text.substr(begin, end - begin));
#endif
}

template <class T>
[[nodiscard]] std::string name_of(std::string_view member) {
  std::string name = last_name(type_text<std::remove_cv_t<std::remove_reference_t<T>>>());
  if (!member.empty()) name += "::" + std::string(member);
  return name;
}

}  // namespace doc_detail

// The documentation of a value's type, or of one of its members.
template <class T>
  requires(!std::is_convertible_v<const T&, std::string_view>)
void doc(const T&, std::string_view member = {}, std::ostream& out = std::cout) {
  doc(doc_detail::name_of<T>(member), out);
}

template <class T>
  requires(!std::is_convertible_v<const T&, std::string_view>)
void doc_link(const T&, std::string_view member = {}, std::ostream& out = std::cout) {
  doc_link(doc_detail::name_of<T>(member), out);
}

template <class T>
  requires(!std::is_convertible_v<const T&, std::string_view>)
bool doc_open(const T&, std::string_view member = {}, std::ostream& out = std::cout) {
  return doc_open(doc_detail::name_of<T>(member), out);
}

// Without a value: pluss::tils::doc<pluss::Series>("sum").
template <class T>
void doc(std::string_view member = {}, std::ostream& out = std::cout) {
  doc(doc_detail::name_of<T>(member), out);
}

}  // namespace pluss::tils
