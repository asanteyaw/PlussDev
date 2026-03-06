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
      .caption("$\\Delta\\sigma_t \\leq \\omega$, $\\frac{1}{2}$, $\\bar{\\omega}_t$, $\\hat{\\omega}$, "
               "$\\omega_{t_n}$, $\\omega_{t,k}$, $\\left(\\omega+\\alpha\\right)^2$, $\\{x\\}$");
  pane.trend(std::vector<double>{0.0, 1.0}, std::vector<double>{0.0, 1.0});

  const std::string pdf = book.to_pdf();
  assert(pdf.rfind("%PDF-1.4", 0) == 0);
  assert(pdf.find("\\frac") == std::string::npos);
  assert(pdf.find("\\mu") == std::string::npos);
  assert(pdf.find("\\bar") == std::string::npos);
  assert(pdf.find("\\hat") == std::string::npos);
  assert(pdf.find("\\omega_{t_n}") == std::string::npos);
  assert(pdf.find("\\left") == std::string::npos);
  assert(pdf.find("\\right") == std::string::npos);
  assert(pdf.find("/BaseFont /Symbol") != std::string::npos);
  assert(pdf.find("/F2 ") != std::string::npos);
  assert(pdf.find(" Ts") != std::string::npos);
  return 0;
}
