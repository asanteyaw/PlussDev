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

TEST_CASE("Note NLP maskers recognize their canonical forms",
          "[coden][nlp_mask]") {
  REQUIRE(text(pluss::note::make({
                   "IDs 42, 1,234.50, 2024-01-02, +7 and none"})
                   .nlp().mask_numbers()) ==
          std::vector<std::string>{
              "IDs [NUM], [NUM], [NUM], +[NUM] and none"});

  REQUIRE(text(pluss::note::make({
                   "mail a+b@example.co.uk, x@y.io; keep a@b and @handle"})
                   .nlp().mask_emails()) ==
          std::vector<std::string>{
              "mail [EMAIL], [EMAIL]; keep a@b and @handle"});

  REQUIRE(text(pluss::note::make({
                   "See HTTPS://example.com/a?q=1. Then www.x.io, not ftp://x.io"})
                   .nlp().mask_urls()) ==
          std::vector<std::string>{
              "See [URL]. Then [URL], not ftp://x.io"});

  REQUIRE(text(pluss::note::make({
                   "@root [@alice], x@y.io and mid@name"})
                   .nlp().mask_mentions()) ==
          std::vector<std::string>{
              "[MENTION] [[MENTION]], x@y.io and mid@name"});

  REQUIRE(text(pluss::note::make({
                   "#root (#ship); [#release_2] and C#"})
                   .nlp().mask_hashtags()) ==
          std::vector<std::string>{
              "[HASHTAG] ([HASHTAG]); [[HASHTAG]] and C#"});

  REQUIRE(text(pluss::note::make({
                   "2024-01-02, 12/31/2024, 31.12.2024, January 2, 2024"})
                   .nlp().mask_dates()) ==
          std::vector<std::string>{
              "[DATE], [DATE], [DATE], [DATE]"});

  REQUIRE(text(pluss::note::make({
                   "00:00 23:59 7pm 07:05 AM; keep 24:00 and 19:30pm"})
                   .nlp().mask_times()) ==
          std::vector<std::string>{
              "[TIME] [TIME] [TIME] [TIME]; keep 24:00 and 19:30pm"});

  REQUIRE(text(pluss::note::make({
                   "$12.50, € 20, £9, ¥100, CAD 42 and 30 eur"})
                   .nlp().mask_currency()) ==
          std::vector<std::string>{
              "[CUR], [CUR], [CUR], [CUR], [CUR] and [CUR]"});
}

TEST_CASE("Note NLP maskers preserve boundaries and expose syntax limits",
          "[coden][nlp_mask]") {
  const auto dates = pluss::note::make({
      "keep 2024-13-01 and 2024-01/02; mask Feb 31, 2024"});
  REQUIRE(text(dates.nlp().mask_dates()) ==
          std::vector<std::string>{
              "keep 2024-13-01 and 2024-01/02; mask [DATE]"});

  const auto urls = pluss::note::make({
      "(https://x.io/a), https://x.io/p?q=one&n=2!"});
  REQUIRE(text(urls.nlp().mask_urls()) ==
          std::vector<std::string>{"([URL]), [URL]!"});

  const auto mentions = pluss::note::make({"mail a@b.io; tag:@ann/#go"});
  REQUIRE(text(mentions.nlp().mask_mentions()) ==
          std::vector<std::string>{"mail a@b.io; tag:[MENTION]/#go"});
  REQUIRE(text(mentions.nlp().mask_hashtags()) ==
          std::vector<std::string>{"mail a@b.io; tag:@ann/[HASHTAG]"});

  const auto already = pluss::note::make(
      {"[NUM] [EMAIL] [URL] [MENTION] [HASHTAG] [DATE] [TIME] [CUR]"});
  REQUIRE(text(already.nlp().mask_numbers()) == text(already));
  REQUIRE(text(already.nlp().mask_emails()) == text(already));
  REQUIRE(text(already.nlp().mask_urls()) == text(already));
  REQUIRE(text(already.nlp().mask_mentions()) == text(already));
  REQUIRE(text(already.nlp().mask_hashtags()) == text(already));
  REQUIRE(text(already.nlp().mask_dates()) == text(already));
  REQUIRE(text(already.nlp().mask_times()) == text(already));
  REQUIRE(text(already.nlp().mask_currency()) == text(already));
}

TEST_CASE("Note NLP maskers preserve structure and absence",
          "[coden][nlp_mask]") {
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"$1", "a@b.io", "7pm", "#tag"},
      std::array<std::int64_t, 2>{2, 2});
  const auto masked = matrix.nlp().mask_currency();
  REQUIRE(masked.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(text(masked) ==
          std::vector<std::string>{"[CUR]", "a@b.io", "7pm", "#tag"});

  const auto empty = pluss::note::make(std::vector<std::string>{});
  REQUIRE(empty.nlp().mask_numbers().sizes() ==
          std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().mask_numbers().empty());

  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_numbers().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_emails().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_urls().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_mentions().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_hashtags().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_dates().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_times().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_currency().defined());
}

TEST_CASE("Note NLP maskers require Text dtype", "[coden][nlp_mask]") {
  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"42"}, std::array<std::int64_t, 1>{1});
  REQUIRE_THROWS(bytes.nlp().mask_numbers());
  REQUIRE_THROWS(bytes.nlp().mask_emails());
  REQUIRE_THROWS(bytes.nlp().mask_urls());
  REQUIRE_THROWS(bytes.nlp().mask_mentions());
  REQUIRE_THROWS(bytes.nlp().mask_hashtags());
  REQUIRE_THROWS(bytes.nlp().mask_dates());
  REQUIRE_THROWS(bytes.nlp().mask_times());
  REQUIRE_THROWS(bytes.nlp().mask_currency());

  REQUIRE_THROWS(pluss::note::make({true, false}).nlp().mask_numbers());
  REQUIRE_THROWS(pluss::note::make(
                     std::vector<std::int64_t>{1, 2}).nlp().mask_numbers());
  REQUIRE_THROWS(pluss::note::make(
                     std::vector<double>{1.0, 2.0}).nlp().mask_numbers());
}

TEST_CASE("Note PII masking applies category precedence and flags",
          "[coden][nlp_mask]") {
  const auto source = pluss::note::make({
      "mail a@b.io via https://x.io to @ann #team on 2024-01-02 at 7pm for €20 id 42"});
  REQUIRE(text(source.nlp().mask_pii()) == std::vector<std::string>{
      "mail [EMAIL] via [URL] to [MENTION] [HASHTAG] on [DATE] at [TIME] for [CUR] id [NUM]"});
  REQUIRE(text(source.nlp().mask_pii(1)) == std::vector<std::string>{
      "mail a@b.io via https://x.io to @ann #team on [NUM] at [NUM]pm for €[NUM] id [NUM]"});
  REQUIRE(text(source.nlp().mask_pii(2 | 4)) == std::vector<std::string>{
      "mail [EMAIL] via [URL] to @ann #team on 2024-01-02 at 7pm for €20 id 42"});
  REQUIRE(text(source.nlp().mask_pii(1 << 20)) == text(source));
  REQUIRE(text(source.nlp().mask_pii(-1)) == text(source.nlp().mask_pii()));
  REQUIRE_FALSE(pluss::Note::undefined().nlp().mask_pii().defined());

  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"a@b.io"}, std::array<std::int64_t, 1>{1});
  REQUIRE_THROWS(bytes.nlp().mask_pii());
}
