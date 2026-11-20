// Prepare support tickets for sharing: mask personal data, strip markup and normalise the text, then check nothing leaked.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto tickets = pluss::make_note({"<p>Hi, I'm Dana (dana.k@example.org). My order 88213 never arrived.</p>",
                                   "Call me on 555-0142 or see https://shop.example.com/orders/88213 for the tracking.",
                                   "Refund of $49.99 requested on 2026-11-02 – “still waiting”."});
  show("original", tickets);

  // 1. Remove markup, then mask the personal data. Each masker replaces what it finds with a placeholder.
  auto text = tickets.nlp().remove_html();
  auto masked = text.nlp().mask_emails().nlp().mask_urls().nlp().mask_dates().nlp().mask_currency().nlp().mask_numbers();
  show("masked", masked);

  // 2. Normalise typography so that downstream matching is not defeated by curly quotes and dashes.
  auto tidy = masked.normalize_quotes().normalize_dashes();
  show("normalised", tidy);

  // 3. Audit: no address, link or digit may remain. An audit only proves what it tests, so test for each kind of leak you care about.
  // The Bool Notes must stay alive while their span is read: bools() borrows from them.
  const auto has_email = tidy.contains("@"), has_url = tidy.contains("http"), has_digit = tidy.contains_any_digit();
  int leaks = 0;
  for (const auto f : has_email.bools()) leaks += f ? 1 : 0;
  for (const auto f : has_url.bools()) leaks += f ? 1 : 0;
  for (const auto f : has_digit.bools()) leaks += f ? 1 : 0;
  std::cout << "leaks found: " << leaks << "\n";
  const auto has_name = tidy.contains("Dana");
  std::cout << "names are not masked by these maskers: ticket 0 still names Dana: " << (has_name.bools()[0] ? "yes" : "no") << "\n";
}
