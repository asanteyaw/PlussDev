// Availability from heartbeats: a service reports in about every minute. Find the gaps, rank the outages and compute uptime.
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Heartbeats as seconds after 08:00:00 UTC on 2026-11-13. Normally one a minute; there are three gaps.
  const std::int64_t base = 1794528000 + 8 * 3600;
  std::vector<double> offsets;
  for (int s = 0; s <= 300; s += 60) offsets.push_back(s);            // 08:00 - 08:05
  for (int s = 600; s <= 780; s += 60) offsets.push_back(s);          // gap of 5 min, then 08:10 - 08:13
  for (int s = 1200; s <= 1500; s += 60) offsets.push_back(s);        // gap of 7 min, then 08:20 - 08:25
  for (int s = 1560; s <= 1680; s += 60) offsets.push_back(s);        // 08:26 - 08:28
  for (int s = 1980; s <= 2100; s += 60) offsets.push_back(s);        // gap of 5 min, then 08:33 - 08:35
  auto beats = pluss::Timeline::from_epoch_seconds(pluss::make_series(offsets).to(ScalarType::Int64).add(base));
  const auto clock = beats.strftime("%H:%M").host();   // keep the owning copy alive while reading it
  std::cout << beats.numel() << " heartbeats from " << clock.vector().front() << " to " << clock.vector().back() << "\n";

  // 1. The gap before each heartbeat is the difference of neighbouring instants: shift the axis by one and subtract.
  const auto n = beats.numel();
  auto later = pluss::Timeline::from_epoch_seconds(pluss::make_series(offsets).slice(0, 1, n).to(ScalarType::Int64).add(base));
  auto earlier = pluss::Timeline::from_epoch_seconds(pluss::make_series(offsets).slice(0, 0, n - 1).to(ScalarType::Int64).add(base));
  auto gaps = later - earlier;                                       // a Duration per pair of neighbouring heartbeats
  const auto gap_ns = gaps.host();
  std::vector<double> gap_minutes;
  for (const auto ns : gap_ns) gap_minutes.push_back(static_cast<double>(ns) / 60e9);
  auto gap = pluss::make_series(gap_minutes);
  show("minutes between heartbeats", gap);

  // 2. An outage is a gap longer than two expected beats: compare the Duration with the limit.
  auto outage = gaps.gt(pluss::duration::minutes(2));
  show("outage?", outage);
  auto outage_minutes = gap.masked_select(outage);
  show("outage lengths (minutes)", outage_minutes);
  std::cout << "outages " << outage_minutes.numel() << ", longest " << outage_minutes.max().item<double>() << " min, total down " << outage_minutes.sum().item<double>() << " min\n";

  // 3. When did the worst outage start? The start is the earlier heartbeat of the pair.
  auto worst = gap.argmax().item<std::int64_t>();
  const auto began = earlier.strftime("%H:%M").host();
  std::cout << "worst outage began at " << began.vector()[static_cast<std::size_t>(worst)] << "\n";

  // 4. Availability over the observed window, taking the minutes beyond one expected beat per outage as down time.
  const double window = (static_cast<double>(offsets.back()) - offsets.front()) / 60.0;
  const double down = outage_minutes.sub(1.0).sum().item<double>();
  std::cout << "window " << window << " min, down " << down << " min, availability " << 100.0 * (1.0 - down / window) << " %\n";
}
