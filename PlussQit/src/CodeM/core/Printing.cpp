#include "CodeM/core/Printing.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace pluss::codem {

static void append_scalar_(std::ostringstream& oss, const Series& s, std::int64_t off) {
  switch (s.scalar_type()) {
    case ScalarType::Float64: oss << reinterpret_cast<const double*>(s.raw_data())[off]; return;
    case ScalarType::Float32: oss << reinterpret_cast<const float*>(s.raw_data())[off]; return;
    case ScalarType::Float16: oss << float16_to_float(reinterpret_cast<const Float16*>(s.raw_data())[off]); return;
    case ScalarType::BFloat16: oss << bfloat16_to_float(reinterpret_cast<const BFloat16*>(s.raw_data())[off]); return;
    case ScalarType::Int64: oss << reinterpret_cast<const std::int64_t*>(s.raw_data())[off]; return;
    case ScalarType::Int32: oss << reinterpret_cast<const std::int32_t*>(s.raw_data())[off]; return;
    case ScalarType::Int16: oss << reinterpret_cast<const std::int16_t*>(s.raw_data())[off]; return;
    case ScalarType::Int8: oss << (int)reinterpret_cast<const std::int8_t*>(s.raw_data())[off]; return;
    case ScalarType::UInt64: oss << reinterpret_cast<const std::uint64_t*>(s.raw_data())[off]; return;
    case ScalarType::UInt32: oss << reinterpret_cast<const std::uint32_t*>(s.raw_data())[off]; return;
    case ScalarType::UInt16: oss << reinterpret_cast<const std::uint16_t*>(s.raw_data())[off]; return;
    case ScalarType::UInt8: oss << (unsigned)reinterpret_cast<const std::uint8_t*>(s.raw_data())[off]; return;
    case ScalarType::Bool: oss << (reinterpret_cast<const bool*>(s.raw_data())[off] ? "true" : "false"); return;
    case ScalarType::Complex64: {
      const auto v = reinterpret_cast<const Complex64*>(s.raw_data())[off];
      oss << "(" << v.real() << (v.imag()>=0?"+":"") << v.imag() << "j)";
      return;
    }
    case ScalarType::Complex128: {
      const auto v = reinterpret_cast<const Complex128*>(s.raw_data())[off];
      oss << "(" << v.real() << (v.imag()>=0?"+":"") << v.imag() << "j)";
      return;
    }
    default: oss << "<dtype?>"; return;
  }
}

static void format_recursive_(std::ostringstream& oss, const Series& s,
                             const std::vector<std::int64_t>& sizes,
                             const std::vector<std::int64_t>& strides,
                             std::int64_t dim, std::int64_t base_off,
                             const PrintOptions& opt) {
  if (dim == (std::int64_t)sizes.size()) {
    append_scalar_(oss, s, base_off);
    return;
  }
  const auto n = sizes[(std::size_t)dim];
  const auto st = strides[(std::size_t)dim];

  oss << "[";
  const bool summarize = (s.numel() > opt.max_elements && n > 2*opt.edge_items);
  const std::int64_t head = summarize ? opt.edge_items : n;
  const std::int64_t tail_start = summarize ? (n - opt.edge_items) : n;

  for (std::int64_t i=0; i<head; ++i) {
    if (i>0) oss << (dim== (std::int64_t)sizes.size()-1 ? ", " : ",\n  ");
    format_recursive_(oss, s, sizes, strides, dim+1, base_off + i*st, opt);
  }
  if (summarize) {
    oss << (dim== (std::int64_t)sizes.size()-1 ? ", ... , " : ",\n  ...\n ");
    for (std::int64_t i=tail_start; i<n; ++i) {
      if (i>tail_start) oss << (dim== (std::int64_t)sizes.size()-1 ? ", " : ",\n  ");
      format_recursive_(oss, s, sizes, strides, dim+1, base_off + i*st, opt);
    }
  }
  oss << "]";
}

std::string to_string(const Series& s, const PrintOptions& opt) {
  std::ostringstream oss;
  oss << std::setprecision(8);
  if (!s.defined()) return "Series(undefined)";
  if (opt.show_shape) {
    oss << "Series(shape=[";
    for (std::size_t i=0;i<s.sizes().size();++i) {
      if (i) oss << ",";
      oss << s.sizes()[i];
    }
    oss << "]";
    if (opt.show_dtype) oss << ", dtype=" << dtype_of(s.scalar_type()).name;
    oss << ")\n";
  }
  auto c = s.contiguous();
  format_recursive_(oss, c, std::vector<std::int64_t>(c.sizes().begin(), c.sizes().end()),
                    std::vector<std::int64_t>(c.strides().begin(), c.strides().end()),
                    0, 0, opt);
  return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Series& s) {
  os << to_string(s);
  return os;
}

} // namespace pluss::codem
