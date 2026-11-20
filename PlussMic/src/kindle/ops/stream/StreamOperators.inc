namespace {
// A comparison gives a flag stream: it keeps the labels but not the metadata of its operand.
Stream wrap_flag_with_label(const Stream& lhs,
                            const pluss::table::Shelf& out,
                            const std::optional<pluss::table::Shelf>& rhs_label = std::nullopt) {
  if (lhs.has_label()) return Stream::from_shelf(out, *lhs.label());
  if (rhs_label.has_value()) return Stream::from_shelf(out, rhs_label);
  return Stream::from_shelf(out);
}

// Arithmetic keeps the meaning of the values, so the result carries the metadata of the stream operand.
Stream wrap_binary_with_label(const Stream& lhs,
                              const pluss::table::Shelf& out,
                              const std::optional<pluss::table::Shelf>& rhs_label = std::nullopt) {
  return carry_meta(wrap_flag_with_label(lhs, out, rhs_label), lhs);
}
} // namespace

#define PLUSS_SIGNAL_DEFINE_MATH_OP(OP)                                                     \
  Stream operator OP(const Stream& lhs, double rhs) {                                       \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    return wrap_binary_with_label(lhs, lhs.to_shelf() OP rhs);                             \
  }                                                                                          \
  Stream operator OP(double lhs, const Stream& rhs) {                                       \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_label(rhs, lhs OP rhs.to_shelf());                             \
  }                                                                                          \
  Stream operator OP(const Stream& lhs, const Stream& rhs) {                                \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_label(lhs, lhs.to_shelf() OP rhs.to_shelf(), copy_label(rhs)); \
  }

#define PLUSS_SIGNAL_DEFINE_CMP_OP(OP)                                                      \
  Stream operator OP(const Stream& lhs, double rhs) {                                       \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    return wrap_flag_with_label(lhs, lhs.to_shelf() OP rhs);                             \
  }                                                                                          \
  Stream operator OP(double lhs, const Stream& rhs) {                                       \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_flag_with_label(rhs, lhs OP rhs.to_shelf());                             \
  }                                                                                          \
  Stream operator OP(const Stream& lhs, const Stream& rhs) {                                \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_flag_with_label(lhs, lhs.to_shelf() OP rhs.to_shelf(), copy_label(rhs)); \
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
