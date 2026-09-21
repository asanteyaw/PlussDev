#include <algorithm>
#include <array>
#include <cstdlib>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CodeN/native/cpu/TextKernels.h"
#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

namespace {

pluss::Note make_note_1d(const std::vector<std::string>& values) {
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(values.size())};
  return pluss::make_note(values, std::span<const std::int64_t>(shape.data(), shape.size()));
}

bool contains_value(const pluss::Note& n, std::string_view needle) {
  for (std::int64_t i = 0; i < n.numel(); ++i) {
    if (n.at(i) == needle) return true;
  }
  return false;
}

void set_env_mode(const char* key, const char* mode) {
#if defined(_WIN32)
  _putenv_s(key, (mode == nullptr) ? "" : mode);
#else
  if (mode == nullptr || mode[0] == '\0') {
    unsetenv(key);
  } else {
    setenv(key, mode, 1);
  }
#endif
}

bool notes_equal_with_diag(const pluss::Note& a, const pluss::Note& b, std::string& diag) {
  if (a.defined() != b.defined()) {
    diag = "defined() mismatch";
    return false;
  }
  if (!a.defined()) return true;
  if (a.dim() != b.dim()) {
    diag = "dim mismatch";
    return false;
  }
  if (a.sizes() != b.sizes()) {
    diag = "sizes mismatch";
    return false;
  }
  if (a.numel() != b.numel()) {
    diag = "numel mismatch";
    return false;
  }
  for (std::int64_t i = 0; i < a.numel(); ++i) {
    if (a.at(i) != b.at(i)) {
      std::ostringstream oss;
      oss << "first mismatch at i=" << i
          << " lhs=\"" << std::string(a.at(i)) << "\""
          << " rhs=\"" << std::string(b.at(i)) << "\"";
      diag = oss.str();
      return false;
    }
  }
  diag.clear();
  return true;
}

bool token_batches_equal_with_diag(const pluss::TokenBatch& a,
                                   const pluss::TokenBatch& b,
                                   std::string& diag) {
  const auto a_off = a.offsets().integers();
  const auto b_off = b.offsets().integers();
  if (!std::equal(a_off.begin(), a_off.end(), b_off.begin(), b_off.end())) {
    diag = "offset mismatch";
    return false;
  }
  return notes_equal_with_diag(a.tokens(), b.tokens(), diag);
}

} // namespace

TEST_CASE("CodeN perf regression corpus: sort/unique/deduplicate semantics", "[coden][perf][patch22]") {
  const std::vector<std::string> values{
      "",
      "beta",
      "東京",
      "beta",
      "naïve",
      "",
      "ßeta",
      "alpha",
      "東京",
      "alpha"};
  const auto note = make_note_1d(values);

  SECTION("sort keeps lexicographic non-decreasing order on mixed UTF-8 corpus") {
    const auto sorted = note.sort();
    REQUIRE(sorted.numel() == note.numel());
    for (std::int64_t i = 1; i < sorted.numel(); ++i) {
      REQUIRE(std::string(sorted.at(i - 1)) <= std::string(sorted.at(i)));
    }

    const auto sorted_again = note.sort();
    REQUIRE(sorted_again.numel() == sorted.numel());
    for (std::int64_t i = 0; i < sorted.numel(); ++i) {
      REQUIRE(sorted_again.at(i) == sorted.at(i));
    }
  }

  SECTION("unique emits sorted distinct set") {
    const auto uniq = note.unique();
    REQUIRE(uniq.numel() == 6);
    for (std::int64_t i = 1; i < uniq.numel(); ++i) {
      REQUIRE(std::string(uniq.at(i - 1)) < std::string(uniq.at(i)));
    }
    REQUIRE(contains_value(uniq, ""));
    REQUIRE(contains_value(uniq, "alpha"));
    REQUIRE(contains_value(uniq, "beta"));
    REQUIRE(contains_value(uniq, "naïve"));
    REQUIRE(contains_value(uniq, "ßeta"));
    REQUIRE(contains_value(uniq, "東京"));
  }

  SECTION("deduplicate preserves first-seen order on the same corpus") {
    const auto dedup = note.deduplicate();
    REQUIRE(dedup.numel() == 6);
    REQUIRE(dedup.at(0) == "");
    REQUIRE(dedup.at(1) == "beta");
    REQUIRE(dedup.at(2) == "東京");
    REQUIRE(dedup.at(3) == "naïve");
    REQUIRE(dedup.at(4) == "ßeta");
    REQUIRE(dedup.at(5) == "alpha");
  }

  SECTION("empty token is retained exactly once by unique and deduplicate") {
    const std::vector<std::string> repeated_empty{"", "", "", "alpha", "alpha"};
    const auto t = make_note_1d(repeated_empty);

    const auto uniq = t.unique();
    REQUIRE(uniq.numel() == 2);
    REQUIRE(contains_value(uniq, ""));
    REQUIRE(contains_value(uniq, "alpha"));

    const auto dedup = t.deduplicate();
    REQUIRE(dedup.numel() == 2);
    REQUIRE(dedup.at(0) == "");
    REQUIRE(dedup.at(1) == "alpha");
  }

  SECTION("undefined Note stays undefined through unique/deduplicate") {
    const auto undef = pluss::Note::undefined();
    REQUIRE_FALSE(undef.unique().defined());
    REQUIRE_FALSE(undef.deduplicate().defined());
  }

  SECTION("take gathers by permutation with negative indices and rejects out-of-range indices") {
    const std::vector<std::int64_t> pick{2, -1, 0};
    const auto taken = note.take(std::span<const std::int64_t>(pick.data(), pick.size()));
    REQUIRE(taken.numel() == 3);
    REQUIRE(taken.at(0) == "東京");
    REQUIRE(taken.at(1) == "alpha");
    REQUIRE(taken.at(2) == "");

    const std::vector<std::int64_t> bad_hi{10};
    REQUIRE_THROWS(note.take(std::span<const std::int64_t>(bad_hi.data(), bad_hi.size())));

    const std::vector<std::int64_t> bad_lo{-11};
    REQUIRE_THROWS(note.take(std::span<const std::int64_t>(bad_lo.data(), bad_lo.size())));
  }
}

TEST_CASE("CodeN encoded fast path parity unique/deduplicate", "[coden][perf][patch22][encoded]") {
  const std::vector<std::string> values{
      "alpha", "beta",  "alpha", "東京", "beta", "東京",
      "naïve", "alpha", "ßeta",  "beta", "ßeta", "alpha"};
  const auto note = make_note_1d(values);

  set_env_mode("PLUSS_CODEN_ENCODE_FASTPATH", "0");
  const auto uniq_off = note.unique();
  const auto dedup_off = note.deduplicate();

  set_env_mode("PLUSS_CODEN_ENCODE_FASTPATH", "1");
  const auto uniq_on = note.unique();
  const auto dedup_on = note.deduplicate();

  set_env_mode("PLUSS_CODEN_ENCODE_FASTPATH", nullptr);

  REQUIRE(uniq_off.numel() == uniq_on.numel());
  for (std::int64_t i = 0; i < uniq_off.numel(); ++i) {
    REQUIRE(uniq_off.at(i) == uniq_on.at(i));
  }

  REQUIRE(dedup_off.numel() == dedup_on.numel());
  for (std::int64_t i = 0; i < dedup_off.numel(); ++i) {
    REQUIRE(dedup_off.at(i) == dedup_on.at(i));
  }
}

TEST_CASE("CodeN text kernels compare/hash UTF-8 edge checks", "[coden][perf][patch22][kernels]") {
  using op = pluss::coden::native::cpu::LexCompareOp;
  namespace tk = pluss::coden::native::cpu;

  REQUIRE(tk::compare_lex_bytes("alpha", "alpha", op::Eq));
  REQUIRE_FALSE(tk::compare_lex_bytes("alpha", "beta", op::Eq));
  REQUIRE(tk::compare_lex_bytes("naïve", "東京", op::Lt));
  REQUIRE(tk::compare_lex_bytes("東京", "naïve", op::Gt));
  REQUIRE(tk::compare_lex_bytes("ßeta", "ßeta", op::Le));

  const std::array<std::string_view, 4> lhs{"a", "beta", "東京", "ßeta"};
  const std::array<std::string_view, 4> rhs{"a", "alpha", "東京", "zeta"};
  std::array<std::uint8_t, 4> out{};
  tk::compare_lex_bytes_batch(
      std::span<const std::string_view>(lhs.data(), lhs.size()),
      std::span<const std::string_view>(rhs.data(), rhs.size()),
      op::Gt,
      std::span<std::uint8_t>(out.data(), out.size()));
  REQUIRE(out[0] == 0);
  REQUIRE(out[1] == 1);
  REQUIRE(out[2] == 0);
  REQUIRE(out[3] == 1);

  const auto h0 = tk::hash_bytes_fnv1a("alpha");
  const auto h1 = tk::hash_bytes_fnv1a("alpha");
  const auto h2 = tk::hash_bytes_fnv1a("beta");
  REQUIRE(h0 == h1);
  REQUIRE(h0 != h2);

  std::array<std::uint64_t, 4> hb{};
  tk::hash_bytes_fnv1a_batch(
      std::span<const std::string_view>(lhs.data(), lhs.size()),
      std::span<std::uint64_t>(hb.data(), hb.size()));
  REQUIRE(hb[0] == tk::hash_bytes_fnv1a(lhs[0]));
  REQUIRE(hb[1] == tk::hash_bytes_fnv1a(lhs[1]));
  REQUIRE(hb[2] == tk::hash_bytes_fnv1a(lhs[2]));
  REQUIRE(hb[3] == tk::hash_bytes_fnv1a(lhs[3]));
}

TEST_CASE("CodeN nlp_keywords frequency ordering and stopword filtering", "[coden][perf][patch22][count]") {
  const std::array<std::int64_t, 1> shape{2};
  const auto note = pluss::make_note(
      std::vector<std::string>{
          "beta alpha beta gamma alpha alpha the and beta",
          "omega omega zeta zeta and the"},
      std::span<const std::int64_t>(shape.data(), shape.size()));

  const auto kw = note.nlp().keywords(3);
  const auto kw_offsets = kw.offsets().integers();
  REQUIRE(kw_offsets.size() == 3);
  REQUIRE(kw_offsets[0] == 0);
  REQUIRE(kw_offsets[1] == 3);
  REQUIRE(kw_offsets[2] == 5);

  // Row 0: alpha(3), beta(3), gamma(1); tie alpha/beta resolved lexicographically.
  REQUIRE(kw.tokens().at(0) == "alpha");
  REQUIRE(kw.tokens().at(1) == "beta");
  REQUIRE(kw.tokens().at(2) == "gamma");

  // Row 1: omega(2), zeta(2); tie resolved lexicographically.
  REQUIRE(kw.tokens().at(3) == "omega");
  REQUIRE(kw.tokens().at(4) == "zeta");

  const auto stop_only = pluss::make_note("the and in to with");
  const auto kw_stop_only = stop_only.nlp().keywords(5);
  const auto kw_stop_only_offsets = kw_stop_only.offsets().integers();
  REQUIRE(kw_stop_only_offsets.size() == 2);
  REQUIRE(kw_stop_only_offsets[0] == 0);
  REQUIRE(kw_stop_only_offsets[1] == 0);
  REQUIRE(kw_stop_only.tokens().numel() == 0);
}

TEST_CASE("CodeN parallel lane execution parity (sort + tokenbatch)", "[coden][perf][patch22][parallel]") {
  // Build many rows so tokenbatch and lane sort thresholds are crossed.
  std::vector<std::string> rows;
  rows.reserve(32768);
  for (int i = 0; i < 32768; ++i) {
    rows.push_back((i % 2 == 0) ? "beta alpha beta gamma alpha alpha the and beta"
                                : "omega omega zeta zeta and the");
  }
  const std::array<std::int64_t, 2> shape{512, 64};
  const auto note2d = pluss::make_note(rows, std::span<const std::int64_t>(shape.data(), shape.size()));

  set_env_mode("PLUSS_CODEN_PARALLEL", "off");
  const auto sorted_off = note2d.sort();
  const auto kw_off = note2d.nlp().keywords(3);

  set_env_mode("PLUSS_CODEN_PARALLEL", "on");
  const auto sorted_on = note2d.sort();
  const auto kw_on = note2d.nlp().keywords(3);

  set_env_mode("PLUSS_CODEN_PARALLEL", nullptr);

  bool same_sort = (sorted_off.numel() == sorted_on.numel());
  for (std::int64_t i = 0; same_sort && i < sorted_off.numel(); ++i) {
    if (sorted_off.at(i) != sorted_on.at(i)) same_sort = false;
  }
  REQUIRE(same_sort);

  const auto kw_off_offsets = kw_off.offsets().integers();
  const auto kw_on_offsets = kw_on.offsets().integers();
  REQUIRE(std::equal(kw_off_offsets.begin(), kw_off_offsets.end(),
                     kw_on_offsets.begin(), kw_on_offsets.end()));
  bool same_kw = (kw_off.tokens().numel() == kw_on.tokens().numel());
  for (std::int64_t i = 0; same_kw && i < kw_off.tokens().numel(); ++i) {
    if (kw_off.tokens().at(i) != kw_on.tokens().at(i)) same_kw = false;
  }
  REQUIRE(same_kw);
}

TEST_CASE("CodeN reference mode parity across optimized routes", "[coden][perf][patch22][reference]") {
  std::vector<std::string> rows;
  rows.reserve(32768);
  for (int i = 0; i < 32768; ++i) {
    rows.push_back((i % 2 == 0) ? "beta alpha beta gamma alpha alpha the and beta"
                                : "omega omega zeta zeta and the");
  }
  const std::array<std::int64_t, 2> shape2d{512, 64};
  const auto note2d = pluss::make_note(rows, std::span<const std::int64_t>(shape2d.data(), shape2d.size()));
  const auto note1d = make_note_1d(rows);
  const auto scalar = pluss::make_note("alpha omega");

  // Reference run.
  set_env_mode("PLUSS_CODEN_EXECUTION_MODE", "reference");
  set_env_mode("PLUSS_CODEN_PARALLEL", "on");
  set_env_mode("PLUSS_CODEN_ENCODE_FASTPATH", "force");
  set_env_mode("PLUSS_CODEN_COUNT_FASTPATH", "on");
  const auto sort_ref = note2d.sort();
  const auto unique_ref = note1d.unique();
  const auto dedup_ref = note1d.deduplicate();
  const auto kw_ref = note2d.nlp().keywords(3);
  const auto inter_ref = note2d.nlp().intersect_tokens(scalar);

  // Optimized run.
  set_env_mode("PLUSS_CODEN_EXECUTION_MODE", "optimized");
  const auto sort_opt = note2d.sort();
  const auto unique_opt = note1d.unique();
  const auto dedup_opt = note1d.deduplicate();
  const auto kw_opt = note2d.nlp().keywords(3);
  const auto inter_opt = note2d.nlp().intersect_tokens(scalar);

  set_env_mode("PLUSS_CODEN_EXECUTION_MODE", nullptr);
  set_env_mode("PLUSS_CODEN_PARALLEL", nullptr);
  set_env_mode("PLUSS_CODEN_ENCODE_FASTPATH", nullptr);
  set_env_mode("PLUSS_CODEN_COUNT_FASTPATH", nullptr);

  std::string diag;
  INFO(diag);
  REQUIRE(notes_equal_with_diag(sort_ref, sort_opt, diag));
  REQUIRE(notes_equal_with_diag(unique_ref, unique_opt, diag));
  REQUIRE(notes_equal_with_diag(dedup_ref, dedup_opt, diag));
  REQUIRE(token_batches_equal_with_diag(kw_ref, kw_opt, diag));
  REQUIRE(token_batches_equal_with_diag(inter_ref, inter_opt, diag));
}
