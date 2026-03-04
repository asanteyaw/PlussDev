#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../output_paths.h"
#include "pluss/codem/series_factory.h"
#include "pluss/plot/plot.h"

namespace {

pluss::Series make_series_1d(const std::vector<double>& values) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())});
}

pluss::Series make_series_2d(const std::vector<double>& values, std::int64_t rows, std::int64_t cols) {
  return pluss::make_series(values, {rows, cols});
}

void save_triplet(const pluss::plotism::Figure2D& fig, const char* source_file, const std::string& stem) {
  const auto svg = plusspl::examples::output_path(source_file, stem + ".svg");
  const auto png = plusspl::examples::output_path(source_file, stem + ".png");
  const auto pdf = plusspl::examples::output_path(source_file, stem + ".pdf");
  fig.save_svg(svg, 1200, 900);
  fig.save_png(png, 1200, 900);
  fig.save_pdf(pdf, 1200, 900);
  std::cout << "Saved " << stem << " -> " << svg << ", " << png << ", " << pdf << "\n";
}

} // namespace

int main() {
  using namespace pluss::plotism;

  const auto x = make_series_1d({1.0, 2.0, 4.0, 8.0, 16.0, 32.0});
  const auto y = make_series_1d({1.2, 1.9, 3.3, 5.8, 10.5, 18.4});
  const auto grouped = make_series_2d({
                                            1.1, 2.2, 3.1,
                                            1.4, 2.4, 3.3,
                                            1.6, 2.7, 3.5,
                                            1.9, 2.9, 3.8,
                                            2.2, 3.1, 4.0,
                                            3.6, 4.4, 5.2,
                                        },
                                        6,
                                        3);
  const auto theta = make_series_1d({0.0, 0.7, 1.4, 2.2, 2.9, 3.8, 4.6, 5.3, 6.0});
  const auto rho = make_series_1d({0.7, 1.0, 1.5, 1.2, 0.8, 1.4, 1.7, 1.2, 0.9});

  auto semilogx_fig = semilogx(x, y, "semilogx").with_title("MATLAB-style semilogx");
  auto semilogy_fig = semilogy(x, y, "semilogy").with_title("MATLAB-style semilogy");
  auto loglog_fig = loglog(x, y, "loglog").with_title("MATLAB-style loglog");
  auto stem_fig = stem(x, y, "stem").with_title("MATLAB-style stem");
  auto box_fig = boxplot(grouped, "boxplot", {"A", "B", "C"}).with_title("MATLAB-style boxplot");
  auto violin_fig = violin(grouped, "violin", {"A", "B", "C"}).with_title("MATLAB-style violin");
  auto polar_fig = polarplot(theta, rho, "polar").with_title("MATLAB-style polarplot");

  semilogx_fig.theme("paper").font_family("IBM Plex Sans");
  semilogy_fig.theme("paper").font_family("IBM Plex Sans");
  loglog_fig.theme("paper").font_family("IBM Plex Sans");
  stem_fig.theme("paper").font_family("IBM Plex Sans");
  box_fig.theme("paper").font_family("IBM Plex Sans");
  violin_fig.theme("paper").font_family("IBM Plex Sans");
  polar_fig.theme("paper").font_family("IBM Plex Sans");

  save_triplet(semilogx_fig, __FILE__, "plusspl_2d_semilogx");
  save_triplet(semilogy_fig, __FILE__, "plusspl_2d_semilogy");
  save_triplet(loglog_fig, __FILE__, "plusspl_2d_loglog");
  save_triplet(stem_fig, __FILE__, "plusspl_2d_stem");
  save_triplet(box_fig, __FILE__, "plusspl_2d_boxplot");
  save_triplet(violin_fig, __FILE__, "plusspl_2d_violin");
  save_triplet(polar_fig, __FILE__, "plusspl_2d_polarplot");

  return 0;
}
