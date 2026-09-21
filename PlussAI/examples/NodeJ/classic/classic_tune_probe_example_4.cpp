#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::ProbeSpecSampler sampler =
      [](std::int64_t, std::uint64_t) -> cls::StatusOr<cls::ModelSpec> {
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  };

  const cls::StatusOr<cls::TuneReport> report = cls::probe(sampler, 0, batch);
  std::cout << "ok = " << std::boolalpha << report.ok() << "\n";
  std::cout << "message = \"" << report.status().message << "\"\n";

  return 0;
}
