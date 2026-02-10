#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>
#include <PlussQit/core/IListRef.h>

namespace pluss::native {

using cat_serial_fn = void(*)(const Series &, const MaterializedITensorListRef&, int64_t);
DECLARE_DISPATCH(cat_serial_fn, cat_serial_stub)

} // namespace pluss::native
