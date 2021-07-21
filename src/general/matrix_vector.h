/*******************************************************************************
FILE : matrix_vector.h

LAST MODIFIED : 15 March 2005

DESCRIPTION:
Code for performing vector calculations - normalize, dot product etc. -, and
matrix calculations - multiplication, LU decomposition etc. - on small matrices.

Data format:
* All vectors and matrices are to be passed as one-dimensional arrays of
	double precision reals. Index v[0] is the first value.
* When vectors are passed as parameters to subroutines they are preceded by
	the [int] size of the vector. Similarly, matrix parameters are preceded by
	num_rows, num_columns, or just size if a square matrix is expected.
	Furthermore, if a routine expects vectors or matrices of a certain dimension,
	then the size will be evident from its name, eg. normalize3().
* Values in rows of matrices are sequential in the array. The first value in
	the second row is therefore m[num_columns].

Request:
Bit by bit all bits of vector and matrix code should be put in here - and not
be scattered around the source code as it is now. Routines known to contain
vector or matrix routines that should be in here are listed in matrix_vector.c.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MATRIX_VECTOR_H)
#define MATRIX_VECTOR_H

#include "general/value.h"

int identity_matrix4(double *a);
/*******************************************************************************
LAST MODIFIED : 16 November 2004

DESCRIPTION :
Set matrix <a> to the 4x4 identity.
==============================================================================*/

int cross_product3(double *a,double *b,double *result);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Returns in <result> the cross product of the two vectors = a (x) b.
==============================================================================*/

double dot_product3(double *a,double *b);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Returns the dot product of the two vectors = a.b.
==============================================================================*/

/**
 * Returns the norm/length/magnitude of vector <v>.
 */
double norm3(const double *v);

double normalize3(double *v);
/*******************************************************************************
LAST MODIFIED : 15 October 2001

DESCRIPTION :
Normalizes vector v - of dimension 3 - and returns its length.
Zero vectors are returned unchanged.
==============================================================================*/

FE_value normalize_FE_value3(FE_value *v);
/*******************************************************************************
LAST MODIFIED : 15 October 2001

DESCRIPTION :
Normalizes vector v - of dimension 3 - and returns its length.
Zero vectors are returned unchanged.
==============================================================================*/

int normalize_float3(float vector[3]);
/*******************************************************************************
LAST MODIFIED : 27 December 1995

DESCRIPTION :
Normalizes the given <vector>.
==============================================================================*/

int cross_product_float3(float vector_1[3],float vector_2[3],
	float result[3]);
/*******************************************************************************
LAST MODIFIED : 22 September 1999

DESCRIPTION :
Calculates the normalized cross product of <vector_1> and <vector_2> and puts
it in <result>.
==============================================================================*/

double scalar_triple_product3(double *a,double *b,double *c);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Returns the scalar triple product, [abc]=a.(bxc) of vectors a,b and c.
==============================================================================*/

int copy_matrix(int m,int n,double *a,double *a_copy);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Copies the contents of m rows x n columns matrix <a> to <a_copy>.
==============================================================================*/

int identity_matrix(int n, double *a);
/*******************************************************************************
LAST MODIFIED : 6 April 2001

DESCRIPTION :
Set matrix <a> to the <n> x <n> identity.
==============================================================================*/

int identity_matrix_FE_value(int n, FE_value *a);
/*******************************************************************************
LAST MODIFIED : 6 April 2001

DESCRIPTION :
Set matrix <a> to the <n> x <n> identity.
==============================================================================*/

int identity_matrix_float(int n, float *a);
/*******************************************************************************
LAST MODIFIED : 15 November 2001

DESCRIPTION :
Set matrix <a> to the <n> x <n> identity.
==============================================================================*/

int multiply_matrix(int m,int s,int n,double *a,double *b,double *c);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Returns in the m rows x n columns matrix <c> that is the product of
m x s matrix <a> and s x n matrix <b>.
==============================================================================*/

int multiply_matrix_FE_value(int m,int s,int n,FE_value *a,FE_value *b,
	FE_value *c);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Returns in the m rows x n columns matrix <c> that is the product of
m x s matrix <a> and s x n matrix <b>.
==============================================================================*/

int multiply_matrix_float(int m,int s,int n,float *a,float *b,float *c);
/*******************************************************************************
LAST MODIFIED : 2 February 1998

DESCRIPTION :
Returns in the m rows x n columns matrix <c> that is the product of
m x s matrix <a> and s x n matrix <b>.
Uses matrices of floats, not doubles. For matrices od doubles, use "multiply_matrix()"
==============================================================================*/

int print_matrix(int m,int n,double *a,char *number_format);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Prints out the contents of m rows x n columns matrix <a>.
==============================================================================*/

int transpose_matrix(int m,int n,double *a,double *a_transpose);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Return the transpose of m rows x n columns matrix <a> in <a_transpose> - which
will then be n rows x m columns.
==============================================================================*/

int matrix_is_symmetric(int n, double *a, double factor);
/*******************************************************************************
LAST MODIFIED : 7 November 2000

DESCRIPTION :
Returns true if n x n matrix <a> is symmetric about the main diagonal by
checking that the difference between all opposing off-diagonal values is less
<factor> times the largest [absolute] value in <a>.
==============================================================================*/

int LU_decompose(int n, double *a, int *indx, double *d,
	double singular_tolerance);
/*******************************************************************************
LAST MODIFIED : 15 March 2005

DESCRIPTION :
Performs LU decomposition on n x n matrix <a>. <indx> should be a preallocated
integer array of dimension n. On return, <a> contains the lower and upper
decomposed matrix such that a = LU. The indx array contains information about
the partial pivoting performed during decomposition, while the sign of *d
indicates whether an even =+ve, or odd =-ve number of row interchanges have been
performed - I think ;-).
<singular_tolerance> is compared to the magnitude of each pivot.  If any of the
pivot values are less than this parameter the return code will be zero.  The
decomposition is still completed
Adapted from "Numerical Recipes in C".
==============================================================================*/

int LU_backsubstitute(int n,double *a,int *indx,double *b);
/*******************************************************************************
LAST MODIFIED : 28 January 1998

DESCRIPTION :
Partner routine for LU_decompose which takes the n x n LU matrix (a) and indx
vector returned by LU_decompose and solves for x, where LU.x=b. The solution
vector x is returned in the former right-hand-side vector <b>.
Adapted from "Numerical Recipes in C".
==============================================================================*/

int Jacobi_eigenanalysis(int n, double *a, double *d, double *v, int *nrot);
/*******************************************************************************
LAST MODIFIED : 5 April 2002

DESCRIPTION :
Computes all eigenvalues and eigenvectors of real symmetric n x n matrix <a>.
The eigenvalues of <a> are returned in <d>, the columns of v contain the
normalised eigenvectors. <nrot> returns the number of jacobi rotations performed
by the algorithm. The returned eigenvalues/vectors are NOT sorted; use the
eigensort function if required.
The elements of <a> above the diagonal are destroyed by this function.
Adapted from "Numerical Recipes in C".
==============================================================================*/

int eigensort(int n, double *d, double *v);
/*******************************************************************************
LAST MODIFIED : 5 April 2002

DESCRIPTION :
Sorts the n eigenvalues in <d> and corresponding eigenvectors in the columns of
nxn matrix <v> from largest absolute value to smallest.
Adapted from "Numerical Recipes".
==============================================================================*/

/**
 * Calculates the inverse of 2X2 FE_value matrix <a>, returning it in <a_inv>.
 * @return  1 on success, 0 on failure.
 */
int invert_FE_value_matrix2(const FE_value *a, FE_value *a_inv);

/**
 * Calculates the inverse of 3X3 FE_value matrix <a>, returning it in <a_inv>.
 * @return  1 on success, 0 on failure.
 */
int invert_FE_value_matrix3(const FE_value *a, FE_value *a_inv);

int multiply_FE_value_matrix3(FE_value *a,FE_value *b,FE_value *result);
/*******************************************************************************
LAST MODIFIED : 15 March 1999

DESCRIPTION :
Calculates and returns in <result> the matrix product of 3x3 FE_value matrices
<a> and <b>.
==============================================================================*/

/**
 * Calculates and returns in <result> the vector cross product of 2 3-component
 * FE_value vectors <a> and <b>.
 */
int cross_product_FE_value_vector3(const FE_value *a, const FE_value *b, FE_value *result);

/**
 * Calculates and returns in <result> the vector cross product of 3 4-component
 * FE_value vectors <a>, <b> and <c>.
 */
int cross_product_FE_value_vector4(const FE_value *a, const FE_value *b, const FE_value *c, FE_value *result);

#endif /* !defined (MATRIX_VECTOR_H) */
