// A trap, shown on purpose. If `lower` is not given, the variables are free: they may be negative. The program minimizes
// x0 - x1 subject to x0 + x1 >= 2 and x0 + x1 <= 4. With x >= 0 the smallest value of x0 - x1 is -4, at x = (0, 4). With the
// variables free, x0 can be pushed toward minus infinity along x0 = -t, x1 = 4 + t, which satisfies both rows for every t, so the
// program is unbounded. The program solves it both ways and prints the status, so that the effect of leaving the bounds out is
// visible. Always state `lower` explicitly.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  lp::Problem p;
  p.c = vec({1, -1});
  p.a_ub = mat({-1, -1, 1, 1}, 2, 2);  // x0 + x1 >= 2 ; x0 + x1 <= 4
  p.b_ub = vec({-2, 4});

  const auto describe = [](const char* label, const lp::Problem& problem) {
    const auto r = lp::solve(problem);
    const auto status = static_cast<lp::Status>(static_cast<int>(r.status.item<double>()));
    std::cout << std::left << std::setw(30) << label << (status == lp::Status::Optimal ? "Optimal" : status == lp::Status::Unbounded ? "Unbounded" : "other");
    if (status == lp::Status::Optimal) {
      const auto x = r.x.host<double>();
      std::cout << std::fixed << std::setprecision(4) << "   x = (" << x[0] << ", " << x[1] << "), objective " << r.objective.item<double>();
    }
    std::cout << "\n";
  };
  describe("no bounds given (free)", p);
  lp::Problem nonnegative = p;
  nonnegative.lower = vec({0, 0});
  describe("lower = (0, 0)", nonnegative);
  return 0;
}
