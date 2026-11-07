#pragma once

// PlussTils umbrella: utility-oriented APIs spanning aux helpers and view tooling.

#include "pluss/tils/auxiliary.h"
#include "pluss/tils/env.h"
#include "pluss/tils/view.h"

#if __has_include("pluss/bi/bi.h") && \
    __has_include("pluss/bundle/bundle.h") && \
    __has_include("pluss/table/table.h")
#include "pluss/bi/bi.h"
#endif
