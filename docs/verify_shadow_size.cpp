#include <vector>
// Replays the shader's exact stepping policy: step budget + accuracy at the
// production settings (stepScale = 0.20, maxSteps = 320, escape = 1000).
#include <cstdio>
#include <cmath>
#include <algorithm>
double M = 1.0, A = 0.0;
struct IM { double tt, tp, pp, rr, hh; };
void metricInv(double r, double th, IM& g, IM& dr, IM& dth){
    double a=A,a2=a*a; double s=sin(th),c=cos(th);
    if(fabs(s)<1e-4) s=(s<0)?-1e-4:1e-4;
    double s2=s*s,r2=r*r;
    double Sig=r2+a2*c*c,dSig_r=2*r,dSig_h=-2*a2*s*c;
    double Del=r2-2*M*r+a2,dDel_r=2*r-2*M;
    double ra=r2+a2, Aa=ra*ra-a2*Del*s2;
    double dA_r=4*r*ra-a2*dDel_r*s2, dA_h=-2*a2*Del*s*c;
    double f=1.0/(Sig*Del),f2=f*f;
    double df_r=-(dSig_r*Del+Sig*dDel_r)*f2, df_h=-(dSig_h*Del)*f2;
    g.tt=-Aa*f; dr.tt=-(dA_r*f+Aa*df_r); dth.tt=-(dA_h*f+Aa*df_h);
    double k=-2*M*a; g.tp=k*r*f; dr.tp=k*(f+r*df_r); dth.tp=k*r*df_h;
    double B=Del/s2-a2,dB_r=dDel_r/s2,dB_h=-2*Del*c/(s2*s);
    g.pp=B*f; dr.pp=dB_r*f+B*df_r; dth.pp=dB_h*f+B*df_h;
    double iS=1.0/Sig,iS2=iS*iS;
    g.rr=Del*iS; dr.rr=(dDel_r*Sig-Del*dSig_r)*iS2; dth.rr=(-Del*dSig_h)*iS2;
    g.hh=iS; dr.hh=-dSig_r*iS2; dth.hh=-dSig_h*iS2;
}
struct St{double r,th,ph,pr,pth;};
void rhs(const St&x,double E,double L,St&d){
    IM g,gr,gh; metricInv(x.r,x.th,g,gr,gh);
    double pt=-E,pf=L;
    d.r=g.rr*x.pr; d.th=g.hh*x.pth; d.ph=g.tp*pt+g.pp*pf;
    d.pr =-0.5*(gr.tt*pt*pt+2*gr.tp*pt*pf+gr.pp*pf*pf+gr.rr*x.pr*x.pr+gr.hh*x.pth*x.pth);
    d.pth=-0.5*(gh.tt*pt*pt+2*gh.tp*pt*pf+gh.pp*pf*pf+gh.rr*x.pr*x.pr+gh.hh*x.pth*x.pth);
}
St add(const St&a,const St&b,double h){return{a.r+h*b.r,a.th+h*b.th,a.ph+h*b.ph,a.pr+h*b.pr,a.pth+h*b.pth};}
void rk4(St&x,double E,double L,double h){
    St k1,k2,k3,k4; rhs(x,E,L,k1); rhs(add(x,k1,.5*h),E,L,k2);
    rhs(add(x,k2,.5*h),E,L,k3); rhs(add(x,k3,h),E,L,k4);
    x.r+=h/6*(k1.r+2*k2.r+2*k3.r+k4.r);   x.th+=h/6*(k1.th+2*k2.th+2*k3.th+k4.th);
    x.ph+=h/6*(k1.ph+2*k2.ph+2*k3.ph+k4.ph);
    x.pr+=h/6*(k1.pr+2*k2.pr+2*k3.pr+k4.pr); x.pth+=h/6*(k1.pth+2*k2.pth+2*k3.pth+k4.pth);
}
double STEPSCALE=0.20, DISKOUT=20.0; int ENABLEDISK=1;
double stepSize(double r){
    double h = STEPSCALE*r/(1.0+8.0*M/r);
    if(ENABLEDISK && r < DISKOUT*1.3) h = std::min(h, 2.5*STEPSCALE);
    return std::max(h,0.002);
}
void initRay(double r0,double alpha,St&x,double&E,double&L){
    double th0=M_PI/2,a=A,a2=a*a,s=1,c=0;
    double Sig=r0*r0, Del=r0*r0-2*M*r0+a2, ra=r0*r0+a2, Aa=ra*ra-a2*Del;
    double lapse=sqrt(Sig*Del/Aa), omega=2*M*a*r0/Aa, varpi=sqrt(Aa/Sig);
    double nr=-cos(alpha),nph=sin(alpha);
    double pt=1.0/lapse, pph=omega/lapse+nph/varpi;
    double gtt=-(1-2*M*r0/Sig), gtp=-2*M*a*r0/Sig, gpp=Aa/Sig;
    E=-(gtt*pt+gtp*pph); L=(gtp*pt+gpp*pph);
    x={r0,th0,0.0,sqrt(Sig/Del)*nr,0.0};
}

double DENOM=8.0, CAPMUL=2.5, MARGIN=0.75;
double stepSize2(double r){
    double h=STEPSCALE*r/(1.0+DENOM*M/r);
    if(ENABLEDISK && r<DISKOUT*1.3) h=std::min(h,CAPMUL*STEPSCALE);
    return std::max(h,0.002);
}
// returns the conserved impact parameter b = L/E of the marginal ray
double critB(double spin,double r0,int budget,int&p99){
    A=spin; double rh=M+sqrt(std::max(0.0,M*M-A*A));
    double rph=2*M*(1+cos((2.0/3.0)*acos(std::max(-1.0,std::min(1.0,-A/M)))));
    double rstop=rh+MARGIN*std::max(rph-rh,0.0);
    double lo=0.0,hi=0.6; double bAtLo=0;
    auto cap=[&](double alpha,double&bOut){
        St x; double E,L; initRay(r0,alpha,x,E,L); bOut=L/E;
        for(int i=0;i<budget;i++){
            if(x.r<=rstop) return 1;
            if(x.r>=1000.0) return 0;
            rk4(x,E,L,stepSize2(x.r));
            if(x.th<0){x.th=-x.th;x.ph+=M_PI;x.pth=-x.pth;}
            if(x.th>M_PI){x.th=2*M_PI-x.th;x.ph+=M_PI;x.pth=-x.pth;}
        }
        return 1;
    };
    for(int i=0;i<60;i++){double m=.5*(lo+hi),bb; if(cap(m,bb)!=0){lo=m;bAtLo=bb;} else hi=m;}
    std::vector<int> st;
    for(int i=0;i<1500;i++){
        double alpha=-0.6+1.2*i/1499.0; St x;double E,L;initRay(r0,alpha,x,E,L);int n=0;
        for(;n<budget;n++){ if(x.r<=rstop)break; if(x.r>=1000.0)break;
            rk4(x,E,L,stepSize2(x.r));
            if(x.th<0){x.th=-x.th;x.ph+=M_PI;x.pth=-x.pth;}
            if(x.th>M_PI){x.th=2*M_PI-x.th;x.ph+=M_PI;x.pth=-x.pth;} }
        st.push_back(n);
    }
    std::sort(st.begin(),st.end()); p99=st[(int)(st.size()*0.99)];
    return bAtLo;
}
int main(){
    printf("Conserved impact parameter b = L/E of the marginally captured ray\n");
    printf("(camera at r = 26 M, shader step policy)\n\n");
    printf("%-8s %-7s %-13s %-13s %-10s %-6s\n","step","a/M","numeric b","analytic b","rel.err","p99");
    for(double st:{0.25}){
      STEPSCALE=st; CAPMUL=2.5;
      for(double spin:{0.0,0.5,0.85,0.95}){
        int p99; double b=critB(spin,26.0,200000,p99);
        double rph=2*M*(1+cos((2.0/3.0)*acos(-spin)));
        double ban=(spin==0.0)?3*sqrt(3.0)*M
            :-(rph*rph*rph-3*M*rph*rph+spin*spin*rph+spin*spin*M)/(spin*(rph-M));
        printf("%-8.2f %-7.2f %-13.6f %-13.6f %-10.2e %-6d\n",st,spin,b,ban,fabs(b-ban)/fabs(ban),p99);
      }
    }
    return 0;
}
