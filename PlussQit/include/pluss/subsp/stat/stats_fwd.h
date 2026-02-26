#pragma once

#include "pluss/export.h"

#include <cstdint>

namespace pluss::codem {
class Series;
}

namespace pluss::subsp::stat {

CODEM_API pluss::codem::Series mean(const pluss::codem::Series& x);
CODEM_API pluss::codem::Series mean(const pluss::codem::Series& x, std::int64_t dim, bool keepdim = false);

CODEM_API pluss::codem::Series var(const pluss::codem::Series& x, bool unbiased = true);
CODEM_API pluss::codem::Series var(const pluss::codem::Series& x,
                                   std::int64_t dim,
                                   bool unbiased = true,
                                   bool keepdim = false);

CODEM_API pluss::codem::Series std(const pluss::codem::Series& x, bool unbiased = true);
CODEM_API pluss::codem::Series std(const pluss::codem::Series& x,
                                   std::int64_t dim,
                                   bool unbiased = true,
                                   bool keepdim = false);

CODEM_API pluss::codem::Series nanvar(const pluss::codem::Series& x);
CODEM_API pluss::codem::Series nanstd(const pluss::codem::Series& x);
CODEM_API pluss::codem::Series skewness(const pluss::codem::Series& x);
CODEM_API pluss::codem::Series kurtosis(const pluss::codem::Series& x);

CODEM_API pluss::codem::Series movsum(const pluss::codem::Series& x, std::int64_t window, std::int64_t dim = -1);
CODEM_API pluss::codem::Series movmean(const pluss::codem::Series& x, std::int64_t window, std::int64_t dim = -1);
CODEM_API pluss::codem::Series movmax(const pluss::codem::Series& x, std::int64_t window, std::int64_t dim = -1);
CODEM_API pluss::codem::Series movmin(const pluss::codem::Series& x, std::int64_t window, std::int64_t dim = -1);
CODEM_API pluss::codem::Series movstd(const pluss::codem::Series& x, std::int64_t window, std::int64_t dim = -1);
CODEM_API pluss::codem::Series movvar(const pluss::codem::Series& x, std::int64_t window, std::int64_t dim = -1);

CODEM_API pluss::codem::Series normr(const pluss::codem::Series& x);
CODEM_API pluss::codem::Series normc(const pluss::codem::Series& x);
CODEM_API pluss::codem::Series standardize(const pluss::codem::Series& x);
CODEM_API pluss::codem::Series center(const pluss::codem::Series& x);

CODEM_API pluss::codem::Series pdist2(const pluss::codem::Series& x, const pluss::codem::Series& y);
CODEM_API pluss::codem::Series cdist(const pluss::codem::Series& x, const pluss::codem::Series& y);

} // namespace pluss::subsp::stat
