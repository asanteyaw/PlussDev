cmake_minimum_required(VERSION 3.28)
project(PlussTiq LANGUAGES CXX)

add_library(PlussTiq INTERFACE)

option(PLUSSMA_ENABLE_MATH "Build PlussMa Math subspace" ON)
option(PLUSSMA_ENABLE_STATS "Build PlussMa Stats subspace" ON)
option(PLUSSMA_ENABLE_ENG "Build PlussMa Engineering subspace" ON)
option(PLUSSMA_ENABLE_NATSCI "Build PlussMa Natural Sciences subspace" ON)
option(PLUSSMA_ENABLE_ACTUARIAL "Build PlussMa Actuarial subspace" ON)
option(PLUSSMA_ENABLE_EXAMPLES "Build PlussMa examples" ON)

# Math modules
option(PLUSSMA_MATH_ENABLE_ALGEBRA "Build PlussMa Math::Algebra" ON)
option(PLUSSMA_MATH_ENABLE_ANALYSIS "Build PlussMa Math::Analysis" ON)
option(PLUSSMA_MATH_ENABLE_ECONOMICS "Build PlussMa Math::Economics" ON)
option(PLUSSMA_MATH_ENABLE_FINANCE "Build PlussMa Math::Finance" ON)
option(PLUSSMA_MATH_ENABLE_NUMERICAL "Build PlussMa Math::Numerical" ON)
option(PLUSSMA_MATH_ENABLE_PROBABILITY "Build PlussMa Math::Probability" ON)

# Math::Numerical modules
option(PLUSSMA_MATH_NUMERICAL_ENABLE_INTEGRATION "Build PlussMa Math::Numerical::integration" ON)
option(PLUSSMA_MATH_NUMERICAL_ENABLE_INTERPOLATION "Build PlussMa Math::Numerical::interpolation" ON)
option(PLUSSMA_MATH_NUMERICAL_ENABLE_LINEAR_ALGEBRA "Build PlussMa Math::Numerical::linear_algebra" ON)
option(PLUSSMA_MATH_NUMERICAL_ENABLE_ODE "Build PlussMa Math::Numerical::ode" ON)
option(PLUSSMA_MATH_NUMERICAL_ENABLE_OPTIMIZATION "Build PlussMa Math::Numerical::optimization" ON)
option(PLUSSMA_MATH_NUMERICAL_ENABLE_SPECTRAL_METHODS "Build PlussMa Math::Numerical::spectral_methods" ON)

option(PLUSSMA_MATH_FINANCE_ENABLE_PRICING "Build PlussMa Math::Finance::pricing" ON)
option(PLUSSMA_MATH_FINANCE_ENABLE_HEDGING "Build PlussMa Math::Finance::hedging" ON)
option(PLUSSMA_MATH_FINANCE_ENABLE_FILTERING "Build PlussMa Math::Finance::filtering" ON)
option(PLUSSMA_MATH_FINANCE_ENABLE_STOCHASTIC_PROCESSES "Build PlussMa Math::Finance::stochastic_processes" ON)

# Stats modules
option(PLUSSMA_STATS_ENABLE_BAYESIAN "Build PlussMa Stats::bayesian" ON)
option(PLUSSMA_STATS_ENABLE_DISTRIBUTIONS "Build PlussMa Stats::distributions" ON)
option(PLUSSMA_STATS_ENABLE_ESTIMATION "Build PlussMa Stats::estimation" ON)
option(PLUSSMA_STATS_ENABLE_INFERENCE "Build PlussMa Stats::inference" ON)
option(PLUSSMA_STATS_ENABLE_NONPARAMETRIC "Build PlussMa Stats::nonparametric" ON)
option(PLUSSMA_STATS_ENABLE_REGRESSION "Build PlussMa Stats::regression" ON)
option(PLUSSMA_STATS_ENABLE_TIME_SERIES "Build PlussMa Stats::time_series" ON)

# Engineering modules
option(PLUSSMA_ENG_ENABLE_CONTROL "Build PlussMa Eng::control" ON)
option(PLUSSMA_ENG_ENABLE_CIRCUIT "Build PlussMa Eng::circuit" ON)
option(PLUSSMA_ENG_ENABLE_FLUID "Build PlussMa Eng::fluid" ON)
option(PLUSSMA_ENG_ENABLE_LTI "Build PlussMa Eng::lti" ON)
option(PLUSSMA_ENG_ENABLE_POWER "Build PlussMa Eng::power" ON)
option(PLUSSMA_ENG_ENABLE_SIGNAL "Build PlussMa Eng::signal" ON)
option(PLUSSMA_ENG_ENABLE_STRUCTURE "Build PlussMa Eng::structure" ON)

# Natural Sciences modules
option(PLUSSMA_NATSCI_ENABLE_PHYSICS "Build PlussMa NatSci::physics" ON)
option(PLUSSMA_NATSCI_ENABLE_CHEMISTRY "Build PlussMa NatSci::chemistry" ON)
option(PLUSSMA_NATSCI_ENABLE_BIOLOGY "Build PlussMa NatSci::biology" ON)
option(PLUSSMA_NATSCI_ENABLE_COMMON "Build PlussMa NatSci::common" ON)

target_include_directories(PlussTiq INTERFACE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../include>
)

target_include_directories(pluss PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Shared numerical engines used across otherwise independent subspaces.
add_subdirectory(State)

if(PLUSSMA_ENABLE_STATS AND PLUSSMA_STATS_ENABLE_TIME_SERIES AND NOT PLUSSMA_STATS_ENABLE_DISTRIBUTIONS)
    message(FATAL_ERROR "PlussMa config invalid: Stats::time_series requires Stats::distributions")
endif()
if(PLUSSMA_ENABLE_MATH AND PLUSSMA_MATH_ENABLE_FINANCE AND PLUSSMA_MATH_FINANCE_ENABLE_HEDGING AND NOT PLUSSMA_MATH_FINANCE_ENABLE_PRICING)
    message(FATAL_ERROR "PlussMa config invalid: Math::Finance::hedging requires Math::Finance::pricing")
endif()

if(PLUSSMA_ENABLE_ENG)
    add_subdirectory(Eng)
endif()
if(PLUSSMA_ENABLE_MATH)
    add_subdirectory(Math)
endif()
if(PLUSSMA_ENABLE_NATSCI)
    add_subdirectory(NatSci)
endif()
if(PLUSSMA_ENABLE_STATS)
    add_subdirectory(Stats)
endif()
if(PLUSSMA_ENABLE_ACTUARIAL)
    add_subdirectory(Actuarial)
endif()
if(PLUSSMA_ENABLE_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
	add_subdirectory(examples)
endif()

target_link_libraries(PlussTiq INTERFACE PlussQit)

set_target_properties(PlussTiq PROPERTIES CXX_STANDARD 20)
