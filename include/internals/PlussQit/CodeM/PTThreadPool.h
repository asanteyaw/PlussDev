#pragma once

#include <PlussQit/Parallel.h>
#include <MSharp/core/thread_pool.h>

namespace pluss {

class PLUSS_API PTThreadPool : public MSharp::ThreadPool {
 public:
  explicit PTThreadPool(int pool_size, int numa_node_id = -1)
      : MSharp::ThreadPool(pool_size, numa_node_id, []() {
          MSharp::setThreadName("PTThreadPool");
          pluss::init_num_threads();
        }) {}
};

} // namespace pluss
