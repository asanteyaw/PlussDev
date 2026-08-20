#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::make_note;
using pluss::make_series;
using pluss::make_timeindex;
using pluss::make_timeline;
using pluss::pl::flash::Dataset;
using pluss::pl::flash::FramePolicy;
using pluss::pl::flash::FromFrameOptions;
using pluss::pl::flash::ToFrameOptions;
using pluss::table::DataFrame;
using pluss::table::Thread;

pluss::Series bool_mask(std::initializer_list<bool> flags) {
  auto out = pluss::Series::zeros({static_cast<std::int64_t>(flags.size())}, pluss::ScalarType::Bool);
  auto* ptr = reinterpret_cast<bool*>(out.raw_data());
  std::size_t i = 0;
  for (const bool flag : flags) {
    ptr[i++] = flag;
  }
  return out;
}

void print_note_csv(std::string_view label, const pluss::Note& note) {
  std::cout << label << "=";
  for (std::int64_t i = 0; i < note.numel(); ++i) {
    if (i > 0) {
      std::cout << ",";
    }
    std::cout << note.at(i);
  }
  std::cout << "\n";
}

void print_bool_series_csv(std::string_view label, const pluss::Series& values) {
  const auto host = values.to(pluss::Platform::CPU).to_vec<bool>();
  std::cout << label << "=";
  for (std::size_t i = 0; i < host.size(); ++i) {
    if (i > 0) {
      std::cout << ",";
    }
    std::cout << (host[i] ? 1 : 0);
  }
  std::cout << "\n";
}

std::string first_line(const pluss::Error& err) {
  std::string text = err.what();
  const auto pos = text.find('\n');
  if (pos != std::string::npos) {
    text.resize(pos);
  }
  return text;
}

void example_from_frame_basic() {
  std::cout << "== from_frame_basic ==\n";
  const auto mask = bool_mask({true, false, true});
  const auto frame = DataFrame({
      {"city", Thread(make_note({"Accra", "", "Kumasi"}), mask)},
      {"score", Thread(make_series(std::vector<double>{91.5, 0.0, 88.0}), mask)},
      {"stamp", Thread(make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
      }), mask)},
  }).title("grades");

  const auto data = Dataset::from_table(frame);
  print_note_csv("order", *data.feature_order);
  print_note_csv("kinds", *data.feature_kinds);
  std::cout << "counts="
            << data.numeric_values.size() << ","
            << data.text_values.size() << ","
            << data.temporal_values.size() << "\n";
  std::cout << "city_valid=" << data.text_validity_mask("city").has_value() << "\n";
  std::cout << "score_valid=" << data.numeric_validity_mask("score").has_value() << "\n";
  std::cout << "stamp_valid=" << data.temporal_validity_mask("stamp").has_value() << "\n";
  std::cout << "\n";
}

void example_from_frame_subset() {
  std::cout << "== from_frame_subset ==\n";
  const auto frame = DataFrame({
      {"city", make_note({"Accra", "Tema", "Kumasi"})},
      {"score", make_series(std::vector<double>{91.5, 84.0, 88.0})},
      {"stamp", make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
      })},
  });

  const auto data = Dataset::from_table(
      frame,
      {.features = {"stamp", "city"}, .include_series = false});
  print_note_csv("order", *data.feature_order);
  print_note_csv("kinds", *data.feature_kinds);
  std::cout << "counts="
            << data.numeric_values.size() << ","
            << data.text_values.size() << ","
            << data.temporal_values.size() << "\n";
  std::cout << "\n";
}

void example_from_frame_timeindex() {
  std::cout << "== from_frame_timeindex ==\n";
  const auto frame = DataFrame({
      {"event_idx", make_timeindex({
          "2026-01-01T09:00:00Z",
          "2026-01-01T10:00:00Z",
          "2026-01-01T11:00:00Z",
      })},
  });

  const auto data = Dataset::from_table(frame);
  print_note_csv("order", *data.feature_order);
  print_note_csv("kinds", *data.feature_kinds);
  std::cout << "temporal_present=" << data.temporal("event_idx").has_value() << "\n";
  std::cout << "\n";
}

void example_from_frame_timeindex_disabled() {
  std::cout << "== from_frame_timeindex_disabled ==\n";
  const auto frame = DataFrame({
      {"event_idx", make_timeindex({
          "2026-01-01T09:00:00Z",
          "2026-01-01T10:00:00Z",
      })},
  });

  try {
    (void)Dataset::from_table(frame, {.include_timeindex_as_timeline = false});
  } catch (const pluss::Error& err) {
    std::cout << "error=" << first_line(err) << "\n";
  }
  std::cout << "\n";
}

void example_from_frame_text_disabled() {
  std::cout << "== from_frame_text_disabled ==\n";
  const auto frame = DataFrame({
      {"city", make_note({"Accra", "Tema"})},
      {"score", make_series(std::vector<double>{91.5, 84.0})},
  });

  try {
    (void)Dataset::from_table(frame, {.include_text = false});
  } catch (const pluss::Error& err) {
    std::cout << "error=" << first_line(err) << "\n";
  }
  std::cout << "\n";
}

void example_frame_rectangular() {
  std::cout << "== frame_rectangular ==\n";
  const auto frame = DataFrame({
      {"city", make_note({"Accra", "Tema", "Kumasi"})},
      {"score", make_series(std::vector<double>{91.5, 84.0, 88.0})},
      {"stamp", make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
      })},
  }).title("grades");

  const auto data = Dataset::from_table(frame);
  const auto rebuilt = data.frame({.title = "rebuilt"});
  std::cout << rebuilt << "\n";
  std::cout << "title=" << rebuilt.title() << "\n";
  std::cout << "\n";
}

void example_frame_padnull() {
  std::cout << "== frame_padnull ==\n";
  Dataset data;
  data.feature_order = make_note({"time", "value", "label"});
  data.feature_kinds = make_note({"temporal", "numeric", "text"});
  data.temporal_names = make_note({"time"});
  data.temporal_values.push_back(make_timeline({
      "2026-01-01T00:00:00Z",
      "2026-01-02T00:00:00Z",
  }));
  data.temporal_validity.push_back(std::nullopt);
  data.numeric_names = make_note({"value"});
  data.numeric_values.push_back(make_series(std::vector<double>{10.0, 20.0, 30.0}));
  data.numeric_validity.push_back(std::nullopt);
  data.text_names = make_note({"label"});
  data.text_values.push_back(make_note({"alpha", "beta"}));
  data.text_validity.push_back(std::nullopt);

  const auto padded = data.frame({.policy = FramePolicy::PadNull, .title = "padded"});
  std::cout << padded << "\n";
  print_bool_series_csv("time_valid", padded.features("time").notnull().as_series());
  print_bool_series_csv("label_valid", padded.features("label").notnull().as_series());
  std::cout << "\n";
}

void example_frame_feature_order() {
  std::cout << "== frame_feature_order ==\n";
  Dataset data;
  data.feature_order = make_note({"label", "value"});
  data.feature_kinds = make_note({"text", "numeric"});
  data.numeric_names = make_note({"value"});
  data.numeric_values.push_back(make_series(std::vector<double>{10.0, 20.0}));
  data.numeric_validity.push_back(std::nullopt);
  data.text_names = make_note({"label"});
  data.text_values.push_back(make_note({"alpha", "beta"}));
  data.text_validity.push_back(std::nullopt);

  const auto rebuilt = data.frame();
  std::cout << rebuilt << "\n";
  print_note_csv("features", pluss::make_note(rebuilt.features()));
  std::cout << "\n";
}

void example_frame_strict_error() {
  std::cout << "== frame_strict_error ==\n";
  Dataset data;
  data.feature_order = make_note({"time", "value"});
  data.feature_kinds = make_note({"temporal", "numeric"});
  data.temporal_names = make_note({"time"});
  data.temporal_values.push_back(make_timeline({
      "2026-01-01T00:00:00Z",
      "2026-01-02T00:00:00Z",
  }));
  data.temporal_validity.push_back(std::nullopt);
  data.numeric_names = make_note({"value"});
  data.numeric_values.push_back(make_series(std::vector<double>{10.0, 20.0, 30.0}));
  data.numeric_validity.push_back(std::nullopt);

  try {
    (void)data.frame();
  } catch (const pluss::Error& err) {
    std::cout << "error=" << first_line(err) << "\n";
  }
  std::cout << "\n";
}

} // namespace

int main() {
  example_from_frame_basic();
  example_from_frame_subset();
  example_from_frame_timeindex();
  example_from_frame_timeindex_disabled();
  example_from_frame_text_disabled();
  example_frame_rectangular();
  example_frame_padnull();
  example_frame_feature_order();
  example_frame_strict_error();
  return 0;
}
