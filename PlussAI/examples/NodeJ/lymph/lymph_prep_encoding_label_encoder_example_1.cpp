#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::Thread;

  // LabelEncoder is OrdinalEncoder with one_based = false by default: a routed
  // column becomes a dense 0..K-1 integer code. The historical name comes from
  // encoding a target label column.
  const Thread grade(pluss::make_note({"bronze", "silver", "gold", "silver", "bronze", "gold"}));

  lymph::prep::LabelEncoder encoder;
  const pluss::Series codes = encoder.learn_map(grade);
  std::cout << "codes (0-based):\n" << codes << "\n";

  std::cout << "map(new):\n"
            << encoder.map(Thread(pluss::make_note({"gold", "bronze"}))) << "\n";
  std::cout << "mapped_names_[0] = " << encoder.mapped_names_[0] << "\n";

  return 0;
}
