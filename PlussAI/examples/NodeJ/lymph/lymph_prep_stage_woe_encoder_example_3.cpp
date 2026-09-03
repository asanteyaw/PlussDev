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

  const pluss::Series channel = pluss::make_series({0,1,2,0,1,2,3,3,0,1}, {10}, f64);
  const pluss::Series bal = pluss::make_series({1.,2.,3.,4.,5.,6.,7.,8.,9.,10.}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,1,0,0,1,1,1,0,0}, {10}, f64);
  const DataFrame x({{"channel", Thread(channel)}, {"bal", Thread(bal)}});

  // WoE on the categorical channel, robust scaling on the balance column.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::robust_scaler(), lymph::prep::route::names({"bal"}))
       .append(lymph::prep::stage::woe_encoder(), lymph::prep::route::names({"channel"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"channel", Thread(channel)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
