#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error(std::string(message));
}

void expect(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

template <class Fn>
void expect_throw(Fn&& fn, std::string_view message) {
  bool threw = false;
  try {
    fn();
  } catch (...) {
    threw = true;
  }
  expect(threw, message);
}

void test_timeline_index_entry() {
  const auto timeline = pluss::timeline::make({
      "2025-01-01T00:00:00Z",
      "2025-01-02T00:00:00Z",
  });
  const auto index = timeline.index();
  expect(index.timeline().ticks() == timeline.ticks(),
         "Timeline::index must preserve temporal values");

  const auto unordered = pluss::timeline::make({
      "2025-01-02T00:00:00Z",
      "2025-01-01T00:00:00Z",
  });
  expect_throw([&] { (void)unordered.index(); },
               "Timeline::index must reject unordered axes by default");
  expect(unordered.index(false).size() == 2,
         "Timeline::index(false) must preserve an unchecked rank-one axis");

  const pluss::table::Slate migrated(timeline);
  expect(migrated.kind() == pluss::table::SlateKind::Timeline,
         "Slate must canonicalize legacy TimeIndex input as Timeline");
  expect(migrated.as_timeline().ticks() == timeline.ticks(),
         "TimeIndex-to-Slate migration must preserve every timestamp");
  expect(migrated.as_timeline().index().timeline().ticks() == timeline.ticks(),
         "Slate Timeline payload must expose ordered indexing through Timeline::index");

  const pluss::kindle::Stream stream(timeline);
  expect(stream.kind() == pluss::kindle::StreamKind::Timeline,
         "Stream must canonicalize legacy TimeIndex input as Timeline");
  expect(stream.as_timeline().ticks() == timeline.ticks(),
         "TimeIndex-to-Stream migration must preserve every timestamp");
  expect(stream.as_timeline().index().timeline().ticks() == timeline.ticks(),
         "Stream Timeline payload must expose ordered indexing through Timeline::index");
  expect(stream.to_json().find("\"kind\": \"Timeline\"") != std::string::npos,
         "Stream serialization must emit the canonical Timeline kind");
  expect(stream.to_json().find("TimeIndex") == std::string::npos,
         "Stream serialization must not emit the legacy TimeIndex kind");

  const auto round_trip = pluss::kindle::Stream::from_slate(migrated).to_slate();
  expect(round_trip.kind() == pluss::table::SlateKind::Timeline,
         "Slate and Stream round trips must not reintroduce TimeIndex storage");

  const auto ticks = timeline.ticks();
  const auto first = pluss::timeline::from_epoch(
      std::vector<std::int64_t>{ticks[0]}, std::vector<std::int64_t>{1}, std::string(timeline.tz()));
  const auto second = pluss::timeline::from_epoch(
      std::vector<std::int64_t>{ticks[1]}, std::vector<std::int64_t>{1}, std::string(timeline.tz()));
  const pluss::scribble::Slot slot(
      std::vector<pluss::scribble::Slot::Value>{first, second});
  expect(slot.kind() == pluss::scribble::SlotKind::Timeline,
         "Slot must canonicalize legacy TimeIndex cells as Timeline");
  expect(std::holds_alternative<pluss::Timeline>(slot.at(0)),
         "Slot storage must not retain a TimeIndex payload");
  expect(std::get<pluss::Timeline>(slot.at(0)).ticks() ==
             std::vector<std::int64_t>{ticks[0]},
         "TimeIndex-to-Slot migration must preserve each scalar timestamp");
  expect(slot.to_json().find("Timeline") != std::string::npos,
         "Slot serialization must emit canonical Timeline cells");
  expect(slot.to_json().find("TimeIndex") == std::string::npos,
         "Slot serialization must not emit legacy TimeIndex cells");
  const auto slot_round_trip = slot.kindelize().to_slate();
  expect(slot_round_trip.kind() == pluss::table::SlateKind::Timeline,
         "Slot, Stream, and Slate round trips must preserve canonical Timeline kind");
  expect(slot_round_trip.as_timeline().ticks() == ticks,
         "Slot, Stream, and Slate round trips must preserve timestamp order");

  const pluss::kindle::Catalog catalog(
      std::vector<std::pair<std::string, pluss::kindle::Stream>>{{"when", stream}});
  expect(catalog.to_json().find("TimeIndex") == std::string::npos,
         "Catalog serialization must not reintroduce the legacy TimeIndex kind");
}

void test_duration_slate_foundation() {
  const auto values = pluss::duration::nanos(std::vector<std::int64_t>{
      1'000'000'000LL,
      pluss::Duration::kNaT,
      -2'000'000'000LL,
  });
  const pluss::table::Slate slate(values);

  expect(slate.kind() == pluss::table::SlateKind::Duration,
         "Slate must preserve Duration as its own runtime kind");
  expect(slate.ptype() == "Duration", "Slate must report the Duration public type");
  expect(slate.dtype() == "duration64[ns]", "Slate must report nanosecond duration dtype");
  expect(slate.as_duration().host() == values.host(),
         "Slate must not convert Duration storage to another core type");

  const auto numeric = slate.cast().series();
  expect(numeric.kind() == pluss::table::SlateKind::Series,
         "Duration-to-Series cast must produce a numeric Slate");
  expect(numeric.as_series().scalar_type() == pluss::codem::ScalarType::Int64,
         "Duration-to-Series cast must preserve nanoseconds as Int64");
  expect(numeric.as_series().host<std::int64_t>() == values.host(),
         "Duration-to-Series cast must preserve exact signed nanoseconds and NaT");

  const auto restored = numeric.cast().duration();
  expect(restored.kind() == pluss::table::SlateKind::Duration,
         "Int64 Series-to-Duration cast must restore the temporal kind");
  expect(restored.as_duration().host() == values.host(),
         "Int64 Series-to-Duration cast must preserve every nanosecond");

  const auto rendered = slate.cast().note();
  expect(rendered.kind() == pluss::table::SlateKind::Note,
         "Duration-to-Note cast must produce text");
  expect(rendered.as_note().vector() == values.to_note_iso().vector(),
         "Duration-to-Note cast must use duration notation");

  expect_throw([&] { (void)slate.cast().timeline(); },
               "Duration must not be reinterpreted as epoch timestamps");

  const auto selected = slate.take(std::vector<std::int64_t>{2, 0});
  expect(selected.kind() == pluss::table::SlateKind::Duration,
         "Slate::take must preserve Duration kind");
  expect(selected.as_duration().host() ==
             std::vector<std::int64_t>{-2'000'000'000LL, 1'000'000'000LL},
         "Slate::take must preserve requested Duration order");

  const pluss::table::Slate scalar(
      pluss::duration::nanos(std::vector<std::int64_t>{1'500'000'000LL}));
  expect(scalar.value().host().kind() ==
             pluss::table::SlateValue::HostAny::Kind::Duration,
         "SlateValue must distinguish duration nanoseconds from epoch nanoseconds");
  expect(scalar.value().to_int() == 1'500'000'000LL,
         "SlateValue must expose exact duration nanoseconds");
  expect(scalar.value().to_str() == scalar.as_duration().to_note_iso().scalar(),
         "SlateValue must use Duration formatting rather than timestamp formatting");

  expect_throw(
      [] {
        (void)pluss::table::Slate(pluss::duration::nanos(
            std::vector<std::int64_t>{1, 2},
            std::vector<std::int64_t>{1, 2}));
      },
      "Slate must reject non-rank-one Duration payloads");

  const pluss::table::Slate floating(pluss::make_series({1.0, 2.0}));
  expect_throw([&] { (void)floating.cast().duration(); },
               "floating-point Series must not silently truncate into Duration");

  const pluss::table::Slate decimal_text(
      pluss::make_note({" -3000000001 ", "0", "4500000002"}));
  const auto parsed = decimal_text.cast().duration();
  expect(parsed.as_duration().host() ==
             std::vector<std::int64_t>{-3'000'000'001LL, 0, 4'500'000'002LL},
         "Note-to-Duration cast must parse exact signed decimal nanoseconds");
  expect_throw(
      [&] {
        (void)pluss::table::Slate(pluss::make_note({"1s"})).cast().duration();
      },
      "Note-to-Duration cast must not guess units from formatted duration text");

  const auto nullable = pluss::table::Slate(
      pluss::make_note({"12", "invalid", "-9"}))
                            .cast({.invalid = pluss::table::ParseInvalid::Null})
                            .duration();
  expect(nullable.as_duration().host() == std::vector<std::int64_t>{12, 0, -9},
         "nulling Duration parse must retain exact values in valid lanes");
  expect(pluss::table::detail::validity_mask(nullable)->host<bool>() ==
             std::vector<bool>{true, false, true},
         "nulling Duration parse must invalidate only malformed lanes");
  expect(slate.info_string().find("Kind: Duration") != std::string::npos,
         "Slate diagnostics must identify Duration explicitly");

  const auto filled = slate.fillnull(pluss::duration::nanos(9));
  expect(filled.as_duration().host() ==
             std::vector<std::int64_t>{1'000'000'000LL, 9, -2'000'000'000LL},
         "Duration fillnull must replace intrinsic NaT without changing observed lanes");
  expect(!pluss::table::detail::has_validity_mask(filled),
         "Duration fillnull must clear absence after every absent lane is replaced");

  const auto replaced = filled.replace(
      pluss::duration::nanos(9), pluss::duration::nanos(-11));
  expect(replaced.as_duration().host() ==
             std::vector<std::int64_t>{1'000'000'000LL, -11, -2'000'000'000LL},
         "Duration replace must compare and substitute exact nanoseconds");
  const auto nulled = replaced.replace(
      pluss::duration::nanos(-11), pluss::series::null);
  expect(pluss::table::detail::validity_mask(nulled)->host<bool>() ==
             std::vector<bool>{true, false, true},
         "Duration replace-to-null must update validity without changing payload kind");

  const pluss::table::Slate repeated(pluss::duration::nanos({7, -2, 7, 4}));
  expect(repeated.mode().as_duration().host() == std::vector<std::int64_t>{7},
         "Duration mode must preserve the Duration kind and exact winning value");
}

void test_duration_io_boundary() {
  const pluss::table::DataFrame frame({
      {"elapsed", pluss::table::Slate(pluss::duration::nanos(
          std::vector<std::int64_t>{-3'000'000'001LL, 0, 4'500'000'002LL}))},
  });

  const auto path = std::filesystem::temp_directory_path() /
                    "plussmic_duration_migration.csv";
  frame.save_csv(path.string());
  std::ifstream input(path, std::ios::binary);
  const std::string csv((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());
  std::filesystem::remove(path);
  expect(csv == "elapsed\n-3000000001\n0\n4500000002\n",
         "CSV output must preserve exact signed Duration nanoseconds");

  const auto filtered = frame.sql().execute(
      "SELECT elapsed FROM df WHERE elapsed < 1 ORDER BY elapsed");
  expect(filtered.features("elapsed").as_series().host<std::int64_t>() ==
             std::vector<std::int64_t>{-3'000'000'001LL, 0},
         "SQLite binding must preserve Duration nanoseconds for predicates and ordering");
}

void test_duration_stream_boundary() {
  const auto values = pluss::duration::nanos(std::vector<std::int64_t>{
      -3'000'000'000LL,
      0,
      4'500'000'000LL,
  });
  const pluss::kindle::Stream stream(values);

  expect(stream.kind() == pluss::kindle::StreamKind::Duration,
         "Stream must preserve Duration as its own runtime kind");
  expect(stream.ptype() == "Duration", "Stream must report the Duration public type");
  expect(stream.dtype() == "duration64[ns]",
         "Stream must report nanosecond duration dtype");
  expect(stream.as_duration().host() == values.host(),
         "Stream must retain exact signed duration nanoseconds");

  const auto round_trip = stream.to_slate();
  expect(round_trip.kind() == pluss::table::SlateKind::Duration,
         "Stream-to-Slate conversion must preserve Duration kind");
  expect(round_trip.as_duration().host() == values.host(),
         "Stream-to-Slate conversion must preserve Duration values");

  const auto rebuilt = pluss::kindle::Stream::from_slate(round_trip);
  expect(rebuilt.kind() == pluss::kindle::StreamKind::Duration,
         "Slate-to-Stream conversion must preserve Duration kind");
  expect(rebuilt.take(std::vector<std::int64_t>{2, 0}).as_duration().host() ==
             std::vector<std::int64_t>{4'500'000'000LL, -3'000'000'000LL},
         "row selection must preserve Duration values and order");

  expect(stream.to_json().find("\"kind\": \"Duration\"") != std::string::npos,
         "Stream JSON must identify Duration without timestamp relabeling");

  const auto slot = stream.scribbelize();
  expect(slot.kind() == pluss::scribble::SlotKind::Duration,
         "Stream-to-Slot conversion must preserve Duration kind");
  expect(std::get<pluss::Duration>(slot.at(2)).host()[0] == 4'500'000'000LL,
         "Duration Slot cells must retain exact signed nanoseconds");

  const auto slot_stream = slot.kindelize();
  expect(slot_stream.kind() == pluss::kindle::StreamKind::Duration,
         "Slot-to-Stream conversion must preserve Duration kind");
  expect(slot_stream.as_duration().host() == values.host(),
         "Slot-to-Stream conversion must preserve Duration order");

  const pluss::scribble::Registry registry({{"elapsed", slot}});
  const auto catalog = registry.kindelize();
  expect(catalog.keys("elapsed").kind() == pluss::kindle::StreamKind::Duration,
         "Registry-to-Catalog conversion must preserve Duration kind");
  expect(catalog.keys("elapsed").as_duration().host() == values.host(),
         "Registry-to-Catalog conversion must preserve Duration values");
  expect(catalog.keyaxis().duration().keys() == std::vector<std::string>{"elapsed"},
         "Catalog duration key selection must recognize Duration streams");
  expect(registry.fieldaxis().duration().has("elapsed"),
         "Registry duration field selection must recognize Duration slots");
  const pluss::scribble::Axel axel(std::vector<pluss::scribble::Registry>{registry});
  expect(axel.fieldaxis().duration().registry(0).has("elapsed"),
         "Axel duration field selection must preserve each registry");
  const pluss::kindle::Archive archive(
      std::vector<pluss::kindle::Catalog>{catalog});
  expect(archive.keyaxis().duration().catalog(0).has("elapsed"),
         "Archive duration key selection must preserve each catalog");

  const auto missing = pluss::kindle::Stream(pluss::duration::nanos(
      std::vector<std::int64_t>{1, pluss::Duration::kNaT, 3}));
  expect(missing.fillnull(pluss::duration::nanos(7)).as_duration().host() ==
             std::vector<std::int64_t>{1, 7, 3},
         "Stream duration fill must replace intrinsic NaT exactly");

  const auto appended = stream.to_slate().append(pluss::table::Slate(
      pluss::duration::nanos(std::vector<std::int64_t>{2'000'000'000LL})));
  expect(appended.kind() == pluss::table::SlateKind::Duration,
         "Slate::append must preserve Duration kind");
  expect(appended.as_duration().host() ==
             std::vector<std::int64_t>{
                 -3'000'000'000LL, 0, 4'500'000'000LL, 2'000'000'000LL},
         "Slate::append must preserve exact Duration order");

  const auto sorted = appended.arrange();
  expect(sorted.as_duration().host() ==
             std::vector<std::int64_t>{
                 -3'000'000'000LL, 0, 2'000'000'000LL, 4'500'000'000LL},
         "Slate::arrange must order Duration by signed nanoseconds");
  expect(appended.min().as_duration().host() ==
             std::vector<std::int64_t>{-3'000'000'000LL},
         "Slate::min must return a Duration scalar");
  expect(appended.max().as_duration().host() ==
             std::vector<std::int64_t>{4'500'000'000LL},
         "Slate::max must return a Duration scalar");

  const auto labeled = stream.to_slate().label().set(
      pluss::table::Slate(pluss::make_note({"b", "a", "c"})));
  const auto aligned = labeled.align(
      pluss::table::Slate(pluss::make_note({"c", "b", "d"})),
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Caselbl});
  expect(aligned.kind() == pluss::table::SlateKind::Duration,
         "label alignment must preserve Duration kind");
  const auto aligned_values = aligned.as_duration().host();
  expect(aligned_values.size() == 3 &&
             aligned_values[0] == 4'500'000'000LL &&
             aligned_values[1] == -3'000'000'000LL,
         "label alignment must reorder observed Duration lanes");
  expect(pluss::table::detail::validity_mask(aligned)->host<bool>() ==
             std::vector<bool>{true, true, false},
         "label alignment must mark an absent Duration row invalid");
}

void test_duration_dataframe_boundary() {
  const pluss::table::DataFrame frame({
      {"elapsed", pluss::table::Slate(pluss::duration::nanos(
          std::vector<std::int64_t>{1, 2, 3}))},
      {"lag", pluss::table::Slate(pluss::duration::nanos(
          std::vector<std::int64_t>{10, 20, 30}))},
  });

  const auto blocks = frame.matrix();
  expect(blocks.duration_span().size() == 1,
         "DataFrame::matrix must retain Duration columns in a dedicated block");
  const auto matrix = blocks.as_duration();
  expect(std::vector<std::int64_t>(matrix.sizes().begin(), matrix.sizes().end()) ==
             std::vector<std::int64_t>{3, 2},
         "DataFrame::matrix Duration block must use [rows, columns] shape");
  expect(matrix.host() == std::vector<std::int64_t>{1, 10, 2, 20, 3, 30},
         "DataFrame::matrix must pack Duration columns in row-major order");
  expect(frame.feataxis().duration().features() ==
             std::vector<std::string>{"elapsed", "lag"},
         "DataFrame duration feature selection must preserve feature order");

  const pluss::table::DataFrame encoded({
      {"elapsed", pluss::table::Slate(pluss::make_note({"-3", "0", "9"}))},
  });
  expect(encoded.cast().duration().features("elapsed").as_duration().host() ==
             std::vector<std::int64_t>{-3, 0, 9},
         "DataFrame duration cast must parse exact decimal nanoseconds");

  const pluss::table::DataFrame absent({
      {"elapsed", pluss::table::Slate(pluss::duration::nanos(
          std::vector<std::int64_t>{1, pluss::Duration::kNaT, 3}))},
      {"name", pluss::table::Slate(pluss::make_note({"a", "b", "c"}))},
  });
  const auto filled = absent.fillnull(pluss::duration::nanos(8));
  expect(filled.features("elapsed").as_duration().host() ==
             std::vector<std::int64_t>{1, 8, 3},
         "DataFrame::fillnull(Duration) must target Duration features only");
  expect(filled.features("name").as_note().vector() ==
             std::vector<std::string_view>{"a", "b", "c"},
         "DataFrame::fillnull(Duration) must preserve unrelated feature kinds");

  const auto replaced = filled.replace(
      pluss::duration::nanos(8), pluss::duration::nanos(-5));
  expect(replaced.features("elapsed").as_duration().host() ==
             std::vector<std::int64_t>{1, -5, 3},
         "DataFrame::replace(Duration, Duration) must preserve Duration columns");
}

void test_dataframe_case_study_accessor() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto frame = DataFrame({
      {"city", Slate(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},
      {"units", Slate(pluss::make_series({10, 7, 12, 5}))},
      {"active", Slate(pluss::make_series({true, false, true, false}))},
  }).labels().set(Slate(pluss::make_note({"A", "B", "B", "D"})));

  expect(frame.case_study().at(-1).features("city").as_note().at(0) == "Tamale",
         "case_study().at must normalize negative positions");
  expect(frame.case_study().slice(1, 3).ncases() == 2,
         "case_study().slice must use half-open bounds");
  expect(frame.case_study().slice(-3, -1).features("units").as_series().host<std::int64_t>() ==
             std::vector<std::int64_t>{7, 12},
         "case_study().slice must normalize negative bounds");
  expect(frame.case_study().slice(3, 1).ncases() == 0,
         "case_study().slice must return an empty frame for reversed bounds");
  expect(frame.case_study().take({3, 0, 3}).features("units").as_series().host<std::int64_t>() ==
             std::vector<std::int64_t>{5, 10, 5},
         "case_study().take must preserve requested order and duplicates");
  const std::vector<std::int64_t> rows{2, 0};
  expect(frame.case_study().take(pluss::RangeView<std::int64_t>(rows))
                 .features("units").as_series().host<std::int64_t>() ==
             std::vector<std::int64_t>{12, 10},
         "case_study().take must accept a borrowed position range");
  expect(frame.case_study().take({}).shape() ==
             std::pair<std::int64_t, std::int64_t>{0, 3},
         "case_study().take must preserve the schema for an empty gather");
  expect(frame.case_study().labels("B").ncases() == 2,
         "case_study().labels must retain every duplicate label match");
  expect(frame.case_study().labels({"D", "A"}).features("city").as_note().vector() ==
             std::vector<std::string_view>{"Tamale", "Accra"},
         "case_study().labels must follow requested label order");
  expect(frame.case_study().span("B", "D").ncases() == 3,
         "case_study().span must include both label endpoints");
  expect(frame.case_study().span("B", "B").ncases() == 2,
         "case_study().span must cover all duplicate endpoint labels");
  expect(frame.case_study().span("D", "A").ncases() == 0,
         "case_study().span must return an empty frame for reversed endpoints");
  const Slate start(pluss::make_note({"A"}));
  const Slate stop(pluss::make_note({"B"}));
  expect(frame.case_study().span(start, stop).ncases() == 3,
         "case_study().span must accept scalar Slate endpoints");
  expect(frame.case_study().where(frame.features("active")).features({"city", "units"}).shape() ==
             std::pair<std::int64_t, std::int64_t>{2, 2},
         "case_study().where must compose with ordinary feature projection");
  expect(frame.case_study().where(pluss::make_series({false, true, true, false})).ncases() == 2,
         "case_study().where must accept a positional Bool Series");

  const auto structured = DataFrame({
      {"region", Slate(pluss::make_note({"West", "West", "East"}))},
      {"account", Slate(pluss::make_note({"101", "102", "201"}))},
      {"sales", Slate(pluss::make_series({3, 5, 8}))},
  }).labels().set({"region", "account"});
  expect(structured.case_study().pick(
             DataFrame::CasePick{.label = "region", .value = "West"}).ncases() == 2,
         "case_study().pick must support partial structured-label matches");
  expect(structured.case_study().pick(
             DataFrame::CasePick{.key = {"East", "201"}}).features("sales")
             .as_series().host<std::int64_t>() == std::vector<std::int64_t>{8},
         "case_study().pick must support full structured keys");

  expect_throw([&] { (void)frame.case_study().at(9); },
               "case_study().at must reject out-of-range positions");
  expect_throw([&] { (void)frame.case_study().take({0, 4}); },
               "case_study().take must reject any out-of-range position");
  expect_throw([&] { (void)frame.case_study().labels("missing"); },
               "case_study().labels must reject absent labels");
  expect_throw([&] { (void)frame.case_study().span("A", "missing"); },
               "case_study().span must reject absent endpoints");
  expect_throw([&] { (void)frame.case_study().where(pluss::make_series({1, 0, 1, 0})); },
               "case_study().where must reject non-Bool masks");
  expect_throw([&] { (void)frame.case_study().where(pluss::make_series({true, false})); },
               "case_study().where must reject length-mismatched masks");

  const auto numbered = DataFrame({{"v", Slate(pluss::make_series({4, 9, 16, 25}))}})
      .labels().set(Slate(pluss::make_series({10, 20, 30, 40})));
  expect(numbered.case_study().labels(20).features("v").as_series().host<std::int64_t>() ==
             std::vector<std::int64_t>{9},
         "case_study().labels must accept integer labels");
  expect(numbered.case_study().span(20, 30).features("v").as_series().host<std::int64_t>() ==
             std::vector<std::int64_t>{9, 16},
         "case_study().span must accept integer label bounds");
}

} // namespace

int main() {
  test_timeline_index_entry();
  test_duration_slate_foundation();
  test_duration_io_boundary();
  test_duration_stream_boundary();
  test_duration_dataframe_boundary();
  test_dataframe_case_study_accessor();
  return 0;
}
