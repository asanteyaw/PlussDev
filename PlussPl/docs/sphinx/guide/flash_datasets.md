<!-- generated from flash_datasets.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Flash from a dataset

[Back to PlussPl by example](../guide.md)

Many `flash` functions take a whole `Dataset` and the names of its columns, so the program says which variables to plot instead of extracting them. This page lists a bundled dataset, draws joint, pair and triplots of it by name, and then does the same for your own table and your own matrix.

## How it works

`flash::load_dataset(name)` returns one of thirteen bundled datasets. A `Dataset` keeps numeric, text and temporal columns separately, with `numeric_names`, `text_names` and `temporal_names` listing them, and reports `nrows()` and `ncols()`. `jointplot(studio, data, x_name, y_name, kind)` plots two named columns, with `kind` one of `"scatter"`, `"reg"`, `"hex"` or `"kde"`, or a `JointPlotOptions` for finer control. `pairplot(studio, data, PairPlotOptions)` and `triplot(studio, data, TriplotOptions)` use every numeric column, restricted by the options. Your own data enters through `Dataset::from_table(frame)` for a `DataFrame`, or through a `MatrixFrameOptions` that names the feature axis and the features of a matrix.

## Exploring a dataset

The program prints the shape and columns of `petals`, displays every joint kind, every pair-grid triangle and every triplot kind, and wraps a table and a matrix of its own.

```cpp
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
```

Output

```text
petals: 150 rows, 8 columns
  numeric sepal_length
  numeric sepal_width
  numeric petal_length
  numeric petal_width
  numeric color_score
  text    species
  text    habitat
  text    band
own table: 10 rows, 2 columns
matrix pair grid drawn for 4 named features
```
Four kinds of joint view of the same two columns:

![scatter](assets/flash_joint_scatter.svg)

![reg](assets/flash_joint_reg.svg)

![hex](assets/flash_joint_hex.svg)

![kde](assets/flash_joint_kde.svg)

The options with density margins:

![kde with density margins](assets/flash_joint_kde_margins.svg)

The pair grid with each triangle and kind:

![full](assets/flash_pairs_full.svg)

![lower](assets/flash_pairs_lower.svg)

![upper](assets/flash_pairs_upper.svg)

![lower with fitted lines](assets/flash_pairs_reg.svg)

The triplot, once per kind:

![cloud](assets/flash_triplot_cloud.svg)

![path](assets/flash_triplot_path.svg)

![sheet](assets/flash_triplot_sheet.svg)

![lattice](assets/flash_triplot_lattice.svg)

![spire](assets/flash_triplot_spire.svg)

![ridge](assets/flash_triplot_ridge.svg)

Your own table and your own matrix:

![regression joint view of a table of our own](assets/flash_own_table.svg)

![a pair grid of a matrix with named features](assets/flash_matrix_pairs.svg)

The listing shows 150 rows and eight columns, five numeric and three text. The pair grid draws each numeric column against every other. `triangle` chooses the full grid or one half of it, `kind` chooses points or fitted lines, and `diag_kind` chooses a histogram or a density on the diagonal. The four joint kinds summarise the same 150 points as points, a fitted line, hexagonal counts and a density. The wrapped table reports its own shape, ten rows by two columns, and the matrix of ten rows by four columns is named `a` to `d` through `MatrixFrameOptions`, so labels on the grid come from you rather than from positions.

## Decisions

- **Series or dataset:** use series for one plot, a dataset when you will draw several views of the same columns.
- **Kind string or options:** a kind string for the default look, an options value for bins, levels, triangles and sizes.
- **Your own table:** wrap it with `Dataset::from_table` once and reuse it.
- **A matrix:** give `MatrixFrameOptions` a feature axis and names; without names the features are numbered.

## Pitfalls

The program below makes four mistakes on purpose and prints the message each one produces.

```cpp
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
```

Output

```text
bundled dataset 'nothing' exists: no
unknown column: flash::jointplot(dataset): no numeric column named 'no_such_column'
text column as a number: flash::jointplot(dataset): no numeric column named 'species'
unknown kind: flash::jointplot: unknown kind 'bubbles'; the kinds are scatter, reg, hex and kde
wrong number of feature names: flash: feature_names has 3 names for 4 features
```
A column name that is not a numeric column of the dataset, including a text column such as `species`, is reported with the name; a kind string outside the four lists the valid ones; and a matrix whose feature names do not match its feature count is refused with both counts rather than labelled wrongly.

## Where next

Reference pages: [`flash::load_dataset`](../api/flash/load_dataset.md), [`flash::Dataset`](../api/flash/Dataset.md), [`flash::jointplot`](../api/flash/jointplot.md), [`flash::pairplot`](../api/flash/pairplot.md), [`flash::triplot`](../api/flash/triplot.md). Back to [PlussPl by example](../guide.md), or on to [Colour and style](colour_and_style.md).
