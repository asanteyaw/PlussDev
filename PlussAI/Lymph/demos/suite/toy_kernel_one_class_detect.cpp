#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::Config;
  using pluss::ScalarType;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values{};
  values.reserve(66);
  for (int i = 0; i < 30; ++i) {
    values.push_back(static_cast<double>(i) * 0.01);
    values.push_back(static_cast<double>(i) * 0.02);
  }
  values.push_back(8.0);
  values.push_back(9.0);
  values.push_back(-7.0);
  values.push_back(8.5);
  values.push_back(7.5);
  values.push_back(-8.0);

  lymph::KernelFamilyOptions kernel{};
  kernel.nu = 0.08;
  kernel.gamma = 0.2;
  const lymph::detect::Result outliers =
      lymph::detect::model(lymph::model::one_class_svm(kernel))
          .run(lymph::FeatureBatch{
              .x = pluss::make_series(values, {33, 2}, f64),
              .layout = lymph::ObservationLayout::Rows,
          });

  std::cout << "one-class flagged: " << outliers.flagged_count() << "\n";
  return 0;
}
