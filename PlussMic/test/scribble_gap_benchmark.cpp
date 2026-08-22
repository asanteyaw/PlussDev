#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::scribble::Axel;
using pluss::scribble::Registry;
using pluss::scribble::Slot;
using Clock = std::chrono::steady_clock;

template <class Fn>
double elapsed_ms(Fn&& fn) {
  const auto started = Clock::now();
  fn();
  return std::chrono::duration<double, std::milli>(Clock::now() - started).count();
}

Slot make_series_slot(std::int64_t n, std::int64_t null_every = 0) {
  std::vector<Slot::Value> out;
  out.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    if (null_every > 0 && i % null_every == 0) {
      out.emplace_back(std::monostate{});
    } else {
      out.emplace_back(pluss::make_series(static_cast<double>(i % 1000)));
    }
  }
  return Slot(std::move(out));
}

Slot make_note_slot(std::int64_t n, std::int64_t null_every = 0) {
  std::vector<Slot::Value> out;
  out.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    if (null_every > 0 && i % null_every == 0) {
      out.emplace_back(std::monostate{});
    } else {
      const auto year = 2024 + static_cast<int>((i / 366) % 2);
      const auto month = 1 + static_cast<int>((i % 12));
      const auto day = 1 + static_cast<int>((i % 28));
      char buf[16];
      std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
      out.emplace_back(pluss::make_note(std::string(buf)));
    }
  }
  return Slot(std::move(out));
}

Slot make_mixed_slot(std::int64_t n) {
  std::vector<Slot::Value> out;
  out.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    switch (i % 4) {
      case 0:
        out.emplace_back(pluss::make_series(static_cast<double>(i % 1000)));
        break;
      case 1:
        out.emplace_back(pluss::make_note("tag_" + std::to_string(i % 128)));
        break;
      case 2:
        out.emplace_back(pluss::timeline::from_epoch(1704067200000000000LL + i * 86400000000000LL));
        break;
      default:
        out.emplace_back(std::monostate{});
        break;
    }
  }
  return Slot(std::move(out));
}

Registry make_registry(std::int64_t n) {
  return Registry({
      {"value", make_series_slot(n, 17)},
      {"opened", make_note_slot(n, 19)},
      {"meta", make_mixed_slot(n)},
  });
}

Axel make_axel(std::int64_t depth, std::int64_t n) {
  std::vector<Registry> regs;
  regs.reserve(static_cast<std::size_t>(depth));
  for (std::int64_t i = 0; i < depth; ++i) {
    regs.push_back(make_registry(n));
  }
  return Axel(std::move(regs));
}

} // namespace

int main(int argc, char** argv) {
  const std::int64_t rows =
      argc > 1 ? std::max<std::int64_t>(1000, std::stoll(argv[1])) : 200000;
  const std::int64_t depth =
      argc > 2 ? std::max<std::int64_t>(1, std::stoll(argv[2])) : 8;

  const auto series_slot = make_series_slot(rows, 17);
  const auto note_slot = make_note_slot(rows, 19);
  const auto mixed_slot = make_mixed_slot(rows);
  const auto reg = make_registry(rows);
  const auto ax = make_axel(depth, rows);
  volatile std::int64_t sink = 0;

  const auto slot_fillnull = elapsed_ms([&] {
    const auto result = series_slot.fillnull(0.0);
    sink += result.size();
  });
  const auto slot_note_to_timeline = elapsed_ms([&] {
    const auto result = note_slot.cast({
        .format = "%Y-%m-%d",
        .invalid = pluss::table::ParseInvalid::Null,
    }).timeline();
    sink += result.size();
  });
  const auto slot_mixed_to_note = elapsed_ms([&] {
    const auto result = mixed_slot.cast().note();
    sink += result.size();
  });
  const auto slot_mixed_isnull = elapsed_ms([&] {
    const auto result = mixed_slot.isnull();
    sink += result.size();
  });
  const auto registry_isnull = elapsed_ms([&] {
    const auto result = reg.fields({"meta", "value"}).isnull();
    sink += result.nfields();
  });
  const auto registry_cast = elapsed_ms([&] {
    const auto result = reg.fields({"opened"}).cast(
        {.format = "%Y-%m-%d", .invalid = pluss::table::ParseInvalid::Null}).timeline();
    sink += result.nfields();
  });
  const auto registry_fillnull = elapsed_ms([&] {
    const auto result = reg.fields({"value"}).fillnull(0.0);
    sink += result.nfields();
  });
  const auto axel_cast = elapsed_ms([&] {
    const auto result = ax.fields({"opened"}).cast(
        {.format = "%Y-%m-%d", .invalid = pluss::table::ParseInvalid::Null}).timeline();
    sink += result.depth();
  });

  std::cout << "{\"rows\":" << rows
            << ",\"depth\":" << depth
            << ",\"slot_fillnull_ms\":" << slot_fillnull
            << ",\"slot_note_timeline_ms\":" << slot_note_to_timeline
            << ",\"slot_mixed_note_ms\":" << slot_mixed_to_note
            << ",\"slot_mixed_isnull_ms\":" << slot_mixed_isnull
            << ",\"registry_isnull_ms\":" << registry_isnull
            << ",\"registry_cast_ms\":" << registry_cast
            << ",\"registry_fillnull_ms\":" << registry_fillnull
            << ",\"axel_cast_ms\":" << axel_cast
            << ",\"sink\":" << sink << "}\n";
}
