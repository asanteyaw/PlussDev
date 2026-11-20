// Does a bigger bill mean a bigger tip? Explore a restaurant dataset: the raw relationship split by day, the fitted line, what the fit leaves
// unexplained, and whether those leftovers look normal.
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
  auto loaded = flash::load_dataset("tips");
  const auto tips = loaded->frame();
  auto bill = tips.features("total_bill").as_series();
  auto tip = tips.features("tip").as_series();
  auto day = tips.features("day").as_note();
  std::cout << tips.ncases() << " parties; mean bill " << bill.mean().item<double>() << ", mean tip " << tip.mean().item<double>() << "\n";

  // 1. The relationship, with one colour per day of the week.
  auto fig = studio("tips_study");
  fig.mosaic(2, 2);
  fig.headline("Bill and tip");
  auto &raw = fig.pane2("Tip against bill, by day");
  flash::scatterplot(raw, bill, tip, day);
  raw.x_label("total bill").y_label("tip");

  // 2. The straight-line fit and its numbers: the slope is the tip per unit of bill.
  auto &fit = fig.pane2("Fitted line");
  flash::regplot(fit, bill, tip);
  fit.x_label("total bill").y_label("tip");
  const double mean_x = bill.mean().item<double>(), mean_y = tip.mean().item<double>();
  const double slope = ((bill - mean_x) * (tip - mean_y)).sum().item<double>() / (bill - mean_x).square().sum().item<double>();
  const double intercept = mean_y - slope * mean_x;
  std::cout << "tip = " << intercept << " + " << slope << " x bill\n";
  std::cout << "correlation " << ((bill - mean_x) * (tip - mean_y)).sum().item<double>() / std::sqrt((bill - mean_x).square().sum().item<double>() * (tip - mean_y).square().sum().item<double>()) << "\n";

  // 3. What the line leaves: residuals against the bill (a pattern here means the line is the wrong shape) and their histogram.
  auto &residual = fig.pane2("Residuals");
  flash::residplot(residual, bill, tip);
  auto &hist = fig.pane2("Distribution of the residuals");
  flash::resid_histplot(hist, bill, tip);
  fig.show();

  // 4. A quantile-quantile plot says whether the residuals are normal: points on the diagonal mean yes.
  auto qq = studio("tips_qq");
  auto residuals = tip - (bill * slope + intercept);
  auto &q = qq.pane2("Normal quantile plot of the residuals");
  flash::qqplot(q, residuals);
  qq.show();
  std::cout << "largest residual " << residuals.abs().max().item<double>() << ", residual std " << residuals.std().item<double>() << "\n";
}
