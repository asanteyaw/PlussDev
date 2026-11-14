<!-- generated from kindle_streams_archives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Kindle: streams and archives

[Back to PlussMic by example](../guide.md)

The two other Kindle types: the `Stream` (one column with a mask, labels and metadata) and the `Archive` (a stack of catalogs, like an Atlas whose members may have different lengths). This page repairs a sensor stream, ranks named stations, scales train, validation and test sets and combines monthly catalogs of different sizes.

## The mental model

A stream is one vector with three optional extras: a validity mask, case labels and a metadata map. Every transformation returns a new stream; the mask and labels travel where they make sense, and the metadata does not (the first program shows it). An archive is a stack of catalogs addressed by a depth stream, and unlike an Atlas it allows catalogs of different record counts.

## Repairing a sensor stream

Eight readings with two dropouts and an impossible spike, labelled with a sensor name and a unit in the metadata.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::kindle::Stream;

static void show(const char *label, const Stream &s) { std::cout << label << ": " << pluss::series::print(s.as_series()) << "\n"; }

int main() {
  // Eight temperature readings with two dropouts and one impossible spike. The mask says which readings are present.
  auto values = pluss::make_series({20.5, 20.7, 0.0, 21.0, 450.0, 21.3, 0.0, 21.6});
  auto present = pluss::make_series({true, true, false, true, true, true, false, true});
  Stream raw(values, present);
  raw = raw.with_meta("sensor", "roof-3").with_meta("unit", "celsius");
  std::cout << "raw: " << raw.nrecords() << " records, " << raw.isnull().as_series().sum().item<double>() << " missing, meta sensor=" << raw.meta("sensor").value_or("?") << "\n";

  // 1. Cap impossible values first: a room reading above 60 is a fault, so clip it to the limit.
  auto capped = raw.clip(0.0, 60.0);
  show("clipped", capped);

  // 2. Fill the dropouts with the last good value.
  auto filled = capped.ffill();
  show("forward-filled", filled);

  // 3. Metadata describes the stream you attached it to; a transformation returns a new stream without it. Look, then re-attach the
  //    provenance and add a note of what was done, so the repaired stream explains itself later.
  std::cout << "metadata after repair: sensor=" << filled.meta("sensor").value_or("(not carried over)") << "\n";
  auto documented = filled.with_meta("sensor", raw.meta("sensor").value_or("?")).with_meta("unit", raw.meta("unit").value_or("?")).with_meta("repair", "clip 0-60, ffill");
  std::cout << "re-attached: sensor=" << documented.meta("sensor").value_or("?") << ", unit=" << documented.meta("unit").value_or("?") << ", repair=" << documented.meta("repair").value_or("?") << "\n";

  // 4. Keep only the trusted readings: those that were present and not above the limit.
  auto trusted_mask = raw.notnull().as_series().logical_and(raw.as_series().lt(60.0));
  auto trusted = raw.filter(trusted_mask);
  show("trusted readings only", trusted);
  std::cout << "trusted " << trusted.nrecords() << " of " << raw.nrecords() << "; mean " << trusted.mean().as_series().items<double>()[0] << "\n";
}
```

Output

```text
raw: 8 records, 2 missing, meta sensor=roof-3
clipped: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[20.5000, 20.7000, 0.0000, 21.0000, 60.0000, 21.3000, 0.0000, 21.6000]
forward-filled: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[20.5000, 20.7000, 20.7000, 21.0000, 60.0000, 21.3000, 21.3000, 21.6000]
metadata after repair: sensor=(not carried over)
re-attached: sensor=roof-3, unit=celsius, repair=clip 0-60, ffill
trusted readings only: Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[20.5000, 20.7000, 21.0000, 21.3000, 21.6000]
trusted 5 of 8; mean 21.02
```
`clip` caps the spike at the 60 limit and `ffill` carries the last good value over each dropout. The program then looks at the metadata after the repair and finds it gone: transformations return a stream without the metadata attached to the original, so the program re-attaches the sensor and unit and adds a note of what was done. The last block keeps only the trusted readings with `filter`, five of eight, with a mean of `21.02`.

## Named records

A stream of wind speeds with a station name for each record.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::kindle::Stream;
using pluss::table::Slate;

int main() {
  Stream wind(pluss::make_series({12.4, 31.0, 8.2, 25.7, 40.3, 19.9}));
  auto named = wind.with_label(Slate(pluss::make_note({"harbour", "ridge", "valley", "airport", "summit", "coast"})));
  std::cout << "labelled: " << (named.has_label() ? "yes" : "no") << ", records " << named.nrecords() << "\n";
  std::cout << named << "\n";

  // 1. Look records up by position through the labels: the label stream says which station each position is.
  auto labels = named.label()->as_note().host();
  const auto &names = labels.vector();
  auto values = named.as_series().items<double>();

  // 2. Rank the stations, strongest wind first, and report the top three by name.
  auto order = named.as_series().argsort(0, true).items<std::int64_t>();
  std::cout << "strongest winds:";
  for (int i = 0; i < 3; ++i) std::cout << " " << names[static_cast<std::size_t>(order[static_cast<std::size_t>(i)])] << " (" << values[static_cast<std::size_t>(order[static_cast<std::size_t>(i)])] << ")";
  std::cout << "\n";

  // 3. A threshold alert: which stations exceed 25 km/h? filter keeps the labels with the values.
  auto windy = named.filter(named.as_series().gt(25.0));
  auto windy_names = windy.label()->as_note().host();
  std::cout << "above 25:";
  for (const auto &n : windy_names.vector()) std::cout << " " << n;
  std::cout << " (" << windy.nrecords() << " of " << named.nrecords() << ")\n";

  // 4. Clearing the labels returns the plain stream.
  std::cout << "after clear_label: " << (named.clear_label().has_label() ? "yes" : "no") << "\n";
}
```

Output

```text
labelled: yes, records 6
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=6
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
harbour      | 12.4                                                                                                     
ridge        | 31                                                                                                       
valley       | 8.2                                                                                                      
airport      | 25.7                                                                                                     
summit       | 40.3                                                                                                     
coast        | 19.9                                                                                                     
========================================================================================================================

strongest winds: summit (40.3) ridge (31) airport (25.7)
above 25: ridge airport summit (3 of 6)
after clear_label: no
```
`with_label` names the records and the printed stream shows each reading beside its station. Ranking with `argsort` and reading the names through the labels gives the strongest winds, `summit` (`40.3`), `ridge` (`31`) and `airport` (`25.7`); `filter` keeps the labels with the values, so the three stations above `25` come out with their names; and `clear_label` returns the plain stream.

## Train, validation and test

Eight, two and three records: three catalogs of different sizes in one archive, scaled with the training statistics.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Archive;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

static Catalog part(const std::vector<double> &x) {
  std::vector<double> y;
  for (double v : x) y.push_back(5.0 + 1.5 * v);
  return Catalog({"x"_k = Stream(pluss::make_series(x)), "y"_k = Stream(pluss::make_series(y))});
}

int main() {
  Archive splits({part({10, 12, 14, 16, 18, 20, 22, 24}), part({26, 28}), part({30, 32, 34})}, Stream(pluss::make_note({"train", "valid", "test"})));
  std::cout << "catalogs " << splits.depth() << ", records per catalog from " << splits.minrecords() << " to " << splits.maxrecords() << "\n";

  // 1. Fit the scaler on the training catalog alone.
  auto train_x = splits.loc("train").keys("x");
  const double mean = train_x.mean().as_series().items<double>()[0];
  const double sd = train_x.std().as_series().items<double>()[0];
  std::cout << "training mean " << mean << ", std " << sd << "\n";

  // 2. Apply that scaler to every catalog with map_catalogs.
  auto scaled = splits.map_catalogs([&](const Catalog &c) { return c.set("x_scaled", (c.keys("x") - mean) / sd); });
  for (const char *name : {"train", "valid", "test"}) {
    auto c = scaled.loc(name);
    std::cout << name << ": " << c.nrows() << " records, mean of x_scaled " << c.keys("x_scaled").mean().as_series().items<double>()[0] << "\n";
  }

  // 3. A leak check: the validation and test values are expressed in units of the training spread, so they lie outside the training range.
  std::cout << "largest validation x_scaled " << scaled.loc("valid").keys("x_scaled").max().as_series().items<double>()[0] << " (training max "
            << scaled.loc("train").keys("x_scaled").max().as_series().items<double>()[0] << ")\n";
}
```

Output

```text
catalogs 3, records per catalog from 2 to 8
training mean 17, std 4.89898
train: 8 records, mean of x_scaled 0
valid: 2 records, mean of x_scaled 2.04124
test: 3 records, mean of x_scaled 3.06186
largest validation x_scaled 2.24537 (training max 1.42887)
```
The archive reports records per catalog from `2` to `8`: that is allowed here and refused by an Atlas. The scaler is fitted on the training catalog alone (mean `17`, standard deviation `4.9`) and applied to every catalog with `map_catalogs`. The training mean of the scaled feature is exactly `0`, and the validation and test means are `2.04` and `3.06`, outside the training range: in training units the new data are far from where the model was fitted, which is the signal you want to see and not hide.

## Months of different sizes

Three monthly catalogs where products appear and disappear.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace pluss::kindle::literals;
using pluss::kindle::Archive;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

static Catalog month(std::vector<std::string> product, std::vector<double> units, std::vector<double> revenue) {
  return Catalog({"product"_k = Stream(pluss::make_note(product)), "units"_k = Stream(pluss::make_series(units)), "revenue"_k = Stream(pluss::make_series(revenue))});
}

int main() {
  Archive year({month({"widget", "gadget", "gizmo"}, {120, 80, 40}, {1200, 1600, 800}),
                month({"widget", "gadget", "gizmo", "gear"}, {150, 70, 55, 30}, {1500, 1400, 1100, 900}),
                month({"widget", "gizmo"}, {90, 60}, {900, 1200})},
               Stream(pluss::make_note({"2026-09", "2026-10", "2026-11"})));
  std::cout << "months " << year.depth() << ", products per month from " << year.minrecords() << " to " << year.maxrecords() << "\n";

  // 1. One line per month, computed across the whole archive.
  auto totals = year.aggregate_over_catalogs([](const std::vector<Catalog> &catalogs) {
    std::vector<double> units, revenue;
    for (const auto &c : catalogs) {
      units.push_back(c.keys("units").sum().as_series().items<double>()[0]);
      revenue.push_back(c.keys("revenue").sum().as_series().items<double>()[0]);
    }
    return Catalog({"units"_k = Stream(pluss::make_series(units)), "revenue"_k = Stream(pluss::make_series(revenue))});
  });
  std::cout << totals << "\n";

  // 2. Which product is present in every month? Intersect the product names, month by month.
  std::set<std::string> everywhere;
  for (std::int64_t i = 0; i < year.depth(); ++i) {
    const auto names = year.dloc(i).keys("product").as_note().host();
    std::set<std::string> now(names.vector().begin(), names.vector().end());
    if (i == 0) everywhere = now;
    else {
      std::set<std::string> keep;
      for (const auto &n : everywhere) if (now.count(n)) keep.insert(n);
      everywhere = keep;
    }
  }
  std::cout << "products sold in every month:";
  for (const auto &n : everywhere) std::cout << " " << n;
  std::cout << "\n";

  // 3. November alone, by label.
  std::cout << year.loc("2026-11") << "\n";
}
```

Output

```text
months 3, products per month from 2 to 4
========================================================================================================================
kindle::Catalog  keys=2
keys: [units, revenue]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
units                    | Series       | Series[float64]  | 3       | [240, 305, 150]                                  
revenue                  | Series       | Series[float64]  | 3       | [3600, 4900, 2100]                               
========================================================================================================================

products sold in every month: gizmo widget
========================================================================================================================
kindle::Catalog  keys=3
keys: [product, units, revenue]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
product                  | Note         | Note             | 2       | [widget, gizmo]                                  
units                    | Series       | Series[float64]  | 2       | [90, 60]                                         
revenue                  | Series       | Series[float64]  | 2       | [900, 1200]                                      
========================================================================================================================
```
The archive holds `2` to `4` products per month. `aggregate_over_catalogs` sees all months and returns one row per month, units `240`, `305`, `150` and revenue `3600`, `4900`, `2100`. Intersecting the product names month by month finds the products sold in every month (`gizmo` and `widget`), and `loc` reads November alone.

## Decisions

- **Stream or catalog:** a stream for one column's cleaning and provenance, a catalog for several.
- **Metadata:** attach it last, after the transformations, or re-attach it as the program does.
- **Archive or Atlas:** an archive when sizes differ; an Atlas when they must not.
- **Leakage:** fit scalers on the training catalog only, then map them over every catalog.

## Where next

Reference pages: [`clip`](../api/stream/clip.md), [`ffill`](../api/stream/ffill.md), [`with_meta`](../api/stream/with_meta.md), [`meta`](../api/stream/meta.md), [`filter`](../api/stream/filter.md), [`with_label`](../api/stream/with_label.md), [`clear_label`](../api/stream/clear_label.md), [`map_catalogs`](../api/archive/map_catalogs.md), [`aggregate_over_catalogs`](../api/archive/aggregate_over_catalogs.md), [`loc`](../api/archive/loc.md), [`minrecords`](../api/archive/minrecords.md), [`maxrecords`](../api/archive/maxrecords.md). Next: [Scribble](scribble.md).
