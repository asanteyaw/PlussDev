#pragma once

#include <PlussQit/core/symbol.h>
#include <MSharp/util/ArrayRef.h>
#include <optional>
#include <ostream>

namespace pluss {

enum class NameType: uint8_t { BASIC, WILDCARD };

struct PLUSS_API Dimname {
  static Dimname fromSymbol(Symbol name);
  static Dimname wildcard();
  static bool isValidName(const std::string& name);

  NameType type() const { return type_; }
  Symbol symbol() const { return name_; }

  bool isBasic() const { return type_ == NameType::BASIC; }
  bool isWildcard() const { return type_ == NameType::WILDCARD; }

  bool matches(Dimname other) const;
  std::optional<Dimname> unify(Dimname other) const;

 private:
  Dimname(Symbol name)
    : name_(name), type_(NameType::BASIC) {}
  Dimname(Symbol name, NameType type)
    : name_(name), type_(type) {}

  Symbol name_;
  NameType type_;
};

using DimnameList = MSharp::ArrayRef<Dimname>;

PLUSS_API std::ostream& operator<<(std::ostream& out, const Dimname& dimname);

inline bool operator==(const Dimname& lhs, const Dimname& rhs) {
  return lhs.symbol() == rhs.symbol();
}

inline bool operator!=(const Dimname& lhs, const Dimname& rhs) {
  return !(lhs == rhs);
}

} // namespace pluss
