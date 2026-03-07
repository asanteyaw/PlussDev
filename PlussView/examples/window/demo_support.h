#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <ranges>
#include <span>
#include <string_view>
#include <thread>

#include "pluss/view/window.h"

namespace pluss::view::window::demo {

namespace detail {

[[nodiscard]] inline int parse_int(const std::string_view value, const int fallback) {
  int parsed = fallback;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end) {
    return fallback;
  }
  return parsed;
}

[[nodiscard]] inline int env_int(const std::string_view key, const int fallback) {
  std::array<char, 64> local_key{};
  if (key.empty() || key.size() >= local_key.size()) {
    return fallback;
  }

  std::ranges::copy(key, local_key.begin());
  const char* raw = std::getenv(local_key.data());
  if (raw == nullptr) {
    return fallback;
  }
  return parse_int(std::string_view{raw}, fallback);
}

} // namespace detail

[[nodiscard]] inline std::chrono::milliseconds demo_hold_duration(
    const std::chrono::milliseconds fallback = std::chrono::milliseconds{-1}) {
  const int ms = detail::env_int("PLUSSVIEW_DEMO_HOLD_MS", static_cast<int>(fallback.count()));
  return std::chrono::milliseconds{ms};
}

inline void hold_shell_open(IWindowShell& shell, const std::chrono::milliseconds duration) {
  if (duration.count() == 0) {
    return;
  }

  const bool timed = duration.count() > 0;
  const auto deadline =
      timed ? (std::chrono::steady_clock::now() + duration) : std::chrono::steady_clock::time_point{};
  constexpr auto frame_sleep = std::chrono::milliseconds{16};

  while (!timed || std::chrono::steady_clock::now() < deadline) {
    const auto events = shell.poll_events();
    const bool closed = std::ranges::any_of(events, [](const ShellEvent& event) {
      return event.kind == ShellEventKind::Closed;
    });
    if (closed) {
      break;
    }
    std::this_thread::sleep_for(frame_sleep);
  }
}

inline void run_session_preview(IWindowSession& session, const std::chrono::milliseconds duration) {
  if (duration.count() == 0) {
    return;
  }

  const bool timed = duration.count() > 0;
  const auto deadline =
      timed ? (std::chrono::steady_clock::now() + duration) : std::chrono::steady_clock::time_point{};
  constexpr auto frame_sleep = std::chrono::milliseconds{16};
  while (!timed || std::chrono::steady_clock::now() < deadline) {
    if (!session.tick()) {
      break;
    }
    std::this_thread::sleep_for(frame_sleep);
  }
}

inline void run_registry_preview(WindowRegistry& registry,
                                 const std::span<const WindowSessionId> ids,
                                 const std::chrono::milliseconds duration) {
  if (duration.count() == 0 || ids.empty()) {
    return;
  }

  const bool timed = duration.count() > 0;
  const auto deadline =
      timed ? (std::chrono::steady_clock::now() + duration) : std::chrono::steady_clock::time_point{};
  constexpr auto frame_sleep = std::chrono::milliseconds{16};
  while (!timed || std::chrono::steady_clock::now() < deadline) {
    const bool all_ok = std::ranges::all_of(ids, [&registry](const WindowSessionId id) {
      return registry.tick(id);
    });
    if (!all_ok) {
      break;
    }
    std::this_thread::sleep_for(frame_sleep);
  }
}

} // namespace pluss::view::window::demo
