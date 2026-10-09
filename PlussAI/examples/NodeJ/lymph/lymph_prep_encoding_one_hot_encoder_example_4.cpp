#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::Slate;

  // OneHotEncoder, OrdinalEncoder, LabelEncoder and FrequencyEncoder each carry
  // a single-column overload: pass one Slate, get one Series back -- no frame,
  // no features() pin. It handles string (note-backed) categories directly.
  const Slate region(pluss::make_note({"north", "west", "north", "south", "east"}));

  lymph::prep::OneHotEncoder encoder;
  const pluss::Series indicators = encoder.learn_map(region);
  std::cout << "one-hot matrix (rows x categories, last column is unknown):\n"
            << indicators << "\n";

  const Slate query(pluss::make_note({"south", "north"}));
  std::cout << "map(query):\n" << encoder.map(query) << "\n";
  std::cout << "column labels: ";
  for (const auto& n : encoder.mapped_names_) std::cout << n << " ";
  std::cout << "\n";

  return 0;
}
