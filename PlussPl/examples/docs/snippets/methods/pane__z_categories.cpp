using namespace pluss;
using namespace pluss::pl;

Studio fig("demo3d", Theme::paper());
auto& pane = fig.pane3("demo3d");
auto gx = linspace(-2.0, 2.0, 12, ScalarType::Float64);
auto gy = linspace(-2.0, 2.0, 12, ScalarType::Float64);
auto gz = gx.sin().unsqueeze(0).matmul(gy.cos().unsqueeze(1));
pane.sheet(gx, gy, gz).label("surface");
pane.z_categories({"L1", "L2", "L3"}, 0.0, 1.0);
