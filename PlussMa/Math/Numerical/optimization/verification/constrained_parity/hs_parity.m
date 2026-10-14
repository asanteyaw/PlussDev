function hs_parity()
  o_ip  = optimoptions('fmincon','Display','off','Algorithm','interior-point','MaxIterations',1000,'MaxFunctionEvaluations',1e5);
  o_sqp = optimoptions('fmincon','Display','off','Algorithm','sqp','MaxIterations',1000,'MaxFunctionEvaluations',1e5);
  P = {};
  % name, obj, nonlcon, lb, ub, x0, fstar
  P{end+1} = {'HS6',  @(x)(1-x(1))^2, @(x)deal([],10*(x(2)-x(1)^2)), [],[], [-1.2 1], 0};
  P{end+1} = {'HS12', @(x)0.5*x(1)^2+x(2)^2-x(1)*x(2)-7*x(1)-7*x(2), @(x)deal(4*x(1)^2+x(2)^2-25,[]), [],[], [0 0], -30};
  P{end+1} = {'HS21', @(x)0.01*x(1)^2+x(2)^2-100, @(x)deal(10-10*x(1)+x(2),[]), [2 -50],[50 50], [-1 -1], -99.96};
  P{end+1} = {'HS35', @(x)9-8*x(1)-6*x(2)-4*x(3)+2*x(1)^2+2*x(2)^2+x(3)^2+2*x(1)*x(2)+2*x(1)*x(3), @(x)deal(x(1)+x(2)+2*x(3)-3,[]), [0 0 0],[], [0.5 0.5 0.5], 1/9};
  P{end+1} = {'HS71', @(x)x(1)*x(4)*(x(1)+x(2)+x(3))+x(3), @(x)deal(25-x(1)*x(2)*x(3)*x(4), sum(x.^2)-40), [1 1 1 1],[5 5 5 5], [1 5 5 1], 17.0140173};
  P{end+1} = {'HS26', @(x)(x(1)-x(2))^2+(x(2)-x(3))^4, @(x)deal([],x(1)*(1+x(2)^2)+x(3)^4-3), [],[], [-2.6 2 2], 0};
  P{end+1} = {'infeasible nl ineq', @(x)sum((x-3).^2), @(x)deal(sum(x.^2)+1,[]), [],[], [0 0], NaN};
  P{end+1} = {'infeasible nl eq',   @(x)sum((x-3).^2), @(x)deal([],sum(x.^2)+1), [],[], [1 1], NaN};
  for k = 1:numel(P)
    p = P{k};
    for alg = 1:2
      if alg==1, o=o_ip; an='interior-point'; else, o=o_sqp; an='sqp'; end
      try
        [x,f,e,out] = fmincon(p{2},p{6},[],[],[],[],p{4},p{5},p{3},o);
        [c,ceq] = p{3}(x); v = max([0; c(:); abs(ceq(:))]);
        fprintf('%-20s %-15s exitflag=%2d it=%3d f=%.8g (f*=%.8g) viol=%.2e\n',p{1},an,e,out.iterations,f,p{7},v);
      catch err
        fprintf('%-20s %-15s ERROR %s\n',p{1},an,strtok(err.message,newline));
      end
    end
  end
end
