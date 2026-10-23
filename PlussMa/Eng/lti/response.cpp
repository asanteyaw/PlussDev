#include "pluss/subsp/eng/lti/response.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "pluss/codem/series_factory.h"

#include <complex>
#include <vector>

namespace pluss::subsp::eng::lti {
namespace {

ScalarType response_dtype(const StateSpace &system, const Series &omega) {
  const ScalarType common =
      codem::promote_types(system.a().scalar_type(), omega.scalar_type());
  return common == ScalarType::Float64 || common == ScalarType::Complex128
             ? ScalarType::Complex128
             : ScalarType::Complex64;
}

ScalarType response_dtype(const PoleZero &system, const Series &omega) {
  const ScalarType common =
      codem::promote_types(system.poles().scalar_type(), omega.scalar_type());
  return common == ScalarType::Float64 || common == ScalarType::Complex128
             ? ScalarType::Complex128
             : ScalarType::Complex64;
}

Series imag_unit(ScalarType dtype, Platform platform) {
  return pluss::series::make(
             std::vector<std::complex<double>>{{0.0, 1.0}},
             Config{}.with_dtype(dtype).with_platform(Platform::CPU))
      .reshape({})
      .to(platform);
}

} // namespace

Series freqresp(const StateSpace &system, const Series &omega) {
  PLUSS_CHECK(omega.defined(), "eng::lti::freqresp: omega must be defined");
  PLUSS_CHECK(omega.dim() == 1 && omega.numel() > 0,
              "eng::lti::freqresp: omega must be a non-empty vector");
  PLUSS_CHECK(!omega.dtype().is_complex(),
              "eng::lti::freqresp: omega must be real-valued");
  PLUSS_CHECK(omega.dtype().is_floating(),
              "eng::lti::freqresp: omega must have a floating dtype");
  PLUSS_CHECK(omega.platform() == system.a().platform(),
              "eng::lti::freqresp: omega and system must share one platform");

  const ScalarType dtype = response_dtype(system, omega);
  const ScalarType real_dtype = dtype == ScalarType::Complex128
                                    ? ScalarType::Float64
                                    : ScalarType::Float32;
  const Series jw =
      omega.to(real_dtype).to(dtype).mul(imag_unit(dtype, omega.platform()));
  const Series point =
      system.is_cont() ? jw : jw.mul(system.sample_time()).exp();
  const Series eye =
      pluss::series::eye(
          system.states(), system.states(),
          Config{}.with_dtype(real_dtype).with_platform(system.a().platform()))
          .to(dtype);
  const Series resolvent = point.reshape({omega.numel(), 1, 1})
                               .mul(eye)
                               .sub(system.a().to(dtype).unsqueeze(-3));
  const Series solution =
      resolvent.linearalg.lu_solve(system.b().to(dtype).unsqueeze(-3));
  const Series response =
      system.c().to(dtype).unsqueeze(-3).matmul(solution).add(
          system.d().to(dtype).unsqueeze(-3));
  return response.moveaxis(-3, -1);
}

Series freqresp(const PoleZero &system, const Series &omega) {
  PLUSS_CHECK(omega.defined(), "eng::lti::freqresp: omega must be defined");
  PLUSS_CHECK(omega.dim() == 1 && omega.numel() > 0,
              "eng::lti::freqresp: omega must be a non-empty vector");
  PLUSS_CHECK(omega.dtype().is_floating() && !omega.dtype().is_complex(),
              "eng::lti::freqresp: omega must have a real floating dtype");
  PLUSS_CHECK(omega.platform() == system.poles().platform(),
              "eng::lti::freqresp: omega and system must share one platform");

  const ScalarType dtype = response_dtype(system, omega);
  const Series jw = omega.to(dtype).mul(imag_unit(dtype, omega.platform()));
  const Series point =
      system.is_cont() ? jw : jw.mul(system.sample_time()).exp();
  const Series points = point.unsqueeze(-1);
  Series numerator =
      series::ones({omega.numel()},
                   Config{}.with_dtype(dtype).with_platform(omega.platform()));
  if (system.zeros().numel() > 0)
    numerator = points.sub(system.zeros().to(dtype).unsqueeze(0)).prod(-1);
  Series denominator =
      series::ones({omega.numel()},
                   Config{}.with_dtype(dtype).with_platform(omega.platform()));
  if (system.poles().numel() > 0)
    denominator = points.sub(system.poles().to(dtype).unsqueeze(0)).prod(-1);
  return numerator.mul(system.gain().to(dtype)).div(denominator);
}

} // namespace pluss::subsp::eng::lti
