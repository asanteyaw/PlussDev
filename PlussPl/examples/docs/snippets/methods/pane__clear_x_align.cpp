using namespace pluss;
using namespace pluss::pl;

Studio fig("demo", Theme::paper());
auto& pane = fig.pane2("demo");
auto x = make_series({0.0, 1.0, 2.0, 3.0});
auto y = make_series({1.0, 0.5, 1.4, 0.9});
pane.trend(x, y).label("signal");
pane.clear_x_align();
