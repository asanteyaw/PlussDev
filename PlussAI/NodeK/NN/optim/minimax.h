#include "adam.h"
// minimax is a combination of the Adam optimizer and the InternalProtocol functionality of optima(...) in PlussMa.
// Its vision is very simple, have the usual Adam optimizer that scales to several tens of thousands of parameters, but with built in constrainst handling, NaN recovery, etc of the InternalProtocol.
// It will be implemented as a wrapper around the Adam optimizer with the additional functionality of the InternalProtocol built in, and will be designed to be easily extensible in the future.
// Intended usage:
// pluss::nn::optim::Minimax optimizer(pluss::nn::optim::MinimaxOptions(...));