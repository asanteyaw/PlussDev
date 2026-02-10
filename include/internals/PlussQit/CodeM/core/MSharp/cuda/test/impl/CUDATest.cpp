#include <gtest/gtest.h>

#include <MSharp/cuda/impl/CUDATest.h>

using namespace MSharp::cuda::impl;

TEST(CUDATest, SmokeTest) {
  c10_cuda_test();
}
