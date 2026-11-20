// NaN and infinity are values, never missing cells. These tests pin the isnan / isinf / dropnan / fillnan family (and its notnan /
// notinf / dropinf / fillinf partners) on every layer: Shelf, DataFrame, Atlas, Stream, Catalog, Archive, Slot, Registry, Axel.
#include <cmath>
#include <filesystem>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"
#include "pluss/tils.h"
#include "bundle/Locus.h"
#include "table/display/Display.h"
#include "bundle/Nexus.h"

namespace {

using pluss::kindle::Archive;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using pluss::scribble::Axel;
using pluss::scribble::Registry;
using pluss::scribble::Slot;
using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Shelf;

constexpr double kNan = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

void expect(bool condition, std::string_view msg) {
  if (!condition) throw std::runtime_error(std::string(msg));
}

std::vector<bool> flags(const pluss::Series& s) {
  const auto host = s.to(pluss::codem::ScalarType::Bool).host<bool>();
  return {host.begin(), host.end()};
}
std::vector<bool> flags(const Shelf& s) { return flags(s.as_series()); }
std::vector<bool> flags(const Stream& s) { return flags(s.as_series()); }
std::vector<double> doubles(const pluss::Series& s) {
  const auto host = s.to(pluss::codem::ScalarType::Float64).host<double>();
  return {host.begin(), host.end()};
}
using B = std::vector<bool>;

// Rows: 1, NaN, +inf, -inf, NULL(masked), 5
Shelf make_shelf() {
  const auto values = pluss::make_series(std::vector<double>{1.0, kNan, kInf, -kInf, 0.0, 5.0});
  const auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true}).to(pluss::codem::ScalarType::Bool);
  return Shelf(values, present);
}

void test_shelf() {
  const auto s = make_shelf();
  expect(flags(s.isnull()) == B({0, 0, 0, 0, 1, 0}), "isnull marks only the masked cell");
  expect(flags(s.isnan()) == B({0, 1, 0, 0, 0, 0}), "isnan marks only NaN (a NULL is not NaN)");
  expect(flags(s.notnan()) == B({1, 0, 1, 1, 1, 1}), "notnan is the complement of isnan (NULL is not NaN)");
  expect(flags(s.isinf()) == B({0, 0, 1, 1, 0, 0}), "isinf marks +inf and -inf");
  expect(flags(s.notinf()) == B({1, 1, 0, 0, 1, 1}), "notinf is the complement of isinf");
  expect(flags(s.invalid()) == B({0, 1, 1, 1, 1, 0}), "invalid is NULL or NaN or inf");
  expect(s.dropnan().size() == 5, "dropnan removes the NaN row only");
  expect(s.dropinf().size() == 4, "dropinf removes both infinities");
  expect(flags(s.dropnan().isnull()) == B({0, 0, 0, 1, 0}), "dropnan keeps the NULL row");
  const auto filled = s.fillnan(0.0);
  expect(doubles(filled.as_series())[1] == 0.0, "fillnan replaces NaN");
  expect(std::isinf(doubles(filled.as_series())[2]), "fillnan leaves inf alone");
  expect(flags(filled.isnull()) == B({0, 0, 0, 0, 1, 0}), "fillnan leaves NULL as NULL");
  const auto finf = s.fillinf(9.0);
  expect(doubles(finf.as_series())[2] == 9.0 && doubles(finf.as_series())[3] == 9.0, "fillinf replaces both infinities");
  expect(std::isnan(doubles(finf.as_series())[1]), "fillinf leaves NaN alone");
  // fillnull fills only NULL, never NaN.
  const auto fnull = s.fillnull(7.0);
  expect(std::isnan(doubles(fnull.as_series())[1]) && doubles(fnull.as_series())[4] == 7.0, "fillnull fills NULL but not NaN");
  // A non-float payload can hold neither.
  const Shelf ints(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}));
  expect(flags(ints.isnan()) == B({0, 0, 0}) && flags(ints.notnan()) == B({1, 1, 1}), "integers hold no NaN");
  expect(flags(ints.isinf()) == B({0, 0, 0}) && ints.dropinf().size() == 3, "integers hold no inf");
}

Stream make_stream();

// The catch-all family, the sign-specific infinity flags and the per-column count report.
void test_invalid_sign_and_counts() {
  // Integers are always finite: invalid() / isfinite() answer for them instead of raising an error.
  const Shelf whole(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}));
  expect(flags(whole.invalid()) == B({0, 0, 0}), "an integer shelf has no invalid cell");
  expect(flags(whole.isfinite()) == B({1, 1, 1}), "an integer is finite");
  const auto s = make_shelf();  // 1, NaN, +inf, -inf, NULL, 5
  expect(flags(s.isposinf()) == B({0, 0, 1, 0, 0, 0}), "isposinf marks only +inf");
  expect(flags(s.isneginf()) == B({0, 0, 0, 1, 0, 0}), "isneginf marks only -inf");
  expect(s.nnull() == 1 && s.nnan() == 1 && s.nposinf() == 1 && s.nneginf() == 1, "one cell of each kind");
  // One sign at a time: a cap for +inf and a floor for -inf.
  const auto capped = s.fillposinf(100.0).fillneginf(-100.0);
  expect(doubles(capped.as_series())[2] == 100.0 && doubles(capped.as_series())[3] == -100.0, "fillposinf / fillneginf set each sign");
  expect(std::isinf(doubles(s.fillposinf(100.0).as_series())[3]), "fillposinf leaves -inf alone");
  expect(std::isinf(doubles(s.fillneginf(-100.0).as_series())[2]), "fillneginf leaves +inf alone");
  expect(s.dropposinf().size() == 5 && s.dropneginf().size() == 5, "dropposinf / dropneginf drop one infinity each");
  expect(flags(s.dropposinf().isneginf()) == B({0, 0, 1, 0, 0}), "dropposinf keeps -inf");
  expect(s.dropinvalid().size() == 2, "dropinvalid keeps only the two ordinary values");
  const auto filled = s.fillinvalid(0.0);
  expect(flags(filled.isnull()) == B({0, 0, 0, 0, 0, 0}), "fillinvalid fills NULL");
  expect(doubles(filled.as_series()) == std::vector<double>({1.0, 0.0, 0.0, 0.0, 0.0, 5.0}), "fillinvalid fills NaN and inf");

  const DataFrame df({{"x", s}, {"y", Shelf(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, kNan}))}});
  const auto kinds = df.count_kinds();
  expect(kinds.features("null").as_series().to(pluss::codem::ScalarType::Int64).host<std::int64_t>()[0] == 1, "count_kinds null");
  expect(kinds.features("nan").as_series().to(pluss::codem::ScalarType::Int64).host<std::int64_t>()[1] == 1, "count_kinds nan");
  expect(kinds.features("finite").as_series().to(pluss::codem::ScalarType::Int64).host<std::int64_t>()[0] == 2, "count_kinds finite");
  expect(df.dropinvalid().ncases() == 1, "DataFrame::dropinvalid drops a row when any feature is invalid");
  expect(df.dropinvalid({"y"}).ncases() == 5, "DataFrame::dropinvalid on y drops its NaN row");
  expect(df.dropinvalid({"x", "y"}, true).ncases() == 6, "dropinvalid(all) needs every selected feature invalid");
  expect(flags(df.isposinf().features("x")) == B({0, 0, 1, 0, 0, 0}), "DataFrame::isposinf");
  expect(doubles(df.fillposinf(9.0).features("x").as_series())[2] == 9.0, "DataFrame::fillposinf");
  expect(doubles(df.fillneginf(-9.0).features("x").as_series())[3] == -9.0, "DataFrame::fillneginf");
  expect(df.dropposinf().ncases() == 5 && df.dropneginf().ncases() == 5, "DataFrame::dropposinf / dropneginf");
  expect(Atlas({df, df}).dropposinf().frame(1).ncases() == 5, "Atlas::dropposinf delegates");
  // An Atlas is a strict cube: a drop is decided across the whole stack, so the frames keep the same height.
  const DataFrame tidy({{"x", Shelf(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}))},
                        {"y", Shelf(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}))}});
  const auto mixed = Atlas({df, tidy}).dropnan();
  expect(mixed.frame(0).ncases() == 4 && mixed.frame(1).ncases() == 4, "Atlas::dropnan removes the same rows (1 and 5) from every frame");
  expect(Atlas({df, tidy}).dropnull().frame(1).ncases() == 5, "Atlas::dropnull removes the NULL row from every frame");
  expect(Atlas({df, df}).dropinvalid().frame(1).ncases() == 1, "Atlas::dropinvalid delegates");
  const Catalog cat(std::vector<std::pair<std::string, Stream>>{{"a", make_stream()}});
  expect(cat.dropinvalid().keys("a").size() == 2, "Catalog::dropinvalid");
  expect(cat.count_kinds().features("nan").as_series().to(pluss::codem::ScalarType::Int64).host<std::int64_t>()[0] == 1, "Catalog::count_kinds");
}

void test_dataframe_and_atlas() {
  const DataFrame df({
      {"x", make_shelf()},
      {"i", Shelf(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}))},
      {"y", Shelf(pluss::make_series(std::vector<double>{1.0, 2.0, kNan, 4.0, 5.0, kNan}))},
  });
  const auto nan_flags = df.isnan();
  expect(flags(nan_flags.features("x")) == B({0, 1, 0, 0, 0, 0}), "DataFrame::isnan column x");
  expect(flags(nan_flags.features("i")) == B({0, 0, 0, 0, 0, 0}), "DataFrame::isnan of an integer column is all false");
  expect(flags(nan_flags.features("y")) == B({0, 0, 1, 0, 0, 1}), "DataFrame::isnan column y");
  expect(flags(df.notinf().features("x")) == B({1, 1, 0, 0, 1, 1}), "DataFrame::notinf column x");
  expect(flags(df.isinf().features("y")) == B({0, 0, 0, 0, 0, 0}), "DataFrame::isinf column y");
  // any (default): a row goes if ANY selected float column is NaN; all: only if ALL selected are.
  expect(df.dropnan().ncases() == 3, "dropnan (any) drops rows 1, 2 and 5");
  expect(df.dropnan({"x", "y"}, true).ncases() == 6, "dropnan(all) drops a row only if every selected column is NaN");
  expect(df.dropnan({"x"}).ncases() == 5, "dropnan on x drops the NaN row of x");
  expect(df.dropnan({"i"}).ncases() == 6, "dropnan on a non-float column never drops");
  expect(df.dropinf().ncases() == 4, "dropinf drops the two infinity rows");
  expect(df.fillnan(0.0).features("y").as_series().to(pluss::codem::ScalarType::Float64).host<double>()[2] == 0.0, "fillnan");
  expect(std::isnan(doubles(df.fillinf(1.0).features("x").as_series())[1]), "fillinf leaves NaN");
  expect(flags(df.isnull().features("x")) == B({0, 0, 0, 0, 1, 0}), "isnull sees only the masked cell");
  // Atlas delegates frame by frame.
  const Atlas atlas({df, df});
  expect(atlas.dropnan().frame(0).ncases() == 3, "Atlas::dropnan delegates to every frame");
  expect(atlas.dropinf().frame(1).ncases() == 4, "Atlas::dropinf delegates to every frame");
  expect(flags(atlas.isnan().frame(1).features("y")) == B({0, 0, 1, 0, 0, 1}), "Atlas::isnan delegates to every frame");
}

Stream make_stream() {
  return Stream(pluss::make_series(std::vector<double>{1.0, kNan, kInf, 4.0}, {4}));
}

void test_stream_catalog_archive() {
  const auto s = make_stream();
  expect(flags(s.isnan()) == B({0, 1, 0, 0}), "Stream::isnan");
  expect(flags(s.isinf()) == B({0, 0, 1, 0}), "Stream::isinf");
  expect(flags(s.notnan()) == B({1, 0, 1, 1}) && flags(s.notinf()) == B({1, 1, 0, 1}), "Stream::notnan/notinf");
  expect(s.dropnan().size() == 3 && s.dropinf().size() == 3, "Stream::dropnan/dropinf");
  expect(doubles(s.fillnan(0.0).as_series())[1] == 0.0, "Stream::fillnan");
  expect(doubles(s.fillinf(0.0).as_series())[2] == 0.0, "Stream::fillinf");

  const Catalog cat(std::vector<std::pair<std::string, Stream>>{
      {"a", make_stream()},
      {"b", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, kNan}, {4}))},
  });
  expect(flags(cat.isnan().keys("b")) == B({0, 0, 0, 1}), "Catalog::isnan delegates to every stream");
  expect(cat.dropnan().keys("a").size() == cat.dropnan().keys("b").size(), "Catalog::dropnan keeps streams aligned");
  expect(cat.dropnan().keys("a").size() == 2, "Catalog::dropnan drops a row when ANY stream has NaN there");
  expect(cat.dropinf().keys("a").size() == 3, "Catalog::dropinf");
  const Archive arc(std::vector<Catalog>{cat, cat});
  expect(arc.dropnan().catalog(1).keys("a").size() == 2, "Archive::dropnan delegates to every catalog");
  expect(flags(arc.isinf().catalog(0).keys("a")) == B({0, 0, 1, 0}), "Archive::isinf delegates to every catalog");
}

Slot::Value cell(std::vector<double> v) {
  const auto n = static_cast<std::int64_t>(v.size());
  return pluss::make_series(std::move(v), {n});
}

void test_slot_registry_axel() {
  // Multi-element cells take the cell-level path: a cell is flagged when it contains at least one NaN / inf.
  const Slot slot({cell({1.0, 2.0}), cell({1.0, kNan}), cell({kInf, 2.0}), cell({3.0, 4.0})});
  const auto nan_mask = slot.isnan();
  expect(!std::get<pluss::Series>(nan_mask.at(0)).host<bool>()[0] && std::get<pluss::Series>(nan_mask.at(1)).host<bool>()[0],
         "Slot::isnan flags a cell that contains NaN");
  expect(slot.dropnan().size() == 3, "Slot::dropnan removes cells that contain NaN");
  expect(slot.dropinf().size() == 3, "Slot::dropinf removes cells that contain infinity");
  const auto filled = slot.fillnan(0.0);
  expect(doubles(std::get<pluss::Series>(filled.at(1)))[1] == 0.0, "Slot::fillnan replaces NaN inside a cell");
  // A Series cell is never NULL; only an empty slot entry is.
  expect(slot.dropnull().size() == 4, "Slot::dropnull keeps Series cells, even with NaN inside");

  // Note, Timeline and Duration cells can hold neither NaN nor infinity: they are never flagged, dropped or changed.
  const Slot texts({pluss::make_note(std::vector<std::string>{"a", "b"})});
  expect(texts.dropnan().size() == 1 && texts.dropinf().size() == 1, "Note cells are never dropped by dropnan/dropinf");
  expect(std::get<pluss::Note>(texts.fillnan(0.0).at(0)).numel() == 2, "fillnan leaves Note cells unchanged");

  const Registry reg({{"a", slot}, {"b", Slot({cell({1.0}), cell({2.0}), cell({3.0}), cell({4.0})})}});
  expect(reg.dropnan().fields("a").size() == 3, "Registry::dropnan delegates to its slots");
  expect(reg.isinf().fields("a").size() == 4, "Registry::isinf keeps every slot");
  const Axel axel(std::vector<Registry>{reg, reg});
  expect(axel.dropnan().registry(1).fields("a").size() == 3, "Axel::dropnan delegates to every registry");
}

// Every storage model carries the same missing-value family: NULL via isnull/notnull/invalid/dropnull/fillnull.
void test_null_family_everywhere() {
  const DataFrame df({{"x", make_shelf()}});
  const Atlas atlas({df, df});
  expect(flags(atlas.isnull().frame(0).features("x")) == B({0, 0, 0, 0, 1, 0}), "Atlas::isnull flags only NULL");
  expect(flags(atlas.notnull().frame(1).features("x")) == B({1, 1, 1, 1, 0, 1}), "Atlas::notnull");
  expect(flags(atlas.invalid().frame(0).features("x")) == B({0, 1, 1, 1, 1, 0}), "Atlas::invalid is NULL or NaN or inf");
  expect(atlas.dropnull().frame(0).ncases() == 5, "Atlas::dropnull drops only the NULL row");
  expect(atlas.fillnull(0.0).frame(1).features("x").isnull().as_series().any().item<bool>() == false, "Atlas::fillnull fills NULL");
  expect(std::isnan(doubles(atlas.fillnull(0.0).frame(0).features("x").as_series())[1]), "Atlas::fillnull leaves NaN");

  const pluss::bundle::Nexus nexus({{"a", df}, {"b", df}});
  expect(nexus.dropnan().frame("a").ncases() == 5, "Nexus::dropnan applies to every table");
  expect(nexus.dropnull().frame("b").ncases() == 5, "Nexus::dropnull applies to every table");
  expect(flags(nexus.isinf().frame("a").features("x")) == B({0, 0, 1, 1, 0, 0}), "Nexus::isinf");
  expect(flags(nexus.invalid().frame("b").features("x")) == B({0, 1, 1, 1, 1, 0}), "Nexus::invalid");

  const pluss::bundle::Locus locus({{"a", df.kindelize()}, {"b", df.kindelize()}});
  expect(locus.catalog("a").keys("x").size() == 6, "Locus holds the catalogs");
  expect(locus.dropnan().catalog("a").keys("x").size() == 5, "Locus::dropnan applies to every catalog");
  expect(flags(locus.isnan().catalog("b").keys("x")) == B({0, 1, 0, 0, 0, 0}), "Locus::isnan");
  expect(locus.dropinf().catalog("a").keys("x").size() == 4, "Locus::dropinf");
}

// PlussView spells the kinds in capitals and styles them differently: NULL is "missing" (red), NAN / INF / -INF are values ("nonfinite").
void test_html_spelling_and_classes() {
  const DataFrame frame({{"x", make_shelf()}});
  const auto html = pluss::table::display::render_html(frame, {});
  expect(html.find(">NULL<") != std::string::npos, "the HTML view prints a missing cell as NULL");
  expect(html.find(">NAN<") != std::string::npos && html.find(">INF<") != std::string::npos && html.find(">-INF<") != std::string::npos,
         "the HTML view prints NaN, +inf and -inf in capitals");
  expect(html.find("nonfinite") != std::string::npos, "the HTML view gives NaN and the infinities their own style class");
  const auto null_at = html.find(">NULL<");
  const auto nan_at = html.find(">NAN<");
  const auto null_cell = html.rfind("<td", null_at);
  const auto nan_cell = html.rfind("<td", nan_at);
  expect(html.substr(null_cell, null_at - null_cell).find("missing") != std::string::npos, "NULL keeps the missing (red) class");
  expect(html.substr(nan_cell, nan_at - nan_cell).find("missing") == std::string::npos &&
             html.substr(nan_cell, nan_at - nan_cell).find("nonfinite") != std::string::npos,
         "NaN is styled as a value, never as missing");
}

// A replacement value is one value matched against every entry: Timeline replace used to demand equal lengths.
void test_timeline_replace_scalar() {
  const Shelf stamps(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"}));
  const auto out = stamps.replace(pluss::make_timeline({"2024-02-01T00:00:00Z"}), pluss::make_timeline({"2000-01-01T00:00:00Z"}));
  expect(out.as_timeline().ticks()[1] == pluss::make_timeline({"2000-01-01T00:00:00Z"}).ticks()[0], "the matching timestamp is replaced");
  expect(out.as_timeline().ticks()[0] == stamps.as_timeline().ticks()[0], "other timestamps are untouched");
}

// A workbook of several sheets round-trips through save_excel / load_excel.
void test_workbook_roundtrip() {
  const DataFrame orders({{"id", Shelf(pluss::make_series(std::vector<double>{101, 102, 103}))}});
  const DataFrame regions({{"id", Shelf(pluss::make_series(std::vector<double>{201, 202}))}});
  const auto path = (std::filesystem::temp_directory_path() / "pluss_workbook_roundtrip.xlsx").string();
  pluss::loader::save_excel(pluss::loader::Workbook{{"Orders", orders}, {"Regions", regions}}, path);
  const auto loaded = DataFrame::load_excel(path, nullptr);
  std::filesystem::remove(path);
  expect(loaded.is_workbook() && loaded.workbook().size() == 2, "both sheets are written");
  expect(loaded.sheets("Regions").ncases() == 2 && loaded.sheets("Orders").ncases() == 3, "each sheet keeps its rows");
}

}  // namespace

int main() {
  try {
    test_shelf();
    test_dataframe_and_atlas();
    test_invalid_sign_and_counts();
    test_stream_catalog_archive();
    test_slot_registry_axel();
    test_null_family_everywhere();
    test_html_spelling_and_classes();
    test_timeline_replace_scalar();
    test_workbook_roundtrip();
  } catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  std::cout << "nonfinite family tests passed\n";
  return 0;
}
