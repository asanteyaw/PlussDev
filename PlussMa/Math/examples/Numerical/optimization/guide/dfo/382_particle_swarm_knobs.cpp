// The settings of `ParticleSwarmConfig`, one at a time, on the 3-D Rastrigin function in [-5.12, 5.12]^3 with a budget of
// 20000 evaluations; three seeds per row (minimum for seeds 1, 2, 3) because one run of a random method says little.
//   population_size            particles (default 200)
//   cognitive, social          pull toward the particle's own best / the swarm's best (default 2, 2)
//   inertia_max, inertia_min   momentum at the start / end of the run with the linear schedule (default 0.99, 0.1)
//   linear_inertia_schedule    false keeps a constant `inertia`
//   velocity_clamp_ratio       0 = no clamp; otherwise the speed limit as a fraction of the box width
//   time_varying_acceleration  true moves cognitive/social to final_cognitive/final_social over the run
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rastrigin(const pluss::Series& x) {
  const double pi = 3.14159265358979323846;
  return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();
}

int main() {
  std::vector<std::pair<const char*, dfo::ParticleSwarmConfig>> cases;
  dfo::ParticleSwarmConfig base;
  cases.emplace_back("defaults", base);
  auto small = base; small.population_size = 30;
  cases.emplace_back("population_size 30", small);
  auto cog = base; cog.cognitive = 3.0; cog.social = 1.0;
  cases.emplace_back("cognitive 3, social 1", cog);
  auto soc = base; soc.cognitive = 1.0; soc.social = 3.0;
  cases.emplace_back("cognitive 1, social 3", soc);
  auto constant = base; constant.linear_inertia_schedule = false; constant.inertia = 0.7;
  cases.emplace_back("constant inertia 0.7", constant);
  auto clamp = base; clamp.velocity_clamp_ratio = 0.1;
  cases.emplace_back("velocity_clamp_ratio 0.1", clamp);
  auto tv = base; tv.time_varying_acceleration = true;
  cases.emplace_back("time_varying_acceleration", tv);
  auto nocenter = base; nocenter.center_particle = false;
  cases.emplace_back("center_particle off", nocenter);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    c.lower_bounds = filled(3, -5.12);
    c.upper_bounds = filled(3, 5.12);
    c.max_evaluations = 20000;
    std::vector<double> minima;  // one per seed
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      c.rng_seed = seed;
      minima.push_back(dfo::particle_swarm_minimizer(rastrigin, filled(3, 3.0), c).minimum);
    }
    out.writeln(label, minima);
  }
}
