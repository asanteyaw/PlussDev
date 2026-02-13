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
}
