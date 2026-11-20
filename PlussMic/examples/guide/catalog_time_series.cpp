// Daily readings to monthly statistics in the Catalog model: a time stream, a value stream, resampling to calendar months.
#include <pluss/pluss.h>

#include <iostream>
#include <map>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog daily({"when"_k = Stream(pluss::make_timeline({"2026-01-05T00:00:00Z", "2026-01-20T00:00:00Z", "2026-01-28T00:00:00Z", "2026-02-03T00:00:00Z",
                                                          "2026-02-17T00:00:00Z", "2026-03-02T00:00:00Z", "2026-03-16T00:00:00Z", "2026-03-30T00:00:00Z"})),
                 "close"_k = Stream(pluss::make_series({100.0, 104.0, 102.0, 108.0, 111.0, 107.0, 112.0, 118.0}))});
  std::cout << daily << "\n";

  // 1. Day-to-day returns of the value stream.
  auto returns = daily.keys("close").ts().returns(1);
  std::cout << "returns: " << pluss::series::print(returns.as_series()) << "\n";

  // 2. A rolling mean over three observations, and the previous value as a lag stream.
  std::cout << "rolling mean (3): " << pluss::series::print(daily.keys("close").ts().rolling(3).as_series()) << "\n";
  std::cout << "lag 1: " << pluss::series::print(daily.keys("close").ts().lag(1).as_series()) << "\n";

  // 3. Calendar features of the time stream become new streams: the month's name is the grouping key for a monthly summary.
  auto with_month = daily.set("month", daily.keys("when").tl().month_name());
  std::cout << "months: " << pluss::note::print(with_month.keys("month").as_note()) << "\n";
  std::map<std::string, Catalog> by_month;
  for (const auto &[name, part] : with_month.group().by("month")) by_month.emplace(name, part);
  for (const auto &month : {"January", "February", "March"}) {
    const auto &part = by_month.at(month);
    std::cout << month << ": " << part.nrows() << " observations, mean close " << part.keys("close").mean().as_series().items<double>()[0] << ", last close "
              << part.keys("close").tail(1).as_series().items<double>()[0] << "\n";
  }
}
