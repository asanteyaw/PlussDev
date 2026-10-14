"""Compare the Pluss chemistry tranche with Cantera (run with a Python that has cantera, numpy, scipy).

  python -m venv /tmp/ctvenv && /tmp/ctvenv/bin/pip install cantera numpy scipy
  /tmp/ctvenv/bin/python compare_cantera.py

The reference arrays (kinetics_reference.h, equilibrium_reference.h) and mech.json are produced by
the independent NumPy mirror in this directory (gen_ref.py / gen_eq.py); this script checks them and
the mechanism data against Cantera's own h2o2.yaml and solvers.
"""
import os, re, json, numpy as np, cantera as ct
def hdr(path):
    d={}
    for m in re.finditer(r"inline const double (\w+)\[[^\]]*\] = \{([^}]*)\};",open(path).read()):
        d[m.group(1)]=np.array([float(x) for x in m.group(2).split(',')])
    return d
HERE=os.path.dirname(os.path.abspath(__file__))
K=hdr(os.path.join(HERE,'..','kinetics','test','kinetics_reference.h'))
E=hdr(os.path.join(HERE,'..','equilibrium','test','equilibrium_reference.h'))
mech=json.load(open(os.path.join(HERE,'mech.json')))
g=ct.Solution('h2o2.yaml'); names=g.species_names
# --- 1. mechanism data: thermo coefficients and rate parameters vs my fixture
worst=0
for i,sp in enumerate(mech['species']):
    t=g.species(i).thermo
    c=np.array(t.coeffs)   # [Tmid, high(7), low(7)]
    mine=np.array(sp['high']+sp['low'])
    worst=max(worst,np.abs(c[1:]-mine).max()/np.abs(mine).max() if np.abs(mine).max()>0 else 0)
    assert sp['name']==names[i] and abs(t.coeffs[0]-sp['t'][1])<1e-9
print("thermo coefficients: max abs difference over all species/coefficients = %.3e"%max(np.abs(np.array(g.species(i).thermo.coeffs)[1:]-np.array(mech['species'][i]['high']+mech['species'][i]['low'])).max() for i in range(10)))
unit_cal=4184.0 # J/kmol per cal/mol
bad=0
for r,rx in enumerate(mech['reactions']):
    cr=g.reaction(r); d=cr.input_data
    # compare equation stoichiometry
    # compare NET stoichiometry (Cantera writes explicit-collider reactions as three-body
    # reactions with the collider cancelled, e.g. "H + 2 O2 <=> HO2 + O2" -> H + O2 <=> HO2)
    net_ct={k:cr.products.get(k,0)-cr.reactants.get(k,0) for k in set(cr.reactants)|set(cr.products)}
    net_me={k:rx['p'].get(k,0)-rx['r'].get(k,0) for k in set(rx['r'])|set(rx['p'])}
    net_ct={k:v for k,v in net_ct.items() if v!=0}; net_me={k:v for k,v in net_me.items() if v!=0}
    if net_ct!=net_me:
        print("net stoichiometry mismatch reaction",r+1,net_ct,net_me); bad+=1
    rate=cr.rate
    ty=rx['type']
    if ty==0 or ty==1:
        A,b,Ea=rate.pre_exponential_factor,rate.temperature_exponent,rate.activation_energy  # kmol,m,s ; J/kmol
        # my A is cm3/mol/s with Ea cal/mol; convert reaction order into kmol m s
        order=sum(rx['r'].values())+(1 if ty==1 else 0)
        A_mine=rx['A']*(1e-6)**(order-1)*(1e3)**(order-1)   # cgs(cm,mol) -> m,kmol
        ea_mine=rx['Ea']*4184.0
        if abs(A/A_mine-1)>1e-12 or abs(b-rx['b'])>1e-12 or abs(Ea-ea_mine)>1e-6*max(1,abs(ea_mine)):
            print("rate mismatch reaction",r+1,A,A_mine,b,rx['b'],Ea,ea_mine); bad+=1
        if ty==1:
            eff=dict(cr.third_body.efficiencies)
            if {k:v for k,v in rx['eff'].items()}!=eff: print("efficiency mismatch",r+1,eff,rx['eff']); bad+=1
    else:
        lo,hi=rate.low_rate,rate.high_rate
        order=sum(rx['r'].values())
        A_hi=rx['A']*(1e-6*1e3)**(order-1); A_lo=rx['A0']*(1e-6*1e3)**(order+1-1)
        ok=(abs(hi.pre_exponential_factor/A_hi-1)<1e-12 and abs(hi.activation_energy-rx['Ea']*4184)<1e-6*max(1,abs(rx['Ea']*4184)) and abs(lo.pre_exponential_factor/A_lo-1)<1e-12 and abs(lo.temperature_exponent-rx['b0'])<1e-12)
        tp=rate.falloff_coeffs
        ok=ok and np.allclose(tp,rx['tp'],rtol=1e-12)
        if not ok: print("falloff mismatch",r+1); bad+=1
print("mechanism data mismatches (stoichiometry, Arrhenius, efficiencies, falloff):",bad,"of",len(mech['reactions']),"reactions")
# --- 2. rates, Kc, wdot at the three reference states (SI mol/m3)
C=K['conc_si']; temps=K['temps']; nr=len(mech['reactions'])
worst_kc=0;worst_w=0;worst_q=0
for s,T in enumerate(temps):
    g.TD=T,1.0; g.concentrations=C/1000.0     # kmol/m3
    wd=g.net_production_rates*1000.0
    ref=K['wdot_si'][s*10:(s+1)*10]
    worst_w=max(worst_w,np.max(np.abs(wd-ref)/np.maximum(np.abs(ref),1e-12*np.abs(ref).max())))
    kc_ct=g.equilibrium_constants          # kmol/m3 units
    dn=np.array([sum(g.reaction(r).products.values())-sum(g.reaction(r).reactants.values()) for r in range(nr)])
    kc_si=kc_ct*(1000.0**dn)
    ref_kc=K['kc_si'][s*nr:(s+1)*nr]
    worst_kc=max(worst_kc,np.max(np.abs(kc_si/ref_kc-1)))
print("net production rates: max relative difference Cantera vs mine = %.3e"%worst_w)
print("equilibrium constants Kc: max relative difference = %.3e"%worst_kc)
# --- 3. ignition: closed adiabatic constant-volume reactor
for k,T0 in enumerate((1000.0,1100.0,1200.0)):
    g.TPX=T0,ct.one_atm,{'H2':2,'O2':1,'AR':7}
    r=ct.IdealGasReactor(g,energy='on'); r.volume=1.0
    net=ct.ReactorNet([r]); net.rtol=1e-10; net.atol=1e-20
    ts=[0.0];Ts=[T0]
    while ts[-1]<5e-3:
        tt=net.step(); ts.append(tt); Ts.append(r.T)
    ts=np.array(ts);Ts=np.array(Ts)
    ig=ts[np.argmax(np.gradient(Ts,ts))]
    g.TPX=T0,ct.one_atm,{'H2':2,'O2':1,'AR':7}
    r2=ct.IdealGasReactor(g,energy='on'); r2.volume=1.0
    n2=ct.ReactorNet([r2]); n2.rtol=1e-10; n2.atol=1e-20; n2.advance(5e-3)
    print("T0=%.0f  ignition(max dT/dt): Cantera %.6e s  mine %.6e s (ratio %.5f);  T(5 ms): Cantera %.4f K  mine %.4f K"%(T0,ig,K['ignition_s'][k],ig/K['ignition_s'][k],r2.T,K['final_T'][k]))
    if T0==1100.0:
        c=r2.thermo.concentrations*1000.0   # mol/m3
        print("     concentrations at 5 ms (mol/m3), Cantera vs mine:",{n:(float("%.5e"%c[i]),float("%.5e"%K['final_conc_1100_si'][i])) for i,n in enumerate(names) if n in ('H2O','H2','O2','OH')})
# --- 4. equilibrium solver vs Cantera (Gibbs minimiser)
for (T,P),mol in zip(E['tp_cases_T_P'].reshape(-1,2),E['tp_moles'].reshape(-1,10)):
    g.TPX=T,P,{'H2':2,'O2':1,'AR':7}; g.equilibrate('TP')
    xm=mol/mol.sum()
    big=xm>1e-3
    print("TP T=%.0f P=%.0f atm: max |dx| = %.2e ; max relative error (x>1e-3) = %.2e ; max relative error (all x>1e-8) = %.2e"%(T,P/ct.one_atm,np.abs(g.X-xm).max(),np.max(np.abs(g.X-xm)[big]/xm[big]),np.max(np.abs(g.X-xm)[xm>1e-8]/xm[xm>1e-8])))
g.TPX=298.15,ct.one_atm,{'H2':2,'O2':1,'AR':7}; g.equilibrate('HP')
print("HP (2H2+O2+7Ar): Cantera T=%.6f K  mine %.6f K"%(g.T,E['hp_T'][0]))
g.TPX=298.15,ct.one_atm,{'H2':2,'O2':1,'N2':3.76}; g.equilibrate('HP')
print("HP (H2/air):     Cantera T=%.6f K  mine %.6f K"%(g.T,E['air_T'][0]))
g.TPX=1100.0,ct.one_atm,{'H2':0.2,'O2':0.1,'AR':0.7}; g.equilibrate('UV')
print("UV (reactor state): Cantera T=%.6f K  mine %.6f K"%(g.T,E['uv_T'][0]))
