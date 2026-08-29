#pragma once

#include "pluss/ai/lymph/balance.h"
#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/prep.h"
#include "pluss/ai/lymph/selection.h"
#include "pluss/ai/lymph/selection_options.h"
#include "pluss/ai/lymph/shape.h"
#include "pluss/ai/lymph/time.h"
#include "pluss/ai/lymph/workflow.h"

namespace pluss::ai::lymph::options {

using Assess = AssessOptions;
using ClassReport = ClassReportOptions;
using Confusion = ConfusionOptions;
using CrossVal = selection::CrossValOptions;
using DFBatch = DataFrameBatchOptions;
using Fit = selection::FitOptions;
using HoldoutSplit = prep::HoldoutSplitOptions;
using Infer = InferOptions;
using Profile = shape::ProfileOptions;
using Split = SplitOptions;
using Time = workflow::TimeOptions;
using TimeSplit = prep::TimeSeriesSplitOptions;
using Train = TrainOptions;
using Tune = TuneOptions;
using Workflow = workflow::WorkflowOptions;

using Bayes = BayesFamilyOptions;
using Cluster = ClusterFamilyOptions;
using Decompose = DecomposeFamilyOptions;
using Detect = DetectFamilyOptions;
using Ensemble = EnsembleFamilyOptions;
using Kernel = KernelFamilyOptions;
using Linear = LinearFamilyOptions;
using Neighbor = NeighborFamilyOptions;
using Transform = TransformOptions;
using Tree = TreeFamilyOptions;

using CategoryIndexer = prep::CategoryIndexerOptions;
using FrequencyEncoder = prep::FrequencyEncoderOptions;
using FunctionTransformer = prep::FunctionTransformerOptions;
using IterativeImputer = prep::IterativeImputerOptions;
using LabelEncoder = prep::LabelEncoderOptions;
using MaxAbsScaler = prep::MaxAbsScalerOptions;
using MinMaxScaler = prep::MinMaxScalerOptions;
using NeighborImputer = prep::NeighborImputerOptions;
using OneHotEncoder = prep::OneHotEncoderOptions;
using OrdinalEncoder = prep::OrdinalEncoderOptions;
using OutlierIsolator = prep::OutlierIsolatorOptions;
using PolynomialLift = prep::PolynomialLiftOptions;
using PowerMap = prep::PowerMapOptions;
using QuantileMap = prep::QuantileMapOptions;
using RobustScaler = prep::RobustScalerOptions;
using SimpleImputer = prep::SimpleImputerOptions;
using StandardScaler = prep::StandardScalerOptions;
using ThresholdBinarizer = prep::ThresholdBinarizerOptions;
using VectorNormalizer = prep::VectorNormalizerOptions;

} // namespace pluss::ai::lymph::options
