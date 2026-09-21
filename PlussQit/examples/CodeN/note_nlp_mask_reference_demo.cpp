#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Note& note) {
  std::cout << label << '\n' << pluss::note::print(note) << '\n';
}

template <typename Fn>
void rejected(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

template <typename Fn>
void contract(std::string_view name, Fn&& transform) {
  const auto empty = transform(
      pluss::note::make(std::vector<std::string>{}));
  std::cout << name << " empty dim=" << empty.dim()
            << " numel=" << empty.numel() << '\n';
  std::cout << name << " undefined=" << std::boolalpha
            << transform(pluss::Note::undefined()).defined() << '\n';
  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"42"}, std::array<std::int64_t, 1>{1});
  rejected(std::string(name) + " Bytes", [&] { (void)transform(bytes); });
}

} // namespace

int main() {
  std::cout << "=== numbers ===\n";
  show("numeric runs",
       pluss::note::make({"IDs 42, 1,234.50, 2024-01-02, +7 and none"})
           .nlp().mask_numbers());
  show("separator boundaries",
       pluss::note::make({"1/2 3:45 6_000 8-9; 1..2 and x7y"})
           .nlp().mask_numbers());

  std::cout << "\n=== emails ===\n";
  show("email forms",
       pluss::note::make({"a+b@example.co.uk", "x.y@sub.example.io"})
           .nlp().mask_emails());
  show("email boundaries",
       pluss::note::make({"mail x@y.io, keep a@b and @handle"})
           .nlp().mask_emails());

  std::cout << "\n=== urls ===\n";
  show("URL forms",
       pluss::note::make({"HTTPS://example.com/a?q=1", "www.x.io/path#part"})
           .nlp().mask_urls());
  show("URL punctuation",
       pluss::note::make({"(https://x.io/a), www.x.io! not ftp://x.io"})
           .nlp().mask_urls());

  std::cout << "\n=== mentions ===\n";
  show("mention boundaries",
       pluss::note::make({"@root [@alice], tag:@ann; x@y.io mid@name"})
           .nlp().mask_mentions());

  std::cout << "\n=== hashtags ===\n";
  show("hashtag boundaries",
       pluss::note::make({"#root (#ship); [#release_2] C# mid#word"})
           .nlp().mask_hashtags());

  std::cout << "\n=== dates ===\n";
  show("date forms",
       pluss::note::make({"2024-01-02", "12/31/2024", "31.12.2024",
                          "January 2, 2024", "Sep 7"})
           .nlp().mask_dates());
  show("date boundaries",
       pluss::note::make({
           "keep 2024-13-01 and 2024-01/02; syntax Feb 31, 2024"})
           .nlp().mask_dates());

  std::cout << "\n=== times ===\n";
  show("time forms",
       pluss::note::make({"00:00", "23:59", "7pm", "07:05 AM"})
           .nlp().mask_times());
  show("time boundaries",
       pluss::note::make({"keep 24:00 19:30pm 12:60; mask 12:00pm"})
           .nlp().mask_times());

  std::cout << "\n=== currency ===\n";
  show("currency symbols",
       pluss::note::make({"$12.50", "€ 20", "£9", "¥100", "$2M"})
           .nlp().mask_currency());
  show("currency codes",
       pluss::note::make({"CAD 42", "30 eur", "CHF1,250.00", "keep 99"})
           .nlp().mask_currency());

  std::cout << "\n=== pii composition ===\n";
  const auto pii = pluss::note::make({
      "mail a@b.io via https://x.io to @ann #team on 2024-01-02 at 7pm for €20 id 42"});
  show("all categories", pii.nlp().mask_pii());
  show("numbers only", pii.nlp().mask_pii(1));
  show("emails and URLs", pii.nlp().mask_pii(2 | 4));
  show("unknown positive bits", pii.nlp().mask_pii(1 << 20));

  std::cout << "\n=== shared contract ===\n";
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"$1", "a@b.io", "7pm", "#tag"},
      std::array<std::int64_t, 2>{2, 2});
  show("matrix currency", matrix.nlp().mask_currency());
  const auto already = pluss::note::make({
      "[NUM] [EMAIL] [URL] [MENTION] [HASHTAG] [DATE] [TIME] [CUR]"});
  show("placeholders unchanged", already.nlp().mask_numbers()
                                     .nlp().mask_emails()
                                     .nlp().mask_urls()
                                     .nlp().mask_mentions()
                                     .nlp().mask_hashtags()
                                     .nlp().mask_dates()
                                     .nlp().mask_times()
                                     .nlp().mask_currency());
  show("empty", pluss::note::make(std::vector<std::string>{})
                    .nlp().mask_numbers());
  std::cout << "undefined defined=" << std::boolalpha
            << pluss::Note::undefined().nlp().mask_numbers().defined() << '\n';

  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"42"}, std::array<std::int64_t, 1>{1});
  rejected("Bytes", [&] { (void)bytes.nlp().mask_numbers(); });
  rejected("Bool", [&] {
    (void)pluss::note::make({true, false}).nlp().mask_numbers();
  });
  rejected("Int64", [&] {
    (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
        .nlp().mask_numbers();
  });
  rejected("Float64", [&] {
    (void)pluss::note::make(std::vector<double>{1.0, 2.0})
        .nlp().mask_numbers();
  });

  std::cout << "\n=== per-method contracts ===\n";
  contract("currency", [](const pluss::Note& n) {
    return n.nlp().mask_currency();
  });
  contract("dates", [](const pluss::Note& n) { return n.nlp().mask_dates(); });
  contract("emails", [](const pluss::Note& n) { return n.nlp().mask_emails(); });
  contract("hashtags", [](const pluss::Note& n) {
    return n.nlp().mask_hashtags();
  });
  contract("mentions", [](const pluss::Note& n) {
    return n.nlp().mask_mentions();
  });
  contract("numbers", [](const pluss::Note& n) {
    return n.nlp().mask_numbers();
  });
  contract("times", [](const pluss::Note& n) { return n.nlp().mask_times(); });
  contract("urls", [](const pluss::Note& n) { return n.nlp().mask_urls(); });
  contract("pii", [](const pluss::Note& n) { return n.nlp().mask_pii(); });
}
