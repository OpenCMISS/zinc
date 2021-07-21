/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _NR_COMPLEX_H_
#define _NR_COMPLEX_H_

#ifndef _FCOMPLEX_DECLARE_T_
typedef struct FCOMPLEX {ZnReal r,i;} fcomplex;
#define _FCOMPLEX_DECLARE_T_
#endif /* _FCOMPLEX_DECLARE_T_ */

#if defined(__STDC__) || defined(ANSI) || defined(NRANSI) /* ANSI */

fcomplex Cadd(fcomplex a, fcomplex b);
fcomplex Csub(fcomplex a, fcomplex b);
fcomplex Cmul(fcomplex a, fcomplex b);
fcomplex Complexr(ZnReal re, ZnReal im);
fcomplex Conjg(fcomplex z);
fcomplex Cdiv(fcomplex a, fcomplex b);
ZnReal Cabs(fcomplex z);
fcomplex Csqrt(fcomplex z);
fcomplex RCmul(ZnReal x, fcomplex a);

#else /* ANSI */
/* traditional - K&R */

fcomplex Cadd();
fcomplex Csub();
fcomplex Cmul();
fcomplex Complexr();
fcomplex Conjg();
fcomplex Cdiv();
ZnReal Cabs();
fcomplex Csqrt();
fcomplex RCmul();

#endif /* ANSI */

#endif /* _NR_COMPLEX_H_ */
