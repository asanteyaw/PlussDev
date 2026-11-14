<!-- generated from kindle_catalogs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Kindle: catalogs

[Back to PlussMic by example](../guide.md)

Kindle is PlussMic's stream-first model. Where a `DataFrame` is a table, a `Catalog` is a named set of typed streams that you can transform one at a time, with reports on what every operation did. This page redoes the common table jobs the Kindle way: a sales report, an audited join, a text search, a file round trip, time-series features, feature preparation and change detection.

## The mental model

A `Stream` is one typed vector with an optional validity mask, labels and metadata. A `Catalog` is a set of streams with names, and its natural verbs are the stream verbs: `set` a stream, `keys` to pick some, `map` a function over streams, `group().by(key)` to split. Printing a catalog shows each stream's kind, dtype, length and a preview rather than a table; `tabelize()` converts to a `DataFrame` when you want rows and columns.

## A sales report

The same six cities and the same questions as the DataFrame report.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <map>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog sales({"city"_k = Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale", "Takoradi", "Cape Coast"})),
                 "region"_k = Stream(pluss::make_note({"south", "south", "middle", "north", "south", "middle"})),
                 "units"_k = Stream(pluss::make_series({10.0, 7.0, 12.0, 5.0, 9.0, 6.0})),
                 "price"_k = Stream(pluss::make_series({3.0, 4.0, 2.5, 5.0, 3.5, 4.5}))});
  std::cout << sales << "\n";

  // 1. A derived stream: set adds a new named stream computed from existing ones.
  auto enriched = sales.set("revenue", sales.keys("units") * sales.keys("price"));
  std::cout << "streams: " << enriched.nkeys() << ", rows: " << enriched.nrows() << "\n";

  // 2. Filter by a condition on a stream, then order by revenue.
  auto strong = enriched.where(enriched.keys("revenue").as_series().ge(28.0));
  std::cout << "rows with revenue of at least 28: " << strong.nrows() << "\n";
  std::cout << enriched.sort("revenue", false).head(3) << "\n";

  // 3. Grouping splits a catalog into one catalog per key value; summarise each.
  auto groups = enriched.group().by("region");
  std::map<std::string, Catalog> ordered(groups.begin(), groups.end());
  for (const auto &[region, part] : ordered) {
    std::cout << region << ": " << part.nrows() << " cities, units " << part.keys("units").sum().as_series().items<double>()[0] << ", mean revenue "
              << part.keys("revenue").mean().as_series().items<double>()[0] << "\n";
  }
}
```

Output

```text
========================================================================================================================
kindle::Catalog  keys=4
keys: [city, region, units, price]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
city                     | Note         | Note             | 6       | [Accra, Tema, Kumasi, ..., Cape Coast]           
region                   | Note         | Note             | 6       | [south, south, middle, ..., middle]              
units                    | Series       | Series[float64]  | 6       | [10, 7, 12, ..., 6]                              
price                    | Series       | Series[float64]  | 6       | [3, 4, 2.5, ..., 4.5]                            
========================================================================================================================

streams: 5, rows: 6
rows with revenue of at least 28: 4
========================================================================================================================
kindle::Catalog  keys=5
keys: [city, region, units, price, revenue]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
city                     | Note         | Note             | 3       | [Takoradi, Accra, Kumasi]                        
region                   | Note         | Note             | 3       | [south, south, middle]                           
units                    | Series       | Series[float64]  | 3       | [9, 10, 12]                                      
price                    | Series       | Series[float64]  | 3       | [3.5, 3, 2.5]                                    
revenue                  | Series       | Series[float64]  | 3       | [31.5, 30, 30]                                   
========================================================================================================================

middle: 2 cities, units 18, mean revenue 28.5
north: 1 cities, units 5, mean revenue 25
south: 3 cities, units 26, mean revenue 29.8333
```
`set` adds the revenue stream, `where` keeps the four rows with revenue of at least 28, and `sort` orders them. The grouped summaries match the DataFrame report: middle `18` units and mean revenue `28.5`, north `5` and `25`, south `26` and `29.83`.

## Joining with an audit

Customers and orders, with an order for a customer who does not exist. The program audits the keys before joining and reads the report of the join afterwards.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog customers({"customer"_k = Stream(pluss::make_note({"ada", "grace", "alan", "edsger"})), "country"_k = Stream(pluss::make_note({"UK", "US", "UK", "NL"}))});
  Catalog orders({"order_id"_k = Stream(pluss::make_series({101, 102, 103, 104, 105})),
                  "customer"_k = Stream(pluss::make_note({"ada", "grace", "ada", "alan", "barbara"})),
                  "amount"_k = Stream(pluss::make_series({120.0, 75.5, 40.0, 310.0, 22.5}))});

  // 1. Audit before joining: is each side unique on the key, are there records without a key, and how many have no partner?
  auto audit = customers.audit_keys(orders, {"customer"}, {"customer"});
  std::cout << "customers: " << audit.left_records << " records, unique " << (audit.left_unique ? "yes" : "no") << ", orphans " << audit.left_orphans << "\n";
  std::cout << "orders:    " << audit.right_records << " records, unique " << (audit.right_unique ? "yes" : "no") << ", orphans " << audit.right_orphans << "\n";

  // 2. The join. Left keeps every customer; "validate" states the expected relationship so a surprise raises an error instead of
  //    silently multiplying rows.
  pluss::kindle::CatalogJoinSpec spec;
  spec.how = pluss::relational::JoinHow::Left;
  spec.left_keys = {"customer"};
  spec.right_keys = {"customer"};
  auto joined = customers.join(orders, spec);
  std::cout << joined << "\n";

  // 3. join_detailed returns the same result with a report: nothing is lost uncounted.
  auto detailed = customers.join_detailed(orders, spec);
  std::cout << "report: " << detailed.report.summary() << "\n";

  // 4. A wrong expectation is an error, not a quiet duplication: customers and orders are one-to-many, not one-to-one.
  spec.validate = pluss::relational::RelationshipCardinality::OneToOne;
  try {
    (void)customers.join(orders, spec);
    std::cout << "accepted as one-to-one\n";
  } catch (const pluss::Error &e) {
    std::cout << "rejected: " << e.message() << "\n";
  }
}
```

Output

```text
customers: 4 records, unique yes, orphans 1
orders:    5 records, unique no, orphans 1
========================================================================================================================
kindle::Catalog  keys=4
keys: [customer, country, order_id, amount]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
customer                 | Note         | Note             | 5       | [ada, ada, grace, ..., edsger]                   
country                  | Note         | Note             | 5       | [UK, UK, US, ..., NL]                            
order_id                 | Series       | Series[int64]    | 5       | [101, 103, 102, ..., NA]                         
amount                   | Series       | Series[float64]  | 5       | [120, 40, 75.5, ..., NA]                         
========================================================================================================================

report: 5 output record(s); 4 matched pair(s). left: 1 of 4 record(s) without a partner (0 without a key), 0 omitted. right: 1 of 5 without a partner (0 without a key), 1 omitted.
rejected: Catalog::join: right keys repeat, contradicting the declared one-to-one contract
```
The audit says the customer keys are unique with one orphan (Edsger has no order), the order keys repeat, and one order has no customer. The left join keeps all four customers and the join report counts what happened: five output records, four matched pairs, one customer without a partner and one order omitted. Declaring the relationship as one-to-one makes the join *fail* with `right keys repeat`: a wrong assumption about the data is an error, not a silently wrong table.

## Searching text

Five incident reports and a query. The program cleans the text, greps for a pattern, finds the most frequent words and ranks the reports by cosine similarity to the query.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog reports({"id"_k = Stream(pluss::make_series({1, 2, 3, 4, 5})),
                   "text"_k = Stream(pluss::make_note({"Heavy rain on the ring road, two lanes closed",
                                                         "Clear weather downtown, traffic normal",
                                                         "Storm warning issued, rain expected overnight",
                                                         "Road works on the ring road until Friday",
                                                         "Sunny and dry across the region"}))});

  // 1. Clean the text stream: lower case and punctuation removed. clean returns a catalog holding the cleaned text.
  auto cleaned = reports.clean("text");
  std::cout << "cleaned: " << cleaned.keys("text").as_note().host().vector()[0] << "\n";

  // 2. grep keeps the rows whose text matches a pattern.
  auto rainy = reports.grep("text", "rain");
  std::cout << "reports that mention rain: " << rainy.nrows() << ", ids " << pluss::series::print(rainy.keys("id").as_series()) << "\n";

  // 3. keywords: the most frequent words across the whole collection, as a small catalog of (keyword, count).
  auto top = reports.keywords("text", 3);
  std::cout << "most frequent words: " << pluss::note::print(top.keys("keyword").as_note()) << " with counts " << pluss::series::print(top.keys("count").as_series()) << "\n";

  // 4. Rank the reports against a query by cosine similarity of their word vectors.
  auto queries = pluss::make_note({"rain on the road", "rain on the road", "rain on the road", "rain on the road", "rain on the road"});
  auto scored = reports.set("query", Stream(queries)).similarity("text", "query");
  auto similarity = scored.keys("text__query_similarity").as_series();
  std::cout << "similarity to the query: " << pluss::series::print(similarity) << "\n";
  auto order = similarity.argsort(0, true);
  std::cout << "best matches first (report ids): " << pluss::series::print(reports.keys("id").as_series().index_select(0, order)) << "\n";
}
```

Output

```text
cleaned: heavy rain on the ring road two lanes closed
reports that mention rain: 2, ids Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 3]
most frequent words: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["road", "the", "on"] with counts Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 3, 2]
similarity to the query: Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.6667, 0.0000, 0.2041, 0.6325, 0.2041]
best matches first (report ids): Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[1, 4, 3, 5, 2]
```
`clean` lower-cases and removes punctuation; `grep` finds the two reports that mention rain (ids 1 and 3). `keywords` gives the most frequent words in the whole collection, which here are `road`, `the` and `on`: stop words are not removed, so common words rank high. The similarity ranking is the useful result: reports 1 and 4, the two about the ring road, come first, and report 2, which shares no words with the query, comes last with a similarity of `0`.

## Loading and saving files

Six lines of a CSV with a missing reading and a missing count. The program loads it into streams, fills and drops, widens a dtype, summarises per sensor and saves.

```cpp
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

using pluss::kindle::Catalog;

int main() {
  const auto dir = std::filesystem::temp_directory_path();
  const auto raw_path = dir / "pluss_guide_catalog_raw.csv";
  const auto clean_path = dir / "pluss_guide_catalog_clean.csv";
  {
    std::ofstream file(raw_path);
    file << "sensor,reading,count\nA,12.5,3\nB,,4\nA,13.1,\nB,14.0,2\nB,15.2,5\nA,11.9,1\n";
  }
  auto raw = Catalog::load_csv(raw_path.string());
  std::cout << "loaded " << raw.nrows() << " rows, " << raw.nkeys() << " streams\n" << raw << "\n";

  // 1. A row without a reading is useless: dropnull removes the rows where any stream is missing, so first fill what may be missing.
  auto counted = raw.set("count", raw.keys("count").fillnull(0.0));
  auto kept = counted.dropnull();
  std::cout << "rows kept after filling counts and dropping missing readings: " << kept.nrows() << " of " << raw.nrows() << "\n";

  // 2. Counts are whole numbers in the file but averages need a floating dtype: widen the stream.
  auto prepared = kept.set("count", kept.keys("count").astype(pluss::ScalarType::Float64));
  std::cout << "count dtype is now " << prepared.keys("count").as_series().scalar_type() << "\n";

  // 3. One line per sensor.
  std::map<std::string, Catalog> by_sensor;
  for (const auto &[name, part] : prepared.group().by("sensor")) by_sensor.emplace(name, part);
  for (const auto &[name, part] : by_sensor)
    std::cout << "sensor " << name << ": " << part.nrows() << " readings, mean " << part.keys("reading").mean().as_series().items<double>()[0] << ", total count "
              << part.keys("count").sum().as_series().items<double>()[0] << "\n";

  // 4. Save and read back; the totals must survive.
  prepared.save_csv(clean_path.string());
  auto again = Catalog::load_csv(clean_path.string());
  std::cout << "reloaded " << again.nrows() << " rows; reading total " << again.keys("reading").sum().as_series().items<double>()[0] << " vs "
            << prepared.keys("reading").sum().as_series().items<double>()[0] << "\n";
  std::filesystem::remove(raw_path);
  std::filesystem::remove(clean_path);
}
```

Output

```text
loaded 6 rows, 3 streams
========================================================================================================================
kindle::Catalog  keys=3
keys: [sensor, reading, count]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
sensor                   | Note         | Note             | 6       | [A, B, A, ..., A]                                
reading                  | Series       | Series[float64]  | 6       | [12.5, NA, 13.1, ..., 11.9]                      
count                    | Series       | Series[int64]    | 6       | [3, 4, NA, ..., 1]                               
========================================================================================================================

rows kept after filling counts and dropping missing readings: 5 of 6
count dtype is now float64
sensor A: 3 readings, mean 12.5, total count 4
sensor B: 2 readings, mean 14.6, total count 7
reloaded 5 rows; reading total 66.7 vs 66.7
```
Filling the counts and then `dropnull` keeps five of six rows: the row without a reading is dropped. The count stream is widened to `float64` so that sums and means are computed in floating point, and the per-sensor lines (sensor A mean `12.5` over three readings, sensor B mean `14.6` over two) are the report. The reloaded file has the same reading total as what was saved, `66.7`.

## Time-series features

Eight daily closing prices. The time stream gives calendar features and the value stream gives returns, a rolling mean and a lag.

```cpp
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
```

Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [when, close]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
when                     | Timeline     | Timeline         | 8       | [2026-01-05T00:00:00.000~, 2026-01-20T00:00:00.0~
close                    | Series       | Series[float64]  | 8       | [100, 104, 102, ..., 118]                        
========================================================================================================================

returns: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[nan, 0.0400, -0.0192, 0.0588, 0.0278, -0.0360, 0.0467, 0.0536]
rolling mean (3): Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[100.0000, 102.0000, 102.0000, 104.6667, 107.0000, 108.6667, 110.0000, 112.3333]
lag 1: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[nan, 100.0000, 104.0000, 102.0000, 108.0000, 111.0000, 107.0000, 112.0000]
months: Note(shape=[8], dim=1, numel=8, encoding=utf8)
["January", "January", "January", "February", "February", "March", "March", "March"]
January: 3 observations, mean close 102, last close 102
February: 2 observations, mean close 109.5, last close 111
March: 3 observations, mean close 112.333, last close 118
```
The returns are `nan` for the first day and then `0.04`, `-0.019` and so on. The rolling mean of three observations is `100`, `102`, `102`, `104.67`, ..., and the lag stream is the series shifted by one. The month names, a Note stream derived from the time stream, are the grouping key for a monthly summary: January has three observations with a mean close of `102` and a last close of `102`, February two with `109.5` and `111`, March three with `112.33` and `118`.

## Preparing model inputs

A household table with three numeric streams and an identifier. The program standardises the numeric streams only and crosses to a table.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog features({"household"_k = Stream(pluss::make_note({"h1", "h2", "h3", "h4", "h5", "h6"})),
                    "income"_k = Stream(pluss::make_series({52.0, 61.0, 48.0, 75.0, 43.0, 58.0})),
                    "size"_k = Stream(pluss::make_series({3.0, 4.0, 2.0, 5.0, 1.0, 3.0})),
                    "area"_k = Stream(pluss::make_series({85.0, 110.0, 62.0, 140.0, 45.0, 95.0}))});

  // 1. Standardise the numeric streams: each is replaced by (value - mean) / std. keyaxis().series() selects the numeric streams only.
  auto numeric = features.keyaxis().series();
  std::cout << "numeric streams: " << numeric.nkeys() << " of " << features.nkeys() << "\n";
  auto standardised = numeric.map([](const Stream &s) {
    // mean() and std() are one-record streams, so read them out as numbers before using them in arithmetic.
    const double mean = s.mean().as_series().items<double>()[0], sd = s.std().as_series().items<double>()[0];
    return (s - mean) / sd;
  });
  std::cout << standardised << "\n";

  // 2. Check: every standardised stream has mean 0 and standard deviation 1.
  for (const char *name : {"income", "size", "area"})
    std::cout << name << ": mean " << standardised.keys(name).mean().as_series().items<double>()[0] << ", std " << standardised.keys(name).std().as_series().items<double>()[0] << "\n";

  // 3. Put the identifier back next to the prepared streams and cross to the table model for the next stage.
  auto ready = standardised.set("household", features.keys("household"));
  auto table = ready.tabelize();
  std::cout << table.to_string() << "\n";
}
```

Output

```text
numeric streams: 3 of 4
========================================================================================================================
kindle::Catalog  keys=3
keys: [income, size, area]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
income                   | Series       | Series[float64]  | 6       | [-0.368620917732, 0.427600264569, -0.72249699875~
size                     | Series       | Series[float64]  | 6       | [0, 0.707106781187, -0.707106781187, ..., 0]     
area                     | Series       | Series[float64]  | 6       | [-0.13261130309, 0.604118158523, -0.810402407775~
========================================================================================================================

income: mean 2.22045e-16, std 1
size: mean 0, std 1
area: mean -7.40149e-17, std 1
=============================================================
     | income         size           area           household
-------------------------------------------------------------
0    | -0.3686209177   0             -0.1326113031  h1       
1    |  0.4276002646   0.7071067812   0.6041181585  h2       
2    | -0.7224969988  -0.7071067812  -0.8104024078  h3       
3    |  1.666166548    1.414213562    1.488193512   h4       
4    | -1.1648421     -1.414213562   -1.311378442   h5       
5    |  0.1621932038   0              0.1620804816  h6       
-------------------------------------------------------------
[6 cs x 4 feat]
```
`keyaxis().series()` selects the three numeric streams and `map` applies the standardisation to each. Each stream's mean and standard deviation are one-record streams, so the lambda reads them out as numbers before the arithmetic. The check shows mean `0` (to rounding) and standard deviation `1` for every stream, and `tabelize()` returns a `DataFrame` that carries the identifier next to the prepared columns.

## Detecting change

Two loads of an inventory table. The program compares the streams and explains the difference, and then shows what a fingerprint is.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

static Catalog load(double stock_b) {
  return Catalog({"sku"_k = Stream(pluss::make_note({"A100", "B200", "C300"})), "stock"_k = Stream(pluss::make_series({40.0, stock_b, 7.0})), "price"_k = Stream(pluss::make_series({9.5, 20.0, 31.0}))});
}

// Equal content: same dtype and the same values in the same positions.
static bool same_content(const Stream &a, const Stream &b) {
  if (a.size() != b.size()) return false;
  if (a.kind() == pluss::kindle::StreamKind::Note) return a.as_note().host().vector() == b.as_note().host().vector();
  return a.as_series().eq(b.as_series()).all().item<bool>();
}

int main() {
  auto yesterday = load(12.0);
  auto today = load(9.0);

  // 1. Compare every stream of the two loads.
  bool stock_changed = false, price_changed = false;
  for (const auto &name : yesterday) {
    const bool same = same_content(yesterday.keys(name), today.keys(name));
    std::cout << "  " << name << ": " << (same ? "unchanged" : "CHANGED") << "\n";
    if (name == "stock") stock_changed = !same;
    if (name == "price") price_changed = !same;
  }

  // 2. Say how the changed stream differs.
  auto delta = today.keys("stock") - yesterday.keys("stock");
  std::cout << "stock change: " << pluss::series::print(delta.as_series()) << "\n";

  // 3. Rebuild only what depends on a changed stream.
  std::cout << "reorder report must be rebuilt: " << (stock_changed ? "yes" : "no") << "; price list must be rebuilt: " << (price_changed ? "yes" : "no") << "\n";

  // 4. A fingerprint identifies the stored data a catalog shares, not what it contains. A copy shares the storage and so the fingerprint;
  //    a table built separately with the same content has its own.
  auto copy = yesterday;
  auto rebuilt = load(12.0);
  std::cout << "copy has the same fingerprint:    " << (copy.fingerprint() == yesterday.fingerprint() ? "yes" : "no") << "\n";
  std::cout << "rebuilt has the same fingerprint: " << (rebuilt.fingerprint() == yesterday.fingerprint() ? "yes" : "no") << " (equal content: " << (same_content(rebuilt.keys("stock"), yesterday.keys("stock")) ? "yes" : "no") << ")\n";
}
```

Output

```text
  sku: unchanged
  stock: CHANGED
  price: unchanged
stock change: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, -3.0000, 0.0000]
reorder report must be rebuilt: yes; price list must be rebuilt: no
copy has the same fingerprint:    yes
rebuilt has the same fingerprint: no (equal content: yes)
```
Comparing stream by stream finds that only `stock` changed, by `-3` for the second item, so only the reorder report needs rebuilding. The fingerprint lines are a different tool: a fingerprint identifies the *storage* a catalog shares. A copy has the same fingerprint, and a table built separately with identical content does not, so a fingerprint is a cache key and not a content check.

## Decisions

- **Stream or table:** use a catalog when the unit of work is a stream (clean, filter, fill, join per stream); convert with `tabelize()` for row-oriented reports.
- **Joins:** audit the keys first, state the relationship you expect, and read the report afterwards.
- **Reductions give streams:** `mean()` and `std()` return one-record streams; read them as numbers before using them in arithmetic.
- **Change detection:** compare content, not fingerprints.

## Where next

Reference pages: [`set`](../api/catalog/set.md), [`where`](../api/catalog/where.md), [`sort`](../api/catalog/sort.md), [`group`](../api/catalog/group.md), [`audit_keys`](../api/catalog/audit_keys.md), [`join`](../api/catalog/join.md), [`join_detailed`](../api/catalog/join_detailed.md), [`clean`](../api/catalog/clean.md), [`grep`](../api/catalog/grep.md), [`similarity`](../api/catalog/similarity.md), [`keyaxis`](../api/catalog/keyaxis.md), [`tabelize`](../api/catalog/tabelize.md), [`load_csv`](../api/catalog/load_csv.md), [`save_csv`](../api/catalog/save_csv.md). Next: [Streams and archives](kindle_streams_archives.md).
