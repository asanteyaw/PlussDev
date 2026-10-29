#pragma once

// MPS reader (fixed or free format; names without embedded spaces): ROWS (N/L/G/E), COLUMNS with INTORG/INTEND markers,
// RHS (an RHS on the objective row is the negated objective constant), RANGES, BOUNDS (UP LO FX FR MI PL BV LI UI; SC treated as UP),
// OBJSENSE MAX/MIN. The first N row is the objective; further N rows are dropped. A maximisation is returned as the negated
// minimisation with `negated_objective = true` so the caller can flip the reported value back.

#include "pluss/subsp/math/lp/sparse.h"

#include <fstream>
#include <map>
#include <sstream>
#include <unordered_map>

namespace pluss::subsp::math::lp {

struct MpsModel final {
  Model model;
  bool negated_objective{false};
  std::string name;
};

namespace detail {
// Fixed-format field extraction (columns 2-3, 5-12, 15-22, 25-36, 40-47, 50-61); blank fields are dropped, inner spaces of names kept.
inline std::vector<std::string> fixed_fields(const std::string &line) {
  static const int begin[6] = {1, 4, 14, 24, 39, 49}, end[6] = {3, 12, 22, 36, 47, 61};
  std::vector<std::string> out;
  for (int k = 0; k < 6; ++k) {
    if (static_cast<int>(line.size()) <= begin[k])
      break;
    std::string f = line.substr(static_cast<std::size_t>(begin[k]), static_cast<std::size_t>(end[k] - begin[k]));
    const auto a = f.find_first_not_of(' ');
    if (a == std::string::npos)
      continue;
    const auto b = f.find_last_not_of(' ');
    out.push_back(f.substr(a, b - a + 1));
  }
  return out;
}
} // namespace detail

inline MpsModel read_mps(std::istream &in, bool fixed = false) {
  MpsModel out;
  Model &m = out.model;
  std::unordered_map<std::string, std::int32_t> row_id, col_id;
  std::vector<char> row_type;     // 'L','G','E' per constraint row
  std::vector<double> rhs;        // per constraint row
  std::vector<double> range;      // per constraint row (NaN = none)
  std::string objective_row;
  std::vector<std::int32_t> tr, tc;
  std::vector<double> tv;
  bool in_integer_block = false, maximize = false;
  std::string line, section;
  auto is_number = [](const std::string &s) {
    if (s.empty())
      return false;
    char *end = nullptr;
    std::strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0';
  };
  auto get_col = [&](const std::string &name) -> std::int32_t {
    auto it = col_id.find(name);
    if (it != col_id.end())
      return it->second;
    const auto id = static_cast<std::int32_t>(m.col_names.size());
    col_id[name] = id;
    m.col_names.push_back(name);
    m.cost.push_back(0.0);
    m.col_lo.push_back(0.0);
    m.col_hi.push_back(kInf);
    m.integer.push_back(in_integer_block ? 1 : 0);
    return id;
  };
  std::vector<std::string> pending_bound_cols; // columns needing integer default bounds
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '*')
      continue;
    std::vector<std::string> tok;
    if (fixed && (line[0] == ' ' || line[0] == '\t') ) {
      tok = detail::fixed_fields(line);
    } else {
      std::istringstream ss(line);
      for (std::string t; ss >> t;)
        tok.push_back(t);
    }
    if (tok.empty())
      continue;
    if (line[0] != ' ' && line[0] != '\t') { // section header
      section = tok[0];
      if (section == "NAME")
        out.name = tok.size() > 1 ? tok[1] : "";
      if (section == "OBJSENSE" && tok.size() > 1)
        maximize = tok[1] == "MAX" || tok[1] == "MAXIMIZE";
      if (section == "ENDATA")
        break;
      continue;
    }
    if (section == "OBJSENSE") {
      maximize = tok[0] == "MAX" || tok[0] == "MAXIMIZE";
    } else if (section == "ROWS") {
      if (tok[0] == "N") {
        if (objective_row.empty())
          objective_row = tok[1];
        else
          row_id[tok[1]] = -2; // extra free row: ignored
      } else {
        row_id[tok[1]] = static_cast<std::int32_t>(row_type.size());
        row_type.push_back(tok[0][0]);
        rhs.push_back(0.0);
        range.push_back(std::numeric_limits<double>::quiet_NaN());
        m.row_names.push_back(tok[1]);
      }
    } else if (section == "COLUMNS") {
      if (tok.size() >= 3 && tok[1] == "'MARKER'") {
        in_integer_block = tok[2] == "'INTORG'";
        continue;
      }
      const auto j = get_col(tok[0]);
      for (std::size_t k = 1; k + 1 < tok.size(); k += 2) {
        const double v = std::stod(tok[k + 1]);
        if (tok[k] == objective_row) {
          m.cost[static_cast<std::size_t>(j)] = v;
        } else {
          auto it = row_id.find(tok[k]);
          if (it == row_id.end())
            throw std::runtime_error("mps: unknown row " + tok[k]);
          if (it->second >= 0) {
            tr.push_back(it->second);
            tc.push_back(j);
            tv.push_back(v);
          }
        }
      }
    } else if (section == "RHS" || section == "RANGES") {
      std::size_t k = (tok.size() % 2 == 1) ? 1 : 0;
      for (; k + 1 < tok.size(); k += 2) {
        const double v = std::stod(tok[k + 1]);
        if (tok[k] == objective_row) {
          if (section == "RHS")
            m.offset = -v;
          continue;
        }
        auto it = row_id.find(tok[k]);
        if (it == row_id.end() || it->second < 0)
          continue;
        if (section == "RHS")
          rhs[static_cast<std::size_t>(it->second)] = v;
        else
          range[static_cast<std::size_t>(it->second)] = v;
      }
    } else if (section == "BOUNDS") {
      const std::string &type = tok[0];
      const bool valueless = type == "FR" || type == "MI" || type == "PL" || type == "BV";
      std::string cname;
      double v = 0.0;
      if (valueless) {
        cname = tok.size() >= 3 ? tok[2] : tok[1];
      } else if (tok.size() >= 4) {
        cname = tok[2];
        v = std::stod(tok[3]);
      } else if (tok.size() == 3) {
        cname = tok[1];
        v = std::stod(tok[2]);
      } else {
        throw std::runtime_error("mps: malformed BOUNDS line: " + line);
      }
      auto it = col_id.find(cname);
      if (it == col_id.end())
        continue;
      const auto j = static_cast<std::size_t>(it->second);
      if (type == "UP" || type == "UI" || type == "SC") {
        m.col_hi[j] = v;
        if (v < 0.0 && m.col_lo[j] == 0.0)
          m.col_lo[j] = -kInf;
        if (type == "UI")
          m.integer[j] = 1;
      } else if (type == "LO" || type == "LI") {
        m.col_lo[j] = v;
        if (type == "LI")
          m.integer[j] = 1;
      } else if (type == "FX") {
        m.col_lo[j] = m.col_hi[j] = v;
      } else if (type == "FR") {
        m.col_lo[j] = -kInf;
        m.col_hi[j] = kInf;
      } else if (type == "MI") {
        m.col_lo[j] = -kInf;
      } else if (type == "PL") {
        m.col_hi[j] = kInf;
      } else if (type == "BV") {
        m.col_lo[j] = 0.0;
        m.col_hi[j] = 1.0;
        m.integer[j] = 1;
      }
    }
  }
  m.cols = static_cast<std::int32_t>(m.cost.size());
  m.rows = static_cast<std::int32_t>(row_type.size());
  m.a = SparseMatrix::from_triplets(m.rows, m.cols, tr, tc, tv);
  m.row_lo.resize(static_cast<std::size_t>(m.rows));
  m.row_hi.resize(static_cast<std::size_t>(m.rows));
  for (std::int32_t i = 0; i < m.rows; ++i) {
    const double b = rhs[static_cast<std::size_t>(i)], r = range[static_cast<std::size_t>(i)];
    double lo = -kInf, hi = kInf;
    switch (row_type[static_cast<std::size_t>(i)]) {
    case 'L': hi = b; lo = std::isnan(r) ? -kInf : b - std::abs(r); break;
    case 'G': lo = b; hi = std::isnan(r) ? kInf : b + std::abs(r); break;
    default:
      lo = hi = b;
      if (!std::isnan(r)) {
        if (r >= 0.0)
          hi = b + r;
        else
          lo = b + r;
      }
    }
    m.row_lo[static_cast<std::size_t>(i)] = lo;
    m.row_hi[static_cast<std::size_t>(i)] = hi;
  }
  if (maximize) {
    for (auto &c : m.cost)
      c = -c;
    m.offset = -m.offset;
    out.negated_objective = true;
  }
  return out;
}

// Reads a file; fixed format (names with spaces) is detected from the ROWS section (free format has exactly two fields per row line).
inline MpsModel read_mps_file(const std::string &path) {
  std::ifstream f(path);
  if (!f)
    throw std::runtime_error("mps: cannot open " + path);
  bool fixed = false, in_rows = false;
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '*')
      continue;
    if (line[0] != ' ' && line[0] != '\t') {
      in_rows = line.rfind("ROWS", 0) == 0;
      if (line.rfind("COLUMNS", 0) == 0)
        break;
      continue;
    }
    if (in_rows) {
      std::istringstream ss(line);
      int count = 0;
      for (std::string t; ss >> t;)
        ++count;
      if (count > 2) {
        fixed = true;
        break;
      }
    }
  }
  f.clear();
  f.seekg(0);
  return read_mps(f, fixed);
}

} // namespace pluss::subsp::math::lp
