<!-- generated from files_and_time.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Files, missing data and time

[Back to PlussMic by example](../guide.md)

Real data comes from files, has holes, and has dates. This page reads a messy CSV, repairs it, summarises it, writes it back and checks the round trip; and then turns daily prices into a monthly view.

## The mental model

A loaded frame remembers which values are missing. `NULL` is a state of the value, not a number, and the table operations (`aggregate`, `group`, `fillnull`, `dropnull`) respect it. The numbers underneath, which you reach with `as_series()`, are an array with a placeholder where a value is missing; that is the escape hatch to the full `Series` API, and it does not know about missingness. Time works the same way: dates live in a `Timeline` feature, numeric streams are read with `.ts()`, and resampling needs the dates on the case axis.

## A file round trip

Six lines of a CSV with a missing station, a missing temperature and a missing humidity. The program loads it, drops the row without a station, fills the remaining holes with each feature's mean, summarises per station, saves the clean table and reads it back.

```cpp
#include <pluss/pluss.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

using pluss::table::DataFrame;

int main() {
  const auto dir = std::filesystem::temp_directory_path();
  const auto raw_path = dir / "pluss_guide_raw.csv";
  const auto clean_path = dir / "pluss_guide_clean.csv";
  {
    std::ofstream file(raw_path);
    file << "station,date,temperature,humidity\n"
            "north,2026-11-01,12.5,81\n"
            "north,2026-11-02,,79\n"
            "south,2026-11-01,15.0,70\n"
            ",2026-11-02,14.2,72\n"
            "south,2026-11-03,16.1,\n"
            "north,2026-11-03,11.8,85\n";
  }

  // 1. Read it. The frame knows its shape and the type of every feature.
  auto raw = DataFrame::load_csv(raw_path.string());
  std::cout << "read " << raw.ncases() << " rows, " << raw.nfeatures() << " features\n";
  std::cout << raw.to_string() << "\n";

  // 2. Repair: a row without a station is unusable and is dropped; a missing measurement becomes the feature's own mean.
  auto kept = raw.dropnull({"station"}, true);
  // aggregate skips missing values; reading a column through as_series() would see a placeholder zero where a value is missing.
  auto means = kept.aggregate({{"temperature", "temperature", DataFrame::SummaryOp::Mean}, {"humidity", "humidity", DataFrame::SummaryOp::Mean}}, false);
  const double temp_mean = means.features("temperature").as_series().items<double>()[0];
  const double hum_mean = means.features("humidity").as_series().items<double>()[0];
  // humidity was read as a whole-number feature, so fill it with a whole number: round the mean yourself.
  const double hum_fill = std::round(hum_mean);
  auto clean = kept.fillnull(temp_mean, {"temperature"}).fillnull(hum_fill, {"humidity"});
  std::cout << "after cleaning: " << clean.ncases() << " rows; filled temperature with " << temp_mean << " and humidity with " << hum_fill << " (mean " << hum_mean << ")" << "\n";
  std::cout << clean.to_string() << "\n";

  // 3. Summarise per station.
  auto report = clean.group({.dropnull = true, .sort = true})
                    .by({"station"})
                    .agg({{"days", "date", DataFrame::GroupOp::Count}, {"mean_temp", "temperature", DataFrame::GroupOp::Mean}, {"max_humidity", "humidity", DataFrame::GroupOp::Max}});
  std::cout << report.to_string() << "\n";

  // 4. Save and read back: what comes back must equal what was saved.
  clean.save_csv(clean_path.string());
  auto again = DataFrame::load_csv(clean_path.string());
  std::cout << "reloaded " << again.ncases() << " rows; humidity total " << again.features("humidity").as_series().sum().item<double>() << " vs " << clean.features("humidity").as_series().sum().item<double>() << "\n";
  std::filesystem::remove(raw_path);
  std::filesystem::remove(clean_path);
}
```

Output

```text
read 6 rows, 4 features
=================================================
     | station  date        temperature  humidity
-------------------------------------------------
0    | north    2026-11-01  12.5         81      
1    | north    2026-11-02  NULL         79      
2    | south    2026-11-01  15           70      
3    | NULL     2026-11-02  14.2         72      
4    | south    2026-11-03  16.1         NULL    
5    | north    2026-11-03  11.8         85      
-------------------------------------------------
[6 cs x 4 feat]
after cleaning: 5 rows; filled temperature with 13.85 and humidity with 79 (mean 78.75)
=================================================
     | station  date        temperature  humidity
-------------------------------------------------
0    | north    2026-11-01  12.5         81      
1    | north    2026-11-02  13.85        79      
2    | south    2026-11-01  15           70      
4    | south    2026-11-03  16.1         79      
5    | north    2026-11-03  11.8         85      
-------------------------------------------------
[5 cs x 4 feat]
===============================================
     | station  days  mean_temp    max_humidity
-----------------------------------------------
0    | north    3     12.71666667  85          
1    | south    2     15.55        79          
-----------------------------------------------
[2 cs x 4 feat]
reloaded 5 rows; humidity total 394 vs 394
```
The frame arrives with `NULL` where the file was empty. `dropnull({"station"}, true)` removes the one row whose station is missing, so five rows remain. The fills use each feature's mean: `13.85` for temperature, and for humidity a mean of `78.75` that the program rounds to `79` itself, because the humidity feature was read as whole numbers and so cannot hold a fraction. The per-station report then has two rows, and the reloaded file has five rows and the same humidity total as the table that was saved, which is the check that the round trip lost nothing.

## Missing is not zero

The means above came from `aggregate`. The next program shows why.

```cpp
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>

using pluss::table::DataFrame;

int main() {
  // Write a four-row file with one missing temperature and read it back.
  const auto path = std::filesystem::temp_directory_path() / "pluss_guide_nulls.csv";
  {
    std::ofstream file(path);
    file << "station,temperature\nnorth,12.5\nnorth,\nsouth,15.0\nsouth,16.1\n";
  }
  auto readings = DataFrame::load_csv(path.string());
  std::filesystem::remove(path);
  std::cout << readings.to_string() << "\n";
  auto table_mean = readings.aggregate({{"mean", "temperature", DataFrame::SummaryOp::Mean}}, false);
  std::cout << "mean through the table (skips the missing row): " << table_mean.features("mean").as_series().items<double>()[0] << "\n";
  std::cout << "mean of the raw series underneath (counts it as 0): " << readings.features("temperature").as_series().mean().item<double>() << "\n";
}
```

Output

```text
===========================
     | station  temperature
---------------------------
0    | north    12.5       
1    | north    NULL       
2    | south    15         
3    | south    16.1       
---------------------------
[4 cs x 2 feat]
mean through the table (skips the missing row): 14.5333
mean of the raw series underneath (counts it as 0): 10.9
```
Through the table, the mean of the three known temperatures is `14.5333`. Through `as_series()` the missing value is the placeholder zero, and the mean of four numbers drops to `10.9`. Take reductions through the table (`aggregate`, `group`) whenever values can be missing, and treat `as_series()` as the tool for work that is already free of nulls.

## From daily to monthly

Eight prices on irregular days. The program computes day-to-day returns, then puts the dates on the case axis and resamples to calendar months.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;

int main() {
  // Eight trading days spread over three months.
  auto prices = DataFrame({
      {"date", pluss::make_timeline({"2026-01-05T00:00:00Z", "2026-01-20T00:00:00Z", "2026-01-28T00:00:00Z", "2026-02-03T00:00:00Z",
                                       "2026-02-17T00:00:00Z", "2026-03-02T00:00:00Z", "2026-03-16T00:00:00Z", "2026-03-30T00:00:00Z"})},
      {"close", pluss::make_series({100.0, 104.0, 102.0, 108.0, 111.0, 107.0, 112.0, 118.0})},
      {"volume", pluss::make_series({1200.0, 900.0, 1500.0, 1100.0, 1300.0, 800.0, 1000.0, 1400.0})},
  });
  std::cout << prices.to_string() << "\n";

  // 1. Day-to-day returns of the closing price (numeric streams use .ts()).
  auto returns = prices.features("close").ts().returns(1);
  std::cout << "returns: " << pluss::series::print(returns.as_series()) << "\n";

  // 2. Put the dates on the case axis and resample to months: mean close and total volume per month.
  auto indexed = prices.labels().set("date", false).features({"close", "volume"});
  auto monthly = indexed.ts().resample("MS").mean();
  std::cout << monthly.to_string() << "\n";
  auto monthly_volume = indexed.ts().resample("MS").sum();
  std::cout << monthly_volume.to_string() << "\n";
}
```

Output

```text
================================
     | date        close  volume
--------------------------------
0    | 2026-01-05  100    1200  
1    | 2026-01-20  104    900   
2    | 2026-01-28  102    1500  
3    | 2026-02-03  108    1100  
4    | 2026-02-17  111    1300  
5    | 2026-03-02  107    800   
6    | 2026-03-16  112    1000  
7    | 2026-03-30  118    1400  
--------------------------------
[8 cs x 3 feat]
returns: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[nan, 0.0400, -0.0192, 0.0588, 0.0278, -0.0360, 0.0467, 0.0536]
=====================================
           | close        volume     
-------------------------------------
2026-01-01 | 102          1200       
2026-02-01 | 109.5        1200       
2026-03-01 | 112.3333333  1066.666667
-------------------------------------
[3 cs x 2 feat]
==========================
           | close  volume
--------------------------
2026-01-01 | 306    3600  
2026-02-01 | 219    2400  
2026-03-01 | 337    3200  
--------------------------
[3 cs x 2 feat]
```
The first return is `nan` because the first day has no predecessor. Resampling with `"MS"` (month start) takes the mean of the closing price within each month, and a second resample with `sum` totals the volume: January averages `102` over three days, February `109.5` over two, March `112.33` over three. Resampling needs a time axis, so the program moves `date` to the case axis with `labels().set("date", false)` first; without that step there is nothing to bin.

## Decisions

- **Missing values:** decide the rule once (drop the row, fill with a statistic, keep the null) and apply it with `dropnull` and `fillnull` before summarising.
- **Fill type:** a whole-number feature takes a whole-number fill; round it yourself so the table shows what is stored.
- **Round trips:** always read back what you wrote and compare a total.
- **Time:** parse dates into a `Timeline` early, keep them on the case axis for resampling, and use `.ts()` for returns and rolling work.

## Where next

Reference pages: [`load_csv`](../api/dataframe/load_csv.md), [`dropnull`](../api/dataframe/dropnull.md), [`fillnull`](../api/dataframe/fillnull.md), [`group`](../api/dataframe/group.md), [`save_csv`](../api/dataframe/save_csv.md), [`aggregate`](../api/dataframe/aggregate.md). Back to [PlussMic by example](../guide.md).
