#include <pluss/pluss.h>

#include <cassert>
#include <string>
#include <vector>

int main() {
  using namespace pluss::pl;

  Studio book("pdf-text-smoke", Theme::paper());
  book.size(480.0, 320.0);
  Pane& pane = book.pane2("math");
  pane.x_label("x")
      .y_label("$\\mu + \\sigma + \\gamma + \\omega + \\alpha + \\beta + \\lambda$")
      .caption("$\\Delta\\sigma_t \\leq \\omega$, $\\frac{1}{2}$");
  pane.trend(std::vector<double>{0.0, 1.0}, std::vector<double>{0.0, 1.0});

  const std::string pdf = book.to_pdf();
  assert(pdf.rfind("%PDF-1.4", 0) == 0);
  assert(pdf.find("mu + sigma + gamma + omega + alpha + beta + lambda") != std::string::npos);
  assert(pdf.find("Delta sigma_t <= omega") != std::string::npos);
  assert(pdf.find("(1)/(2)") != std::string::npos);
  return 0;
}
