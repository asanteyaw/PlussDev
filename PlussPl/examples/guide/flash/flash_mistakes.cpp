// What goes wrong when you call flash by name, and the message each mistake produces.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::pl;

template <class F> static void report(const char *what, F &&call) {
  try {
    call();
    std::cout << what << ": no error\n";
  } catch (const pluss::Error &e) {
    std::cout << what << ": " << e.message() << "\n";
  }
}

int main() {
  auto loaded = flash::load_dataset("petals");
  const auto &petals = *loaded;
  std::cout << "bundled dataset 'nothing' exists: " << (flash::load_dataset("nothing").has_value() ? "yes" : "no") << "\n";

  // 1. A column name that is not in the dataset.
  report("unknown column", [&] {
    auto fig = studio("flash_mistake_column");
    flash::jointplot(fig, petals, "sepal_length", "no_such_column", "scatter");
  });

  // 2. A text column where a number is needed.
  report("text column as a number", [&] {
    auto fig = studio("flash_mistake_text");
    flash::jointplot(fig, petals, "sepal_length", "species", "scatter");
  });

  // 3. A kind string that does not exist.
  report("unknown kind", [&] {
    auto fig = studio("flash_mistake_kind");
    flash::jointplot(fig, petals, "sepal_length", "sepal_width", "bubbles");
  });

  // 4. A matrix whose feature names do not match its feature axis.
  report("wrong number of feature names", [&] {
    auto matrix = pluss::series::linspace(0.0, 1.0, 40, pluss::ScalarType::Float64).reshape({10, 4});
    flash::MatrixFrameOptions options;
    options.feature_names = {"a", "b", "c"};
    auto fig = studio("flash_mistake_names");
    flash::pairplot(fig, matrix, options);
  });
}
