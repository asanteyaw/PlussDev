import json, numpy as np, sys
from scipy.integrate import solve_ivp
d=json.load(open('mech.json'))
R=8.31446261815324; CAL=4.184; P0=101325.0
sp=d['species']; names=[s['name'] for s in sp]; S=len(sp); idx={n:i for i,n in enumerate(names)}
rx=d['reactions']; NR=len(rx)
elem=sorted({e for s in sp for e in s['comp']})
Ecomp=np.array([[s['comp'].get(e,0) for e in elem] for s in sp],float)
def thermo(T):
    cp=np.zeros(S);h=np.zeros(S);s=np.zeros(S)
    for i,q in enumerate(sp):
        a=q['low'] if T<=q['t'][1] else q['high']
        cp[i]=a[0]+a[1]*T+a[2]*T**2+a[3]*T**3+a[4]*T**4
        h[i]=a[0]+a[1]*T/2+a[2]*T**2/3+a[3]*T**3/4+a[4]*T**4/5+a[5]/T
        s[i]=a[0]*np.log(T)+a[1]*T+a[2]*T**2/2+a[3]*T**3/3+a[4]*T**4/4+a[6]
    return cp,h,s
nu_r=np.zeros((NR,S)); nu_p=np.zeros((NR,S))
for r,x in enumerate(rx):
    for k,v in x['r'].items(): nu_r[r,idx[k]]+=v
    for k,v in x['p'].items(): nu_p[r,idx[k]]+=v
nu=nu_p-nu_r
effm=np.ones((NR,S))
for r,x in enumerate(rx):
    for k,v in x['eff'].items(): effm[r,idx[k]]=v
R_cal=R/CAL
def kfs(T,C):   # CGS: cm3, mol, s ; Ea in cal/mol
    M=effm@C
    k=np.zeros(NR)
    for r,x in enumerate(rx):
        kinf=x['A']*T**x['b']*np.exp(-x['Ea']/(R_cal*T))
        if x['type']==0: k[r]=kinf
        elif x['type']==1: k[r]=kinf*M[r]
        else:
            k0=x['A0']*T**x['b0']*np.exp(-x['Ea0']/(R_cal*T))
            Pr=k0*M[r]/kinf
            F=1.0
            if x['troe']:
                a,T3,T1,T2=x['tp']
                Fc=(1-a)*np.exp(-T/T3)+a*np.exp(-T/T1)+(np.exp(-T2/T) if T2>0 else 0)
                lf=np.log10(max(Fc,1e-300)); c=-0.4-0.67*lf; n=0.75-1.27*lf
                lp=np.log10(max(Pr,1e-300)); f1=(lp+c)/(n-0.14*(lp+c))
                F=10**(lf/(1+f1**2))
            k[r]=kinf*Pr/(1+Pr)*F
    return k
def Kc(T):  # concentration units mol/cm3
    cp,h,s=thermo(T); g=h-s
    c0=P0/(R*T)*1e-6
    return np.exp(-(nu@g))*c0**(nu.sum(1))
def wdot(T,C):
    C=np.maximum(C,0.0)
    kf=kfs(T,C); kc=Kc(T); kr=kf/kc
    qf=kf*np.prod(C[None,:]**nu_r,axis=1); qr=kr*np.prod(C[None,:]**nu_p,axis=1)
    return (qf-qr)@nu, qf, qr
def rhs(t,y):
    T=y[0];C=y[1:]
    w,_,_=wdot(T,C)
    cp,h,s=thermo(T)
    u=(h-1.0)*R*T          # J/mol
    cv=(cp-1.0)*R          # J/mol/K
    dT=-(w*u).sum()/((C*cv).sum())
    return np.concatenate([[dT],w])
def initial(T0,p=P0,X=None):
    X=X or {'H2':2.0,'O2':1.0,'AR':7.0}
    x=np.zeros(S)
    for k,v in X.items(): x[idx[k]]=v
    x/=x.sum()
    Ctot=p/(R*T0)*1e-6
    return np.concatenate([[T0],x*Ctot])
if __name__=="__main__":
    out={}
    for T0 in (1000.0,1100.0,1200.0):
        y0=initial(T0)
        sol=solve_ivp(rhs,[0,5e-3],y0,method='Radau',rtol=1e-10,atol=1e-22,dense_output=True)
        t=np.linspace(0,5e-3,200001); Tt=sol.sol(t)[0]
        slope=np.gradient(Tt,t); ig=t[np.argmax(slope)]
        out[T0]=dict(ig=ig,Tend=Tt[-1],pend=None)
        print(T0,"ignition(max dT/dt) %.6e"%ig,"T(5ms) %.6f"%Tt[-1], "nsteps",sol.t.size)
        if T0==1100.0:
            for tt in (2e-5,1e-4,ig*0.9,ig*1.1,5e-3):
                st=sol.sol(tt); print("  t=%.4e T=%.6f"%(tt,st[0]), " C(H2O)=%.6e C(H2)=%.6e C(OH)=%.6e C(O2)=%.6e"%(st[1+idx['H2O']],st[1+idx['H2']],st[1+idx['OH']],st[1+idx['O2']]))
