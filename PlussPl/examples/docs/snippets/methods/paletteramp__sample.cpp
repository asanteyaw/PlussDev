using namespace pluss;
using namespace pluss::pl;

PaletteRamp ramp = PaletteRamp::ocean();
Rgba c = ramp.sample(0.5); (void)c;
