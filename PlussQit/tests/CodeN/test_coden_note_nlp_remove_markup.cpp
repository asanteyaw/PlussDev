#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

namespace {

std::vector<std::string> text(const pluss::Note& note) {
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(note.numel()));
  for (std::int64_t i = 0; i < note.numel(); ++i) {
    out.emplace_back(note.at(i));
  }
  return out;
}

} // namespace

TEST_CASE("Note email removal recognizes addresses without consuming prose",
          "[coden][nlp_remove_markup]") {
  const auto source = pluss::note::make({
      "Write a+b@example.co.uk or x.y@sub.example.io today",
      "  keep a@b, @handle, and x@@example.com\nexactly  ",
  });
  REQUIRE(text(source.nlp().remove_emails()) ==
          std::vector<std::string>{
              "Write or today",
              "  keep a@b, @handle, and x@@example.com\nexactly  ",
          });
}

TEST_CASE("Note footer removal is line-exact and reaches every footer form",
          "[coden][nlp_remove_markup]") {
  const auto source = pluss::note::make({
      "Body line\nPage 3 of 9\nConfidential - internal\n"
      "© 2026 Pluss\nCopyright 2026 Pluss\nAll rights reserved\nClosing",
      "  page three\nconfidentiality matters  ",
  });
  REQUIRE(text(source.nlp().remove_footers()) ==
          std::vector<std::string>{
              "Body line Closing",
              "  page three\nconfidentiality matters  ",
          });
}

TEST_CASE("Note hashtag removal honors token boundaries",
          "[coden][nlp_remove_markup]") {
  const auto source = pluss::note::make({
      "Ship #release_2 with (#team), [#ops]; keep C# mid#word and #",
      "  C#\nmid#word stays  ",
  });
  REQUIRE(text(source.nlp().remove_hashtags()) ==
          std::vector<std::string>{
              "Ship with (), []; keep C# mid#word and #",
              "  C#\nmid#word stays  ",
          });
}

TEST_CASE("Note header removal distinguishes headings from hash-prefixed text",
          "[coden][nlp_remove_markup]") {
  const auto source = pluss::note::make({
      "# Title\nBody\n  ## Section\nText\n###### Six\n"
      "####### Seven\n#include <x>\n#topic",
      "  #topic\n#include <vector>  ",
  });
  REQUIRE(text(source.nlp().remove_headers()) ==
          std::vector<std::string>{
              "Body Text ####### Seven #include <x> #topic",
              "  #topic\n#include <vector>  ",
          });
}

TEST_CASE("Note HTML removal recognizes markup but preserves comparisons",
          "[coden][nlp_remove_markup]") {
  const auto source = pluss::note::make({
      "<p>Hello <strong>world</strong>.</p><!-- secret --><br/>Tail &amp; more",
      "<!DOCTYPE html><x-card data-id=\"7\">Value</x-card>",
      "<p>One</p><p>Two</p>",
      "  3 < 5 and x < y > z\nstill text  ",
  });
  REQUIRE(text(source.nlp().remove_html()) ==
          std::vector<std::string>{
              "Hello world. Tail &amp; more",
              "Value",
              "One Two",
              "  3 < 5 and x < y > z\nstill text  ",
          });
}

TEST_CASE("Note markup removers preserve shape absence and Text dtype",
          "[coden][nlp_remove_markup]") {
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"a@b.io", "Page 1", "#tag", "<b>bold</b>"},
      std::array<std::int64_t, 2>{2, 2});
  REQUIRE(matrix.nlp().remove_emails().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().remove_footers().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().remove_hashtags().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().remove_headers().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().remove_html().sizes() ==
          std::vector<std::int64_t>{2, 2});

  const auto once = matrix.nlp().remove_html();
  REQUIRE(text(once.nlp().remove_html()) == text(once));

  const auto empty = pluss::note::make(std::vector<std::string>{});
  REQUIRE(empty.nlp().remove_emails().sizes() == std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().remove_footers().sizes() == std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().remove_hashtags().sizes() == std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().remove_headers().sizes() == std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().remove_html().sizes() == std::vector<std::int64_t>{0});

  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_emails().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_footers().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_hashtags().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_headers().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_html().defined());

  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"<b>x</b>"},
      std::array<std::int64_t, 1>{1});
  REQUIRE_THROWS(bytes.nlp().remove_emails());
  REQUIRE_THROWS(bytes.nlp().remove_footers());
  REQUIRE_THROWS(bytes.nlp().remove_hashtags());
  REQUIRE_THROWS(bytes.nlp().remove_headers());
  REQUIRE_THROWS(bytes.nlp().remove_html());
  REQUIRE_THROWS(pluss::note::make({true}).nlp().remove_html());
  REQUIRE_THROWS(pluss::note::make(std::vector<std::int64_t>{1})
                     .nlp().remove_html());
  REQUIRE_THROWS(pluss::note::make(std::vector<double>{1.0})
                     .nlp().remove_html());
}
