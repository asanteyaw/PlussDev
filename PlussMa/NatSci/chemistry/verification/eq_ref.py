import numpy as np
from scipy.optimize import fsolve, brentq
from h2o2_ref import *   # thermo(T)->cp/R,h/RT,s/R ; names, idx, Ecomp, elem, R, P0
ELEM=elem  # order of first appearance in species list? check vs C++
def gRT(T):
    cp,h,s=thermo(T); return h-s
from scipy.optimize import minimize
def eq_tp_raw(T,P,b,guess=None):
    g=gRT(T); A=Ecomp.T  # [E,S]
    act=np.array([all((b[i]>0) or Ecomp[j,i]==0 for i in range(len(b))) for j in range(S)])
    ia=np.where(act)[0]
    def G(x):
        n=np.exp(x); nt=n.sum()
        return (n*(g[ia]+x-np.log(nt)+np.log(P/P0))).sum()
    def dG(x):
        n=np.exp(x); nt=n.sum()
        mu=g[ia]+x-np.log(nt)+np.log(P/P0)
        return n*mu  # d/dx_j of sum n_j mu_j with sum n_j dmu = 0 identity
    cons=[{'type':'eq','fun':(lambda x,i=i:A[i][ia]@np.exp(x)-b[i]),'jac':(lambda x,i=i:A[i][ia]*np.exp(x))} for i in range(len(b)) if b[i]>0]
    x0=np.log(np.full(len(ia),b.sum()/len(ia)))
    res=minimize(G,x0,jac=dG,constraints=cons,bounds=[(-80.0,8.0)]*len(ia),method='SLSQP',options={'maxiter':2000,'ftol':1e-15})
    # polish with dual Newton (fsolve) from the SLSQP element potentials via least squares
    nj=np.zeros(S); nj[ia]=np.exp(res.x)
    mu=g+np.log(np.where(nj>0,nj,1.0))-np.log(nj.sum())+np.log(P/P0)
    pi=np.linalg.lstsq(A[:,ia].T,mu[ia],rcond=None)[0]
    def species(pi,lnn):
        return np.where(act,np.exp(A.T@pi-g-np.log(P/P0)+lnn),0.0)
    def f(z):
        pi=z[:-1]; lnn=z[-1]
        nj=species(pi,lnn)
        r_=[A[i]@nj-b[i] if b[i]>0 else pi[i] for i in range(len(b))]
        r_.append(nj.sum()-np.exp(lnn)); return r_
    z0=np.concatenate([pi,[np.log(nj.sum())]]) if guess is None else guess
    z,info,ier,msg=fsolve(f,z0,full_output=True,xtol=1e-12)
    assert np.abs(f(z)).max()<1e-10,(ier,msg,np.abs(f(z)).max())
    return species(z[:-1],z[-1]),z
_cache={}
def eq_tp(T,P,b):
    key=tuple(np.round(b,12))
    if key not in _cache:
        _cache[key]=eq_tp_raw(2000.0,P0,b)[1]
    zc=_cache[key]
    path=[(Tt,P0) for Tt in np.linspace(2000.0,T,max(2,int(abs(T-2000.0)/100)+2))]
    path+=[(T,Pp) for Pp in np.geomspace(P0,P,6)]
    z=zc
    for Tt,Pp in path:
        nj,z=eq_tp_raw(Tt,Pp,b,guess=z)
    return nj,z

def moles_h(T,nj): cp,h,s=thermo(T); return (nj*h).sum()*R*T
def eq_hp(P,h0,b,T0=2500.0):
    z=None
    def resid(T):
        nonlocal z
        nj,z=eq_tp(T,P,b)
        return moles_h(T,nj)-h0
    T=brentq(resid,1800.0,3400.0,xtol=1e-10)
    nj,_=eq_tp(T,P,b); return T,nj
def eq_tv(T,V,b):
    g=gRT(T); A=Ecomp.T
    act=np.array([all((b[i]>0) or Ecomp[j,i]==0 for i in range(len(b))) for j in range(S)])
    scale=V*P0/(R*T)
    def species(pi): return np.where(act,scale*np.exp(A.T@pi-g),0.0)
    def f(pi): 
        nj=species(pi); return [A[i]@nj-b[i] if b[i]>0 else pi[i] for i in range(len(b))]
    # solve in log of a better start: pi from atoms
    from scipy.optimize import minimize as _min
    ia=np.where(act)[0]
    def Hh(x):
        n=np.exp(x); return (n*(g[ia]+x+np.log(R*T/(V*P0))-1.0)).sum()   # Helmholtz/RT
    def dH(x):
        n=np.exp(x); return n*(g[ia]+x+np.log(R*T/(V*P0)))
    cons=[{'type':'eq','fun':(lambda x,i=i:A[i][ia]@np.exp(x)-b[i]),'jac':(lambda x,i=i:A[i][ia]*np.exp(x))} for i in range(len(b)) if b[i]>0]
    res=_min(Hh,np.log(np.full(len(ia),b.sum()/len(ia))),jac=dH,constraints=cons,bounds=[(-80.0,8.0)]*len(ia),method='SLSQP',options={'maxiter':2000,'ftol':1e-15})
    nj0=np.zeros(S); nj0[ia]=np.exp(res.x)
    mu=g+np.log(np.where(nj0>0,nj0,1.0))+np.log(R*T/(V*P0))
    pi0=np.linalg.lstsq(A[:,ia].T,mu[ia],rcond=None)[0]
    z=fsolve(f,pi0,xtol=1e-12,full_output=True)
    assert np.abs(f(z[0])).max()<1e-10,(z[2],z[3])
    return species(z[0])
def eq_uv(V,u0,b):
    def resid(T):
        nj=eq_tv(T,V,b); cp,h,s=thermo(T); return (nj*(h-1.0)).sum()*R*T-u0
    T=brentq(resid,1800.0,3400.0,xtol=1e-10)
    return T,eq_tv(T,V,b)
if __name__=="__main__":
    print("elements",elem)
    b=np.zeros(len(elem)); 
    def comp(x):
        b=np.zeros(len(elem))
        for k,v in x.items():
            for e,c in sp[idx[k]]['comp'].items(): b[elem.index(e)]+=v*c
        return b
    b1=comp({'H2':2,'O2':1,'AR':7})
    for T,P in ((2000.,P0),(3000.,P0),(3000.,10*P0)):
        nj,_=eq_tp(T,P,b1); print("TP",T,P/P0,["%.8e"%v for v in nj])
    # HP: reactants at 298.15 K, 1 atm
    nj0=np.zeros(S); nj0[idx['H2']]=2; nj0[idx['O2']]=1; nj0[idx['AR']]=7
    h0=moles_h(298.15,nj0)
    T,nj=eq_hp(P0,h0,b1); print("HP T=%.6f"%T,["%.8e"%v for v in nj])
    b2=comp({'H2':2,'O2':1,'N2':3.76})
    nj2=np.zeros(S); nj2[idx['H2']]=2; nj2[idx['O2']]=1; nj2[idx['N2']]=3.76
    T2,nja=eq_hp(P0,moles_h(298.15,nj2),b2); print("HP air T=%.6f"%T2,["%.8e"%v for v in nja])
    # UV from reactor initial state (1100 K, 1 atm) with n_total = 1 mol
    T0=1100.0; nr=np.zeros(S); nr[idx['H2']]=0.2; nr[idx['O2']]=0.1; nr[idx['AR']]=0.7
    V=nr.sum()*R*T0/P0; cp,h,s=thermo(T0); u0=(nr*(h-1.0)).sum()*R*T0
    b3=comp({'H2':0.2,'O2':0.1,'AR':0.7})
    Tuv,nuv=eq_uv(V,u0,b3); print("UV T=%.6f"%Tuv,["%.8e"%v for v in nuv], "V",V,"u0",u0)
    ntv=eq_tv(2500.0,V,b3); print("TV 2500",["%.8e"%v for v in ntv])
