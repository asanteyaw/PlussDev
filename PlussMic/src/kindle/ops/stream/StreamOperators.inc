namespace {
Stream wrap_binary_with_label(const Stream& lhs,
                              const pluss::table::Thread& out,
                              const std::optional<pluss::table::Thread>& rhs_label = std::nullopt) {
  if (lhs.has_label()) return Stream::from_thread(out, *lhs.label());
  if (rhs_label.has_value()) return Stream::from_thread(out, rhs_label);
  return Stream::from_thread(out);
}
} // namespace

#define PLUSS_SIGNAL_DEFINE_MATH_OP(OP)                                                     \
  Stream operator OP(const Stream& lhs, double rhs) {                                       \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    return wrap_binary_with_label(lhs, lhs.to_thread() OP rhs);                             \
  }                                                                                          \
  Stream operator OP(double lhs, const Stream& rhs) {                                       \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_label(rhs, lhs OP rhs.to_thread());                             \
  }                                                                                          \
  Stream operator OP(const Stream& lhs, const Stream& rhs) {                                \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_label(lhs, lhs.to_thread() OP rhs.to_thread(), copy_label(rhs)); \
  }

#define PLUSS_SIGNAL_DEFINE_CMP_OP(OP)                                                      \
  Stream operator OP(const Stream& lhs, double rhs) {                                       \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    return wrap_binary_with_label(lhs, lhs.to_thread() OP rhs);                             \
  }                                                                                          \
  Stream operator OP(double lhs, const Stream& rhs) {                                       \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_label(rhs, lhs OP rhs.to_thread());                             \
  }                                                                                          \
  Stream operator OP(const Stream& lhs, const Stream& rhs) {                                \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_label(lhs, lhs.to_thread() OP rhs.to_thread(), copy_label(rhs)); \
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
