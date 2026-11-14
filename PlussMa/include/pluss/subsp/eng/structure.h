#pragma once

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/krylov.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::structure {

struct Mesh {
  Series position; // [..., node, axis], with two or three spatial axes
  std::vector<std::int64_t> from;
  std::vector<std::int64_t> to;
};

struct Bars {
  Series area;    // [..., bar]
  Series modulus; // [..., bar]
};

struct Sections {
  Series area;      // [..., member]
  Series inertia;   // [..., member], second moment about the out-of-plane axis
  Series modulus;   // [..., member]
  Series density{}; // Optional [..., member], mass per volume for mass()
};

struct Fix {
  std::vector<std::int64_t> node;
  std::vector<std::int64_t> axis;
  // Optional [..., constrained dof]. Undefined means homogeneous support.
  Series value{};
};

namespace detail {
// Conjugate gradients needs about as many iterations as the stiffness matrix has dimension at worst, and slender or long structures are
// poorly conditioned, so the structural solvers start from a larger budget than the generic Krylov default.
inline pluss::subsp::math::krylov::CgOptions structural_cg() {
  pluss::subsp::math::krylov::CgOptions options;
  options.max_iterations = 8192;
  return options;
}
} // namespace detail

// Uniform loads along the members, in each member's local axes. Intensities are force per length; either may be left undefined.
struct MemberLoad {
  Series transverse{}; // [..., member], along local y (a quarter turn counterclockwise from the member direction)
  Series axial{};      // [..., member], along local x (from the first node to the second)
};

struct SolveOptions {
  pluss::subsp::math::krylov::CgOptions cg{detail::structural_cg()};
  bool fail_on_nonconvergence{true};
};

struct EigOptions {
  std::int64_t count{1};
  std::int64_t max_iterations{64};
  double relative_tolerance{1e-8};
  double absolute_tolerance{1e-10};
  pluss::subsp::math::krylov::CgOptions solve{detail::structural_cg()};
};

struct MotionState {
  Series displacement{}; // Optional [..., node, 3]; undefined means zero.
  Series velocity{};     // Optional [..., node, 3]; undefined means zero.
};

struct MotionOptions {
  // Generalized-alpha high-frequency spectral radius in [0, 1]. One is the
  // average-acceleration Newmark rule; smaller values add numerical damping.
  double spectral_radius{1.0};
  double mass_damping{0.0};
  double stiffness_damping{0.0};
  pluss::subsp::math::krylov::CgOptions solve{detail::structural_cg()};
  bool fail_on_nonconvergence{true};
};

struct HarmonicOptions {
  double mass_damping{0.0};
  double stiffness_damping{0.0};
  pluss::subsp::math::krylov::GmresOptions solve{};
  bool fail_on_nonconvergence{true};
};

struct NonlinearOptions {
  std::int64_t load_steps{4};
  std::int64_t max_iterations{24};
  std::int64_t max_line_search{10};
  std::int64_t max_cutbacks{8};
  double relative_tolerance{1e-8};
  double absolute_tolerance{1e-10};
  double line_search_shrink{0.5};
  double armijo{1e-4};
  double min_load_step{1e-6};
  pluss::subsp::math::krylov::CgOptions solve{detail::structural_cg()};
  bool fail_on_nonconvergence{true};
};

struct NonlinearMotionOptions {
  // Generalized-alpha high-frequency spectral radius in [0, 1]. The default of
  // 0.9 damps the poorly resolved high modes that large rotations of stiff
  // members excite; the energy-conserving value of 1 is unconditionally stable
  // only for linear structures and can diverge here.
  double spectral_radius{0.9};
  double mass_damping{0.0};
  double stiffness_damping{0.0};
  std::int64_t max_iterations{20};
  std::int64_t max_line_search{8};
  double relative_tolerance{1e-8};
  double absolute_tolerance{1e-10};
  double line_search_shrink{0.5};
  double armijo{1e-4};
  pluss::subsp::math::krylov::CgOptions solve{detail::structural_cg()};
  bool fail_on_nonconvergence{true};
};

struct StaticResult {
  Series displacement; // [..., node, axis]
  Series reaction;     // [..., node, axis], zero away from constrained dofs
  Series strain;       // [..., bar]
  Series stress;       // [..., bar]
  Series force;        // [..., bar], tension is positive
  Series energy;       // [...] strain energy
  Series residual_norm;
  Series converged;
  Series breakdown;
  std::int64_t iterations{};
};

struct FrameResult {
  Series displacement; // [..., node, 3] in (ux, uy, rz) order
  Series reaction;     // [..., node, 3], zero away from constrained dofs
  // [..., member, 6] local (Ni, Vi, Mi, Nj, Vj, Mj) end actions
  Series end_force;
  Series energy;
  Series residual_norm;
  Series converged;
  Series breakdown;
  std::int64_t iterations{};
};

struct ModeResult {
  Series omega;     // [..., mode], angular frequency in radians per time
  Series hertz;     // [..., mode], cycles per time
  Series shape;     // [..., mode, node, 3], mass-normalized
  Series residual;  // [..., mode], ||K phi - omega^2 M phi||_2
  Series converged; // [..., mode]
  std::int64_t iterations{};
};

struct BuckResult {
  Series factor;    // [..., mode], multiplier on the reference load
  Series shape;     // [..., mode, node, 3], max-absolute normalized
  Series residual;  // [..., mode], ||K phi - factor K_g phi||_2
  Series converged; // [..., mode]
  std::int64_t iterations{};
};

struct MotionResult {
  Series time;          // [time]
  Series displacement;  // [..., time, node, 3]
  Series velocity;      // [..., time, node, 3]
  Series acceleration;  // [..., time, node, 3]
  Series reaction;      // [..., time, node, 3], zero on free dofs
  Series kinetic;       // [..., time]
  Series strain_energy; // [..., time]
  Series residual_norm; // [..., time]
  Series converged;     // [..., time]
  Series breakdown;     // [..., time]
  Series iterations;    // [time], maximum PCG iterations executed per sample
};

struct HarmonicResult {
  Series omega;          // [frequency], angular frequency in radians per time
  Series displacement;   // [..., frequency, node, 3], complex amplitude
  Series velocity;       // [..., frequency, node, 3], i*omega*displacement
  Series acceleration;   // [..., frequency, node, 3], -omega^2*displacement
  Series reaction;       // [..., frequency, node, 3], zero on free dofs
  Series residual_norm;  // [..., frequency]
  Series relative_residual; // [..., frequency]
  Series converged;      // [..., frequency]
  std::int64_t iterations{};
  std::int64_t operator_evaluations{};
  std::int64_t restarts{};
};

struct NonlinearResult {
  Series displacement; // [..., node, 3] in (ux, uy, rz) order
  Series reaction;     // [..., node, 3], zero away from constrained dofs
  // [..., member, 6] current-local (Ni, Vi, Mi, Nj, Vj, Mj) end actions
  Series end_force;
  Series energy;
  Series residual_norm;
  Series converged;
  double load_factor{};
  std::int64_t iterations{};
  std::int64_t steps{};
  std::int64_t cutbacks{};
};

struct NonlinearMotionResult {
  Series time;          // [time]
  Series displacement;  // [..., time, node, 3]
  Series velocity;      // [..., time, node, 3]
  Series acceleration;  // [..., time, node, 3]
  Series reaction;      // [..., time, node, 3], zero on free dofs
  Series kinetic;       // [..., time]
  Series strain_energy; // [..., time]
  Series residual_norm; // [..., time]
  Series converged;     // [..., time]
  Series iterations;    // [time], maximum Newton iterations per sample
};

class Truss final {
public:
  [[nodiscard]] std::int64_t nodes() const noexcept { return nodes_; }
  [[nodiscard]] std::int64_t bars() const noexcept { return bars_; }
  [[nodiscard]] std::int64_t dim() const noexcept { return dim_; }

private:
  Truss(Series position, Series direction, Series length, Series area,
        Series modulus, Series stiffness, Series from_index, Series to_index,
        std::int64_t nodes, std::int64_t bars, std::int64_t dim)
      : position_(std::move(position)), direction_(std::move(direction)),
        length_(std::move(length)), area_(std::move(area)),
        modulus_(std::move(modulus)), stiffness_(std::move(stiffness)),
        from_index_(std::move(from_index)), to_index_(std::move(to_index)),
        nodes_(nodes), bars_(bars), dim_(dim) {}

  Series position_{};
  Series direction_{};
  Series length_{};
  Series area_{};
  Series modulus_{};
  Series stiffness_{};
  Series from_index_{};
  Series to_index_{};
  std::int64_t nodes_{};
  std::int64_t bars_{};
  std::int64_t dim_{};

  friend Truss truss(const Mesh &, const Bars &);
  friend StaticResult solve(const Truss &, const Series &, const Fix &,
                            const SolveOptions &);
};

class Frame final {
public:
  [[nodiscard]] std::int64_t nodes() const noexcept { return nodes_; }
  [[nodiscard]] std::int64_t members() const noexcept { return members_; }
  [[nodiscard]] std::int64_t dofs() const noexcept { return 3; }

private:
  Frame(Series position, Series direction, Series length, Series area,
        Series inertia, Series modulus, Series density, Series from_index,
        Series to_index, std::int64_t nodes, std::int64_t members)
      : position_(std::move(position)), direction_(std::move(direction)),
        length_(std::move(length)), area_(std::move(area)),
        inertia_(std::move(inertia)), modulus_(std::move(modulus)),
        density_(std::move(density)), from_index_(std::move(from_index)),
        to_index_(std::move(to_index)), nodes_(nodes), members_(members) {}

  Series position_{};
  Series direction_{};
  Series length_{};
  Series area_{};
  Series inertia_{};
  Series modulus_{};
  Series density_{};
  Series from_index_{};
  Series to_index_{};
  std::int64_t nodes_{};
  std::int64_t members_{};

  friend Frame frame(const Mesh &, const Sections &);
  friend FrameResult solve(const Frame &, const Series &, const Fix &,
                           const SolveOptions &);
  friend FrameResult solve(const Frame &, const Series &, const MemberLoad &,
                           const Fix &, const SolveOptions &);
  friend NonlinearResult solve(const Frame &, const Series &, const Fix &,
                               const NonlinearOptions &);
  friend Series mass(const Frame &, const Series &);
  friend ModeResult modes(const Frame &, const Fix &, const EigOptions &);
  friend BuckResult buckling(const Frame &, const Series &, const Fix &,
                             const EigOptions &);
  friend MotionResult motion(const Frame &, const Series &, const Series &,
                             const Fix &, const MotionState &,
                             const MotionOptions &);
  friend NonlinearMotionResult
  motion(const Frame &, const Series &, const Series &, const Fix &,
         const MotionState &, const NonlinearMotionOptions &);
  friend HarmonicResult harmonic(const Frame &, const Series &, const Series &,
                                 const Fix &, const HarmonicOptions &);
};

[[nodiscard]] Truss truss(const Mesh &mesh, const Bars &bars);

[[nodiscard]] Frame frame(const Mesh &mesh, const Sections &sections);

[[nodiscard]] StaticResult solve(const Truss &truss, const Series &load,
                                 const Fix &fix,
                                 const SolveOptions &options = {});

[[nodiscard]] FrameResult solve(const Frame &frame, const Series &load,
                                const Fix &fix,
                                const SolveOptions &options = {});

// Solve a frame carrying uniform member loads as well as nodal loads. The
// member loads are carried exactly (consistent nodal loads), and `end_force`
// includes each member's own load, so it is the true action at the member ends.
[[nodiscard]] FrameResult solve(const Frame &frame, const Series &load,
                                const MemberLoad &member_load, const Fix &fix,
                                const SolveOptions &options = {});

// Solve large-displacement, elastic corotational frame equilibrium.
[[nodiscard]] NonlinearResult solve(const Frame &frame, const Series &load,
                                    const Fix &fix,
                                    const NonlinearOptions &options);

// Apply the assembled consistent mass operator without materializing it.
[[nodiscard]] Series mass(const Frame &frame, const Series &acceleration);

// Extract the lowest elastic modes of a constrained frame.
[[nodiscard]] ModeResult modes(const Frame &frame, const Fix &fix,
                               const EigOptions &options = {});

// Extract positive linearized buckling factors for a reference nodal load.
[[nodiscard]] BuckResult buckling(const Frame &frame,
                                  const Series &reference_load, const Fix &fix,
                                  const EigOptions &options = {});

// Integrate M*a + C*v + K*u = load with C = a*M + b*K.
[[nodiscard]] MotionResult motion(const Frame &frame, const Series &load,
                                  const Series &time, const Fix &fix,
                                  const MotionState &initial = {},
                                  const MotionOptions &options = {});

// Integrate nonlinear corotational equilibrium with generalized-alpha.
[[nodiscard]] NonlinearMotionResult
motion(const Frame &frame, const Series &load, const Series &time,
       const Fix &fix, const MotionState &initial,
       const NonlinearMotionOptions &options);

// Solve (K + i*omega*C - omega^2*M)u = load with C = a*M + b*K.
[[nodiscard]] HarmonicResult
harmonic(const Frame &frame, const Series &load, const Series &omega,
         const Fix &fix, const HarmonicOptions &options = {});

} // namespace pluss::subsp::eng::structure
