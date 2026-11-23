#include <pluss/tils/info.h>
#include <pluss/tils/kit/rng_seed.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "core info test failed: " << message << '\n';
    std::exit(1);
  }
}

}  // namespace

int main() {
  const pluss::Version v = pluss::version();
  require(v.major >= 0 && v.minor >= 0 && v.patch >= 0, "the version numbers are not negative");
  require(!v.text.empty(), "the version has a text form");
  require(v.text.find(std::to_string(v.major)) == 0, "the text form starts with the major number");
  require(v.channel == "steady" || v.channel == "transient" || v.channel == "development", "the channel is one of the known names");
  require(!v.compiler.empty(), "the compiler is named");

  const auto info = pluss::tils::info::build();
  require(info.version == std::string(v.text), "build() reports the same version");
  require(info.system != "unknown", "the operating system is recognised");
  require(info.architecture != "unknown", "the architecture is recognised");
  require(!info.platforms.empty() && info.platforms.front() == "cpu", "cpu is always the first platform");
  const auto text = pluss::tils::info::describe(info);
  require(text.find(info.version) != std::string::npos && text.find("platforms:") != std::string::npos,
          "describe() prints the version and the platforms");

  // Seed derivation: a pure function of (base, index), and the process stream is built on it.
  using pluss::tils::derive_seed;
  require(derive_seed(42, 7) == derive_seed(42, 7), "the same pair gives the same seed");
  require(derive_seed(42, 7) != derive_seed(42, 8), "different indexes give different seeds");
  require(derive_seed(42, 7) != derive_seed(43, 7), "different bases give different seeds");
  pluss::tils::rng_seed(1234);
  const auto first = pluss::tils::next_rng_seed();
  const auto second = pluss::tils::next_rng_seed();
  require(first == derive_seed(1234, 1) && second == derive_seed(1234, 2),
          "the process stream is derive_seed with a running counter");

  // The runs of a sweep get the same seeds whichever threads run them.
  constexpr std::uint64_t runs = 64;
  std::vector<std::uint64_t> serial(runs), parallel(runs);
  for (std::uint64_t i = 0; i < runs; ++i) serial[i] = derive_seed(99, i);
  std::vector<std::thread> workers;
  for (std::uint64_t t = 0; t < 4; ++t) {
    workers.emplace_back([&, t] {
      for (std::uint64_t i = runs; i-- > 0;) {
        if (i % 4 == t) parallel[i] = derive_seed(99, i);
      }
    });
  }
  for (auto& w : workers) w.join();
  require(serial == parallel, "derived seeds do not depend on the thread that computes them");
  require(std::set<std::uint64_t>(serial.begin(), serial.end()).size() == runs, "the runs' seeds are all different");

  std::cout << pluss::tils::info::describe();
  std::cout << "core info tests passed\n";
  return 0;
}
