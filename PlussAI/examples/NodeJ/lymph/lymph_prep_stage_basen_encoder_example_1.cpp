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

  // Five region categories become 3 binary-digit columns (base 2, the default):
  // ceil(log2(5+1)) = 3, versus the 5 columns one-hot would spend.
  const pluss::Series region = pluss::make_series({0,1,2,3,4,0,1,2}, {8}, f64);
  const pluss::Series spend  = pluss::make_series({120.,200.,90.,260.,140.,300.,110.,240.}, {8}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,0,0,0}, {8}, f64);
  const DataFrame x({{"region", Thread(region)}, {"spend", Thread(spend)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::basen_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Thread(region)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
