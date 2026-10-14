function parity()
  dir = '/tmp/g/qp/';
  out = fopen('/tmp/g/qp/matlab_results.txt','w');
  opts_ip  = optimoptions('fmincon','Display','off','Algorithm','interior-point','SpecifyObjectiveGradient',true,'MaxIterations',1000,'MaxFunctionEvaluations',1e5);
  opts_sqp = optimoptions('fmincon','Display','off','Algorithm','sqp','SpecifyObjectiveGradient',true,'MaxIterations',1000,'MaxFunctionEvaluations',1e5);
  qpopts = optimoptions('quadprog','Display','off');
  for k = 0:119
    fid = fopen(sprintf('%sprob_%d.txt',dir,k),'r'); h = fscanf(fid,'%d',4); n=h(1); meq=h(2); mineq=h(3); ub=h(4);
    Q = reshape(fscanf(fid,'%f',n*n),n,n)'; c = fscanf(fid,'%f',n); x0 = fscanf(fid,'%f',n);
    Ae = reshape(fscanf(fid,'%f',meq*n),n,meq)'; be = fscanf(fid,'%f',meq);
    Ai = reshape(fscanf(fid,'%f',mineq*n),n,mineq)'; bi = fscanf(fid,'%f',mineq);
    lb = []; ubd = []; if ub, lb = fscanf(fid,'%f',n); ubd = fscanf(fid,'%f',n); end
    fclose(fid);
    if meq==0, Ae=[]; be=[]; end; if mineq==0, Ai=[]; bi=[]; end
    fun = @(x) qpobj(x,Q,c);
    [~,fq,eq] = quadprog(Q,c,Ai,bi,Ae,be,lb,ubd,x0,qpopts);
    [xi,fi,ei,oi] = fmincon(fun,x0,Ai,bi,Ae,be,lb,ubd,[],opts_ip);
    [xs,fs,es,os] = fmincon(fun,x0,Ai,bi,Ae,be,lb,ubd,[],opts_sqp);
    fprintf(out,'%d %.17g %d %.17g %d %d %.17g %d %d\n',k,fq,eq,fi,ei,oi.iterations,fs,es,os.iterations);
  end
  fclose(out);
  disp('done');
end

function [f,g] = qpobj(x,Q,c)
  f = 0.5*x'*Q*x + c'*x;
  if nargout > 1, g = Q*x + c; end
end
