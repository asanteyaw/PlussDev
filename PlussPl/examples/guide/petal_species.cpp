// Tell three species apart from four measurements: the views built for several variables at once, each with its own way of showing separation.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}


int main() {
  auto loaded = flash::load_dataset("petals");
  const auto petals = loaded->frame();
  auto species = petals.features("species").as_note();
  std::vector<pluss::Series> measures{petals.features("sepal_length").as_series(), petals.features("sepal_width").as_series(), petals.features("petal_length").as_series(), petals.features("petal_width").as_series()};
  auto means = petals.group({.dropnull = true, .sort = true}).by({"species"}).agg({{"petal_length", "petal_length", pluss::table::DataFrame::GroupOp::Mean}, {"petal_width", "petal_width", pluss::table::DataFrame::GroupOp::Mean}});
  std::cout << means.to_string() << "\n";

  auto parallel = studio("petal_parallel");
  flash::parallelplot(parallel, measures, species);
  parallel.show();
  auto andrews = studio("petal_andrews");
  flash::andrews_curves(andrews, measures, species);
  andrews.show();
  auto radviz = studio("petal_radviz");
  flash::radviz(radviz, measures, species);
  radviz.show();
  auto joint = studio("petal_joint");
  flash::jointplot(joint, measures[2], measures[3], "scatter");
  joint.show();
  auto summary = studio("petal_summary");
  flash::summaryplot(summary, measures, {"sepal length", "sepal width", "petal length", "petal width"});
  summary.show();
  std::cout << "five multivariate figures written\n";
}
