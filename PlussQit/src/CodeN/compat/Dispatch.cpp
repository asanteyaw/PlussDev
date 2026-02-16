#include "CodeN/compat/Dispatch.h"

#include <sstream>

#include "CodeN/core/OpsInternal.h"
#include "CodeN/generated/DispatchRegistry.inc"

namespace pluss::coden::_compat {

static void fail(::std::string_view group, ::std::string_view op, ::std::size_t nargs) {
  ::std::ostringstream oss;
  oss << "No such op for " << group << ": " << op << " (nargs=" << nargs << ")\n";
  oss << "Known signatures for '" << op << "':\n";
  bool any = false;
  for (const auto& s : kOpSigs) {
    if (s.name == op) {
      any = true;
      oss << "  - " << s.pretty << "\n";
    }
  }
  if (!any) {
    oss << "  (none)\n";
  }
  PLUSS_CHECK(false, oss.str());
}

static const CodenValue::IntList& as_intlist(const CodenValue& a) {
  if (auto p = ::std::get_if<CodenValue::IntList>(&a.v)) return *p;
  PLUSS_CHECK(false, "CodeN: expected intlist argument");
}
static ::std::int64_t as_i64(const CodenValue& a) {
  if (auto p = ::std::get_if<::std::int64_t>(&a.v)) return *p;
  PLUSS_CHECK(false, "CodeN: expected int64 argument");
}
static ::std::string_view as_str(const CodenValue& a) {
  if (auto p = ::std::get_if<::std::string>(&a.v)) return *p;
  PLUSS_CHECK(false, "CodeN: expected string argument");
}

::pluss::coden::Chord call_chord(::std::string_view op, const ::pluss::coden::Chord& self, const CodenValues& args) {
  if (op == "lower" && args.empty()) return ops_internal::lower(self);
  if (op == "strip" && args.empty()) return ops_internal::strip(self);
  if (op == "replace" && args.size() == 2) return ops_internal::replace(self, as_str(args[0]), as_str(args[1]));

  // Unicode-ish hooks (ICU optional)
  if (op == "normalize" && args.size() == 1) return ops_internal::normalize(self, as_i64(args[0]));
  if (op == "casefold" && (args.size() == 0 || args.size() == 1)) {
    ::std::string_view loc = args.empty() ? ::std::string_view{} : as_str(args[0]);
    return ops_internal::casefold(self, loc);
  }

  // Regex hooks
  if (op == "regex_replace" && args.size() == 2) return ops_internal::regex_replace(self, as_str(args[0]), as_str(args[1]));

  fail("Chord", op, args.size());
}

bool call_bool_chord(::std::string_view op, const ::pluss::coden::Chord& self, const CodenValues& args) {
  if (op == "contains" && args.size() == 1) return ops_internal::contains(self, as_str(args[0]));
  if (op == "regex_match" && args.size() == 1) return ops_internal::regex_match(self, as_str(args[0]));
  if (op == "regex_search" && args.size() == 1) return ops_internal::regex_search(self, as_str(args[0]));
  fail("Chord->bool", op, args.size());
}

TokenBatch call_tokenbatch_chord(::std::string_view op, const ::pluss::coden::Chord& self, const CodenValues& args) {
  if (op == "split" && args.size() == 1) return ops_internal::split(self, as_str(args[0]));
  if (op == "split_whitespace" && args.empty()) return ops_internal::split_whitespace(self);
  fail("Chord->TokenBatch", op, args.size());
}

::pluss::coden::Note call_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  if (op == "lower" && args.empty()) return ops_internal::lower(self);
  if (op == "strip" && args.empty()) return ops_internal::strip(self);
  if (op == "reshape" && args.size() == 1) return ops_internal::reshape(self, as_intlist(args[0]));
  if (op == "transpose" && args.size() == 2) return ops_internal::transpose(self, as_i64(args[0]), as_i64(args[1]));
  if (op == "permute" && args.size() == 1) return ops_internal::permute(self, as_intlist(args[0]));
  if (op == "select" && args.size() == 2) return ops_internal::select(self, as_i64(args[0]), as_i64(args[1]));
  if (op == "narrow" && args.size() == 3) return ops_internal::narrow(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]));

  if (op == "normalize" && args.size() == 1) return ops_internal::normalize(self, as_i64(args[0]));
  if (op == "casefold" && (args.size() == 0 || args.size() == 1)) {
    ::std::string_view loc = args.empty() ? ::std::string_view{} : as_str(args[0]);
    return ops_internal::casefold(self, loc);
  }

  if (op == "regex_replace" && args.size() == 2) return ops_internal::regex_replace(self, as_str(args[0]), as_str(args[1]));

  fail("Note", op, args.size());
}

::pluss::coden::Chord call_chord_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  if (op == "join" && (args.size() == 0 || args.size() == 1)) {
    ::std::string_view sep = args.empty() ? ::std::string_view{} : as_str(args[0]);
    return ops_internal::join(self, sep);
  }
  fail("Note->Chord", op, args.size());
}

::std::vector<::std::uint8_t> call_u8list_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  if (op == "contains" && args.size() == 1) return ops_internal::contains(self, as_str(args[0]));
  if (op == "regex_match" && args.size() == 1) return ops_internal::regex_match(self, as_str(args[0]));
  if (op == "regex_search" && args.size() == 1) return ops_internal::regex_search(self, as_str(args[0]));
  fail("Note->u8list", op, args.size());
}

::std::vector<::std::int64_t> call_i64list_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  if (op == "lengths_bytes" && args.empty()) return ops_internal::lengths_bytes(self);
  fail("Note->i64list", op, args.size());
}

TokenBatch call_tokenbatch_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args) {
  if (op == "split_whitespace" && args.empty()) return ops_internal::split_whitespace(self);
  if (op == "split" && args.size() == 1) return ops_internal::split(self, as_str(args[0]));
  fail("Note->TokenBatch", op, args.size());
}

} // namespace pluss::coden::_compat
