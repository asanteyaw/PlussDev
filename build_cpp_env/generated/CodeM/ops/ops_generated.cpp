#include <cstdint>
#include <vector>
#include "CodeM/core/Errors.h"
#include "CodeM/core/Series.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/TypePromotion.h"

namespace pluss::codem::native::cpu {
  // Elementwise kernels (out-parameter)
  void add(const Series& a, const Series& b, const Series& out);
  void sub(const Series& a, const Series& b, const Series& out);
  void mul(const Series& a, const Series& b, const Series& out);
  void div(const Series& a, const Series& b, const Series& out);
  void neg(const Series& a, Series& out);
  void abs(const Series& a, Series& out);
  void exp(const Series& a, Series& out);
  void log(const Series& a, Series& out);
  void sqrt(const Series& a, Series& out);
  void sin(const Series& a, Series& out);
  void cos(const Series& a, Series& out);
  void tanh(const Series& a, Series& out);
  void eq(const Series& a, const Series& b, const Series& out);
  void ne(const Series& a, const Series& b, const Series& out);
  void lt(const Series& a, const Series& b, const Series& out);
  void le(const Series& a, const Series& b, const Series& out);
  void gt(const Series& a, const Series& b, const Series& out);
  void ge(const Series& a, const Series& b, const Series& out);
  void minimum(const Series& a, const Series& b, const Series& out);
  void maximum(const Series& a, const Series& b, const Series& out);
  void clamp(const Series& a, Series& out, double lo, double hi);
  void pow(const Series& a, const Series& b, const Series& out);
  void sign(const Series& a, Series& out);

  // Return-by-value ops
  Series sum(const Series& a, std::int64_t dim, bool keepdim);
  Series mean(const Series& a, std::int64_t dim, bool keepdim);
  Series amax(const Series& a, std::int64_t dim, bool keepdim);
  Series amin(const Series& a, std::int64_t dim, bool keepdim);
  Series argmax(const Series& a, std::int64_t dim, bool keepdim);
  Series argmin(const Series& a, std::int64_t dim, bool keepdim);
  Series sum_all(const Series& a);
  Series all_all(const Series& a);
  Series any_all(const Series& a);
  Series index_select(const Series& a, std::int64_t dim, const Series& index);
  Series masked_select(const Series& a, const Series& mask);
  Series matmul(const Series& a, const Series& b);
  Series gather(const Series& input, std::int64_t dim, const Series& index);
  Series scatter(const Series& input, std::int64_t dim, const Series& index, const Series& src);
  Series scatter_add(const Series& input, std::int64_t dim, const Series& index, const Series& src);
  Series where(const Series& cond, const Series& a, const Series& b);
  Series masked_fill(const Series& a, const Series& mask, double value);
  Series nonzero(const Series& mask);
  Series take(const Series& a, const Series& index);
  Series put(const Series& a, const Series& index, const Series& src, bool accumulate);
  Series index(const Series& a, const std::vector<Series>& indices);
  Series index_put(const Series& a, const std::vector<Series>& indices, const Series& values, bool accumulate);
  Series cat(const std::vector<Series>& tensors, std::int64_t dim);
  Series stack(const std::vector<Series>& tensors, std::int64_t dim);
}

namespace pluss::codem::ops_internal {

namespace add_ops {
Series add(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);
  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, out_dtype);
  pluss::codem::native::cpu::add(aa, bb, out);
  return out;
}
} // namespace

namespace sub_ops {
Series sub(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);
  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, out_dtype);
  pluss::codem::native::cpu::sub(aa, bb, out);
  return out;
}
} // namespace

namespace mul_ops {
Series mul(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);
  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, out_dtype);
  pluss::codem::native::cpu::mul(aa, bb, out);
  return out;
}
} // namespace

namespace div_ops {
Series div(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);
  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, out_dtype);
  pluss::codem::native::cpu::div(aa, bb, out);
  return out;
}
} // namespace

namespace neg_ops {
Series neg(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::neg(a, out);
  return out;
}
} // namespace

namespace abs_ops {
Series abs(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::abs(a, out);
  return out;
}
} // namespace

namespace exp_ops {
Series exp(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::exp(a, out);
  return out;
}
} // namespace

namespace log_ops {
Series log(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::log(a, out);
  return out;
}
} // namespace

namespace sqrt_ops {
Series sqrt(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::sqrt(a, out);
  return out;
}
} // namespace

namespace sin_ops {
Series sin(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::sin(a, out);
  return out;
}
} // namespace

namespace cos_ops {
Series cos(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::cos(a, out);
  return out;
}
} // namespace

namespace tanh_ops {
Series tanh(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::tanh(a, out);
  return out;
}
} // namespace

namespace sum_ops {
Series sum(const Series& a, std::int64_t dim, bool keepdim) {
  return pluss::codem::native::cpu::sum(a, dim, keepdim);
}
} // namespace

namespace mean_ops {
Series mean(const Series& a, std::int64_t dim, bool keepdim) {
  return pluss::codem::native::cpu::mean(a, dim, keepdim);
}
} // namespace

namespace amax_ops {
Series amax(const Series& a, std::int64_t dim, bool keepdim) {
  return pluss::codem::native::cpu::amax(a, dim, keepdim);
}
} // namespace

namespace amin_ops {
Series amin(const Series& a, std::int64_t dim, bool keepdim) {
  return pluss::codem::native::cpu::amin(a, dim, keepdim);
}
} // namespace

namespace argmax_ops {
Series argmax(const Series& a, std::int64_t dim, bool keepdim) {
  return pluss::codem::native::cpu::argmax(a, dim, keepdim);
}
} // namespace

namespace argmin_ops {
Series argmin(const Series& a, std::int64_t dim, bool keepdim) {
  return pluss::codem::native::cpu::argmin(a, dim, keepdim);
}
} // namespace

namespace index_select_ops {
Series index_select(const Series& a, std::int64_t dim, const Series& index) {
  return pluss::codem::native::cpu::index_select(a, dim, index);
}
} // namespace

namespace masked_select_ops {
Series masked_select(const Series& a, const Series& mask) {
  return pluss::codem::native::cpu::masked_select(a, mask);
}
} // namespace

namespace matmul_ops {
Series matmul(const Series& a, const Series& b) {
  return pluss::codem::native::cpu::matmul(a, b);
}
} // namespace

namespace gather_ops {
Series gather(const Series& input, std::int64_t dim, const Series& index) {
  return pluss::codem::native::cpu::gather(input, dim, index);
}
} // namespace

namespace scatter_ops {
Series scatter(const Series& input, std::int64_t dim, const Series& index, const Series& src) {
  return pluss::codem::native::cpu::scatter(input, dim, index, src);
}
} // namespace

namespace scatter_add_ops {
Series scatter_add(const Series& input, std::int64_t dim, const Series& index, const Series& src) {
  return pluss::codem::native::cpu::scatter_add(input, dim, index, src);
}
} // namespace

namespace where_ops {
Series where(const Series& cond, const Series& a, const Series& b) {
  return pluss::codem::native::cpu::where(cond, a, b);
}
} // namespace

namespace masked_fill_ops {
Series masked_fill(const Series& a, const Series& mask, double value) {
  return pluss::codem::native::cpu::masked_fill(a, mask, value);
}
} // namespace

namespace nonzero_ops {
Series nonzero(const Series& mask) {
  return pluss::codem::native::cpu::nonzero(mask);
}
} // namespace

namespace take_ops {
Series take(const Series& a, const Series& index) {
  return pluss::codem::native::cpu::take(a, index);
}
} // namespace

namespace put_ops {
Series put(const Series& a, const Series& index, const Series& src, bool accumulate) {
  return pluss::codem::native::cpu::put(a, index, src, accumulate);
}
} // namespace

namespace index_ops {
Series index(const Series& a, const std::vector<Series>& indices) {
  return pluss::codem::native::cpu::index(a, indices);
}
} // namespace

namespace index_put_ops {
Series index_put(const Series& a, const std::vector<Series>& indices, const Series& values, bool accumulate) {
  return pluss::codem::native::cpu::index_put(a, indices, values, accumulate);
}
} // namespace

namespace cat_ops {
Series cat(const std::vector<Series>& tensors, std::int64_t dim) {
  return pluss::codem::native::cpu::cat(tensors, dim);
}
} // namespace

namespace stack_ops {
Series stack(const std::vector<Series>& tensors, std::int64_t dim) {
  return pluss::codem::native::cpu::stack(tensors, dim);
}
} // namespace

namespace eq_ops {
Series eq(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  // comparisons: bool output, numeric inputs must be same dtype after promotion
  const auto in_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == in_dtype) ? a : a.to(in_dtype);
  auto bb = (b.scalar_type() == in_dtype) ? b : b.to(in_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, ScalarType::Bool);
  pluss::codem::native::cpu::eq(aa, bb, out);
  return out;
}
} // namespace

namespace ne_ops {
Series ne(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  // comparisons: bool output, numeric inputs must be same dtype after promotion
  const auto in_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == in_dtype) ? a : a.to(in_dtype);
  auto bb = (b.scalar_type() == in_dtype) ? b : b.to(in_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, ScalarType::Bool);
  pluss::codem::native::cpu::ne(aa, bb, out);
  return out;
}
} // namespace

namespace lt_ops {
Series lt(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  // comparisons: bool output, numeric inputs must be same dtype after promotion
  const auto in_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == in_dtype) ? a : a.to(in_dtype);
  auto bb = (b.scalar_type() == in_dtype) ? b : b.to(in_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, ScalarType::Bool);
  pluss::codem::native::cpu::lt(aa, bb, out);
  return out;
}
} // namespace

namespace le_ops {
Series le(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  // comparisons: bool output, numeric inputs must be same dtype after promotion
  const auto in_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == in_dtype) ? a : a.to(in_dtype);
  auto bb = (b.scalar_type() == in_dtype) ? b : b.to(in_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, ScalarType::Bool);
  pluss::codem::native::cpu::le(aa, bb, out);
  return out;
}
} // namespace

namespace gt_ops {
Series gt(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  // comparisons: bool output, numeric inputs must be same dtype after promotion
  const auto in_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == in_dtype) ? a : a.to(in_dtype);
  auto bb = (b.scalar_type() == in_dtype) ? b : b.to(in_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, ScalarType::Bool);
  pluss::codem::native::cpu::gt(aa, bb, out);
  return out;
}
} // namespace

namespace ge_ops {
Series ge(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  // comparisons: bool output, numeric inputs must be same dtype after promotion
  const auto in_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == in_dtype) ? a : a.to(in_dtype);
  auto bb = (b.scalar_type() == in_dtype) ? b : b.to(in_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, ScalarType::Bool);
  pluss::codem::native::cpu::ge(aa, bb, out);
  return out;
}
} // namespace

namespace minimum_ops {
Series minimum(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);
  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, out_dtype);
  pluss::codem::native::cpu::minimum(aa, bb, out);
  return out;
}
} // namespace

namespace maximum_ops {
Series maximum(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);
  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, out_dtype);
  pluss::codem::native::cpu::maximum(aa, bb, out);
  return out;
}
} // namespace

namespace clamp_ops {
Series clamp(const Series& a, double lo, double hi) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::clamp(a, out, lo, hi);
  return out;
}
} // namespace

namespace pow_ops {
Series pow(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) throw_error("op: undefined input");
  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);
  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());
  Series out = Series::empty(shape, out_dtype);
  pluss::codem::native::cpu::pow(aa, bb, out);
  return out;
}
} // namespace

namespace sign_ops {
Series sign(const Series& a) {
  if (!a.defined()) throw_error("op: undefined input");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  pluss::codem::native::cpu::sign(a, out);
  return out;
}
} // namespace

namespace sum_all_ops {
Series sum_all(const Series& a) {
  return pluss::codem::native::cpu::sum_all(a);
}
} // namespace

namespace all_all_ops {
Series all_all(const Series& a) {
  return pluss::codem::native::cpu::all_all(a);
}
} // namespace

namespace any_all_ops {
Series any_all(const Series& a) {
  return pluss::codem::native::cpu::any_all(a);
}
} // namespace

} // namespace pluss::codem::ops_internal