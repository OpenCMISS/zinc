/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#if !defined (ALT_VFSCANF_H)
#define ALT_VFSCANF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int alt_vfscanf(FILE *fp, char const *fmt0, va_list ap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ALT_VFSCANF_H */