// Clean one text column of survey answers that includes blanks: normalise, classify with patterns, count the categories.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Shelf;

int main() {
  // Eight free-text answers to "Do you use the product daily?". Two are blank (missing).
  auto raw = pluss::make_note({"  YES ", "no", "Yes, mostly", "", "N/A", "yes", "", "Not really"});
  auto present = pluss::make_series({true, true, true, false, true, true, false, true});
  Shelf answers(raw, present);
  std::cout << answers.to_string() << "\n";

  // 1. Normalise: trim, lower-case. The nt() accessor holds the text operations; missing values stay missing.
  auto clean = answers.nt().strip().nt().lower();
  std::cout << clean.to_string() << "\n";

  // 2. Classify with patterns: answers that start with "yes" and answers that start with "no" (not "n/a").
  auto yes = clean.nt().matches("yes");
  auto no = clean.nt().matches("no");
  std::cout << "yes?\n" << yes.to_string() << "\n";
  std::cout << "no?\n" << no.to_string() << "\n";

  // 3. How many of each, and what remains unclassified?
  std::cout << "yes " << yes.sum().to_dbl() << ", no " << no.sum().to_dbl() << ", missing " << clean.isnull().sum().to_dbl() << ", total " << clean.size() << "\n";

  // 4. The distinct normalised answers and how often each occurs.
  std::cout << clean.distinct().to_string() << "\n";
}
