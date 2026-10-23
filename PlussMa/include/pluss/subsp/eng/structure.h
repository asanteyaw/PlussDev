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

struct SolveOptions {
  pluss::subsp::math::krylov::CgOptions cg{};
  bool fail_on_nonconvergence{true};
};

struct EigOptions {
  std::int64_t count{1};
  std::int64_t max_iterations{64};
  double relative_tolerance{1e-8};
  double absolute_tolerance{1e-10};
  pluss::subsp::math::krylov::CgOptions solve{};
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
  friend Series mass(const Frame &, const Series &);
  friend ModeResult modes(const Frame &, const Fix &, const EigOptions &);
  friend BuckResult buckling(const Frame &, const Series &, const Fix &,
                             const EigOptions &);
};

[[nodiscard]] Truss truss(const Mesh &mesh, const Bars &bars);

[[nodiscard]] Frame frame(const Mesh &mesh, const Sections &sections);

[[nodiscard]] StaticResult solve(const Truss &truss, const Series &load,
                                 const Fix &fix,
                                 const SolveOptions &options = {});

[[nodiscard]] FrameResult solve(const Frame &frame, const Series &load,
                                const Fix &fix,
                                const SolveOptions &options = {});

// Apply the assembled consistent mass operator without materializing it.
[[nodiscard]] Series mass(const Frame &frame, const Series &acceleration);

// Extract the lowest elastic modes of a constrained frame.
[[nodiscard]] ModeResult modes(const Frame &frame, const Fix &fix,
                               const EigOptions &options = {});

// Extract positive linearized buckling factors for a reference nodal load.
[[nodiscard]] BuckResult buckling(const Frame &frame,
                                  const Series &reference_load, const Fix &fix,
                                  const EigOptions &options = {});

} // namespace pluss::subsp::eng::structure
