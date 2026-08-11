#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Series x = pluss::series::linspace(0.1, 5.0, 60, ScalarType::Float64);
  const Series y = x.log().mul(3.0).add(x.sqrt());
  const DataFrame raw({{"x", Thread(x)}, {"y", Thread(y)}});

  lymph::Payload data = lymph::payload::make(raw).features({"x"}).label("y");
  lymph::Workflow wf = lymph::workflow::make(lymph::models::linear_regression());
  wf.add(lymph::stage::custom("log1p_all", [](const Series& input) {
    return input.add(1.0).log();
  }));

  wf.learn_(data);
  const Series pred = wf.infer(data);

  const std::filesystem::path bundle =
      std::filesystem::temp_directory_path() / "plussai_toy_custom_stage_payload_save.bundle";
  wf.save(bundle.string(), data);
  std::filesystem::remove(bundle);
  std::filesystem::remove(bundle.string() + ".policy");

  std::cout << "custom stage predictions: " << pred.numel() << "\n";
  std::cout << "opaque transforms: " << wf.has_opaque_transforms() << "\n";
  return 0;
}
