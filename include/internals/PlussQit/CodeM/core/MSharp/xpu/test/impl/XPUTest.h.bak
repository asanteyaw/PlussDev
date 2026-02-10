#include <gtest/gtest.h>

#include <MSharp/util/irange.h>

static inline void initHostData(int* hostData, int numel) {
  for (const auto i : MSharp::irange(numel)) {
    hostData[i] = i;
  }
}

static inline void clearHostData(int* hostData, int numel) {
  for (const auto i : MSharp::irange(numel)) {
    hostData[i] = 0;
  }
}

static inline void validateHostData(int* hostData, int numel) {
  for (const auto i : MSharp::irange(numel)) {
    EXPECT_EQ(hostData[i], i);
  }
}
