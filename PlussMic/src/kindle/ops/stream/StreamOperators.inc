namespace {
Stream wrap_binary_with_cases(const Stream& lhs,
                              const pluss::table::Thread& out,
                              const std::optional<pluss::table::Thread>& rhs_cases = std::nullopt) {
  if (lhs.has_cases()) return Stream::from_thread(out, *lhs.cases());
  if (rhs_cases.has_value()) return Stream::from_thread(out, rhs_cases);
  return Stream::from_thread(out);
}
} // namespace

#define PLUSS_SIGNAL_DEFINE_MATH_OP(OP)                                                     \
  Stream operator OP(const Stream& lhs, double rhs) {                                       \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    return wrap_binary_with_cases(lhs, lhs.to_thread() OP rhs);                             \
  }                                                                                          \
  Stream operator OP(double lhs, const Stream& rhs) {                                       \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_cases(rhs, lhs OP rhs.to_thread());                             \
  }                                                                                          \
  Stream operator OP(const Stream& lhs, const Stream& rhs) {                                \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_cases(lhs, lhs.to_thread() OP rhs.to_thread(), copy_cases(rhs)); \
  }

#define PLUSS_SIGNAL_DEFINE_CMP_OP(OP)                                                      \
  Stream operator OP(const Stream& lhs, double rhs) {                                       \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    return wrap_binary_with_cases(lhs, lhs.to_thread() OP rhs);                             \
  }                                                                                          \
  Stream operator OP(double lhs, const Stream& rhs) {                                       \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_cases(rhs, lhs OP rhs.to_thread());                             \
  }                                                                                          \
  Stream operator OP(const Stream& lhs, const Stream& rhs) {                                \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_cases(lhs, lhs.to_thread() OP rhs.to_thread(), copy_cases(rhs)); \
  }

PLUSS_SIGNAL_DEFINE_MATH_OP(+)
PLUSS_SIGNAL_DEFINE_MATH_OP(-)
PLUSS_SIGNAL_DEFINE_MATH_OP(*)
PLUSS_SIGNAL_DEFINE_MATH_OP(/)
PLUSS_SIGNAL_DEFINE_MATH_OP(%)

PLUSS_SIGNAL_DEFINE_CMP_OP(==)
PLUSS_SIGNAL_DEFINE_CMP_OP(!=)
PLUSS_SIGNAL_DEFINE_CMP_OP(<)
PLUSS_SIGNAL_DEFINE_CMP_OP(<=)
PLUSS_SIGNAL_DEFINE_CMP_OP(>)
PLUSS_SIGNAL_DEFINE_CMP_OP(>=)

#undef PLUSS_SIGNAL_DEFINE_MATH_OP
#undef PLUSS_SIGNAL_DEFINE_CMP_OP

