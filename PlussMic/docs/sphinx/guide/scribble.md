<!-- generated from scribble.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Scribble: heterogeneous records

[Back to PlussMic by example](../guide.md)

Scribble is the model for data that is not one typed column per field: JSON whose fields change type, records that each hold a list, logs that mix text, times and durations, and configuration layers. A `Registry` is a set of named `Slot`s and each cell of a slot can be a whole `Series`, `Note`, `Timeline` or `Duration`, or null.

## The mental model

A `Slot` is a vector of values, and the values need not share a type or a size. A `Registry` is a named set of slots. You do the work on the cells, which are the ordinary Pluss types, and when the data is clean enough you cross to the typed world with `kindelize()` (to a catalog) and `tabelize()` (to a table). The crossing is a *contract check*: a field that is still mixed is refused.

## JSON with mixed types

Five records whose `value` is sometimes a float, sometimes an integer, a boolean or a string that is not a number.

```cpp
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

using pluss::scribble::Registry;

int main() {
  const auto path = std::filesystem::temp_directory_path() / "pluss_guide_events.json";
  {
    std::ofstream file(path);
    file << R"([{"id":1,"value":12.5,"tag":"a"},{"id":2,"value":"n/a","tag":"b"},{"id":3,"value":7,"tag":null},{"id":4,"value":true,"tag":"c"},{"id":5,"value":9.25,"tag":"a"}])";
  }
  auto events = Registry::load_json(path.string());
  std::filesystem::remove(path);
  std::cout << "fields " << events.nfields() << ", records " << events.sizes().at("id") << "\n";

  // 1. A profile counts the value types in every field. A field with more than one type is the one that needs attention.
  const auto profiles = events.profiles();
  const std::map<std::string, std::unordered_map<std::string, std::int64_t>> sorted(profiles.begin(), profiles.end());
  for (const auto &[field, counts] : sorted) {
    std::map<std::string, std::int64_t> ordered(counts.begin(), counts.end());
    std::cout << field << ":";
    for (const auto &[type, n] : ordered) std::cout << " " << type << "=" << n;
    std::cout << (ordered.size() > 1 ? "   <- mixed" : "") << "\n";
  }

  // 2. Look at the mixed field: its cells are float, null, integer, boolean, float. The string "n/a" did not survive as a number and is null.
  std::cout << events.get("value")->to_string() << "\n";

  // 3. Make the field one type: widen every cell to Float64 (an integer becomes a float, a boolean 0 or 1), then decide what a null means.
  auto widened = events.get("value")->astype(pluss::ScalarType::Float64);
  auto complete = events.set("value", widened.fillnull(0.0));
  std::cout << "profile of value after cleaning:";
  for (const auto &[type, n] : std::map<std::string, std::int64_t>(complete.profiles().at("value").begin(), complete.profiles().at("value").end())) std::cout << " " << type << "=" << n;
  std::cout << "\n";

  // 4. Cross to the typed world: a Registry converts to a Catalog and from there to a DataFrame. A field that is still mixed would be refused.
  try {
    (void)events.kindelize();
    std::cout << "the raw registry converted\n";
  } catch (const pluss::Error &e) {
    std::cout << "raw registry refused: " << e.message() << "\n";
  }
  auto table = complete.drop("tag").kindelize().tabelize();
  std::cout << table.to_string() << "\n";
  std::cout << "mean value " << table.features("value").mean().to_dbl() << "\n";
}
```

Output

```text
fields 3, records 5
id: Series=5
tag: Note=4 null=1   <- mixed
value: Series=4 null=1   <- mixed
{
  "_type": "Slot",
  "defined": true,
  "size": 5,
  "values": [
    "Series[float64]",
    null,
    "Series[int64]",
    "Series[bool]",
    "Series[float64]"
  ]
}
profile of value after cleaning: Series=5
raw registry refused: Registry::kindelize: slot 'value': slot is not stream-compatible (kind=Series)
==================
     | id    value
------------------
0    | 1     12.5 
1    | 2     0    
2    | 3     7    
3    | 4     1    
4    | 5     9.25 
------------------
[5 cs x 2 feat]
mean value 5.95
```
The profile counts the types in each field: `id` is all numbers, and `tag` and `value` are mixed (one null each). The value field's cells are `float64`, null, `int64`, `bool` and `float64`: the `"n/a"` did not survive as a number. Widening every cell to `float64` and filling the null with `0` makes the field one type; the raw registry is refused by `kindelize` (`slot 'value' is not stream-compatible`), and the cleaned one crosses to a table, where the mean of the value is `5.95`. The decision about what the null means, a zero here, is yours and is one line of the program.

## Records that hold lists

Four patients with a different number of glucose readings each; one has none.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

using pluss::scribble::Registry;
using pluss::scribble::Slot;
using Value = Slot::Value;

int main() {
  Registry patients({
      {"patient", Slot({Value(pluss::make_note({"P-01"})), Value(pluss::make_note({"P-02"})), Value(pluss::make_note({"P-03"})), Value(pluss::make_note({"P-04"}))})},
      {"glucose", Slot({Value(pluss::make_series({5.1, 5.4, 5.0})), Value(pluss::make_series({7.8})), Value(pluss::make_series({6.1, 6.9, 7.4, 8.2, 7.7})), Value(pluss::make_series(std::vector<double>{}))})},
  });
  std::cout << "records " << patients.sizes().at("patient") << "\n";

  // 1. Per-record summaries: each cell is a Series, so the full Series vocabulary applies to it. A record without readings stays empty.
  std::vector<double> count, mean, peak;
  for (const auto &cell : patients.get("glucose")->values()) {
    const auto &readings = std::get<pluss::Series>(cell);
    count.push_back(static_cast<double>(readings.numel()));
    mean.push_back(readings.numel() ? readings.mean().item<double>() : std::numeric_limits<double>::quiet_NaN());
    peak.push_back(readings.numel() ? readings.max().item<double>() : std::numeric_limits<double>::quiet_NaN());
  }
  std::cout << "readings per patient: " << pluss::series::print(pluss::make_series(count)) << "\n";
  std::cout << "mean glucose:         " << pluss::series::print(pluss::make_series(mean)) << "\n";

  // 2. Flag: a patient needs follow-up when the peak exceeds 7.8 mmol/L, and a patient with no readings needs a test.
  auto peaks = pluss::make_series(peak);
  std::cout << "peak above 7.8: " << pluss::series::print(peaks.gt(7.8)) << "\n";
  std::cout << "no readings:    " << pluss::series::print(pluss::make_series(count).eq(0.0)) << "\n";

  // 3. When you want a rectangular table, the summaries become typed columns: the ragged detail has been reduced to numbers.
  std::vector<std::string> names;
  for (const auto &cell : patients.get("patient")->values()) names.push_back(std::string(std::get<pluss::Note>(cell).at(0)));
  auto summary = pluss::table::DataFrame({{"patient", pluss::make_note(names)},
                                          {"readings", pluss::make_series(count)}, {"mean", pluss::make_series(mean)}, {"peak", pluss::make_series(peak)}});
  std::cout << summary.to_string() << "\n";
}
```

Output

```text
records 4
readings per patient: Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3.0000, 1.0000, 5.0000, 0.0000]
mean glucose:         Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[5.1667, 7.8000, 7.2600, nan]
peak above 7.8: Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, false, true, false]
no readings:    Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, false, false, true]
===========================================
     | patient  readings  mean         peak
-------------------------------------------
0    | P-01     3         5.166666667  5.4 
1    | P-02     1         7.8          7.8 
2    | P-03     5         7.26         8.2 
3    | P-04     0         NULL         NULL
-------------------------------------------
[4 cs x 4 feat]
```
Each cell of the `glucose` field is a `Series`, so every record keeps its own length and nothing is padded. Per-record summaries use the ordinary `Series` vocabulary: readings per patient `3`, `1`, `5` and `0`, mean glucose `5.17`, `7.8`, `7.26` and `nan` for the patient without readings. The flags say that patient P-03 peaked above `7.8` and P-04 has no readings and needs a test. The final table has the ragged detail reduced to typed columns.

## An event log

Five job runs, appended one at a time, with text, timestamps and durations in the same structure.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using pluss::scribble::Registry;
using pluss::scribble::Slot;
using Value = Slot::Value;

static Registry append(const Registry &log, const std::string &job, const std::string &status, const char *when, std::int64_t seconds) {
  // push adds a value to the end of one field: a record is the same position in every field.
  return log.push("job", Value(pluss::make_note({job}))).push("status", Value(pluss::make_note({status}))).push("when", Value(pluss::make_timeline({when}))).push("took", Value(pluss::duration::seconds(std::vector<std::int64_t>{seconds})));
}

int main() {
  Registry log({{"job", Slot(std::vector<Value>{})}, {"status", Slot(std::vector<Value>{})}, {"when", Slot(std::vector<Value>{})}, {"took", Slot(std::vector<Value>{})}});
  log = append(log, "backup", "ok", "2026-11-13T01:00:00Z", 340);
  log = append(log, "report", "ok", "2026-11-13T06:30:00Z", 95);
  log = append(log, "sync", "failed", "2026-11-13T09:15:00Z", 12);
  log = append(log, "backup", "ok", "2026-11-14T01:00:00Z", 362);
  log = append(log, "sync", "ok", "2026-11-14T09:15:00Z", 48);
  std::cout << "records " << log.sizes().at("job") << " in fields " << log.nfields() << "\n";

  // 1. The latest event is the last element of every field.
  const auto latest = log.tail(1);
  std::cout << "latest job: " << std::get<pluss::Note>(latest.get("job")->values().front()).at(0) << " (" << std::get<pluss::Note>(latest.get("status")->values().front()).at(0) << ")\n";

  // 2. What failed? Walk the status field; each cell is a one-element Note.
  const auto status = log.get("status")->values();
  const auto jobs = log.get("job")->values();
  for (std::size_t i = 0; i < status.size(); ++i)
    if (std::get<pluss::Note>(status[i]).at(0) == "failed") std::cout << "failed: record " << i << ", job " << std::get<pluss::Note>(jobs[i]).at(0) << "\n";

  // 3. How long did each job take? Durations are elapsed lengths: compare them and add them.
  const auto took = log.get("took")->values();
  double total_seconds = 0;
  for (const auto &cell : took) total_seconds += static_cast<double>(std::get<pluss::Duration>(cell).host()[0]) / 1e9;
  std::cout << "total run time " << total_seconds << " s over " << took.size() << " runs, mean " << total_seconds / took.size() << " s\n";

  // 4. Runs and failures per calendar day: the timestamps are Timeline cells, so format each one and count.
  std::map<std::string, std::pair<int, int>> per_day;   // day -> (runs, failures)
  const auto when = log.get("when")->values();
  for (std::size_t i = 0; i < when.size(); ++i) {
    auto &entry = per_day[std::string(std::get<pluss::Timeline>(when[i]).strftime("%Y-%m-%d").at(0))];
    ++entry.first;
    if (std::get<pluss::Note>(status[i]).at(0) == "failed") ++entry.second;
  }
  for (const auto &[day, counts] : per_day) std::cout << day << ": " << counts.first << " runs, " << counts.second << " failed\n";
}
```

Output

```text
records 5 in fields 4
latest job: sync (ok)
failed: record 2, job sync
total run time 857 s over 5 runs, mean 171.4 s
2026-11-13: 3 runs, 1 failed
2026-11-14: 2 runs, 0 failed
```
`push` appends a value to the end of a field, and a record is the same position in every field. The latest event, the failing job (`sync`, record 2) and the total run time (`857 s`, mean `171.4 s`) come straight from the cells, and the timestamps are `Timeline` cells, so formatting each one gives a per-day count: three runs and one failure on the 13th and two runs and no failure on the 14th.

## Layered configuration

Defaults, a site override and a user override.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <map>
#include <set>
#include <string>

using pluss::scribble::Registry;
using pluss::scribble::Slot;
using Value = Slot::Value;

static Slot text(const char *v) { return Slot(Value(pluss::make_note({v}))); }
static Slot number(double v) { return Slot(Value(pluss::make_series({v}))); }

int main() {
  Registry defaults({{"timeout", number(30)}, {"retries", number(3)}, {"region", text("eu")}, {"log", text("info")}});
  Registry site({{"timeout", number(60)}, {"region", text("us-east")}});
  Registry user({{"log", text("debug")}, {"colour", text("on")}});

  // 1. combine keeps both versions of a clashing field (the second under a suffix); it does not override.
  auto both = defaults.combine(site, "_site");
  std::cout << "combine keeps both:";
  for (const auto &name : both.fields()) std::cout << " " << name;
  std::cout << "\n";

  // 2. Layering is overriding: set each field of a higher layer over the layer below, lowest priority first.
  const auto overlay = [](Registry base, const Registry &top) {
    for (const auto &name : top.fields()) base = base.set(name, *top.get(name));
    return base;
  };
  auto merged = overlay(overlay(defaults, site), user);
  std::map<std::string, std::string> source;
  for (const auto &name : defaults.fields()) source[name] = "defaults";
  for (const auto &name : site.fields()) source[name] = "site";
  for (const auto &name : user.fields()) source[name] = "user";
  const auto value_of = [&](const Registry &r, const std::string &key) -> std::string {
    const auto cell = r.get(key)->values().front();
    if (std::holds_alternative<pluss::Note>(cell)) return std::string(std::get<pluss::Note>(cell).at(0));
    return std::to_string(static_cast<int>(std::get<pluss::Series>(cell).items<double>()[0]));
  };
  for (const auto &name : std::set<std::string>(merged.fields().begin(), merged.fields().end()))
    std::cout << "  " << name << " = " << value_of(merged, name) << "   (from " << source.at(name) << ")\n";

  // 3. Validate: required settings must be present.
  std::cout << "has every required field (timeout, retries, region): " << (merged.has_all({"timeout", "retries", "region"}) ? "yes" : "no") << "\n";
  std::cout << "has a proxy setting: " << (merged.has("proxy") ? "yes" : "no") << "\n";
  auto incomplete = overlay(site, user);
  std::cout << "site + user alone has every required field: " << (incomplete.has_all({"timeout", "retries", "region"}) ? "yes" : "no") << "\n";
}
```

Output

```text
combine keeps both: timeout retries region log timeout_site region_site
  colour = on   (from user)
  log = debug   (from user)
  region = us-east   (from site)
  retries = 3   (from defaults)
  timeout = 60   (from site)
has every required field (timeout, retries, region): yes
has a proxy setting: no
site + user alone has every required field: no
```
`combine` keeps both versions of a clashing field (the second under a suffix, `timeout_site`), which is merging but not overriding. Overriding is `set`, applied layer by layer from the lowest priority up, and the program prints the final value of every setting with the layer it came from: `timeout` and `region` from the site, `log` and `colour` from the user and `retries` from the defaults. `has_all` then validates that the required settings are present, and the site and user layers alone are not enough.

## Decisions

- **Scribble or table:** Scribble while the data is irregular, a table once it is regular; cross with `kindelize()` and `tabelize()`.
- **Mixed fields:** profile first, then decide the type and what a null means.
- **Merging:** `combine` to keep both, `set` to override.
- **Cells are ordinary types:** reach for the `Series`, `Note` and `Timeline` APIs inside the cells.

## Where next

Reference pages: [`load_json`](../api/registry/load_json.md), [`profiles`](../api/registry/profiles.md), [`get`](../api/registry/get.md), [`set`](../api/registry/set.md), [`push`](../api/registry/push.md), [`drop`](../api/registry/drop.md), [`combine`](../api/registry/combine.md), [`has_all`](../api/registry/has_all.md), [`kindelize`](../api/registry/kindelize.md), [`astype`](../api/slot/astype.md), [`fillnull`](../api/slot/fillnull.md). Next: [Bundle](bundle.md).
