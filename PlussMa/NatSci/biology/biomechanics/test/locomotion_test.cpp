// Legged locomotion templates: SLIP apex map vs an independent DOP853 integration with event detection, energy conservation, LIPM closed forms.
#include "pluss/subsp/natsci/biology/biomechanics/locomotion.h"
#include "locomotion_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace lc = pluss::subsp::natsci::biology::biomechanics::locomotion;
namespace ref = locomotion_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
} // namespace

int main() {
  try {
    // ---- SLIP apex map: five parameter sets in one batched call (each lane has its own stiffness, mass, rest length, attack angle)
    {
      const std::size_t n = ref::slip.size() / 10;
      std::vector<double> ya, vx, m, k, l0, al;
      for (std::size_t i = 0; i < n; ++i) {
        ya.push_back(ref::slip[10 * i]); vx.push_back(ref::slip[10 * i + 1]); m.push_back(ref::slip[10 * i + 2]);
        k.push_back(ref::slip[10 * i + 3]); l0.push_back(ref::slip[10 * i + 4]); al.push_back(ref::slip[10 * i + 5]);
      }
      lc::SlipParams p;
      p.mass = vec(m); p.stiffness = vec(k); p.rest_length = vec(l0); p.attack_angle = vec(al);
      const auto step = lc::slip_apex_map(p, vec(ya), vec(vx));
      const auto h = step.apex_height.contiguous().host<double>(), v = step.apex_speed.contiguous().host<double>(), ts = step.stance_time.contiguous().host<double>(), pf = step.peak_force.contiguous().host<double>();
      const auto ok = step.valid.contiguous().host<double>();
      for (std::size_t i = 0; i < n; ++i) {
        const double rh = ref::slip[10 * i + 6];
        if (std::isnan(rh)) {
          expect(ok[i] == 0.0, "case " + std::to_string(i) + ": an impossible step is flagged invalid");
          continue;
        }
        expect(ok[i] != 0.0, "case " + std::to_string(i) + " is a valid step");
        expect(std::abs(h[i] - rh) < 2e-6 && std::abs(v[i] - ref::slip[10 * i + 7]) < 2e-6, "apex state of case " + std::to_string(i) + ": " + std::to_string(h[i]) + ", " + std::to_string(v[i]) + " vs " + std::to_string(rh) + ", " + std::to_string(ref::slip[10 * i + 7]));
        expect(std::abs(ts[i] - ref::slip[10 * i + 8]) < 5e-6, "stance duration of case " + std::to_string(i) + ": " + std::to_string(ts[i]) + " vs " + std::to_string(ref::slip[10 * i + 8]));
        expect(std::abs(pf[i] - ref::slip[10 * i + 9]) < 5e-3 * ref::slip[10 * i + 9] , "peak leg force of case " + std::to_string(i) + ": " + std::to_string(pf[i]) + " vs " + std::to_string(ref::slip[10 * i + 9]));
        // energy conservation across the whole step: m g y + m v^2 / 2 at the two apexes
        const double e0 = m[i] * 9.80665 * ya[i] + 0.5 * m[i] * vx[i] * vx[i], e1 = m[i] * 9.80665 * h[i] + 0.5 * m[i] * v[i] * v[i];
        expect(std::abs(e1 - e0) < 1e-7 * e0, "energy conserved over the step, case " + std::to_string(i));
      }
    }
    // ---- iterating the map from a fixed apex energy settles for these gait parameters (return map used by walking/running controllers)
    {
      lc::SlipParams p;
      p.mass = 80.0; p.stiffness = 20000.0; p.rest_length = 1.0; p.attack_angle = 1.2;
      Series y = vec({1.0}), v = vec({5.0});
      for (int s = 0; s < 6; ++s) {
        const auto st = lc::slip_apex_map(p, y, v);
        expect(st.valid.host<bool>().front(), "a valid step at iteration " + std::to_string(s));
        y = st.apex_height;
        v = st.apex_speed;
      }
      const double e0 = 80.0 * 9.80665 * 1.0 + 0.5 * 80.0 * 25.0, e1 = 80.0 * 9.80665 * y.host<double>().front() + 0.5 * 80.0 * v.host<double>().front() * v.host<double>().front();
      expect(std::abs(e1 - e0) < 1e-6 * e0, "energy conserved over six consecutive steps");
    }
    // ---- linear inverted pendulum: closed forms vs the ODE integration
    {
      const double z0 = ref::lipm_height;
      const Series tc = lc::lipm_time_constant(z0, vec({0.0}));
      expect(std::abs(tc.item<double>() - std::sqrt(z0 / 9.80665)) < 1e-14, "time constant sqrt(z0 / g)");
      for (std::size_t i = 0; i < ref::lipm.size() / 5; ++i) {
        const auto s = lc::lipm_state(vec({ref::lipm[5 * i]}), vec({ref::lipm[5 * i + 1]}), vec({ref::lipm[5 * i + 2]}), tc);
        expect(std::abs(s.x.host<double>().front() - ref::lipm[5 * i + 3]) < 1e-11 && std::abs(s.v.host<double>().front() - ref::lipm[5 * i + 4]) < 1e-11, "LIPM state vs ODE, case " + std::to_string(i));
        // orbital energy is conserved
        const Series e0 = lc::lipm_orbital_energy(vec({ref::lipm[5 * i]}), vec({ref::lipm[5 * i + 1]}), tc), e1 = lc::lipm_orbital_energy(s.x, s.v, tc);
        expect(std::abs(e1.host<double>().front() - e0.host<double>().front()) < 1e-11, "orbital energy conserved");
      }
      // time to a position inverts the flow; the capture point brings the pendulum to rest over the foot
      const Series x0 = vec({-0.2}), v0 = vec({1.1});
      const auto at = lc::lipm_state(x0, v0, vec({0.3}), tc);
      expect(std::abs(lc::lipm_time_to_position(x0, v0, at.x, tc).host<double>().front() - 0.3) < 1e-10, "time to position inverts the flow");
      const Series cp = lc::lipm_capture_point(x0, v0, tc);
      const auto late = lc::lipm_state(x0.sub(cp), v0, vec({3.0}), tc);
      expect(std::abs(late.x.host<double>().front()) < 1e-4 && std::abs(late.v.host<double>().front()) < 1e-3, "foot at the capture point stops the pendulum");
    }
    std::cout << "locomotion tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
