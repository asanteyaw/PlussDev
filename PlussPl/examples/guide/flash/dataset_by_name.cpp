// Work from a Dataset by column name: list the columns, take a joint view of two of them, a pair grid and a triplot of several, then wrap
// your own table and your own matrix the same way.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}

int main() {
  // 1. Load a bundled dataset and look at what it holds.
  auto loaded = flash::load_dataset("petals");
  const auto &petals = *loaded;
  std::cout << "petals: " << petals.nrows() << " rows, " << petals.ncols() << " columns\n";
  for (std::int64_t i = 0; i < petals.numeric_names->numel(); ++i) std::cout << "  numeric " << petals.numeric_names->at(i) << "\n";
  for (std::int64_t i = 0; i < petals.text_names->numel(); ++i) std::cout << "  text    " << petals.text_names->at(i) << "\n";

  // 2. A joint view of two columns, named rather than extracted: each kind string draws a different summary of the same pair.
  const auto first = petals.numeric_names->at(0);
  const auto second = petals.numeric_names->at(1);
  for (const char *kind : {"scatter", "reg", "hex", "kde"}) {
    auto joint = studio(std::string("flash_joint_") + kind);
    flash::jointplot(joint, petals, first, second, kind);
    joint.show();
  }
  // ...or an options struct, here a density with density margins.
  auto density = studio("flash_joint_kde_margins");
  flash::JointPlotOptions kde;
  kde.kind = flash::JointPlotKind::Kde;
  kde.marginal = flash::JointMarginalKind::Kde;
  flash::jointplot(density, petals, first, second, kde);
  density.show();

  // 3. Every numeric column against every other. The triangle chooses which half to draw; kind and diag_kind choose the cells.
  struct PairCase { const char *name; flash::PairPlotTriangle triangle; flash::PairPlotKind kind; flash::PairPlotDiagKind diag; };
  for (const auto &c : std::vector<PairCase>{{"full", flash::PairPlotTriangle::Full, flash::PairPlotKind::Scatter, flash::PairPlotDiagKind::Histogram},
                                             {"lower", flash::PairPlotTriangle::Lower, flash::PairPlotKind::Scatter, flash::PairPlotDiagKind::Density},
                                             {"upper", flash::PairPlotTriangle::Upper, flash::PairPlotKind::Scatter, flash::PairPlotDiagKind::Histogram},
                                             {"reg", flash::PairPlotTriangle::Lower, flash::PairPlotKind::Reg, flash::PairPlotDiagKind::Density}}) {
    auto pairs = studio(std::string("flash_pairs_") + c.name);
    flash::PairPlotOptions pair_options;
    pair_options.triangle = c.triangle;
    pair_options.kind = c.kind;
    pair_options.diag_kind = c.diag;
    flash::pairplot(pairs, petals, pair_options);
    pairs.show();
  }

  // 4. The same three features as a triplot, once per kind.
  const std::vector<std::pair<const char *, flash::TriplotKind>> tri_kinds{{"cloud", flash::TriplotKind::Cloud},     {"path", flash::TriplotKind::Path},
                                                                           {"sheet", flash::TriplotKind::Sheet},     {"lattice", flash::TriplotKind::Lattice},
                                                                           {"spire", flash::TriplotKind::Spire},     {"ridge", flash::TriplotKind::Ridge}};
  for (const auto &[name, kind] : tri_kinds) {
    auto tri = studio(std::string("flash_triplot_") + name);
    flash::TriplotOptions tri_options;
    tri_options.kind = kind;
    tri_options.max_features = 3;
    flash::triplot(tri, petals, tri_options);
    tri.show();
  }

  // 5. Your own table: wrap it, then call the same functions by name.
  auto frame = pluss::table::DataFrame({{"dose", pluss::series::linspace(0.0, 9.0, 10, pluss::ScalarType::Float64)},
                                        {"response", pluss::make_series({0.1, 0.9, 2.1, 2.8, 4.2, 4.9, 6.2, 6.8, 8.1, 9.2})}});
  const auto own = flash::Dataset::from_table(frame);
  std::cout << "own table: " << own.nrows() << " rows, " << own.ncols() << " columns\n";
  auto own_fig = studio("flash_own_table");
  flash::jointplot(own_fig, own, "dose", "response", "reg");
  own_fig.show();

  // 6. Your own matrix: name the feature axis and the features through MatrixFrameOptions.
  auto matrix = pluss::series::linspace(0.0, 1.0, 40, pluss::ScalarType::Float64).reshape({10, 4});
  matrix = matrix + matrix.sin();
  flash::MatrixFrameOptions frame_options;
  frame_options.feature_axis = flash::Axis::Cols;
  frame_options.feature_names = {"a", "b", "c", "d"};
  auto mat_fig = studio("flash_matrix_pairs");
  flash::pairplot(mat_fig, matrix, frame_options);
  mat_fig.show();
  std::cout << "matrix pair grid drawn for " << frame_options.feature_names.size() << " named features\n";
}
