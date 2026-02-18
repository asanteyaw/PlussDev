#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/coden.h>

using pluss::coden::Note;

TEST_CASE("CodeN Note factories support 3D lvalue/rvalue construction", "[coden][patch12]") {
  const std::array<std::int64_t, 3> sizes{2, 2, 2};

  std::vector<std::string> lvalue_flat = {"a", "b", "c", "d", "e", "f", "g", "h"};
  Note lvalue_note = pluss::make_note(lvalue_flat, std::span<const std::int64_t>(sizes.data(), sizes.size()));
  REQUIRE(lvalue_note.defined());
  REQUIRE(lvalue_note.dim() == 3);
  REQUIRE(lvalue_note.numel() == 8);
  REQUIRE(lvalue_note.sizes() == std::vector<std::int64_t>({2, 2, 2}));
  REQUIRE(lvalue_note.at(0) == "a");
  REQUIRE(lvalue_note.at(7) == "h");

  Note rvalue_note = pluss::content(
      std::vector<std::string>{"0", "1", "2", "3", "4", "5", "6", "7"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));
  REQUIRE(rvalue_note.defined());
  REQUIRE(rvalue_note.dim() == 3);
  REQUIRE(rvalue_note.numel() == 8);
  REQUIRE(rvalue_note.sizes() == std::vector<std::int64_t>({2, 2, 2}));
  REQUIRE(rvalue_note.at(0) == "0");
  REQUIRE(rvalue_note.at(7) == "7");
}

TEST_CASE("CodeN NLP remover family resolves and applies over Note batches", "[coden][patch12]") {
  const std::array<std::int64_t, 1> one_d{2};
  std::vector<std::string> docs = {
      "Header\nVisit https://example.com and email dev@example.com @dev #topic <b>Now</b>",
      "- item\n|a|b|\n`inline` and $$x+y$$ plus [link](https://x.y)"
  };
  Note note = pluss::make_note(docs, std::span<const std::int64_t>(one_d.data(), one_d.size()));

  Note cleaned = note
      .nlp_remove_urls()
      .nlp_remove_emails()
      .nlp_remove_mentions()
      .nlp_remove_hashtags()
      .nlp_remove_html()
      .nlp_remove_markdown()
      .nlp_remove_code()
      .nlp_remove_math()
      .nlp_remove_bullets()
      .nlp_remove_tables();

  REQUIRE(cleaned.at(0).find("http") == std::string::npos);
  REQUIRE(cleaned.at(0).find('@') == std::string::npos);
  REQUIRE(cleaned.at(0).find('#') == std::string::npos);
  REQUIRE(cleaned.at(0).find('<') == std::string::npos);
  REQUIRE(cleaned.at(0).find('>') == std::string::npos);
  REQUIRE(cleaned.at(1).find('|') == std::string::npos);
  REQUIRE(cleaned.at(1).find('`') == std::string::npos);
  REQUIRE(cleaned.at(1).find('$') == std::string::npos);
}
