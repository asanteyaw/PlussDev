function matlab_fit(datafile, outdir, nstarts)
% Fits NGARCH, component NGARCH and the RNN-driven TVP NGARCH (Gaussian returns likelihood) with fmincon
% interior-point, on scaled parameters exactly as with_returns.m does, from starts shared with the Optima fit.
if nargin < 3, nstarts = 5; end
T = readtable(datafile); y = T.exret;
if ~exist(outdir,'dir'), mkdir(outdir); end
models = {'ngarch','cngarch','tvp'};
for mi = 1:numel(models)
  name = models{mi};
  S = make_starts(name, nstarts);
  writematrix(S, fullfile(outdir, ['starts_' name '.csv']));
  [lb, ub] = bounds_for(name);
  fid = fopen(fullfile(outdir, ['matlab_' name '.txt']), 'w');
  for s = 1:size(S,1)
    p0 = S(s,:)';
    scaler = 10.^ceil(log10(abs(p0)));
    th0 = p0 ./ scaler;
    fun = @(th) guard(nll(name, th .* scaler, y));
    opts = optimoptions('fmincon','Display','off','Algorithm','interior-point', ...
        'MaxFunctionEvaluations',150000,'MaxIterations',1000);
    t0 = tic;
    try
      [thh, f, flag, out] = fmincon(fun, th0, [],[],[],[], lb./scaler, ub./scaler, [], opts);
      el = toc(t0);
      fprintf(fid, '%s|%d|%d|%d|%d|%.10f|%.3f|%s\n', name, s, flag, out.iterations, out.funcCount, f, el, mat2str((thh.*scaler)', 8));
    catch err
      fprintf(fid, '%s|%d|ERR|0|0|NaN|%.3f|%s\n', name, s, toc(t0), strtok(err.message, newline));
    end
  end
  fclose(fid);
end
end

function f = guard(v)
if ~isreal(v) || ~isfinite(v), f = NaN; else, f = v; end
end

function S = make_starts(name, n)
S = zeros(n, 0);
for s = 1:n
  rng(1000 + s);
  r = @(a,b) (b-a).*rand + a;
  switch name
    case 'ngarch'
      p = [r(0.000025,0.0009) r(0.025,0.09) r(0.8,0.91) r(0.025,0.09) r(-1,0)];
    case 'cngarch'
      p = [r(0.000025,0.0009) r(0.025,0.09) r(0.8,0.91) r(0.025,0.09) r(-1,0) r(-1,0) r(0.025,0.09) r(0.8,0.91)];
    case 'tvp'
      wxh = r(0.00124,0.3)*ones(1,0); wxh = [r(0.00124,0.3) r(0.00124,0.3) r(0.00124,0.3)];
      p = [wxh r(0.00324,0.3) r(0.000224,0.3) r(0.000324,0.3) r(0.00514,0.3) r(0.000124,0.03) ...
           r(0.025,0.09) r(0.8,0.91) r(0.025,0.09) r(-2.0,-0.7)];
  end
  S(s,1:numel(p)) = p;
end
end

function [lb, ub] = bounds_for(name)
switch name
  case 'ngarch', lb=[0;0;0;0;-5]; ub=[1;1;1;5;0];
  case 'cngarch', lb=[0;0;0;0;-100;-100;0;0]; ub=[1;1;1;5;100;100;1;1];
  case 'tvp', lb=[-100*ones(8,1);0;0;0;-100]; ub=[100*ones(8,1);1;1;5;100];
end
end

function f = nll(name, p, y)
n = numel(y); h = var(y);
c = 0.5*log(2*pi);
switch name
  case 'ngarch'
    om=p(1); al=p(2); ph=p(3); la=p(4); g=p(5);
    z = (y(1)+0.5*h-la*sqrt(h))/sqrt(h); f = c+0.5*log(h)+0.5*z^2;
    for j = 2:n
      h = om+ph*(h-om)+al*(h*z^2+2*h*z*g-h);
      if h <= 0, f = NaN; return; end
      z = (y(j)+0.5*h-la*sqrt(h))/sqrt(h);
      f = f + c+0.5*log(h)+0.5*z^2;
    end
  case 'cngarch'
    om=p(1); al=p(2); ph=p(3); la=p(4); g1=p(5); g2=p(6); p1=p(7); rho=p(8);
    q = h; z = (y(1)+0.5*h-la*sqrt(h))/sqrt(h); f = c+0.5*log(h)+0.5*z^2;
    for j = 2:n
      qn = om+rho*(q-om)+p1*h*(z^2+2*z*g2-1);
      hn = qn+ph*(h-q)+al*h*(z^2+2*z*g1-1);
      q = qn; h = hn;
      if h <= 0, f = NaN; return; end
      z = (y(j)+0.5*h-la*sqrt(h))/sqrt(h);
      f = f + c+0.5*log(h)+0.5*z^2;
    end
  case 'tvp'
    Wx=p(1:3); Wh=p(4); bx=p(5); bh=p(6); fw=p(7); fb=p(8); al=p(9); ph=p(10); la=p(11); g=p(12);
    om = fb; hs = 0;
    z = (y(1)+0.5*h-la*sqrt(h))/sqrt(h); f = c+0.5*log(h)+0.5*z^2;
    for j = 2:n
      hs = tanh(Wx(1)*om+Wx(2)*y(j-1)+Wx(3)*h + bx + Wh*hs + bh);
      om = abs(fw*hs+fb);
      h = om+ph*(h-om)+al*h*(z^2+2*z*g-1);
      if h <= 0, f = NaN; return; end
      z = (y(j)+0.5*h-la*sqrt(h))/sqrt(h);
      f = f + c+0.5*log(h)+0.5*z^2;
    end
end
end
