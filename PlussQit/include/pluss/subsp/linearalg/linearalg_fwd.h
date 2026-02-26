#pragma once

#include "pluss/export.h"

#include <cstdint>

namespace pluss::codem {
class Series;
}

namespace pluss::subsp::linearalg {

CODEM_API pluss::codem::Series matmul(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series mm(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series mv(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series dot(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series vdot(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series norm(const pluss::codem::Series& x);

CODEM_API pluss::codem::Series det(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series inverse(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series cholesky(const pluss::codem::Series& a, bool upper = false);
CODEM_API pluss::codem::Series logdet(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series slogdet(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series matrix_power(const pluss::codem::Series& a, std::int64_t n);
CODEM_API pluss::codem::Series matrix_exp(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series pinverse(const pluss::codem::Series& a, double rcond = 1e-12);

CODEM_API pluss::codem::Series triangular_solve(const pluss::codem::Series& a,
                                                const pluss::codem::Series& b,
                                                bool upper = true,
                                                bool unitriangular = false);
CODEM_API pluss::codem::Series cholesky_solve(const pluss::codem::Series& factor,
                                              const pluss::codem::Series& b,
                                              bool upper = false);
CODEM_API pluss::codem::Series cholesky_inverse(const pluss::codem::Series& factor, bool upper = false);
CODEM_API pluss::codem::Series lu_solve(const pluss::codem::Series& lu, const pluss::codem::Series& b);

} // namespace pluss::subsp::linearalg
