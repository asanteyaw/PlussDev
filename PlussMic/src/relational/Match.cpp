#include <optional>
#include <memory>
#include "relational/Match.h"

#include <algorithm>
#include <cstddef>

#include "CodeM/core/Errors.h"
#include "MSharp/Parallel.h"

namespace pluss::relational {

namespace {

constexpr std::int64_t kBlock = 16384;  // left records per parallel block

inline std::uint64_t mix(std::uint64_t x) {
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return x;
}

inline std::uint64_t hash_record(const std::vector<const std::int64_t*>& cols, std::int64_t i) {
  std::uint64_t h = 0x9e3779b97f4a7c15ULL;
  for (const auto* col : cols) {
    h = mix(h ^ static_cast<std::uint64_t>(col[i]));
  }
  return h;
}

inline bool keyed(const std::vector<const std::int64_t*>& cols, std::int64_t i) {
  for (const auto* col : cols) {
    if (col[i] == kNoKey) return false;
  }
  return true;
}

// Open-addressing table over the keyed records of one side. Records with equal
// keys form a chain through next_ in ascending record order.
class KeyTable {
public:
  explicit KeyTable(const KeyColumns& side) : side_(side) {
    const auto n = static_cast<std::size_t>(side.records);
    next_.assign(n, -1);
    std::int64_t keyed_count = 0;
    for (std::int64_t i = 0; i < side.records; ++i) keyed_count += keyed(side.cols, i) ? 1 : 0;
    keyed_ = keyed_count;
    std::size_t cap = 16;
    while (cap < static_cast<std::size_t>(keyed_count) * 2) cap <<= 1;
    mask_ = cap - 1;
    slots_.assign(cap, -1);
    // Insert in descending order so each chain is ascending.
    for (std::int64_t i = side.records - 1; i >= 0; --i) {
      if (!keyed(side.cols, i)) continue;
      std::size_t s = hash_record(side.cols, i) & mask_;
      for (;;) {
        const auto head = slots_[s];
        if (head < 0) {
          slots_[s] = i;
          break;
        }
        if (equal_records(head, i)) {
          next_[static_cast<std::size_t>(i)] = head;
          slots_[s] = i;
          unique_ = false;
          break;
        }
        s = (s + 1) & mask_;
      }
    }
  }

  bool unique() const { return unique_; }
  std::int64_t keyed_count() const { return keyed_; }

  // First record on this side equal to record `i` of `other` (or -1).
  std::int64_t find(const KeyColumns& other, std::int64_t i) const {
    std::size_t s = hash_record(other.cols, i) & mask_;
    for (;;) {
      const auto head = slots_[s];
      if (head < 0) return -1;
      bool eq = true;
      for (std::size_t c = 0; c < side_.cols.size(); ++c) {
        if (side_.cols[c][head] != other.cols[c][i]) {
          eq = false;
          break;
        }
      }
      if (eq) return head;
      s = (s + 1) & mask_;
    }
  }

  std::int64_t next(std::int64_t record) const { return next_[static_cast<std::size_t>(record)]; }

private:
  bool equal_records(std::int64_t a, std::int64_t b) const {
    for (const auto* col : side_.cols) {
      if (col[a] != col[b]) return false;
    }
    return true;
  }

  const KeyColumns& side_;
  std::vector<std::int64_t> next_{};
  std::vector<std::int64_t> slots_{};
  std::size_t mask_{0};
  std::int64_t keyed_{0};
  bool unique_{true};
};

struct Core {
  std::vector<std::int64_t> left{};
  std::vector<std::int64_t> right{};
};

// Left-driven match: every left record, in order, with its right matches.
// emit_matched: output matched pairs; emit_unmatched: output (l, -1) for left
// records with no partner.
Core probe_left(const KeyColumns& L, const KeyColumns& R, const KeyTable& table, bool emit_matched,
                bool emit_unmatched) {
  const std::int64_t nblocks = (L.records + kBlock - 1) / kBlock;
  std::vector<std::int64_t> block_count(static_cast<std::size_t>(nblocks), 0);

  const auto count_block = [&](std::int64_t b) {
    const std::int64_t lo = b * kBlock, hi = std::min(L.records, lo + kBlock);
    std::int64_t total = 0;
    for (std::int64_t i = lo; i < hi; ++i) {
      std::int64_t first = keyed(L.cols, i) ? table.find(L, i) : -1;
      if (first < 0) {
        total += emit_unmatched ? 1 : 0;
      } else if (emit_matched) {
        for (std::int64_t r = first; r >= 0; r = table.next(r)) ++total;
      }
    }
    block_count[static_cast<std::size_t>(b)] = total;
  };
  pluss::msharp::parallel_for(0, nblocks, 1, [&](std::int64_t b0, std::int64_t b1) {
    for (std::int64_t b = b0; b < b1; ++b) count_block(b);
  });

  std::vector<std::int64_t> offset(static_cast<std::size_t>(nblocks) + 1, 0);
  for (std::int64_t b = 0; b < nblocks; ++b) offset[static_cast<std::size_t>(b) + 1] = offset[static_cast<std::size_t>(b)] + block_count[static_cast<std::size_t>(b)];

  Core out;
  out.left.resize(static_cast<std::size_t>(offset.back()));
  out.right.resize(static_cast<std::size_t>(offset.back()));
  (void)R;

  const auto fill_block = [&](std::int64_t b) {
    const std::int64_t lo = b * kBlock, hi = std::min(L.records, lo + kBlock);
    auto p = static_cast<std::size_t>(offset[static_cast<std::size_t>(b)]);
    for (std::int64_t i = lo; i < hi; ++i) {
      std::int64_t first = keyed(L.cols, i) ? table.find(L, i) : -1;
      if (first < 0) {
        if (emit_unmatched) {
          out.left[p] = i;
          out.right[p] = -1;
          ++p;
        }
      } else if (emit_matched) {
        for (std::int64_t r = first; r >= 0; r = table.next(r)) {
          out.left[p] = i;
          out.right[p] = r;
          ++p;
        }
      }
    }
  };
  pluss::msharp::parallel_for(0, nblocks, 1, [&](std::int64_t b0, std::int64_t b1) {
    for (std::int64_t b = b0; b < b1; ++b) fill_block(b);
  });
  return out;
}

void check_side(const KeyColumns& side, const char* name, std::string_view where) {
  for (const auto* col : side.cols) {
    PLUSS_CHECK(col != nullptr || side.records == 0,
                std::string(where) + ": " + name + " key column is missing");
  }
}

}  // namespace

Expansion expand(const std::vector<std::int64_t>& root,
                 const std::vector<std::int64_t>& mid,
                 const MatchResult& next,
                 std::int64_t mid_records,
                 bool keep_unmatched) {
  PLUSS_CHECK(root.size() == mid.size(), "relational::expand: root and mid lists differ in length");
  // Partners of each mid record, as a counting sort over the matched pairs.
  std::vector<std::int64_t> start(static_cast<std::size_t>(mid_records) + 1, 0);
  for (std::size_t p = 0; p < next.left.size(); ++p) {
    if (next.left[p] >= 0 && next.right[p] >= 0) ++start[static_cast<std::size_t>(next.left[p]) + 1];
  }
  for (std::size_t i = 1; i < start.size(); ++i) start[i] += start[i - 1];
  std::vector<std::int64_t> partners(static_cast<std::size_t>(start.back()));
  std::vector<std::int64_t> cursor(start.begin(), start.end() - 1);
  for (std::size_t p = 0; p < next.left.size(); ++p) {
    if (next.left[p] >= 0 && next.right[p] >= 0) {
      partners[static_cast<std::size_t>(cursor[static_cast<std::size_t>(next.left[p])]++)] = next.right[p];
    }
  }

  Expansion out;
  out.root.reserve(root.size());
  out.leaf.reserve(root.size());
  for (std::size_t k = 0; k < root.size(); ++k) {
    const auto m = mid[k];
    std::int64_t lo = 0, hi = 0;
    if (m >= 0 && m < mid_records) {
      lo = start[static_cast<std::size_t>(m)];
      hi = start[static_cast<std::size_t>(m) + 1];
    }
    if (lo == hi) {
      if (keep_unmatched) {
        out.root.push_back(root[k]);
        out.leaf.push_back(-1);
      }
      continue;
    }
    for (auto q = lo; q < hi; ++q) {
      out.root.push_back(root[k]);
      out.leaf.push_back(partners[static_cast<std::size_t>(q)]);
    }
  }
  return out;
}

UniquenessReport measure_uniqueness(const KeyColumns& side) {
  UniquenessReport out;
  if (side.records == 0) return out;
  KeyTable table(side);
  out.unique = table.unique();
  out.keyed = table.keyed_count();
  out.unkeyed = side.records - out.keyed;
  return out;
}

namespace {
MatchResult match_impl(const KeyColumns& left, const KeyColumns& right, const KeyTable* prebuilt_right,
                       const MatchOptions& options);
}  // namespace

struct BuiltSide::Impl {
  explicit Impl(std::vector<std::vector<std::int64_t>> c, std::int64_t n) : codes(std::move(c)), table_input(), table(init(n)) {}
  const KeyColumns& init(std::int64_t n) {
    table_input.records = n;
    for (const auto& col : codes) table_input.cols.push_back(col.data());
    return table_input;
  }
  std::vector<std::vector<std::int64_t>> codes;
  KeyColumns table_input;
  KeyTable table;
};

BuiltSide::BuiltSide(std::vector<std::vector<std::int64_t>> codes, std::int64_t records)
    : impl_(std::make_shared<Impl>(std::move(codes), records)) {}
KeyColumns BuiltSide::view() const { return impl_->table_input; }
std::int64_t BuiltSide::records() const { return impl_->table_input.records; }
std::size_t BuiltSide::bytes() const {
  const auto n = static_cast<std::size_t>(impl_->table_input.records);
  return sizeof(std::int64_t) * n * (impl_->codes.size() + 1) + sizeof(std::int64_t) * 2 * n + 64;
}

MatchResult match(const KeyColumns& left, const KeyColumns& right, const MatchOptions& options) {
  return match_impl(left, right, nullptr, options);
}

MatchResult match(const KeyColumns& left, const BuiltSide& right, const MatchOptions& options) {
  return match_impl(left, right.impl_->table_input, &right.impl_->table, options);
}

namespace {
MatchResult match_impl(const KeyColumns& left, const KeyColumns& right, const KeyTable* prebuilt_right,
                       const MatchOptions& options) {
  MatchResult result;
  auto& report = result.report;
  report.left_records = left.records;
  report.right_records = right.records;

  if (options.how == JoinHow::Cross) {
    const auto total = static_cast<__int128>(left.records) * static_cast<__int128>(right.records);
    PLUSS_CHECK(total <= static_cast<__int128>(std::numeric_limits<std::int32_t>::max()) * 64,
                std::string(options.where) + ": cross join would produce too many records");
    const auto n = static_cast<std::size_t>(total);
    result.left.resize(n);
    result.right.resize(n);
    std::size_t p = 0;
    for (std::int64_t i = 0; i < left.records; ++i) {
      for (std::int64_t j = 0; j < right.records; ++j) {
        result.left[p] = i;
        result.right[p] = j;
        ++p;
      }
    }
    report.pairs = report.matched_pairs = static_cast<std::int64_t>(n);
    return result;
  }

  PLUSS_CHECK(!left.cols.empty() && left.cols.size() == right.cols.size(),
              std::string(options.where) + ": left and right need the same, non-zero number of key columns");
  check_side(left, "left", options.where);
  check_side(right, "right", options.where);

  for (std::int64_t i = 0; i < left.records; ++i) report.left_unkeyed += keyed(left.cols, i) ? 0 : 1;
  for (std::int64_t i = 0; i < right.records; ++i) report.right_unkeyed += keyed(right.cols, i) ? 0 : 1;

  // Hash the side that is not driving the output order; its uniqueness comes free.
  const bool right_driven = options.how == JoinHow::Right || options.how == JoinHow::RightOnly;
  const KeyColumns& drive = right_driven ? right : left;
  const KeyColumns& build = right_driven ? left : right;
  std::optional<KeyTable> local_table;
  const KeyTable* table_ptr = nullptr;
  if (prebuilt_right != nullptr && !right_driven) {
    table_ptr = prebuilt_right;
  } else {
    local_table.emplace(build);
    table_ptr = &*local_table;
  }
  const KeyTable& table = *table_ptr;
  (right_driven ? report.left_unique : report.right_unique) = table.unique();
  (right_driven ? report.left_unique_known : report.right_unique_known) = true;

  // Contract: measured before any pair is produced.
  const auto need_unique = [&](bool left_side) {
    if (!options.validate.has_value()) return false;
    switch (*options.validate) {
      case RelationshipCardinality::OneToOne:
        return true;
      case RelationshipCardinality::OneToMany:
        return left_side;
      case RelationshipCardinality::ManyToOne:
        return !left_side;
      case RelationshipCardinality::ManyToMany:
        return false;
    }
    return false;
  };
  const auto ensure_unique_known = [&](bool left_side) {
    bool& known = left_side ? report.left_unique_known : report.right_unique_known;
    bool& unique = left_side ? report.left_unique : report.right_unique;
    if (known) return;
    unique = measure_uniqueness(left_side ? left : right).unique;
    known = true;
  };
  if (options.measure_left_unique) ensure_unique_known(true);
  if (need_unique(true)) ensure_unique_known(true);
  if (need_unique(false)) ensure_unique_known(false);
  PLUSS_CHECK(!need_unique(true) || report.left_unique,
              std::string(options.where) + ": left keys repeat, contradicting the declared " +
                  (options.validate == RelationshipCardinality::OneToOne ? "one-to-one" : "one-to-many") +
                  " contract");
  PLUSS_CHECK(!need_unique(false) || report.right_unique,
              std::string(options.where) + ": right keys repeat, contradicting the declared " +
                  (options.validate == RelationshipCardinality::OneToOne ? "one-to-one" : "many-to-one") +
                  " contract");

  Core core;
  switch (options.how) {
    case JoinHow::Inner:
      core = probe_left(drive, build, table, true, false);
      break;
    case JoinHow::Left:
    case JoinHow::Outer:
      core = probe_left(drive, build, table, true, true);
      break;
    case JoinHow::LeftOnly:
      core = probe_left(drive, build, table, false, true);
      break;
    case JoinHow::Right:
      core = probe_left(drive, build, table, true, true);
      break;
    case JoinHow::RightOnly:
      core = probe_left(drive, build, table, false, true);
      break;
    case JoinHow::Cross:
      break;
  }
  if (right_driven) std::swap(core.left, core.right);  // probe order was right-driven

  if (options.how == JoinHow::Outer) {
    std::vector<std::uint8_t> hit(static_cast<std::size_t>(right.records), 0);
    for (const auto r : core.right) {
      if (r >= 0) hit[static_cast<std::size_t>(r)] = 1;
    }
    for (std::int64_t r = 0; r < right.records; ++r) {
      if (!hit[static_cast<std::size_t>(r)]) {
        core.left.push_back(-1);
        core.right.push_back(r);
      }
    }
  }

  // The loss report: what matched, what had no partner, what the output omits.
  std::vector<std::uint8_t> lmatched(static_cast<std::size_t>(left.records), 0);
  std::vector<std::uint8_t> rmatched(static_cast<std::size_t>(right.records), 0);
  std::vector<std::uint8_t> lpresent(static_cast<std::size_t>(left.records), 0);
  std::vector<std::uint8_t> rpresent(static_cast<std::size_t>(right.records), 0);
  for (std::size_t p = 0; p < core.left.size(); ++p) {
    const auto l = core.left[p], r = core.right[p];
    if (l >= 0) lpresent[static_cast<std::size_t>(l)] = 1;
    if (r >= 0) rpresent[static_cast<std::size_t>(r)] = 1;
    if (l >= 0 && r >= 0) {
      lmatched[static_cast<std::size_t>(l)] = 1;
      rmatched[static_cast<std::size_t>(r)] = 1;
      ++report.matched_pairs;
    }
  }
  const auto count = [](const std::vector<std::uint8_t>& v) {
    return static_cast<std::int64_t>(std::count(v.begin(), v.end(), std::uint8_t{1}));
  };
  report.pairs = static_cast<std::int64_t>(core.left.size());
  report.left_unmatched = left.records - count(lmatched);
  report.right_unmatched = right.records - count(rmatched);
  report.left_dropped = left.records - count(lpresent);
  report.right_dropped = right.records - count(rpresent);
  result.left = std::move(core.left);
  result.right = std::move(core.right);
  return result;
}

}  // namespace

} // namespace pluss::relational
