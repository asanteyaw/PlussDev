#pragma once
// INTERNAL: a two-dimensional finite-difference solver for
//
//     u_tau = 1/2 a11 u_xx + a12 u_xy + 1/2 a22 u_yy + b1 u_x + b2 u_y - c u ,         u(0) = payoff,
//
// with an optional early-exercise constraint u >= payoff.  The coefficients are supplied as a function of the node, the grids are supplied by
// the caller (ascending, usually stretched), and each of the four edges is one of
//   Dirichlet   the value is imposed as a function of (tau, coordinate along the edge);
//   Degenerate  the coefficients of the normal derivatives of second order vanish there, so only the normal drift is kept, discretised one-sided
//               into the domain (second order); the drift must point into the domain;
//   Frozen      no normal derivative at all (a Neumann condition u_n = 0): only the discount and the tangential terms act.
// The mixed derivative is dropped on the edges.  Time stepping is the Modified Craig-Sneyd ADI scheme of in 't Hout and Foulon (2010) with a
// Rannacher start, and the exercise constraint is imposed by the Ikonen-Toivanen splitting adapted to ADI by Haentjens and in 't Hout (2012).
// Heston (american_pde.cpp) is a client; so is the exchange-option test.
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace pluss::subsp::math::finance::internal::pde2d {

enum class Edge { Dirichlet, Degenerate, Frozen };
enum class Exercise { None, American, Bermudan };

struct Coefficients {
  double a11 = 0.0, a12 = 0.0, a22 = 0.0, b1 = 0.0, b2 = 0.0, c = 0.0;
};

struct Problem {
  std::vector<double> x, y;
  std::function<Coefficients(double x, double y)> generator;
  std::function<double(double x, double y)> payoff;  // initial condition and exercise obstacle
  Edge x_low = Edge::Degenerate, x_high = Edge::Dirichlet, y_low = Edge::Degenerate, y_high = Edge::Frozen;
  // Dirichlet values: (tau, coordinate along the edge); needed only for the edges that are Dirichlet
  std::function<double(double tau, double along)> x_low_value, x_high_value, y_low_value, y_high_value;
  double maturity = 1.0;
  // Optional direction of a parameter (the interest rate): the derivative of the generator's coefficients and of the x_high Dirichlet value (the other edges
  // must not depend on it). With them, Solver::solve returns the exact derivative of the discrete solution in that parameter.
  std::function<Coefficients(double x, double y)> generator_tangent;
  std::function<double(double tau, double along)> x_high_tangent;
};

struct Options {
  Exercise exercise = Exercise::None;
  std::int64_t steps = 100;
  std::vector<double> breaks;  // Bermudan: times to maturity (ascending, inside (0, maturity)) at which the option can be exercised
  double theta = 0.8;          // implicit weight of the ADI sweeps (the correction uses vartheta = 1/2)
};

class Solver {
 public:
  explicit Solver(Problem problem);

  // The solution at tau = maturity, as values on the grid: index j * nx + i for node (x[i], y[j]).
  // With `tangent`, also the derivative of the solution in the parameter of Problem::generator_tangent: the tangent of every step of the scheme, the exercise
  // splitting included (along the active set), carried in step with the value.
  std::vector<double> solve(const Options& options, std::vector<double>* tangent = nullptr);

  // Four-point Lagrange interpolation of a surface at (x0, y0).
  double interpolate(const std::vector<double>& u, double x0, double y0) const;

  const std::vector<double>& x() const { return p_.x; }
  const std::vector<double>& y() const { return p_.y; }
  std::size_t index(std::size_t i, std::size_t j) const { return j * nx_ + i; }

 private:
  struct Lines {  // the operator along one axis, per node: row entries lo, mid, hi and the third entry of an end row
    std::vector<double> lo, mid, hi, ext;
  };

  // The elimination of every line of (I - c A) depends only on c, so it is done once per c and kept: a solve is then multiply-adds only.
  struct Factor {
    std::vector<double> sub, inv, sc;  // sub-diagonal, 1 / pivot, super-diagonal times 1 / pivot, per node
    std::vector<double> first, last;   // elimination factors of the end rows (zero when the row needs none), per line
  };

  Problem p_;
  std::size_t nx_, ny_, n_;
  Lines ax_, ay_;                      // asset-axis (x) and second-axis (y) operators
  Lines dax_, day_;                    // their derivatives in the parameter (when a generator_tangent is given)
  std::vector<double> cross_;          // a12 at every node (0 on edges)
  std::vector<double> wx_lo_, wx_mid_, wx_hi_, wy_lo_, wy_mid_, wy_hi_;  // central first-derivative weights for the mixed term
  std::vector<double> payoff_;
  std::map<double, Factor> factors_x_, factors_y_;
  std::vector<double> bc_x_low_, bc_x_high_, bc_y_low_, bc_y_high_;      // Dirichlet values at the new time level
  // work arrays
  std::vector<double> a0u_, a1u_, a2u_, a0y_, a1y_, a2y_, y0_, y1_, rhs_, sub_, diag_, sup_, scratch_, line_;

  void build_();
  void set_boundary_values_(double tau);
  void apply_mixed_(const std::vector<double>& u, std::vector<double>& out) const;
  void apply_x_(const std::vector<double>& u, std::vector<double>& out) const;
  void apply_y_(const std::vector<double>& u, std::vector<double>& out) const;
  void apply_x_(const Lines& l, const std::vector<double>& u, std::vector<double>& out) const;
  void apply_y_(const Lines& l, const std::vector<double>& u, std::vector<double>& out) const;
  const Factor& factor_x_(double c);
  const Factor& factor_y_(double c);
  void solve_x_(std::vector<double>& rhs, double c);
  void solve_y_(std::vector<double>& rhs, double c);
  void advance_(std::vector<double>& u, std::vector<double>& lambda, double dt, double tau_new, bool douglas, bool american, double theta);
  void advance_tangent_(std::vector<double>& u, std::vector<double>& w, std::vector<double>& lambda, std::vector<double>& lambda_w, double dt, double tau_new, bool douglas, bool american, double theta);
};

// A grid on [lo, hi] clustered around `centre` with a node exactly at it: x = centre + width sinh(xi), xi uniform.  `width` sets the
// clustering (smaller is tighter).
std::vector<double> clustered_grid(double lo, double hi, double centre, double width, std::size_t n);

// A grid on [0, hi] clustered at 0: x = scale sinh(eta), eta uniform.
std::vector<double> origin_grid(double hi, double scale, std::size_t n);

}  // namespace pluss::subsp::math::finance::internal::pde2d
