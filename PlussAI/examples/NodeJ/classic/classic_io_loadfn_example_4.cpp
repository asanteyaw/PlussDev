#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <fstream>
#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const std::string path = "/tmp/pluss_classic_io_load_example_4.bundle";
  std::ofstream not_a_bundle(path, std::ios::binary);
  not_a_bundle << "this file was never written by io::save";
  not_a_bundle.close();

  const cls::StatusOr<cls::io::FlowBundle> loaded = cls::io::load(path);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.status().message << "\"\n";

  return 0;
}
