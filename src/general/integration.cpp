/*******************************************************************************
FILE : integration.c

LAST MODIFIED : 29 December 2002

DESCRIPTION :
Structures and functions for integerating a computed field over a group of
elements.

???DB.  What about time integrals?
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/field_cache.hpp"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"
#include "general/integration.h"
#include "general/mystring.h"

/*
Module types
------------
*/
struct Integration_scheme
/*******************************************************************************
LAST MODIFIED : 26 December 2002

DESCRIPTION :
An object which when given an element will return the weights and abscissae to
be used when integrating over the element.

???DB.  cm has weights and abscissae for each basis.  A computed field may use
	FE fields with different bases for the same element - if weights and abscissae
	were stored with basis it wouldn't be clear which to use
???DB.  May want to have many ways of setting the weights and abscissae for
	elements eg shape, basis for a particular field.  Start with always same
==============================================================================*/
{
	/* the name of the scheme */
	char *name;
	/* the dimension of the elements that can be integrated by this scheme */
	int dimension;
	/* default weights and abscissae.  NB there are
		dimension*number_of_weights_abscissae values is abscissae with xi varying
		fastest */
	int default_number_of_weights_abscissae;
	FE_value *default_abscissae,*default_weights;
}; /* struct Integration_scheme */

struct Integrate_Computed_field_over_element_data
{
	cmzn_fieldcache_id field_cache;
	FE_value *result, *values;
	int error_code,number_of_components;
	struct Computed_field *field;
	struct Integration_scheme *scheme;
}; /* struct Integrate_Computed_field_over_element_data */

/*
Module functions
----------------
*/

static int Integration_scheme_get_dimension(struct Integration_scheme *scheme,
	int *dimension_address)
/*******************************************************************************
LAST MODIFIED : 27 December 2002

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Integration_scheme_get_dimension);
	return_code=0;
	if (scheme&&dimension_address)
	{
		*dimension_address=scheme->dimension;
	}
	LEAVE;

	return (return_code);
} /* Integration_scheme_get_dimension */

static int Integration_scheme_get_weights_and_abscissae(
	struct Integration_scheme *scheme,struct FE_element *element,
	int *number_of_weights_abscissae_address,
	FE_value **weights_address,FE_value **abscissae_address)
/*******************************************************************************
LAST MODIFIED : 27 December 2002

DESCRIPTION :
Returns the number of weights/abscissae, the weights and the abscissae for the
<element>.

NB.  The <scheme> handles the memory for the weights and abscissae.
<*weights_address> and <*abscissae_address> should not be DEALLOCATE'd/free'd.
==============================================================================*/
{
	int return_code;

	ENTER(Integration_scheme_get_weights_and_abscissae);
	return_code=0;
	if (scheme&&element&&number_of_weights_abscissae_address&&weights_address&&
		abscissae_address)
	{
		*number_of_weights_abscissae_address=
			scheme->default_number_of_weights_abscissae;
		*weights_address=scheme->default_weights;
		*abscissae_address=scheme->default_abscissae;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Integration_scheme_get_weights_and_abscissae.  "
			"Invalid argument(s).  %p %p %p %p %p",scheme,element,
			number_of_weights_abscissae_address,weights_address,abscissae_address);
	}
	LEAVE;

	return (return_code);
} /* Integration_scheme_get_weights_and_abscissae */

static int integrate_Computed_field_over_element(struct FE_element *element,
	void *integrate_Computed_field_over_element_data_void)
/*******************************************************************************
LAST MODIFIED : 27 December 2002

DESCRIPTION :
Calculates the integral of the field over the <element> at the specified time
using the given scheme.  The result array needs to be big enough to hold one
value for each component of the field.
==============================================================================*/
{
	FE_value *abscissa,*result, *values,*weight;
	int dimension,i,j,number_of_components,return_code;
	struct Computed_field *field;
	struct Integrate_Computed_field_over_element_data *data;
	struct Integration_scheme *scheme;

	ENTER(integrate_Computed_field_over_element);
	return_code=0;
	if (element&&(data=(struct Integrate_Computed_field_over_element_data *)
		integrate_Computed_field_over_element_data_void)&&(field=data->field)&&
		(scheme=data->scheme)&&(values=data->values)&&(result=data->result))
	{
		if ((CMZN_OK == cmzn_fieldcache_set_element(data->field_cache, element)) &&
			cmzn_field_is_defined_at_location(field, data->field_cache))
		{
			if (Integration_scheme_get_dimension(scheme,&dimension)&&
				(dimension==get_FE_element_dimension(element))&&
				Integration_scheme_get_weights_and_abscissae(scheme,element,&i,&weight,
				&abscissa))
			{
				number_of_components=data->number_of_components;
				return_code=1;
				while (return_code&&(i>0))
				{
					if ((CMZN_OK == data->field_cache->setMeshLocation(element, abscissa)) &&
						(CMZN_OK == cmzn_field_evaluate_real(field, data->field_cache, number_of_components, values)))
					{
						for (j=0;j<number_of_components;j++)
						{
							result[j]=(*weight)*values[j];
						}
						weight++;
						abscissa += dimension;
						i--;
					}
					else
					{
						data->error_code=3;
						return_code=0;
					}
				}
			}
			else
			{
				data->error_code=2;
				return_code=0;
			}
		}
		else
		{
			data->error_code=1;
			return_code=0;
		}
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* integrate_Computed_field_over_element */

/*
Global functions
----------------
*/
struct Integration_scheme *CREATE(Integration_scheme)(char *name,
	struct FE_basis *basis)
/*******************************************************************************
LAST MODIFIED : 26 December 2002

DESCRIPTION :
Creates an integration scheme with the given <name> that will integrate the
<basis> exactly.  Unless further information is added, the created scheme will
return the weights and abscissae for <basis> for every element.  The <basis>
also defines the dimension of the integration scheme.
==============================================================================*/
{
	enum FE_basis_type basis_type;
	FE_value *abscissa,*weight;
	int dimension,i,j,k,link_type,number_of_weights;
	struct Integration_scheme *scheme;

	ENTER(CREATE(Integration_scheme));
	scheme=(struct Integration_scheme *)NULL;
	/* check arguments */
	if (name&&basis)
	{
		if (ALLOCATE(scheme,struct Integration_scheme,1))
		{
			if (ALLOCATE(scheme->name,char,strlen(name)+1))
			{
				strcpy(scheme->name,name);
				if (FE_basis_get_dimension(basis,&dimension))
				{
					scheme->dimension=dimension;
					/* determine the number of weights */
					number_of_weights=1;
					i=0;
					k=1;
					while (k&&(i<dimension)&&FE_basis_get_xi_basis_type(basis,i,
						&basis_type)&&FE_basis_get_next_linked_xi_number(basis,i,&j,
						&link_type)
						/*???DB.  Starting with tensor products */
						&&(0==j)&&(0==link_type)
						)
					{
						switch (basis_type)
						{
							case LINEAR_LAGRANGE:
							{
								/* 1 weight.  Do nothing */
							} break;
							case QUADRATIC_LAGRANGE:
							{
								number_of_weights *= 2;
							} break;
							case CUBIC_HERMITE:
							case CUBIC_HERMITE_SERENDIPITY:
							case CUBIC_LAGRANGE:
							{
								number_of_weights *= 3;
							} break;
							default:
							{
								k=0;
							} break;
						}
						i++;
					}
					if (k&&(i>=dimension))
					{
						weight=(FE_value *)NULL;
						abscissa=(FE_value *)NULL;
						if (ALLOCATE(weight,FE_value,number_of_weights)&&
							ALLOCATE(abscissa,FE_value,number_of_weights*dimension))
						{
							scheme->default_number_of_weights_abscissae=number_of_weights;
							scheme->default_weights=weight;
							scheme->default_abscissae=abscissa;
							*weight=(FE_value)1;
							for (i=dimension;i>0;i--)
							{
								*abscissa=(FE_value)0.5;
								abscissa++;
							}
							number_of_weights=1;
							for (i=0;i<dimension;i++)
							{
								FE_basis_get_xi_basis_type(basis,i,&basis_type);
								weight=scheme->default_weights;
								abscissa=scheme->default_abscissae;
								switch (basis_type)
								{
									case LINEAR_LAGRANGE:
									{
										/* 1 weight.  Do nothing */
									} break;
									case QUADRATIC_LAGRANGE:
									{
										for (j=number_of_weights;j>0;j--)
										{
											*weight *= (FE_value)0.5;
											weight[number_of_weights]= *weight;
											weight++;
											for (k=i-1;k>0;k--)
											{
												abscissa[dimension*number_of_weights]= *abscissa;
												abscissa++;
											}
											*abscissa=(FE_value)0.5-(FE_value)0.288675134594813;
											abscissa[dimension*number_of_weights]=
												(FE_value)0.5+(FE_value)0.288675134594813;
											abscissa += dimension-i+1;
										}
										number_of_weights *= 2;
									} break;
									case CUBIC_HERMITE:
									case CUBIC_HERMITE_SERENDIPITY:
									case CUBIC_LAGRANGE:
									{
										for (j=number_of_weights;j>0;j--)
										{
											weight[number_of_weights]=
												(*weight)*(FE_value)0.444444444444444;
											*weight *= (FE_value)0.277777777777778;
											weight[2*number_of_weights]= *weight;
											weight++;
											for (k=i-1;k>0;k--)
											{
												abscissa[dimension*number_of_weights]= *abscissa;
												abscissa[2*dimension*number_of_weights]= *abscissa;
												abscissa++;
											}
											*abscissa=(FE_value)0.5-(FE_value)0.387298334620741;
											abscissa[dimension*number_of_weights]=(FE_value)0.5;
											abscissa[dimension*2*number_of_weights]=
												(FE_value)0.5+(FE_value)0.387298334620741;
											abscissa += dimension-i+1;
										}
										number_of_weights *= 3;
									} break;
									default:
									{
										// shouldn't be reachable due to last switch default action
										display_message(ERROR_MESSAGE,"CREATE(Integration_scheme).  Invalid basis");
									} break;
								}
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,"CREATE(Integration_scheme).  "
								"Invalid basis");
							DEALLOCATE(weight);
							DEALLOCATE(abscissa);
							DEALLOCATE(scheme->name);
							DEALLOCATE(scheme);
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"CREATE(Integration_scheme).  "
							"Invalid basis");
						DEALLOCATE(scheme->name);
						DEALLOCATE(scheme);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"CREATE(Integration_scheme).  "
						"Could not get basis dimension");
					DEALLOCATE(scheme->name);
					DEALLOCATE(scheme);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"CREATE(Integration_scheme).  "
					"Could not allocate scheme->name");
				DEALLOCATE(scheme);
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CREATE(Integration_scheme).  "
				"Could not allocate scheme");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Integration_scheme).  "
			"Invalid argument(s).  %p %p",name,basis);
	}
	LEAVE;

	return (scheme);
} /* CREATE(Integration_scheme) */

int DESTROY(Integration_scheme)(struct Integration_scheme **scheme_address)
/*******************************************************************************
LAST MODIFIED : 27 December 2002

DESCRIPTION :
Frees memory/deaccess objects in scheme at <*scheme_address>.
==============================================================================*/
{
	int return_code;
	struct Integration_scheme *scheme;

	ENTER(DESTROY(Integration_scheme));
	return_code=0;
	if (scheme_address&&(scheme= *scheme_address))
	{
		DEALLOCATE(scheme->name);
		DEALLOCATE(scheme->default_weights);
		DEALLOCATE(scheme->default_abscissae);
		DEALLOCATE(scheme);
		*scheme_address=(struct Integration_scheme *)NULL;
		return_code=1;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Integration_scheme) */

int integrate(struct Computed_field *field,struct FE_region *domain,
	struct Integration_scheme *scheme,FE_value time,FE_value *result)
/*******************************************************************************
LAST MODIFIED : 27 December 2002

DESCRIPTION :
Calculates the integral of the <field> over the <domain> at the specified <time>
using the given <scheme>.  The <result> array needs to be big enough to hold one
value for each component of the <field>.

???DB.  "integrate" may be to general a name?  Time integrals (solving ODE?)?
==============================================================================*/
{
	int i,number_of_components,return_code;
	struct Integrate_Computed_field_over_element_data
		integrate_Computed_field_over_element_data;

	ENTER(integrate);
	return_code=0;
	/* check arguments */
	if (field&&domain&&scheme&&result)
	{
		/* check that field is numeric */
		if (Computed_field_has_numerical_components(field,(void *)NULL))
		{
			/* evaluate the integral */
			number_of_components=cmzn_field_get_number_of_components(field);
			if ((0<number_of_components)&&
				ALLOCATE(integrate_Computed_field_over_element_data.values,FE_value,
				number_of_components))
			{
				cmzn_fieldmodule_id field_module = cmzn_field_get_fieldmodule(field);
				cmzn_fieldcache_id field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
				cmzn_fieldcache_set_time(field_cache, time);
				integrate_Computed_field_over_element_data.field_cache = field_cache;
				integrate_Computed_field_over_element_data.field=field;
				integrate_Computed_field_over_element_data.scheme=scheme;
				integrate_Computed_field_over_element_data.result=result;
				integrate_Computed_field_over_element_data.number_of_components=
					number_of_components;
				integrate_Computed_field_over_element_data.error_code=0;
				for (i=0;i<number_of_components;i++)
				{
					result[i]=(FE_value)0;
				}
				if (!(return_code=FE_region_for_each_FE_element(domain,
					integrate_Computed_field_over_element,
					&integrate_Computed_field_over_element_data)))
				{
					display_message(ERROR_MESSAGE,"integrate.  "
						"Failed for an element.  %d",
						integrate_Computed_field_over_element_data.error_code);
				}
				DEALLOCATE(integrate_Computed_field_over_element_data.values);
				cmzn_fieldcache_destroy(&field_cache);
				cmzn_fieldmodule_destroy(&field_module);
			}
			else
			{
				display_message(ERROR_MESSAGE,"integrate.  "
					"Could not ALLOCATE values.  %d",number_of_components);
				return_code=0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"integrate.  "
				"Field is not numeric");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"integrate.  "
			"Invalid argument(s).  %p %p %p %p",field,domain,scheme,result);
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* integrate */
