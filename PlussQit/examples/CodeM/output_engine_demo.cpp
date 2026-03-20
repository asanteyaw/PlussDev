#include <pluss/pluss.h>

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

int main() {
  pluss::OutputOptions opts;
  opts.separator = " | ";
  opts.levels_enabled = true;
  opts.min_level = pluss::OutputLevel::Debug;
  opts.auto_flush = false;
  opts.level_prefixes = {"[T]", "[D]", "[I]", "[W]", "[E]"};

  pluss::OutputEngine out{std::cout, opts};

  const std::vector<double> xs{1.25, 2.50, 5.00};
  const std::map<std::string, int> counts{{"A", 3}, {"B", 7}};
  const std::optional<double> maybe{3.14159};
  const std::variant<int, std::string> tag{"alpha"};
  const auto row = std::tuple{42, std::string("tuple"), 9.5};

  out.writeln("vector", xs);
  out.writeln("map", counts, "optional", maybe, "variant", tag, "tuple", row);
  out.writef("name={} age={}", "Ada", 31).endln();

  auto s = pluss::make_series(std::vector<double>{10.0, 12.0, 15.0}, {3});
  out.info("Series example:", s);
  out.debugf("size={} first={}", xs.size(), xs.front());
  out.warn("just a sample warning");
  out.errorf("status={} code={}", "failed", 503);

  return 0;
}

