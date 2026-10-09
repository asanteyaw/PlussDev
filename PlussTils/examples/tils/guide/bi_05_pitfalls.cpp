#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <vector>

namespace {

// Returns true if the callable throws. The message is not printed, so the output
// does not depend on library wording.
template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  namespace bi = pluss::bi;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}, {2}, cfg))},
  });
  const pluss::bundle::Nexus nexus({{"sales", sales}});

  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));

  // Asking for an existing board key returns that board. It does not add a second one.
  const auto boards_before = studio.boards().size();
  studio.board("overview");
  std::cout << "boards before repeating the key: " << boards_before << '\n';
  std::cout << "boards after repeating the key: " << studio.boards().size() << '\n';

  // An export needs a positive size. A zero width is refused and writes nothing.
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_pitfalls_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path target = dir / "rejected.html";
  std::cout << "zero width export rejected: "
            << throws([&] { studio.export_html(target.string(), "Sales", 0, 600); }) << '\n';
  std::cout << "no file written for rejected export: " << !fs::exists(target) << '\n';
  fs::remove_all(dir);
}
