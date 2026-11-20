// How did the class do? The distribution of one exam seen six ways, with the numbers that summarise it.
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
  auto loaded = flash::load_dataset("exams");
  const auto exams = loaded->frame();
  auto final_exam = exams.features("final_exam").as_series();
  auto midterm = exams.features("midterm_1").as_series();
  std::cout << final_exam.numel() << " students; mean " << final_exam.mean().item<double>() << ", median " << final_exam.median().item<double>() << ", std " << final_exam.std().item<double>() << "\n";
  std::cout << "quartiles " << final_exam.quantile(0.25).item<double>() << " " << final_exam.quantile(0.5).item<double>() << " " << final_exam.quantile(0.75).item<double>() << "\n";

  auto fig = studio("exam_scores");
  fig.mosaic(2, 3);
  fig.headline("Final exam scores");
  auto &hist = fig.pane2("Histogram");
  flash::histplot(hist, final_exam, 15);
  auto &dist = fig.pane2("Distribution");
  flash::displot(dist, final_exam, 15);
  auto &kde = fig.pane2("Kernel density");
  flash::kdeplot(kde, final_exam);
  auto &ecdf = fig.pane2("Empirical CDF");
  flash::ecdfplot(ecdf, final_exam);
  auto &rug = fig.pane2("Individual scores");
  flash::rugplot(rug, final_exam);
  auto &bucket = fig.pane2("Native bucket trace");
  bucket.bucket(final_exam, 15);
  fig.show();

  // The ECDF answers "what share scored below x": read it from the data.
  for (double cut : {70.0, 75.0, 80.0})
    std::cout << "share below " << cut << ": " << (final_exam < cut).to(pluss::ScalarType::Float64).mean().item<double>() << "\n";
  std::cout << "correlation of the final with the first midterm: " << ((final_exam - final_exam.mean()) * (midterm - midterm.mean())).sum().item<double>() / std::sqrt((final_exam - final_exam.mean()).square().sum().item<double>() * (midterm - midterm.mean()).square().sum().item<double>()) << "\n";
}
