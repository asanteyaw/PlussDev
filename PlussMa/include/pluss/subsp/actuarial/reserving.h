#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"
#include "pluss/subsp/actuarial/operator.h"

#include <string>
#include <vector>

namespace pluss::subsp::actuarial {

class CODEM_API Triangle final {
public:
  Triangle() = default;

  [[nodiscard]] bool defined() const noexcept;
  [[nodiscard]] const pluss::codem::Series& values() const noexcept;
  [[nodiscard]] const pluss::codem::Series& observed() const noexcept;
  [[nodiscard]] const std::vector<std::string>& origins() const noexcept;
  [[nodiscard]] const std::vector<std::string>& development() const noexcept;
  [[nodiscard]] std::int64_t norigins() const noexcept;
  [[nodiscard]] std::int64_t ndevelopment() const noexcept;

private:
  Triangle(pluss::codem::Series values,
           pluss::codem::Series observed,
           std::vector<std::string> origins,
           std::vector<std::string> development);

  pluss::codem::Series values_ = pluss::series::undefined();
  pluss::codem::Series observed_ = pluss::series::undefined();
  std::vector<std::string> origins_{};
  std::vector<std::string> development_{};

  friend Triangle triangle_make_(const pluss::codem::Series&,
                                 const pluss::codem::Series&,
                                 std::vector<std::string>,
                                 std::vector<std::string>);
};

namespace triangle {

// When observed is omitted, positive cells are treated as observed for
// compatibility with the legacy zero-padded triangle convention.
[[nodiscard]] CODEM_API Triangle cumulative(
    const pluss::codem::Series& values,
    const pluss::codem::Series& observed = pluss::series::undefined(),
    std::vector<std::string> origins = {},
    std::vector<std::string> development = {});

} // namespace triangle

struct CODEM_API ReserveResult final {
  pluss::codem::Series age_to_age = pluss::series::undefined();
  pluss::codem::Series cdf = pluss::series::undefined();
  pluss::codem::Series ultimate = pluss::series::undefined();
  pluss::codem::Series reserve = pluss::series::undefined();
};

using Reserving = Operator<ReserveResult(const Triangle&)>;

namespace operators {

[[nodiscard]] CODEM_API Reserving chain_ladder(
    pluss::codem::Series cdf = pluss::series::undefined());
[[nodiscard]] CODEM_API Reserving bornhuetter_ferguson(
    pluss::codem::Series expected_ultimate,
    pluss::codem::Series percent_reported);

} // namespace operators

} // namespace pluss::subsp::actuarial
