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

} // namespace

int main() {
  std::cout << "=== accessor pipeline ===\n";
  const auto source = pluss::note::make({"<b>Ship</b> #release today"});
  const auto plain = source.nlp().remove_html().nlp().remove_hashtags();
  show("plain", plain);
  show("tokens", plain.nlp().tokens().tokens());

  std::cout << "\n=== emails ===\n";
  show("recognized forms",
       pluss::note::make({
           "Write a+b@example.co.uk or x.y@sub.example.io today"})
           .nlp().remove_emails());
  show("invalid forms unchanged",
       pluss::note::make({
           "  keep a@b, @handle, and x@@example.com\nexactly  "})
           .nlp().remove_emails());

  std::cout << "\n=== footers ===\n";
  show("recognized footer lines",
       pluss::note::make({
           "Body line\nPage 3 of 9\nConfidential - internal\n"
           "© 2026 Pluss\nCopyright 2026 Pluss\nAll rights reserved\nClosing"})
           .nlp().remove_footers());
  show("footer-like prose unchanged",
       pluss::note::make({"  page three\nconfidentiality matters  "})
           .nlp().remove_footers());

  std::cout << "\n=== hashtags ===\n";
  show("recognized hashtags",
       pluss::note::make({
           "Ship #release_2 with (#team), [#ops]; keep C# mid#word and #"})
           .nlp().remove_hashtags());
  show("hash boundaries unchanged",
       pluss::note::make({"  C#\nmid#word stays  "})
           .nlp().remove_hashtags());

  std::cout << "\n=== headers ===\n";
  show("heading lines",
       pluss::note::make({
           "# Title\nBody\n  ## Section\nText\n###### Six\n"
           "####### Seven\n#include <x>\n#topic"})
           .nlp().remove_headers());
  show("hash-prefixed prose unchanged",
       pluss::note::make({"  #topic\n#include <vector>  "})
           .nlp().remove_headers());

  std::cout << "\n=== html ===\n";
  show("tags comments and entities",
       pluss::note::make({
           "<p>Hello <strong>world</strong>.</p><!-- secret --><br/>Tail &amp; more"})
           .nlp().remove_html());
  show("declaration and custom element",
       pluss::note::make({
           "<!DOCTYPE html><x-card data-id=\"7\">Value</x-card>"})
           .nlp().remove_html());
  show("adjacent blocks",
       pluss::note::make({"<p>One</p><p>Two</p>"}).nlp().remove_html());
  show("comparisons unchanged",
       pluss::note::make({"  3 < 5 and x < y > z\nstill text  "})
           .nlp().remove_html());

  std::cout << "\n=== shared contract ===\n";
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"a@b.io", "Page 1", "#tag", "<b>bold</b>"},
      std::array<std::int64_t, 2>{2, 2});
  show("matrix emails", matrix.nlp().remove_emails());
  show("matrix footers", matrix.nlp().remove_footers());
  show("matrix hashtags", matrix.nlp().remove_hashtags());
  show("matrix headers", matrix.nlp().remove_headers());
  show("matrix HTML", matrix.nlp().remove_html());

  const auto once = matrix.nlp().remove_html();
  std::cout << "HTML idempotent=" << std::boolalpha
            << (once.nlp().remove_html() == once).all().bool_item() << '\n';
  show("empty", pluss::note::make(std::vector<std::string>{})
                    .nlp().remove_html());
  std::cout << "undefined defined=" << std::boolalpha
            << pluss::Note::undefined().nlp().remove_html().defined() << '\n';

  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"<b>x</b>"},
      std::array<std::int64_t, 1>{1});
  rejected("Bytes", [&] { (void)bytes.nlp().remove_html(); });
  rejected("Bool", [&] {
    (void)pluss::note::make({true}).nlp().remove_html();
  });
  rejected("Int64", [&] {
    (void)pluss::note::make(std::vector<std::int64_t>{1})
        .nlp().remove_html();
  });
  rejected("Float64", [&] {
    (void)pluss::note::make(std::vector<double>{1.0})
        .nlp().remove_html();
  });
}
