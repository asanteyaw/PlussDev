// The implied-volatility surface of an options market: volatility against moneyness and expiry as a surface, a wireframe, contours,
// ridge lines, bars and a 3D histogram, each answering a different question about the same grid.
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
  auto loaded = flash::load_dataset("options_chain");
  const auto chain = loaded->frame();
  auto moneyness = chain.features("moneyness").as_series();
  auto iv = chain.features("iv").as_series();
  auto volume = chain.features("volume").as_series();
  // Time to expiry in days: the difference of two Timelines is a Duration, and its host copy holds nanoseconds.
  const auto nanos = (chain.features("expiry_date").as_timeline() - chain.features("quote_time").as_timeline()).host();
  std::vector<double> days;
  for (const auto ns : nanos) days.push_back(static_cast<double>(ns) / 86400e9);
  auto expiry = pluss::make_series(days);
  // A few quotes in the data are stamped after their own expiry: drop them, and say how many.
  auto live = expiry.gt(0.0);
  std::cout << "quotes after expiry dropped: " << live.logical_not().sum().item<double>() << " of " << expiry.numel() << "\n";
  moneyness = moneyness.masked_select(live);
  iv = iv.masked_select(live);
  volume = volume.masked_select(live);
  expiry = expiry.masked_select(live);

  // Grid: 12 moneyness bins by 8 expiry bins; each cell holds the mean implied volatility of the quotes that fall in it.
  const int bins = 12, terms_n = 8;
  const double lo = moneyness.min().item<double>(), hi = moneyness.max().item<double>() + 1e-9;
  const double dlo = expiry.min().item<double>(), dhi = expiry.max().item<double>() + 1e-9;
  auto bin = ((moneyness - lo) / (hi - lo) * bins).floor().to(pluss::ScalarType::Int64);
  auto tbin = ((expiry - dlo) / (dhi - dlo) * terms_n).floor().to(pluss::ScalarType::Int64);
  auto cell = bin * terms_n + tbin;
  auto sums = pluss::series::zeros({bins * terms_n}, pluss::ScalarType::Float64).scatter_add(0, cell, iv);
  auto counts = pluss::series::zeros({bins * terms_n}, pluss::ScalarType::Float64).scatter_add(0, cell, pluss::series::ones({iv.numel()}, pluss::ScalarType::Float64));
  // A cell with no quotes takes the market-wide mean volatility, so the surface has no holes.
  auto surface = ((sums + counts.eq(0.0).to(pluss::ScalarType::Float64) * iv.mean().item<double>()) / counts.clamp_min(1.0)).reshape({bins, terms_n});
  auto traded = pluss::series::zeros({bins * terms_n}, pluss::ScalarType::Float64).scatter_add(0, cell, volume).reshape({bins, terms_n});
  auto strikes = pluss::series::arange(0, bins, pluss::ScalarType::Float64) * ((hi - lo) / bins) + lo;
  auto terms = pluss::series::arange(0, terms_n, pluss::ScalarType::Float64) * ((dhi - dlo) / terms_n) + dlo;
  std::cout << "expiries from " << dlo << " to " << dhi << " days; surface " << surface.size(0) << " x " << surface.size(1) << ", volatility from " << surface.min().item<double>() << " to " << surface.max().item<double>() << "\n";
  std::cout << "smile depth at the shortest expiry (edge minus centre): " << (surface.select(1, 0).select(0, 0).item<double>() - surface.select(1, 0).select(0, bins / 2).item<double>()) << "\n";

  auto fig = studio("vol_surface");
  fig.mosaic(2, 3);
  fig.headline("Implied volatility surface");
  auto &sheet = fig.pane3("Sheet");
  sheet.sheet(strikes, terms, surface.t());
  auto &lattice = fig.pane3("Lattice");
  lattice.lattice(strikes, terms, surface.t());
  auto &wire = fig.pane3("plot_wireframe");
  flash::plot_wireframe(wire, strikes, terms, surface.t());
  auto &surf = fig.pane3("plot_surface");
  flash::plot_surface(surf, strikes, terms, surface.t());
  auto &cont = fig.pane3("contour3D");
  flash::contour3D(cont, strikes, terms, surface.t(), 6);
  auto &ridge = fig.pane3("Ridge lines");
  ridge.ridge3(strikes, terms, surface.t());
  fig.show();

  auto fig2 = studio("vol_volume");
  fig2.mosaic(1, 3);
  fig2.headline("Where the trading is");
  auto &spire = fig2.pane3("Spire: volume by cell");
  spire.spire3(strikes, terms, traded.t());
  auto &bars = fig2.pane3("bar3d");
  flash::bar3d(bars, strikes, terms, traded.t());
  auto &hist = fig2.pane3("hist3d: quotes by moneyness and volatility");
  flash::hist3d(hist, moneyness, iv);
  fig2.show();
}
