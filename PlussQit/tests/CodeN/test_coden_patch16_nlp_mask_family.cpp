#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("CodeN NLP mask family is concrete", "[coden][patch16]") {
  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note n = pluss::make_note(
      std::vector<std::string>{
          "email bob@example.com url https://x.y @alice #topic 2025-01-03 09:30 $12.50 id 42",
          "ping jane@acme.org at 7pm on Jan 2, 2026 for GBP 99"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  auto out = n.nlp_mask_pii();
  const std::string s0 = out.at(0);
  REQUIRE(s0.find("[EMAIL]") != std::string::npos);
  REQUIRE(s0.find("[URL]") != std::string::npos);
  REQUIRE(s0.find("[MENTION]") != std::string::npos);
  REQUIRE(s0.find("[HASHTAG]") != std::string::npos);
  REQUIRE(s0.find("[DATE]") != std::string::npos);
  REQUIRE(s0.find("[TIME]") != std::string::npos);
  REQUIRE(s0.find("[CUR]") != std::string::npos);
  REQUIRE(s0.find("[NUM]") != std::string::npos);

  const std::string s1 = out.at(1);
  REQUIRE(s1.find("[EMAIL]") != std::string::npos);
  REQUIRE(s1.find("[TIME]") != std::string::npos);
  REQUIRE(s1.find("[DATE]") != std::string::npos);
  REQUIRE(s1.find("[CUR]") != std::string::npos);
}

TEST_CASE("CodeN nlp_mask_pii flags select subsets", "[coden][patch16]") {
  pluss::Note n = pluss::make_note("contact bob@example.com and use code 42");
  auto numbers_only = n.nlp_mask_pii(1);
  const std::string s = numbers_only.at(0);
  REQUIRE(s.find("bob@example.com") != std::string::npos);
  REQUIRE(s.find("[NUM]") != std::string::npos);
  REQUIRE(s.find("[EMAIL]") == std::string::npos);
}
