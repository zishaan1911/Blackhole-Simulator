// Mirrors the GLSL integrator exactly (in double precision) to check physics.
#include <cstdio>
#include <cmath>
#include <algorithm>

double M = 1.0, A = 0.0;

struct IM { double tt, tp, pp, rr, hh; };

void metricInv(double r, double th, IM& g, IM& dr, IM& dth)
{
    double a = A, a2 = a*a;
    double s = sin(th), c = cos(th);
    if (fabs(s) < 1e-4) s = (s < 0) ? -1e-4 : 1e-4;
    double s2 = s*s, r2 = r*r;

    double Sig = r2 + a2*c*c, dSig_r = 2*r, dSig_h = -2*a2*s*c;
    double Del = r2 - 2*M*r + a2, dDel_r = 2*r - 2*M;
    double rr_a2 = r2 + a2;
    double Aa = rr_a2*rr_a2 - a2*Del*s2;
    double dA_r = 4*r*rr_a2 - a2*dDel_r*s2;
    double dA_h = -2*a2*Del*s*c;

    double f = 1.0/(Sig*Del), f2 = f*f;
    double df_r = -(dSig_r*Del + Sig*dDel_r)*f2;
    double df_h = -(dSig_h*Del)*f2;

    g.tt = -Aa*f;  dr.tt = -(dA_r*f + Aa*df_r);  dth.tt = -(dA_h*f + Aa*df_h);
    double k = -2*M*a;
    g.tp = k*r*f;  dr.tp = k*(f + r*df_r);       dth.tp = k*r*df_h;
    double B = Del/s2 - a2, dB_r = dDel_r/s2, dB_h = -2*Del*c/(s2*s);
    g.pp = B*f;    dr.pp = dB_r*f + B*df_r;      dth.pp = dB_h*f + B*df_h;
    double iS = 1.0/Sig, iS2 = iS*iS;
    g.rr = Del*iS; dr.rr = (dDel_r*Sig - Del*dSig_r)*iS2; dth.rr = (-Del*dSig_h)*iS2;
    g.hh = iS;     dr.hh = -dSig_r*iS2;          dth.hh = -dSig_h*iS2;
}

struct St { double r, th, ph, pr, pth; };

void rhs(const St& x, double E, double L, St& d)
{
    IM g, gr, gh; metricInv(x.r, x.th, g, gr, gh);
    double pt = -E, pf = L;
    d.r  = g.rr * x.pr;
    d.th = g.hh * x.pth;
    d.ph = g.tp*pt + g.pp*pf;
    d.pr  = -0.5*(gr.tt*pt*pt + 2*gr.tp*pt*pf + gr.pp*pf*pf + gr.rr*x.pr*x.pr + gr.hh*x.pth*x.pth);
    d.pth = -0.5*(gh.tt*pt*pt + 2*gh.tp*pt*pf + gh.pp*pf*pf + gh.rr*x.pr*x.pr + gh.hh*x.pth*x.pth);
}

St add(const St& a, const St& b, double h)
{ return { a.r+h*b.r, a.th+h*b.th, a.ph+h*b.ph, a.pr+h*b.pr, a.pth+h*b.pth }; }

void rk4(St& x, double E, double L, double h)
{
    St k1,k2,k3,k4;
    rhs(x, E, L, k1);
    rhs(add(x,k1,0.5*h), E, L, k2);
    rhs(add(x,k2,0.5*h), E, L, k3);
    rhs(add(x,k3,h),     E, L, k4);
    x.r   += h/6*(k1.r   + 2*k2.r   + 2*k3.r   + k4.r);
    x.th  += h/6*(k1.th  + 2*k2.th  + 2*k3.th  + k4.th);
    x.ph  += h/6*(k1.ph  + 2*k2.ph  + 2*k3.ph  + k4.ph);
    x.pr  += h/6*(k1.pr  + 2*k2.pr  + 2*k3.pr  + k4.pr);
    x.pth += h/6*(k1.pth + 2*k2.pth + 2*k3.pth + k4.pth);
}

double hamiltonian(const St& x, double E, double L)
{
    IM g, gr, gh; metricInv(x.r, x.th, g, gr, gh);
    double pt = -E;
    return 0.5*(g.tt*pt*pt + 2*g.tp*pt*L + g.pp*L*L + g.rr*x.pr*x.pr + g.hh*x.pth*x.pth);
}

// Build initial conditions exactly like trace() does, for a camera in the
// equatorial plane at radius r0 looking along -x, with an in-plane ray tilted
// by angle `alpha` from the line of sight.
void initRay(double r0, double alpha, St& x, double& E, double& L)
{
    double th0 = M_PI/2, ph0 = 0.0;
    double a = A, a2 = a*a;
    double s = sin(th0), c = cos(th0);
    double Sig = r0*r0 + a2*c*c;
    double Del = r0*r0 - 2*M*r0 + a2;
    double rr_a2 = r0*r0 + a2;
    double Aa = rr_a2*rr_a2 - a2*Del*s*s;

    double lapse = sqrt(Sig*Del/Aa);
    double omega = 2*M*a*r0/Aa;
    double varpi = sqrt(Aa/Sig)*fabs(s);

    // local direction: mostly inward (-r), tilted in the phi direction
    double nr = -cos(alpha), nth = 0.0, nph = sin(alpha);

    double pt_up  = 1.0/lapse;
    double pph_up = omega/lapse + nph/varpi;

    double g_tt = -(1 - 2*M*r0/Sig);
    double g_tp = -2*M*a*r0*s*s/Sig;
    double g_pp = Aa*s*s/Sig;

    E = -(g_tt*pt_up + g_tp*pph_up);
    L =  (g_tp*pt_up + g_pp*pph_up);
    x = { r0, th0, ph0, sqrt(Sig/Del)*nr, sqrt(Sig)*nth };
}

int main()
{
    // ---- Test 1: null condition conservation -------------------------------
    A = 0.9;
    { St x; double E,L; initRay(30.0, 0.20, x, E, L);
      double h0 = hamiltonian(x,E,L), worst = 0;
      for (int i=0;i<4000 && x.r>1.6 && x.r<2000;i++){
          double h = 0.2*x.r/(1+8*M/x.r);
          rk4(x,E,L,h);
          worst = std::max(worst, fabs(hamiltonian(x,E,L)));
      }
      printf("Test 1  null condition H=0 (a=0.9)\n");
      printf("        H(start) = %.3e   max|H| along ray = %.3e\n\n", h0, worst);
    }

    // ---- Test 2: weak-field deflection vs 4M/b -----------------------------
    A = 0.0;
    printf("Test 2  Schwarzschild light deflection (a=0)\n");
    printf("        %-8s %-15s %-15s %-10s\n","b/M","numeric(rad)","4M/b + 1PN","rel.err");
    for (double b : {40.0, 80.0, 160.0, 320.0}) {
        double r0 = 2.0e6;
        double alpha = asin(b/r0);
        St x; double E,L; initRay(r0, alpha, x, E, L);
        // incoming Cartesian direction at phi=0 (e_r = +x, e_phi = +y)
        double d0x = -cos(alpha), d0y = sin(alpha);
        int n=0;
        while (x.r > 2.05 && x.r < 4.0e6 && n < 2000000) {
            double h = 0.01*x.r/(1+8*M/x.r);
            rk4(x,E,L,h); n++;
        }
        // outgoing Cartesian direction via the Jacobian (equatorial, a=0)
        IM g,gr,gh; metricInv(x.r,x.th,g,gr,gh);
        double rdot = g.rr*x.pr;
        double pdot = g.tp*(-E) + g.pp*L;
        double cp = cos(x.ph), sp = sin(x.ph);
        double vx = rdot*cp - x.r*pdot*sp;
        double vy = rdot*sp + x.r*pdot*cp;
        double nv = hypot(vx,vy); vx/=nv; vy/=nv;
        double deflection = acos(std::max(-1.0,std::min(1.0, d0x*vx + d0y*vy)));
        double pred = 4.0*M/b + 15.0*M_PI*M*M/(4.0*b*b);
        printf("        %-8.0f %-15.6e %-15.6e %-10.2e\n",
               b, deflection, pred, fabs(deflection-pred)/pred);
    }
    printf("\n");

    // ---- Test 3: capture impact parameter (shadow size) --------------------
    printf("Test 3  critical impact parameter (photon capture)\n");
    for (double spin : {0.0, 0.5, 0.9}) {
        A = spin;
        double r0 = 4000.0;
        // bisect on alpha: captured vs escaped, prograde side
        double lo = 0.0, hi = 0.01;   // alpha such that b = r0*sin(alpha)
        auto captured = [&](double alpha){
            St x; double E,L; initRay(r0, alpha, x, E, L);
            double rh = M + sqrt(std::max(0.0, M*M - A*A));
            for (int i=0;i<200000;i++){
                if (x.r <= rh*1.0005+1e-3) return true;
                if (x.r >= 8000.0) return false;
                double h = 0.02*x.r/(1+8*M/x.r);
                rk4(x,E,L,h);
                if (x.th < 0) { x.th=-x.th; x.ph+=M_PI; x.pth=-x.pth; }
                if (x.th > M_PI) { x.th=2*M_PI-x.th; x.ph+=M_PI; x.pth=-x.pth; }
            }
            return true;
        };
        for (int i=0;i<60;i++){ double mid=0.5*(lo+hi); if (captured(mid)) lo=mid; else hi=mid; }
        double bcrit = r0*sin(0.5*(lo+hi));
        // analytic prograde equatorial critical b for Kerr:
        // r_ph = 2M(1+cos(2/3 acos(-a/M))),  b = -a + 6M cos((1/3)acos(-a/M)) ... use
        // b_crit = (r_ph^2 + a^2 ... ) use standard: b = -(r^3-3Mr^2+a^2 r+a^2 M)/(a(r-M))
        double rph = 2*M*(1+cos((2.0/3.0)*acos(-spin)));
        double ban;
        if (spin == 0.0) ban = 3*sqrt(3.0)*M;
        else ban = -(rph*rph*rph - 3*M*rph*rph + spin*spin*rph + spin*spin*M)/(spin*(rph-M));
        printf("        a/M=%.1f   numeric b_crit = %.5f M   analytic = %.5f M   err %.2e\n",
               spin, bcrit, ban, fabs(bcrit-ban)/ban);
    }
    printf("\n");

    // ---- Test 4: ISCO / horizon sanity -------------------------------------
    printf("Test 4  frame dragging: photon fired straight at a Kerr hole\n");
    for (double spin : {0.0, 0.9}) {
        A = spin;
        St x; double E,L; initRay(50.0, 0.0, x, E, L);
        double ph_drag = 0;
        for (int i=0;i<20000 && x.r>1.05*(M+sqrt(std::max(0.0,M*M-A*A))) && x.r<60;i++){
            rk4(x,E,L,0.02*x.r/(1+8*M/x.r));
            ph_drag = x.ph;
        }
        printf("        a/M=%.1f   phi swept by a radially-aimed photon = %+.5f rad\n",
               spin, ph_drag);
    }
    return 0;
}
