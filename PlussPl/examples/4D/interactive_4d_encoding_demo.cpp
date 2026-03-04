#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

#include "../output_paths.h"
#include "pluss/plot/plot.h"

int main() {
  std::vector<double> strike;
  std::vector<double> maturity;
  std::vector<double> value;
  std::vector<double> regime;

  for (std::size_t j = 0; j < 12; ++j) {
    const double t = 0.15 + static_cast<double>(j) * 0.18;
    for (std::size_t i = 0; i < 14; ++i) {
      const double k = 75.0 + static_cast<double>(i) * 4.0;
      const double smile = (k - 100.0) / 28.0;
      strike.push_back(k);
      maturity.push_back(t);
      value.push_back(0.16 + 0.09 * smile * smile + 0.03 * std::sqrt(t) + 0.008 * std::sin(2.0 * t + smile));
      regime.push_back(0.6 + 0.3 * std::cos(1.8 * t) + 0.25 * smile);
    }
  }

  auto fig4 = pluss::plotism::scatter4(strike, maturity, value, regime, "IV points")
                  .with_title("PlussPl 4D Encoding Demo")
                  .x_label("strike")
                  .y_label("expiry")
                  .z_label("implied vol")
                  .w_label("regime")
                  .theme("slate")
                  .font_family("IBM Plex Sans")
                  .view(130.0, 24.0)
                  .camera_distance(4.8)
                  .encode_color("regime", 6);

  auto color_fig = fig4.realize_color_figure();

  const auto svg = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_color.svg");
  const auto png = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_color.png");
  const auto pdf = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_color.pdf");
  color_fig.save_svg(svg, 1200, 900);
  color_fig.save_png(png, 1200, 900);
  color_fig.save_pdf(pdf, 1200, 900);

  auto size_fig = fig4.encode_size("regime", 6).realize_size_figure();
  const auto size_png = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_size.png");
  const auto size_pdf = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_size.pdf");
  size_fig.save_png(size_png, 1200, 900);
  size_fig.save_pdf(size_pdf, 1200, 900);

  auto frame_view = fig4.encode_frame("regime", 4).realize_frame_view();
  const auto frame_pdf = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_frame_view.pdf");
  const auto frame_png = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_frame_view.png");
  const auto frame_svg = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_frame_view.svg");
  frame_view.save_pdf(frame_pdf, 1400, 960);
  frame_view.save_png(frame_png, 1400, 960);
  frame_view.save_svg(frame_svg, 1400, 960);

  auto facet_view = fig4.encode_facet("regime", 4).realize_facet_view();
  const auto facet_pdf = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_facet_view.pdf");
  const auto facet_png = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_facet_view.png");
  const auto facet_svg = plusspl::examples::output_path(__FILE__, "plusspl_interactive_4d_facet_view.svg");
  facet_view.save_pdf(facet_pdf, 1400, 960);
  facet_view.save_png(facet_png, 1400, 960);
  facet_view.save_svg(facet_svg, 1400, 960);

  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PNG: " << png << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";
  std::cout << "Saved size PNG: " << size_png << "\n";
  std::cout << "Saved size PDF: " << size_pdf << "\n";
  std::cout << "Saved frame view PDF: " << frame_pdf << "\n";
  std::cout << "Saved frame view PNG: " << frame_png << "\n";
  std::cout << "Saved frame view SVG: " << frame_svg << "\n";
  std::cout << "Saved facet view PDF: " << facet_pdf << "\n";
  std::cout << "Saved facet view PNG: " << facet_png << "\n";
  std::cout << "Saved facet view SVG: " << facet_svg << "\n";
  color_fig.show();
  return 0;
}
