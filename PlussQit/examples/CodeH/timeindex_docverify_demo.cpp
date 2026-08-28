#include <pluss/pluss.h>

#include <iostream>
#include <string_view>
#include <vector>

namespace {

void show_timeline(std::string_view label, const pluss::codeh::Timeline& t) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::timeline::print(t) << "\n";
}

void show_series(std::string_view label, const pluss::Series& s) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::series::print(s) << "\n";
}

void show_note(std::string_view label, const pluss::Note& n) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::note::print(n) << "\n";
}

} // namespace

int main() {
  using pluss::TimeIndex;

  const auto base = pluss::timeindex::make({
      "2024-11-01T00:00:00Z",
      "2024-11-01T00:20:00Z",
      "2024-11-01T00:40:00Z",
      "2024-11-01T01:00:00Z",
      "2024-11-01T01:20:00Z",
  });

  const auto unsorted_dup = pluss::timeindex::make({
      "2024-11-01T00:40:00Z",
      "2024-11-01T00:00:00Z",
      "2024-11-01T00:20:00Z",
      "2024-11-01T00:20:00Z",
      "2024-11-01T01:00:00Z",
  }, false);

  const auto descending = pluss::timeindex::make({
      "2024-11-01T01:20:00Z",
      "2024-11-01T01:00:00Z",
      "2024-11-01T00:40:00Z",
      "2024-11-01T00:20:00Z",
      "2024-11-01T00:00:00Z",
  }, false);

  std::cout << "base.freq=" << base.freq() << "\n";
  std::cout << "base.is_monotonic_increasing=" << base.is_monotonic_increasing() << "\n";
  std::cout << "base.is_monotonic_decreasing=" << base.is_monotonic_decreasing() << "\n";
  std::cout << "base.is_unique=" << base.is_unique() << "\n";
  std::cout << "base.has_duplicates=" << base.has_duplicates() << "\n";
  std::cout << "descending.is_monotonic_decreasing=" << descending.is_monotonic_decreasing() << "\n";
  std::cout << "unsorted_dup.is_unique=" << unsorted_dup.is_unique() << "\n";
  std::cout << "unsorted_dup.has_duplicates=" << unsorted_dup.has_duplicates() << "\n";
  std::cout << "base.size=" << base.size() << "\n";

  const auto host = base.host();
  std::cout << "base.host=[";
  for (std::size_t i = 0; i < host.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << host[i];
  }
  std::cout << "]\n";

  show_timeline("base.timeline()", base.timeline());

  show_timeline("base.min()", base.min());
  show_timeline("base.max()", base.max());

  show_timeline("unsorted_dup.sort()", unsorted_dup.sort().timeline());
  show_timeline("unsorted_dup.unique()", unsorted_dup.unique().timeline());
  show_series("unsorted_dup.argsort()", unsorted_dup.argsort());

  show_timeline("base.take([4,2,0])",
                base.take(pluss::make_series(std::vector<std::int64_t>{4, 2, 0})).timeline());
  show_timeline("base.slice_index(1,4)", base.slice_index(1, 4).timeline());
  show_timeline("base.head(3)", base.head(3).timeline());
  show_timeline("base.tail(2)", base.tail(2).timeline());

  std::cout << "base.searchsorted(00:55,left)="
            << base.searchsorted(pluss::timeline::make("2024-11-01T00:55:00Z"), false)
            << "\n";
  std::cout << "base.searchsorted(00:55,right)="
            << base.searchsorted(pluss::timeline::make("2024-11-01T00:55:00Z"), true)
            << "\n";
  std::cout << "base.asof(00:55)="
            << base.asof(pluss::timeline::make("2024-11-01T00:55:00Z")) << "\n";
  show_series("base.slice(00:15,01:00)",
              base.slice(pluss::timeline::make("2024-11-01T00:15:00Z"),
                         pluss::timeline::make("2024-11-01T01:00:00Z")));
  show_series("base.slice_time(00:15,01:00)",
              base.slice_time("2024-11-01T00:15:00Z", "2024-11-01T01:00:00Z"));

  const auto right = pluss::timeindex::make({
      "2024-11-01T00:05:00Z",
      "2024-11-01T00:20:00Z",
      "2024-11-01T01:05:00Z",
      "2024-11-01T01:20:00Z",
  });

  show_timeline("base.union_(right)", base.union_(right).timeline());
  show_timeline("base.intersection(right)", base.intersection(right).timeline());
  show_timeline("base.difference(right)", base.difference(right).timeline());

  show_timeline("base.floor(\"H\")", base.floor("H").timeline());
  show_timeline("base.ceil(\"H\")", base.ceil("H").timeline());
  show_timeline("base.round(\"H\")", base.round("H").timeline());
  show_timeline("base.normalize()", base.normalize().timeline());

  show_note("base.to_note_iso()", base.to_note_iso());

  const auto utc_midday = pluss::timeindex::make({
      "2024-11-01T16:00:00Z",
      "2024-11-01T18:30:00Z",
  });
  show_timeline("utc_midday.tz_convert(\"America/New_York\")",
                utc_midday.tz_convert("America/New_York").timeline());
  show_timeline("utc_midday.to_utc()", utc_midday.to_utc().timeline());

  const auto naive_local = pluss::timeindex::make({
      "2024-11-01T09:00:00",
      "2024-11-01T10:15:00",
  }, false);
  show_timeline("naive_local.tz_localize(\"America/New_York\")",
                naive_local.tz_localize("America/New_York").timeline());

  const auto vals = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5});
  const auto win = pluss::duration::minutes(45);
  show_timeline("base.resample_bins(H).labels", base.resample_bins("H").labels);
  show_series("base.resample_bins(H).bin_id", base.resample_bins("H").bin_id);
  show_timeline("base.resample_bins(2H).labels", base.resample_bins("H", 2).labels);
  show_series("base.resample_bins(2H).bin_id", base.resample_bins("H", 2).bin_id);
  show_series("base.resample_count(H)", base.resample_count("H"));
  show_series("base.resample_count(2H)", base.resample_count("H", 2));
  show_series("base.resample_sum(values,H)", base.resample_sum(vals, "H"));
  show_series("base.resample_sum(values,2H)", base.resample_sum(vals, "H", 2));
  show_series("base.resample_mean(values,H)", base.resample_mean(vals, "H"));
  show_series("base.resample_mean(values,2H)", base.resample_mean(vals, "H", 2));

  TimeIndex::ResampleOptions resample_opts;
  resample_opts.freq = "H";
  resample_opts.step = 1;
  show_series("base.resample_agg(values,H,max)", base.resample_agg(vals, resample_opts, "max"));
  show_series("base.resample_agg(values,H,min)", base.resample_agg(vals, resample_opts, "min"));
  show_series("base.resample_agg(values,H,last)", base.resample_agg(vals, resample_opts, "last"));
  show_series("base.resample_ohlc(values,H)", base.resample_ohlc(vals, "H"));
  show_series("base.resample_ohlc(values,2H)", base.resample_ohlc(vals, "H", 2));

  show_series("base.rolling_count(45m)", base.rolling_count(win));
  show_series("base.rolling_count(60m)", base.rolling_count(pluss::duration::minutes(60)));
  show_series("base.rolling_sum(values,45m)", base.rolling_sum(vals, win));
  show_series("base.rolling_sum(values,60m)", base.rolling_sum(vals, pluss::duration::minutes(60)));
  show_series("base.rolling_mean(values,45m)", base.rolling_mean(vals, win));
  show_series("base.rolling_min(values,45m)", base.rolling_min(vals, win));
  show_series("base.rolling_max(values,45m)", base.rolling_max(vals, win));
  show_series("base.rolling_first(values,45m)", base.rolling_first(vals, win));
  show_series("base.rolling_last(values,45m)", base.rolling_last(vals, win));

  const auto left_join = pluss::timeindex::make({
      "2024-11-01T00:00:00Z",
      "2024-11-01T00:20:00Z",
      "2024-11-01T00:40:00Z",
      "2024-11-01T01:00:00Z",
      "2024-11-01T01:20:00Z",
  });
  const auto right_join = pluss::timeindex::make({
      "2024-11-01T00:05:00Z",
      "2024-11-01T00:30:00Z",
      "2024-11-01T01:10:00Z",
  });
  show_series("left_join.join_asof(right_join,backward)",
              left_join.join_asof(right_join, TimeIndex::AsofDirection::Backward));
  show_series("left_join.join_asof(right_join,forward)",
              left_join.join_asof(right_join, TimeIndex::AsofDirection::Forward));
  const auto tol = pluss::duration::minutes(25);
  show_series("left_join.join_asof(right_join,nearest,25m)",
              left_join.join_asof(right_join,
                                  TimeIndex::AsofDirection::Nearest,
                                  &tol,
                                  true));

  const auto aligned = base.align_union(right);
  show_timeline("base.align_union(right).index", aligned.index);
  show_series("base.align_union(right).left_indexer", aligned.left_indexer);
  show_series("base.align_union(right).right_indexer", aligned.right_indexer);

  return 0;
}
