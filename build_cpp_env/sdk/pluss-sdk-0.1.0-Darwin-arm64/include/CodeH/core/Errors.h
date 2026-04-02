#pragma once

// CodeH uses the same error/check machinery as CodeM.
#include "CodeM/core/Errors.h"

#ifndef CODEM_CHECK
#define CODEM_CHECK(cond, msg)            \
  do {                                    \
    if (!(cond)) {                        \
      ::pluss::codem::throw_error((msg)); \
    }                                     \
  } while (false)
#endif

#ifndef CODEM_FAIL
#define CODEM_FAIL(msg) ::pluss::codem::throw_error((msg))
#endif

#ifndef CODEM_WARN
#define CODEM_WARN(msg) ::pluss::codem::warn((msg))
#endif
