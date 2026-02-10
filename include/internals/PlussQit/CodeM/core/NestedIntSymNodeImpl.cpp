#include <PlussQit/core/NestedIntSymNodeImpl.h>
#include <MSharp/core/SymNodeImpl.h>
#include <MSharp/util/Exception.h>

namespace MSharp {

namespace {
bool _eq(const char* op, MSharp::SymNodeImpl* lhs, MSharp::SymNodeImpl* rhs) {
  TORCH_INTERNAL_ASSERT(lhs->is_nested_int());
  std::optional<int64_t> c = rhs->nested_int();
  return (
      c.has_value() && lhs->nested_int() == *c &&
      lhs->nested_int_coeff() == rhs->nested_int_coeff());
}
bool _ge(const char* op, MSharp::SymNodeImpl* lhs, MSharp::SymNodeImpl* rhs) {
  if (auto mb_si = lhs->nested_int()) {
    if (auto mb_si2 = rhs->nested_int()) {
      if (*mb_si == *mb_si2) {
        return lhs->nested_int_coeff() >= rhs->nested_int_coeff();
      }
      TORCH_CHECK(false, "nested int ", op, ": Relation is indeterminate");
    }
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    if (rhs->constant_int() && *rhs->constant_int() <= 2) {
      return true;
    }
    TORCH_CHECK(false, "nested int ", op, ": Relation is indeterminate");
  } else if (rhs->nested_int()) {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    if (lhs->constant_int() && *lhs->constant_int() < 2) {
      return false;
    }
    TORCH_CHECK(false, "nested int ", op, ": Relation is indeterminate");
  }
  TORCH_INTERNAL_ASSERT(false, "expect at least one nested int");
}
} // namespace

MSharp::SymNode NestedIntSymNodeImpl::eq(const MSharp::SymNode& other) {
  return SymNode(MSharp::make_intrusive<ConstantSymNodeImpl<bool>>(
      _eq("eq", this, other.get())));
}

MSharp::SymNode NestedIntSymNodeImpl::ne(const MSharp::SymNode& other) {
  return SymNode(MSharp::make_intrusive<ConstantSymNodeImpl<bool>>(
      !_eq("ne", this, other.get())));
}

MSharp::SymNode NestedIntSymNodeImpl::ge(const MSharp::SymNode& other) {
  return SymNode(MSharp::make_intrusive<ConstantSymNodeImpl<bool>>(
      _ge("ge", this, other.get())));
}

MSharp::SymNode NestedIntSymNodeImpl::gt(const MSharp::SymNode& other) {
  return SymNode(MSharp::make_intrusive<ConstantSymNodeImpl<bool>>(
      !_ge("gt", other.get(), this)));
}

MSharp::SymNode NestedIntSymNodeImpl::lt(const MSharp::SymNode& other) {
  return SymNode(MSharp::make_intrusive<ConstantSymNodeImpl<bool>>(
      !_ge("lt", this, other.get())));
}

MSharp::SymNode NestedIntSymNodeImpl::le(const MSharp::SymNode& other) {
  return SymNode(MSharp::make_intrusive<ConstantSymNodeImpl<bool>>(
      _ge("le", other.get(), this)));
}

MSharp::SymNode NestedIntSymNodeImpl::mul(const MSharp::SymNode& other) {
  TORCH_CHECK(!other->nested_int(), "nested int cannot be multiplied by nested int");
  std::optional<int64_t> c = other->constant_int();
  TORCH_CHECK(c.has_value());
  return SymNode(MSharp::make_intrusive<NestedIntSymNodeImpl>(val_, coeff_ * *c));
}

MSharp::SymNode NestedIntSymNodeImpl::clone() {
  return SymNode(MSharp::make_intrusive<NestedIntSymNodeImpl>(val_, coeff_));
}

} // namespace MSharp
