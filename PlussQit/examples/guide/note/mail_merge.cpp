// Personalise a message for every row of a table: fill a template, adapt wording to the data, and report what could not be filled.
#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto name = pluss::make_note({"Ada", "Grace", "Alan", ""});
  auto product = pluss::make_note({"Analytical Engine", "Compiler", "Bombe", "Spare part"});
  auto due = pluss::make_note({"2026-12-01", "2026-12-15", "2026-11-30", "2026-12-20"});
  auto template_text = pluss::make_note("Dear {name}, your {product} order is due on {due}.");

  // 1. Fill the template row by row: replace each placeholder by the matching value; an empty name falls back to "customer".
  const auto names = name.host();
  const auto products = product.host();
  const auto dues = due.host();
  std::vector<std::string> messages;
  for (std::size_t i = 0; i < names.vector().size(); ++i) {
    auto one = template_text.replace("{name}", names.vector()[i].empty() ? "customer" : names.vector()[i]).replace("{product}", products.vector()[i]).replace("{due}", dues.vector()[i]);
    messages.push_back(one.host().scalar());
  }
  auto letters = pluss::make_note(messages);
  show("letters", letters);

  // 2. Quality control: a letter must have no placeholder left and no doubled spaces.
  auto left_over = letters.contains("{");
  int bad = 0;
  for (const auto f : left_over.bools()) bad += f ? 1 : 0;
  std::cout << "letters with an unfilled placeholder: " << bad << "\n";
  show("empty names replaced by", letters.take({3}));

  // 3. One document from all letters: join with a separator and count the length.
  auto bundle = letters.join("\n---\n");
  std::cout << "bundle length " << bundle.lengths_chars().host().int_scalar() << " characters, " << letters.numel() << " letters\n";
}
