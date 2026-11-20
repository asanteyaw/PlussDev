// Clean a column of customer records: tidy names, validate and normalise email addresses, and find the duplicates.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }
static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto names = pluss::make_note({"  ada   LOVELACE ", "Grace Hopper", "alan turing", "GRACE  hopper", "Edsger  Dijkstra  "});
  auto emails = pluss::make_note({"Ada@Example.com", "grace.hopper@navy.mil", "alan@@bad", "GRACE.HOPPER@navy.mil", "ed@dijkstra.nl"});
  show("raw names", names);

  // 1. Names: trim the ends, collapse runs of spaces to one, and capitalise each word.
  auto clean_names = names.strip().collapse_spaces().title();
  show("clean names", clean_names);

  // 2. Emails: lower-case them, then test each whole address against a name@domain.tld pattern.
  auto lowered = emails.lower();
  auto valid = lowered.regex().match(R"([a-z0-9._-]+@[a-z0-9-]+(\.[a-z0-9-]+)+)");
  show("lower-cased", lowered);
  show("valid address?", valid);
  int invalid = 0;
  for (const auto ok : valid.bools()) invalid += ok ? 0 : 1;   // a Bool Note exposes its lanes as a span
  std::cout << "invalid addresses: " << invalid << "\n";

  // 3. Duplicates: two rows are the same customer when their cleaned names agree.
  auto unique_names = clean_names.unique();
  show("distinct customers", unique_names);
  std::cout << "records " << clean_names.numel() << ", distinct " << unique_names.numel() << "\n";

  // 4. A domain report: split each address at '@' and look at what follows.
  auto domain = lowered.regex().replace(R"(^.*@)", "");
  show("domains", domain);
}
