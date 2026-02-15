#include "CodeM/core/Series.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesImpl.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/native/cpu/ArithmeticKernels.h"
#include "CodeM/native/cpu/AdvancedIndexingKernels.h"
#include "CodeM/native/cpu/BitwiseKernels.h"
#include "CodeM/core/ViewInvariants.h"
#include "CodeM/core/Config.h"
#include <ostream>
#include "CodeM/native/cpu/ScatterAddKernels.h"
#include "CodeM/native/cpu/GatherScatterKernels.h"
#include "CodeM/native/cpu/ConcatStackKernels.h"
#include "CodeM/native/cpu/UnaryMathKernels.h"
#include "CodeM/native/cpu/UnaryMoreKernels.h"
#include "CodeM/native/cpu/ReduceKernels.h"
#include "CodeM/native/cpu/WhereClampKernels.h"
#include "CodeM/native/cpu/CumKernels.h"
#include "CodeM/native/cpu/KernelTable.h"
#include "CodeM/core/TypePromotion.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/Complex.h"

#include <algorithm>
#include <array>
#include <vector>
#include <cstring>
#include <cstring>

namespace pluss::codem {
  #include "CodeM/Core/SeriesCat.cpp"

  // implementations of extra methods in Series.h. It probably implemented elsewhere in the 
  // codebase, but we can put it here for now since it's not yet codegen'd and we want to keep the generated method dispatch clean and focused on the generated methods themselves.

  // Variance / std (dim overloads are used internally but not yet codegen'd).
  Series var(std::int64_t dim, bool unbiased, bool keepdim) const{
    // find where they are implemented and delegate to them when ready.
  }
  Series std(std::int64_t dim, bool unbiased=true, bool keepdim=false) const{
    // if they are not implemented yet, implement them and delegate from here.
  }

  // Norm overloads used throughout the codebase.
  Series norm(double p, std::int64_t dim, bool keepdim=false) const{}

  // PlussQit naming deviations (method-first aliases).
  Series pile(const std::vector<Series>& others, std::int64_t dim=0) const{}
  Series unite(const std::vector<Series>& others, std::int64_t dim=0) const{}
}
