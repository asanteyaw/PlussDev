#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series score = pluss::make_series({0.1, 0.3, 0.55, 0.7, 0.9, 0.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,1,1,1,0}, {6}, f64);
  const DataFrame x({{"score", Thread(score)}, {"region", Thread(region)}});

  lymph::options::ThresholdBinarizer o{};
  o.threshold = 0.5;
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::threshold_binarizer(o), lymph::prep::route::names({"score"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"score", Thread(score)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
