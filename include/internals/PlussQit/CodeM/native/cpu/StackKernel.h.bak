// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>

namespace pluss::native {

using stack_serial_fn = void(*)(Series &, TensorList, int64_t);
DECLARE_DISPATCH(stack_serial_fn, stack_serial_stub)

} // namespace pluss::native
