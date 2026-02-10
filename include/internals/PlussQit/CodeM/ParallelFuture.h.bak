#pragma once

#include <PlussQit/core/ivalue.h>
#include <MSharp/macros/Macros.h>
#include <functional>

namespace pluss {

// Launches intra-op parallel task, returns a future
PLUSS_API MSharp::intrusive_ptr<MSharp::ivalue::Future> intraop_launch_future(
    const std::function<void()>& func);

} // namespace pluss
