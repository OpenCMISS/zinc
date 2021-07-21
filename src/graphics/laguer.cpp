/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <math.h>

#include "opencmiss/zinc/zincconfigure.h"

#define NRANSI
#include "graphics/complex.h"
#undef NRANSI
#include "graphics/laguer.h"

#define EPSS 1.0e-7f
#define MR 8
#define MT 10
#define MAXIT (MT*MR)

#define EPS 2.0e-6
#define MAXM 100

static ZnReal maxarg1,maxarg2;
#define FMAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
				(maxarg1) : (maxarg2))

void laguer(fcomplex a[], int m, fcomplex *x, int *its)
{
	int iter,j;
	ZnReal abx,abp,abm,err;
	fcomplex dx,x1,b,d,f,g,h,sq,gp,gm,g2;
	static ZnReal frac[MR+1] = {0.0f,0.5f,0.25f,0.75f,0.13f,0.38f,0.62f,0.88f,1.0f};

	for (iter=1;iter<=MAXIT;iter++) {
		*its=iter;
		b=a[m];
		err=Cabs(b);
		d=f=Complexr(0.0,0.0);
		abx=Cabs(*x);
		for (j=m-1;j>=0;j--) {
			f=Cadd(Cmul(*x,f),d);
			d=Cadd(Cmul(*x,d),b);
			b=Cadd(Cmul(*x,b),a[j]);
			err=Cabs(b)+abx*err;
		}
		err *= EPSS;
		if (Cabs(b) <= err) return;
		g=Cdiv(d,b);
		g2=Cmul(g,g);
		h=Csub(g2,RCmul(2.0,Cdiv(f,b)));
		sq=Csqrt(RCmul((ZnReal) (m-1),Csub(RCmul((ZnReal) m,h),g2)));
		gp=Cadd(g,sq);
		gm=Csub(g,sq);
		abp=Cabs(gp);
		abm=Cabs(gm);
		if (abp < abm) gp=gm;
		dx=((FMAX(abp,abm) > 0.0 ? Cdiv(Complexr((ZnReal) m,0.0),gp)
			: RCmul(exp(log(1+abx)),Complexr(cos((ZnReal)iter),sin((ZnReal)iter)))));
		x1=Csub(*x,dx);
		if (x->r == x1.r && x->i == x1.i) return;
		if (iter % MT) *x=x1;
		else *x=Csub(*x,RCmul(frac[iter/MT],dx));
	}
	return;
}

void zroots(fcomplex a[], int m, fcomplex roots[], int polish)
{
	void laguer(fcomplex a[], int m, fcomplex *x, int *its);
	int i,its,j,jj;
	fcomplex x,b,c,ad[MAXM];

	for (j=0;j<=m;j++) ad[j]=a[j];
	for (j=m;j>=1;j--) {
		x=Complexr(0.5,0.0);
		laguer(ad,j,&x,&its);
		if (fabs(x.i) <= 2.0*EPS*fabs(x.r)) x.i=0.0;
		roots[j]=x;
		b=ad[j];
		for (jj=j-1;jj>=0;jj--) {
			c=ad[jj];
			ad[jj]=b;
			b=Cadd(Cmul(x,b),c);
		}
	}
	if (polish)
		for (j=1;j<=m;j++)
			laguer(a,m,&roots[j],&its);
	for (j=2;j<=m;j++) {
		x=roots[j];
		for (i=j-1;i>=1;i--) {
			if (roots[i].r <= x.r) break;
			roots[i+1]=roots[i];
		}
		roots[i+1]=x;
	}
}
#undef EPSS
#undef MR
#undef MT
#undef MAXIT
#undef NRANSI

#undef EPS
#undef MAXM
/* (C) Copr. 1986-92 Numerical Recipes Software ")121"1w#3Y". */
