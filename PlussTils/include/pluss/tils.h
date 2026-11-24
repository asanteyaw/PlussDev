#pragma once

// PlussTils umbrella: every public PlussTils header. Include a single header, such as
// <pluss/tils/doc.h> or <pluss/tils/kit/output.h>, to take only that part.

#include "pluss/tils/kit.h"
#include "pluss/tils/print_options.h"
#include "pluss/tils/suppose.h"
#include "pluss/tils/let.h"
#include "pluss/tils/flow.h"
#include "pluss/tils/env.h"
#include "pluss/tils/info.h"
#include "pluss/tils/settings.h"
#include "pluss/tils/cache.h"
#include "pluss/tils/doc.h"
#include "pluss/tils/view.h"

#if __has_include("pluss/bi/bi.h") && \
    __has_include("pluss/bundle/bundle.h") && \
    __has_include("pluss/table/table.h")
#include "pluss/bi/bi.h"
#endif
