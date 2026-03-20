#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  using namespace pluss;

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);

  nn::Schedule model;
  model.then("encoder", nn::Fusion(2, 8, false, cfg, 11ULL))
      .insert(1, "activation", nn::Tanh())
      .then("head", nn::Fusion(8, 1, false, cfg, 13ULL));

  const Series x = make_series(
      std::vector<float>{
          -2.0f, -1.0f,
          -1.0f,  0.5f,
           0.0f,  0.0f,
           1.0f, -0.5f,
           2.0f,  1.0f},
      {5, 2},
      cfg);
  const Series y_target = make_series(
      std::vector<float>{
          -3.5f, -1.0f, 0.5f, 2.0f, 4.5f},
      {5, 1},
      cfg);

  const Series y_initial = model->forward(x);
  const double initial_loss = nn::losses::MSE(y_initial, y_target).to(pluss::pCPU, ScalarType::Float64).item<double>();
  model->eval();
  const Series y_final = model->forward(x);
  const double final_loss = nn::losses::MSE(y_final, y_target).to(pluss::pCPU, ScalarType::Float64).item<double>();

  nn::Schedule typed;
  typed.then_as<std::tuple<int, int>, int>("sum", [](std::tuple<int, int> pair) {
    return std::get<0>(pair) + std::get<1>(pair);
  });
  typed.then_as<int, int>("double", [](int v) { return 2 * v; });
  const int typed_out = typed.run<int>(7, 5);

  std::cout << "nn_schedule_demo\n";
  std::cout << "surface: pluss::nn::Schedule via #include <pluss/pluss.h> + <pluss/ai.h>\n";
  std::cout << "stages: " << model->size() << "\n";
  std::cout << "stage names: ";
  for (const auto& stage : model->stage_manifest()) {
    std::cout << stage.name << " ";
  }
  std::cout << "\n";
  std::cout << "mse (inference path): " << final_loss << "\n";
  std::cout << "mse (initial): " << initial_loss << "\n";
  std::cout << "final prediction: " << y_final << "\n";
  std::cout << "typed schedule run<int>(7,5): " << typed_out << "\n";

  return 0;
}
