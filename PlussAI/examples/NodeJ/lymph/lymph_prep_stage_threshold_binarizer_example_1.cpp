#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // threshold_binarizer turns a numeric column into a 0/1 flag about a cut
  // point. It is stateless -- the threshold is a fixed option, not learned.
  const pluss::Series balance = pluss::make_series({-50., -5., 0., 10., 40., 120.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"balance", Slate(balance)}});

  lymph::options::ThresholdBinarizer o{};
  o.threshold = 5.0;
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::threshold_binarizer(o), lymph::prep::route::names({"balance"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"balance", Slate(balance)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
