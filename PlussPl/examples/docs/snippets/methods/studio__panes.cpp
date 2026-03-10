using namespace pluss;
using namespace pluss::pl;

Studio fig("dashboard", Theme::paper());
fig.mosaic(2, 2, 12.0, 12.0);
auto& pane = fig.pane2("main");
auto x = make_series({0.0, 1.0, 2.0, 3.0});
auto y = make_series({1.0, 0.7, 1.3, 0.8});
pane.trend(x, y).label("signal");
const auto& panes = fig.panes();
