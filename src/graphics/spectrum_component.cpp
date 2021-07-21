/*******************************************************************************
FILE : spectrum_component.cpp

Largely pillaged from graphics/element_group_component.c

LAST MODIFIED : 15 March 2002

DESCRIPTION :
cmzn_spectrumcomponent structure and routines for describing and manipulating the
appearance of spectrums.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>

#include "opencmiss/zinc/zincconfigure.h"

#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/list.h"
#include "general/compare.h"
#include "general/mystring.h"
#include "general/object.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_set.h"
#include "graphics/colour.h"
#include "graphics/graphics_library.h"
#include "graphics/material.h"
#include "graphics/spectrum.h"
#include "graphics/spectrum_component.h"
#include "general/message.h"
#include "general/enumerator_conversion.hpp"
#include "general/enumerator_private.hpp"

/*
Global variables
----------------
*/
/*
Module types
------------
*/

FULL_DECLARE_INDEXED_LIST_TYPE(cmzn_spectrumcomponent);

/*
Module functions
----------------
*/
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(cmzn_spectrumcomponent,position,int,compare_int)

void cmzn_spectrumcomponent_changed(cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		component->changed=1;
		cmzn_spectrum_changed(component->spectrum);
	}
}

int cmzn_spectrumcomponent_set_active(cmzn_spectrumcomponent_id component,
	bool active)
{
	if (component)
	{
		if (component->active != active)
		{
			component->active = active;
			cmzn_spectrum_changed(component->spectrum);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_spectrumcomponent_is_active(cmzn_spectrumcomponent_id component)
{
	if (component)
		return component->active;
	return false;
}

class cmzn_spectrumcomponent_colour_mapping_type_conversion
{
public:
	static const char *to_string(enum cmzn_spectrumcomponent_colour_mapping_type colour)
	{
		const char *enum_string = 0;
		switch (colour)
		{
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_ALPHA:
			enum_string = "ALPHA";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED:
			enum_string = "BANDED";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BLUE:
			enum_string = "BLUE";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_GREEN:
			enum_string = "GREEN";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_MONOCHROME:
			enum_string = "MONOCHROME";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW:
			enum_string = "RAINBOW";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RED:
			enum_string = "RED";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_STEP:
			enum_string = "STEP";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_BLUE:
			enum_string = "WHITE_TO_BLUE";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_RED:
			enum_string = "WHITE_TO_RED";
			break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_GREEN:
			enum_string = "WHITE_TO_GREEN";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_spectrumcomponent_colour_mapping_type cmzn_spectrumcomponent_colour_mapping_type_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_spectrumcomponent_colour_mapping_type,
		cmzn_spectrumcomponent_colour_mapping_type_conversion>(string);
}

char *cmzn_spectrumcomponent_colour_mapping_type_enum_to_string(
	enum cmzn_spectrumcomponent_colour_mapping_type component_colour)
{
	const char *colour_string = cmzn_spectrumcomponent_colour_mapping_type_conversion::to_string(component_colour);
	return (colour_string ? duplicate_string(colour_string) : 0);
}


class cmzn_spectrumcomponent_scale_type_conversion
{
public:
	static const char *to_string(enum cmzn_spectrumcomponent_scale_type colour)
	{
		const char *enum_string = 0;
		switch (colour)
		{
		case CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_LINEAR:
			enum_string = "LINEAR";
			break;
		case CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_LOG:
			enum_string = "LOG";
			break;
		default:
			break;
		}
		return enum_string;
	}
};

enum cmzn_spectrumcomponent_scale_type cmzn_spectrumcomponent_scale_type_enum_from_string(
	const char *string)
{
	return string_to_enum<enum cmzn_spectrumcomponent_scale_type,
		cmzn_spectrumcomponent_scale_type_conversion>(string);
}

char *cmzn_spectrumcomponent_scale_type_enum_to_string(
	enum cmzn_spectrumcomponent_scale_type scale_type)
{
	const char *type_string = cmzn_spectrumcomponent_scale_type_conversion::to_string(scale_type);
	return (type_string ? duplicate_string(type_string) : 0);
}

/*
Global functions
----------------
*/
PROTOTYPE_ENUMERATOR_STRING_FUNCTION(cmzn_spectrumcomponent_colour_mapping_type)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(cmzn_spectrumcomponent_colour_mapping_type));
	switch (enumerator_value)
	{
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_ALPHA:
		{
			enumerator_string = "alpha";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED:
		{
			enumerator_string = "banded";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BLUE:
		{
			enumerator_string = "blue";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_GREEN:
		{
			enumerator_string = "green";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_MONOCHROME:
		{
			enumerator_string = "monochrome";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW:
		{
			enumerator_string = "rainbow";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RED:
		{
			enumerator_string = "red";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_STEP:
		{
			enumerator_string = "step";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_BLUE:
		{
			enumerator_string = "white_to_blue";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_RED:
		{
			enumerator_string = "white_to_red";
		} break;
		case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_GREEN:
		{
			enumerator_string = "white_to_green";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(cmzn_spectrumcomponent_colour_mapping_type) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(cmzn_spectrumcomponent_colour_mapping_type)

struct cmzn_spectrumcomponent *CREATE(cmzn_spectrumcomponent)(void)
/*******************************************************************************
LAST MODIFIED : 14 July 1998

DESCRIPTION :
Allocates memory and assigns fields for a struct cmzn_spectrumcomponent.
==============================================================================*/
{
	struct cmzn_spectrumcomponent *component;

	ENTER(CREATE(cmzn_spectrumcomponent));
	if (ALLOCATE(component,struct cmzn_spectrumcomponent,1))
	{
		component->spectrum = 0;
		component->component_number = 0;
		component->component_scale=CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_LINEAR;
		component->changed=1;
		component->minimum = 0;
		component->maximum = 1;
		component->fix_maximum = false;
		component->fix_minimum = false;
		component->extend_above = 1;
		component->extend_below = 1;
		component->min_value = 0;
		component->max_value = 1;
		component->colour_mapping_type = CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW;
		component->reverse = false;
		component->exaggeration = 1.0;
		component->step_value = 0.5;
		component->number_of_bands = 10;
		component->black_band_proportion = 200;
		component->active=true;
		component->position = 0;
		component->input_field = (struct Computed_field *)NULL;
		component->output_field = (struct Computed_field *)NULL;
		component->is_field_lookup = false;
#if defined (OPENGL_API)
		component->texture_id=0;
#endif /* defined (OPENGL_API) */
		component->access_count=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(cmzn_spectrumcomponent).  "
			"Insufficient memory");
	}
	LEAVE;

	return (component);
} /* CREATE(cmzn_spectrumcomponent) */

int DESTROY(cmzn_spectrumcomponent)(struct cmzn_spectrumcomponent **component_ptr)
/*******************************************************************************
LAST MODIFIED : 10 March 1998

DESCRIPTION :
Frees the memory for the fields of <**component_ptr>, frees the memory for
<**component_ptr> and sets <*component_ptr> to NULL.
==============================================================================*/
{
	struct cmzn_spectrumcomponent *component;
	int return_code;

	ENTER(DESTROY(cmzn_spectrumcomponent));
	if (component_ptr)
	{
		component= *component_ptr;
		if (component)
		{
#if defined (OPENGL_API)
			if (component->texture_id)
			{
				glDeleteTextures(1, &(component->texture_id));
			}
#endif /* defined (OPENGL_API) */
			switch (component->component_scale)
			{
				case CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_LINEAR:
				case CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_LOG:
				{
					/* Don't need to do anything */
				} break;
				default:
				{
				} break;
			}
			if (component->input_field)
			{
				DEACCESS(Computed_field)(&component->input_field);
			}
			if (component->output_field)
			{
				DEACCESS(Computed_field)(&component->output_field);
			}
			/*???RC check temp access_count is zero! */
			if (0!=component->access_count)
			{
				display_message(ERROR_MESSAGE,
					"DESTROY(cmzn_spectrumcomponent).  Non-zero access_count");
			}
			DEALLOCATE(*component_ptr);
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(cmzn_spectrumcomponent).  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(cmzn_spectrumcomponent) */

DECLARE_OBJECT_FUNCTIONS(cmzn_spectrumcomponent)
DECLARE_INDEXED_LIST_FUNCTIONS(cmzn_spectrumcomponent)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(cmzn_spectrumcomponent, \
	position,int,compare_int)

PROTOTYPE_COPY_OBJECT_FUNCTION(cmzn_spectrumcomponent)
/*******************************************************************************
LAST MODIFIED : 10 March 1998

DESCRIPTION :
syntax: COPY(cmzn_spectrumcomponent)(destination,source)
Copies the Spectrum contents from source to destination.
Note: destination->access_count is not changed by COPY.
==============================================================================*/
{
	int return_code;

	ENTER(COPY(cmzn_spectrumcomponent));
	if (source&&destination)
	{
		destination->changed = 1;
		/* copy component used by all component_scales */
		destination->component_number = source->component_number;
		destination->component_scale = source->component_scale;
		destination->reverse = source->reverse;
		destination->position = source->position;
		destination->active = source->active;
		destination->minimum = source->minimum;
		destination->maximum = source->maximum;
		destination->fix_maximum = source->fix_maximum;
		destination->fix_minimum = source->fix_minimum;
		destination->extend_above = source->extend_above;
		destination->extend_below = source->extend_below;
		destination->min_value = source->min_value;
		destination->max_value = source->max_value;
		destination->colour_mapping_type = source->colour_mapping_type;
		destination->exaggeration = source->exaggeration;
		destination->number_of_bands = source->number_of_bands;
		destination->black_band_proportion = source->black_band_proportion;
		destination->is_field_lookup = source->is_field_lookup;
		destination->step_value = source->step_value;
		REACCESS(Computed_field)(&destination->input_field,
			source->input_field);
		REACCESS(Computed_field)(&destination->output_field,
			source->output_field);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"COPY(cmzn_spectrumcomponent).  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* COPY(cmzn_spectrumcomponent) */

int cmzn_spectrumcomponent_copy_and_put_in_list(
	struct cmzn_spectrumcomponent *component,void *list_of_components_void)
/*******************************************************************************
LAST MODIFIED : 27 July 1998

DESCRIPTION :
cmzn_spectrumcomponent iterator function for copying a list_of_components.
Makes a copy of the component and puts it in the list_of_components.
==============================================================================*/
{
	int return_code;
	struct cmzn_spectrumcomponent *copy_component;
	struct LIST(cmzn_spectrumcomponent) *list_of_components;

	ENTER(cmzn_spectrumcomponent_copy_and_put_in_list);
	if (component&&(NULL != (list_of_components=
		(struct LIST(cmzn_spectrumcomponent) *)list_of_components_void)))
	{
		/* create new component to take the copy */
		copy_component=CREATE(cmzn_spectrumcomponent)();
		if (copy_component)
		{
			/* copy and insert in list */
			if (!(return_code=COPY(cmzn_spectrumcomponent)(copy_component,component)&&
				ADD_OBJECT_TO_LIST(cmzn_spectrumcomponent)(copy_component,
					list_of_components)))
			{
				display_message(ERROR_MESSAGE,
					"cmzn_spectrumcomponent_copy_and_put_in_list.  "
					"Could not put copy in list");
			}
			DEACCESS(cmzn_spectrumcomponent)(&copy_component);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_spectrumcomponent_copy_and_put_in_list.  Could not create copy");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_spectrumcomponent_copy_and_put_in_list.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_spectrumcomponent_copy_and_put_in_list */

enum cmzn_spectrumcomponent_scale_type cmzn_spectrumcomponent_get_scale_type(
	cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->component_scale;
	}

	return CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_INVALID;
}

int cmzn_spectrumcomponent_set_scale_type(
	cmzn_spectrumcomponent_id component,
	enum cmzn_spectrumcomponent_scale_type scale_type)
{
	if (component)
	{
		if (scale_type != component->component_scale)
		{
			component->component_scale = scale_type;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_spectrumcomponent_get_field_component(
	cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->component_number + 1;
	}

	return 0;
}

int cmzn_spectrumcomponent_set_field_component(
	cmzn_spectrumcomponent_id component, int component_number)
{
	if (component && (component_number > 0))
	{
		if (component->component_number != (component_number - 1))
		{
			component->component_number = component_number - 1;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_spectrumcomponent_is_colour_reverse(cmzn_spectrumcomponent_id component)
{
	if (component)
		return component->reverse;
	return false;
}

int cmzn_spectrumcomponent_set_colour_reverse(cmzn_spectrumcomponent_id component,
	bool reverse)
{
	if (component)
	{
		if (component->reverse != reverse)
		{
			component->reverse = reverse;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_spectrumcomponent_colour_mapping_type cmzn_spectrumcomponent_get_colour_mapping_type(
	struct cmzn_spectrumcomponent *component)
/*******************************************************************************
LAST MODIFIED : 14 July 1998

DESCRIPTION :
Returns the colour mapping of the cmzn_spectrumcomponent <spectrum>.
==============================================================================*/
{
	enum cmzn_spectrumcomponent_colour_mapping_type type;

	ENTER(cmzn_spectrumcomponent_get_colour_mapping_type);

	if (component)
	{
		type = component->colour_mapping_type;
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_spectrumcomponent_get_colour_mapping_type.  "
			"Invalid argument(s)");
		type = CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW;
	}
	LEAVE;

	return (type);
} /* cmzn_spectrumcomponent_get_colour_mapping_type */

int cmzn_spectrumcomponent_set_colour_mapping_type(struct cmzn_spectrumcomponent *component,
	enum cmzn_spectrumcomponent_colour_mapping_type type)
/*******************************************************************************
LAST MODIFIED : 14 July 1998

DESCRIPTION :
Sets the colour mapping of the cmzn_spectrumcomponent <component>.
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_spectrumcomponent_set_colour_mapping_type);

	if (component)
	{
		if (type != component->colour_mapping_type)
		{
			component->colour_mapping_type = type;
			cmzn_spectrumcomponent_changed(component);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_spectrumcomponent_set_colour_mapping_type.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_spectrumcomponent_set_colour_mapping_type */

double cmzn_spectrumcomponent_get_exaggeration(cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->exaggeration;
	}

	return 0.0;
}

int cmzn_spectrumcomponent_set_exaggeration(struct cmzn_spectrumcomponent *component,
	double value)
{
	if (component)
	{
		if (component->exaggeration != value)
		{
			component->exaggeration = value;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_spectrumcomponent_get_number_of_bands(cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->number_of_bands;
	}
	return 0;
}

int cmzn_spectrumcomponent_set_number_of_bands(cmzn_spectrumcomponent_id component,
	int number_of_bands)
{
	if (component)
	{
		if (component->number_of_bands != number_of_bands)
		{
			component->number_of_bands = number_of_bands;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

int cmzn_spectrumcomponent_get_black_band_proportion(struct cmzn_spectrumcomponent *component)
/*******************************************************************************
LAST MODIFIED : 31 July 1998

DESCRIPTION :
==============================================================================*/
{
	int proportion;

	ENTER(cmzn_spectrumcomponent_get_black_band_proportion);

	if (component)
	{
		proportion = component->black_band_proportion;
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_spectrumcomponent_get_black_band_proportion.  "
			"Invalid argument(s)");
		proportion = 0;
	}
	LEAVE;

	return (proportion);
} /* cmzn_spectrumcomponent_get_black_band_proportion */

int cmzn_spectrumcomponent_set_black_band_proportion(struct cmzn_spectrumcomponent *component,
	int proportion)
/*******************************************************************************
LAST MODIFIED : 31 July 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_spectrumcomponent_set_black_band_proportion);

	if (component)
	{
		if (proportion != component->black_band_proportion)
		{
			component->black_band_proportion = proportion;
			cmzn_spectrumcomponent_changed(component);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_spectrumcomponent_set_black_band_proportion.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_spectrumcomponent_set_black_band_proportion */

double cmzn_spectrumcomponent_get_step_value(cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->step_value;
	}
	return 0.0;
}

int cmzn_spectrumcomponent_set_step_value(struct cmzn_spectrumcomponent *component,
	double param1)
{
	if (component)
	{
		if (component->step_value != param1)
		{
			component->step_value = param1;
			if ( component->step_value <= component->minimum
				|| component->step_value >= component->maximum )
			{
				component->step_value = 0.5 * (component->maximum + component->minimum );
			}
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

double cmzn_spectrumcomponent_get_range_minimum(
	cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->minimum;
	}

	return 0.0;
}

int cmzn_spectrumcomponent_set_range_minimum(cmzn_spectrumcomponent_id component,
	double value)
{
	if (component)
	{
		if (value != component->minimum)
		{
			component->minimum = value;
			if ( component->step_value <= component->minimum
				|| component->step_value >= component->maximum )
			{
				component->step_value = 0.5 * (component->maximum + component->minimum );
			}
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
} /* cmzn_spectrumcomponent_set_range_minimum */

double cmzn_spectrumcomponent_get_range_maximum(
	cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->maximum;
	}

	return 0.0;
}

int cmzn_spectrumcomponent_set_range_maximum(cmzn_spectrumcomponent_id component,
	double value)
{
	if (component)
	{
		if (value != component->maximum)
		{
			component->maximum = value;
			if ( component->step_value <= component->minimum
				|| component->step_value >= component->maximum )
			{
				component->step_value = 0.5 * (component->maximum + component->minimum );
			}
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_spectrumcomponent_is_extend_above(cmzn_spectrumcomponent_id component)
{
	if (component)
		return component->extend_above;
	return false;
}

int cmzn_spectrumcomponent_set_extend_above(cmzn_spectrumcomponent_id component,
	bool extend_above)
{
	if (component)
	{
		if (component->extend_above != extend_above)
		{
			component->extend_above = extend_above;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_spectrumcomponent_is_extend_below(cmzn_spectrumcomponent_id component)
{
	if (component)
		return component->extend_below;
	return false;
}

int cmzn_spectrumcomponent_set_extend_below(cmzn_spectrumcomponent_id component,
	bool extend_below)
{
	if (component)
	{
		if (component->extend_below != extend_below)
		{
			component->extend_below = extend_below;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_spectrumcomponent_is_fix_minimum(struct cmzn_spectrumcomponent *component)
{
	if (component)
		return component->fix_minimum;
	return false;
}

int cmzn_spectrumcomponent_set_fix_minimum(struct cmzn_spectrumcomponent *component,
	bool fix_minimum)
{
	if (component)
	{
		if (fix_minimum != component->fix_minimum)
		{
			component->fix_minimum = fix_minimum;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_spectrumcomponent_is_fix_maximum(struct cmzn_spectrumcomponent *component)
{
	if (component)
		return component->fix_maximum;
	return false;
}

int cmzn_spectrumcomponent_set_fix_maximum(struct cmzn_spectrumcomponent *component,
	bool fix_maximum)
{
	if (component)
	{
		if (fix_maximum != component->fix_maximum)
		{
			component->fix_maximum = fix_maximum;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_spectrumcomponent_get_colour_minimum(cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->min_value;
	}

	return 0.0;
}

int cmzn_spectrumcomponent_set_colour_minimum(
	cmzn_spectrumcomponent_id component, double value)
{
	if (component && value <= 1.0 && value >= 0.0)
	{
		if (value != component->min_value)
		{
			component->min_value = value;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

double cmzn_spectrumcomponent_get_colour_maximum(
	cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		return component->max_value;
	}

	return 0.0;
}

int cmzn_spectrumcomponent_set_colour_maximum(
	cmzn_spectrumcomponent_id component, double value)
{
	if (component && value <= 1.0 && value >= 0.0)
	{
		if (value != component->max_value)
		{
			component->max_value = value;
			cmzn_spectrumcomponent_changed(component);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_spectrumcomponent_clear_changed(
	struct cmzn_spectrumcomponent *component,void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 10 March 1998

DESCRIPTION :
Iterator function to set component->changed to 0 (unchanged).
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_spectrumcomponent_clear_changed);
	USE_PARAMETER(dummy_void);
	if (component)
	{
		component->changed=0;
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_spectrumcomponent_clear_changed.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_spectrumcomponent_clear_changed */

int cmzn_spectrumcomponent_expand_maximum_component_index(
	struct cmzn_spectrumcomponent *component,void *component_index_void)
/*******************************************************************************
LAST MODIFIED : 27 September 2006

DESCRIPTION :
Iterator function to expand the integer stored at <component_index_void>
by the component numbers of each component so we can work out the maximum
component number used.  The first component_index is 0, so this means 1 component.
==============================================================================*/
{
	int *component_index, return_code;

	ENTER(cmzn_spectrumcomponent_expand_maximum_component_index);
	if (component && (component_index = (int *)component_index_void))
	{
		if (component->component_scale == CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_INVALID &&
			component->is_field_lookup)
		{
			int number_of_input_components =
				cmzn_field_get_number_of_components(component->input_field);
			if (*component_index < number_of_input_components - 1)
			{
				/* The maximum index is 1 less than the number of components */
				*component_index = number_of_input_components - 1;
			}
		}
		else
		{
			if (*component_index < component->component_number)
			{
				*component_index = component->component_number;
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_spectrumcomponent_expand_maximum_component_index.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_spectrumcomponent_expand_maximum_component_index */

int cmzn_spectrumcomponent_get_colour_components(
	struct cmzn_spectrumcomponent *component, void *colour_components_void)
/*******************************************************************************
LAST MODIFIED : 4 October 2006

DESCRIPTION :
Iterator function to accumulate the colour_components by setting bits
in the value pointed to by <colour_components_void>.
==============================================================================*/
{
	enum Spectrum_colour_components *colour_components;
	int done, return_code;

	if (component && (colour_components =
			(enum Spectrum_colour_components *)colour_components_void))
	{
		done = 0;
		if (component->component_scale == CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_INVALID &&
			component->is_field_lookup)
		{
			int number_of_components = cmzn_field_get_number_of_components
					(component->output_field);
			if (2 == number_of_components)
			{
				(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
					| SPECTRUM_COMPONENT_RED
					| SPECTRUM_COMPONENT_GREEN
					| SPECTRUM_COMPONENT_BLUE
					| SPECTRUM_COMPONENT_ALPHA);
				done = 1;
			}
			else if (3 == number_of_components)
			{
				(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
					| SPECTRUM_COMPONENT_RED
					| SPECTRUM_COMPONENT_GREEN
					| SPECTRUM_COMPONENT_BLUE);
				done = 1;
			}
			else if (4 <= number_of_components)
			{
				(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
					| SPECTRUM_COMPONENT_RED
					| SPECTRUM_COMPONENT_GREEN
					| SPECTRUM_COMPONENT_BLUE
					| SPECTRUM_COMPONENT_ALPHA);
				done = 1;
			}
		}
		if (!done)
		{
			switch (component->colour_mapping_type)
			{
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_BLUE:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_RED:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_GREEN:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_STEP:
				{
				(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
					| SPECTRUM_COMPONENT_RED
					| SPECTRUM_COMPONENT_GREEN
					| SPECTRUM_COMPONENT_BLUE);
				} break;
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RED:
				{
					(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
						| SPECTRUM_COMPONENT_RED);
				} break;
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_GREEN:
				{
					(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
						| SPECTRUM_COMPONENT_GREEN);
				} break;
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BLUE:
				{
					(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
						| SPECTRUM_COMPONENT_BLUE);
				} break;
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_ALPHA:
				{
					(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
						| SPECTRUM_COMPONENT_ALPHA);
				} break;
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_MONOCHROME:
				{
					(*colour_components) = static_cast<Spectrum_colour_components>((*colour_components)
						| SPECTRUM_COMPONENT_MONOCHROME);
				} break;
				default:
				{
				} break;
			}
		}
		return_code=1;
	}
	else
	{
		return_code=0;
	}

	return (return_code);
}

int cmzn_spectrumcomponent_enable(struct cmzn_spectrumcomponent *component,
	void *render_data_void)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
==============================================================================*/
{
#if defined (OPENGL_API)
	unsigned char pixels[3*1024];
	int high_flag,low_flag,texel,texels_in_band,texels_per_band,i;
#endif /* defined (OPENGL_API) */
#if defined (DEBUG_CODE)
	int error = 0;
#endif /* defined (DEBUG_CODE) */
	int return_code;

	ENTER(cmzn_spectrumcomponent_enable);
	if (component&&render_data_void)
	{
		return_code=1;
		if (component->active)
		{
			switch (component->colour_mapping_type)
			{
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RED:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_GREEN:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_MONOCHROME:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BLUE:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_BLUE:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_RED:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_GREEN:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_ALPHA:
				{
					/* Do nothing but valid. */
				} break;
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED:
				{
#if defined (OPENGL_API)
					if ((component->black_band_proportion)&&(component->number_of_bands))
					{
						if ((component->black_band_proportion)%(component->number_of_bands))
						{
#if defined (DEBUG_CODE)
							printf("  proportion %d number %d >>",
								component->black_band_proportion,component->number_of_bands);
#endif /* defined (DEBUG_CODE) */
							component->black_band_proportion += component->number_of_bands-
								component->black_band_proportion%component->number_of_bands;
#if defined (DEBUG_CODE)
							printf("%d\n",component->black_band_proportion);
#endif /* defined (DEBUG_CODE) */
						}
						texels_in_band=(component->black_band_proportion)/
							(component->number_of_bands);
						texels_per_band=1021/(component->number_of_bands);

						/* the first and last texel are white to allow transparency
							outside the band range */
						i=0;
						pixels[i]=255;
						i++;
						pixels[i]=255;
						i++;
						pixels[i]=255;
						i++;
						while (i<3*1023)
						{
							texel = (i-3)/3+(texels_in_band/2);
							if (component->reverse)
							{
								low_flag = component->extend_above;
								high_flag = component->extend_below;
							}
							else
							{
								low_flag = component->extend_below;
								high_flag = component->extend_above;
							}
							if ((low_flag || texel>texels_in_band) && (high_flag || texel<1020) &&
								((texel%texels_per_band)<texels_in_band))
							{
#if defined (DEBUG_CODE)
								printf("  band pixel %d texel %d\n",i/3, texel);
#endif /* defined (DEBUG_CODE) */
								pixels[i]=0;
								i++;
								pixels[i]=0;
								i++;
								pixels[i]=0;
								i++;
							}
							else
							{
								pixels[i]=255;
								i++;
								pixels[i]=255;
								i++;
								pixels[i]=255;
								i++;
							}
						}
						pixels[i]=255;
						i++;
						pixels[i]=255;
						i++;
						pixels[i]=255;
						i++;

#if defined (DEBUG_CODE)
						while (0 != (error=glGetError()))
						{
							printf("GL ERROR 0: %s\n",gluErrorString(error));
						}
#endif /* defined (DEBUG_CODE) */
						if (!component->texture_id)
						{
							glGenTextures(1, &(component->texture_id));
						}
						if (component->changed)
						{
							glBindTexture(GL_TEXTURE_1D, component->texture_id);
							glTexImage1D(GL_TEXTURE_1D,0,3,1024,0,GL_RGB,GL_UNSIGNED_BYTE,
								pixels);
							glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
							glTexParameterf(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP);
							glTexParameterf(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
							glTexParameterf(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
						}
						else
						{
							glBindTexture(GL_TEXTURE_1D, component->texture_id);
						}
						glEnable(GL_TEXTURE_1D);
#if defined (DEBUG_CODE)
						while (0 != (error=glGetError()))
						{
							printf("GL ERROR 1: %s\n",gluErrorString(error));
						}
#endif /* defined (DEBUG_CODE) */
					}
					else
					{
						if (!component->number_of_bands)
						{
							display_message(ERROR_MESSAGE,
								"cmzn_spectrumcomponent_enable.  Invalid number_of_bands");
							return_code=0;
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"cmzn_spectrumcomponent_enable.  Invalid band_proportion");
							return_code=0;
						}
					}
#endif /* defined (OPENGL_API) */
				} break;
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_STEP:
				{
#if defined (OPENGL_API)
					pixels[0]=255;
					pixels[1]=0;
					pixels[2]=0;
					pixels[3]=0;
					pixels[4]=255;
					pixels[5]=0;
					/* set up a texture */
#if defined (DEBUG_CODE)
					while (0 != (error=glGetError()))
					{
						printf("GL ERROR 0: %s\n",gluErrorString(error));
					}
#endif /* defined (DEBUG_CODE) */
					if (!component->texture_id)
					{
						glGenTextures(1, &(component->texture_id));
					}
					if (component->changed)
					{
						glBindTexture(GL_TEXTURE_1D, component->texture_id);
						glTexImage1D(GL_TEXTURE_1D,0,3,2,0,GL_RGB,GL_UNSIGNED_BYTE,pixels);
						glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
						glTexParameterf(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP);
						glTexParameterf(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
						glTexParameterf(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
					}
					else
					{
						glBindTexture(GL_TEXTURE_1D, component->texture_id);
					}
					glEnable(GL_TEXTURE_1D);
#if defined (DEBUG_CODE)
					while (0 != (error=glGetError()))
					{
						printf("GL ERROR 1: %s\n",gluErrorString(error));
					}
#endif /* defined (DEBUG_CODE) */
#endif /* defined (OPENGL_API) */
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"cmzn_spectrumcomponent_enable.  Unknown colour mapping");
					return_code=0;
				} break;
			}
		}
		component->changed = 0;
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_spectrumcomponent_enable.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_spectrumcomponent_enable */

int cmzn_spectrumcomponent_activate(struct cmzn_spectrumcomponent *component,
	void *render_data_void)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
Modifies the material in the render data to represent the data value
passed in render data.
==============================================================================*/
{
	FE_value *values;
	GLfloat value = 0.0;
	int i, number_of_components, return_code,texels_per_band;
	ZnReal data_component,step_xi,total_texels;
	struct Spectrum_render_data *render_data;

	ENTER(cmzn_spectrumcomponent_activate);
	if (component&&(render_data=(struct Spectrum_render_data *)render_data_void))
	{
		data_component = 0.0;
		return_code=1;
		if (component->component_scale == CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_INVALID &&
			component->is_field_lookup)
		{
			if (component->active)
			{
				cmzn_fieldmodule_id field_module;
				cmzn_fieldcache_id field_cache;
				// GRC probably inefficient to create and destroy cache here; keep it through render pass?
				field_module = cmzn_field_get_fieldmodule(component->output_field);
				field_cache = cmzn_fieldmodule_create_fieldcache(field_module);
				number_of_components = cmzn_field_get_number_of_components
					(component->output_field);
				ALLOCATE(values, FE_value, number_of_components);
				if (component->component_number > 0)
				{
					FE_value dataValue[1];
					GLfloat* tmpPointer;
					tmpPointer = render_data->data + component->component_number;
					CAST_TO_FE_VALUE_C(dataValue,tmpPointer,1);
					cmzn_fieldcache_set_field_real(field_cache, component->input_field, /*number_of_values*/1, dataValue);
				}
				else
				{
					FE_value *feData;
					ALLOCATE(feData, FE_value,render_data->number_of_data_components);
					CAST_TO_FE_VALUE_C(feData,render_data->data,
						render_data->number_of_data_components);
					cmzn_fieldcache_set_field_real(field_cache, component->input_field, render_data->number_of_data_components, feData);
					DEALLOCATE(feData);
				}
				cmzn_field_evaluate_real(component->output_field, field_cache, number_of_components, values);
				cmzn_fieldcache_destroy(&field_cache);
				cmzn_fieldmodule_destroy(&field_module);
				for (i = 0 ; i < number_of_components ; i++)
				{
					/* ensure 0 - 1 */
					if (values[i] > 1.0)
					{
						values[i] = 1.0;
					}
					if (values[i] < 0.0)
					{
						values[i] = 0.0;
					}
				}
				if (1 == number_of_components)
				{
					value = values[0];
					switch (component->colour_mapping_type)
					{
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_ALPHA:
						{
							render_data->rgba[3] = value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW:
							{
								if (value<1.0/3.0)
								{
									render_data->rgba[0]=1.0;
									render_data->rgba[2]=0.0;
									if (value<1.0/6.0)
									{
										render_data->rgba[1]=value*4.5;
									}
									else
									{
										render_data->rgba[1]=0.75+(value-1.0/6.0)*1.5;
									}
								}
								else if (value<2.0/3.0)
								{
									render_data->rgba[1]=1.0;
									if (value<0.5)
									{
										render_data->rgba[0] = 2.5 - 4.5*value;
										render_data->rgba[2] = 1.5*value - 0.5;
									}
									else
									{
										render_data->rgba[0] = 1.0 - 1.5*value;
										render_data->rgba[2] = -2.0 + 4.5*value;
									}
								}
								else
								{
									render_data->rgba[0]=0.0;
									render_data->rgba[2]=1.0;
									if (value<5.0/6.0)
									{
										render_data->rgba[1]=1.0-(value-2.0/3.0)*1.5;
									}
									else
									{
										render_data->rgba[1]=0.75-(value-5.0/6.0)*4.5;
									}
								}
							} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RED:
							{
								render_data->rgba[0]=value;
							} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_GREEN:
							{
								render_data->rgba[1]=value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BLUE:
						{
							render_data->rgba[2]=value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_MONOCHROME:
						{
							render_data->rgba[0]=value;
							render_data->rgba[1]=value;
							render_data->rgba[2]=value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_BLUE:
						{
							render_data->rgba[2]=1.0;
							render_data->rgba[0]=(1-value);
							render_data->rgba[1]=(1-value);
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_RED:
						{
							render_data->rgba[0]=1;
							render_data->rgba[2]=(1-value);
							render_data->rgba[1]=(1-value);
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_GREEN:
						{
							render_data->rgba[1]=1.0;
							render_data->rgba[0]=(1-value);
							render_data->rgba[2]=(1-value);
						} break;
						default:
						{
						} break;
					}
				}
				else if (2 == number_of_components)
				{
					render_data->rgba[0]=values[0];
					render_data->rgba[1]=values[0];
					render_data->rgba[2]=values[0];
					render_data->rgba[3]=values[1];
				}
				else if (3 == number_of_components)
				{
					render_data->rgba[0]=values[0];
					render_data->rgba[1]=values[1];
					render_data->rgba[2]=values[2];
				}
				else if (4 <= number_of_components)
				{
					render_data->rgba[0]=values[0];
					render_data->rgba[1]=values[1];
					render_data->rgba[2]=values[2];
					render_data->rgba[3]=values[3];
				}

				DEALLOCATE(values);
			}
		}
		else
		{
			/* Ignore inactive component or component which act on a component for which
				there is no data */
			if (component->active &&
				(component->component_number < render_data->number_of_data_components))
			{
				data_component = render_data->data[component->component_number];
				/* Always set a value for texture_coordinate based spectrums */
				if ((CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED==component->colour_mapping_type)
					|| (CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_STEP==component->colour_mapping_type)
					|| (((data_component>=component->minimum)||component->extend_below)&&
						((data_component<=component->maximum)||component->extend_above)))
				{
					/* first get value (normalised 0 to 1) from type */
					if (component->maximum != component->minimum)
					{
						switch (component->component_scale)
						{
							case CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_LINEAR:
							{
								value=(data_component-component->minimum)/
									(component->maximum-component->minimum);
							} break;
							case CMZN_SPECTRUMCOMPONENT_SCALE_TYPE_LOG:
							{
								if (component->exaggeration<0)
								{
									value=1.0-log(1-component->exaggeration*
										(component->maximum-data_component)/
										(component->maximum-component->minimum))/
										log(1-component->exaggeration);
								}
								else
								{
									value=log(1+component->exaggeration*
										(data_component-component->minimum)/
										(component->maximum-component->minimum))/
										log(1+component->exaggeration);
								}
							} break;
							default:
							{
								display_message(ERROR_MESSAGE,
									"cmzn_spectrumcomponent_activate.  Unknown type");
								return_code=0;
							} break;
						}
						/* ensure 0 - 1 */
						if (value>1.0)
						{
							value=1.0;
						}
						if (value<0.0)
						{
							value=0.0;
						}
					}
					else
					{
						if (data_component <= component->minimum)
						{
							value = 0.0;
						}
						else
						{
							value = 1.0;
						}
					}
					/* reverse the direction if necessary */
					if (component->reverse)
					{
						value=1.0-value;
					}
					/* apply the value minimums and maximums */
					if (component->colour_mapping_type != CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED)
						value=component->min_value+(component->max_value-component->min_value)*value;
					switch (component->colour_mapping_type)
					{
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED:
						{
							if ((component->number_of_bands)&&(component->black_band_proportion))
							{
								texels_per_band=1021/(component->number_of_bands);
								total_texels=(ZnReal)(texels_per_band*component->number_of_bands);
								if ((component->black_band_proportion/component->number_of_bands)%2)
								{
									value=((value*total_texels+1.5)/1024.0);
								}
								else
								{
									value=((value*total_texels+1.0)/1024.0);
								}
#if defined (OPENGL_API)
								glTexCoord1f(value);
#endif /* defined (OPENGL_API) */
							}
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_STEP:
						{
							step_xi=(component->step_value-component->minimum)/
								(component->maximum-component->minimum);
							if ((0.0==step_xi)||(1.0==step_xi))
							{
								step_xi=0.5;
							}
							if (component->reverse)
							{
								step_xi=1.0-step_xi;
							}
							value=0.5*value*(1.0-value)/(step_xi*(1.0-step_xi))+
								value*(value-step_xi)/(1.0-step_xi);
#if defined (OPENGL_API)
							glTexCoord1f(value);
#endif /* defined (OPENGL_API) */
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_ALPHA:
						{
							render_data->rgba[3] = value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW:
						{
							if (value<1.0/3.0)
							{
								render_data->rgba[0]=1.0;
								render_data->rgba[2]=0.0;
								if (value<1.0/6.0)
								{
									render_data->rgba[1]=value*4.5;
								}
								else
								{
									render_data->rgba[1]=0.75+(value-1.0/6.0)*1.5;
								}
							}
							else if (value<2.0/3.0)
							{
								render_data->rgba[1]=1.0;
								if (value<0.5)
								{
									render_data->rgba[0] = 2.5 - 4.5*value;
									render_data->rgba[2] = 1.5*value - 0.5;
								}
								else
								{
									render_data->rgba[0] = 1.0 - 1.5*value;
									render_data->rgba[2] = -2.0 + 4.5*value;
								}
							}
							else
							{
								render_data->rgba[0]=0.0;
								render_data->rgba[2]=1.0;
								if (value<5.0/6.0)
								{
									render_data->rgba[1]=1.0-(value-2.0/3.0)*1.5;
								}
								else
								{
									render_data->rgba[1]=0.75-(value-5.0/6.0)*4.5;
								}
							}
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RED:
						{
							render_data->rgba[0]=value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_GREEN:
						{
							render_data->rgba[1]=value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BLUE:
						{
							render_data->rgba[2]=value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_MONOCHROME:
						{
							render_data->rgba[0]=value;
							render_data->rgba[1]=value;
							render_data->rgba[2]=value;
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_BLUE:
						{
							render_data->rgba[2]=1.0;
							render_data->rgba[0]=(1-value);
							render_data->rgba[1]=(1-value);
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_RED:
						{
							render_data->rgba[0]=1.0;
							render_data->rgba[2]=(1-value);
							render_data->rgba[1]=(1-value);
						} break;
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_GREEN:
						{
							render_data->rgba[1]=1.0;
							render_data->rgba[0]=(1-value);
							render_data->rgba[2]=(1-value);
						} break;
						default:
						{
						} break;
					}
				}
				else
				{
					switch (component->colour_mapping_type)
					{
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED:
						case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_STEP:
						{
							/* the values are large so they quickly transition to the last
								texel */
							if (data_component>component->maximum)
							{
								value=1000.0;
							}
							else
							{
								value= -999.0;
							}
#if defined (OPENGL_API)
							glTexCoord1f(value);
#endif /* defined (OPENGL_API) */
						} break;
						default:
						{
						} break;
					}
				}
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_spectrumcomponent_activate.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_spectrumcomponent_activate */

int cmzn_spectrumcomponent_disable(struct cmzn_spectrumcomponent *component,
	void *render_data_void)
/*******************************************************************************
LAST MODIFIED : 15 October 1998

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Spectrum_render_data *render_data;

	ENTER(cmzn_spectrumcomponent_disable);
	if (component&&(render_data=(struct Spectrum_render_data *)render_data_void))
	{
		USE_PARAMETER(render_data);
		return_code=1;
		if (component->active)
		{
			switch (component->colour_mapping_type)
			{
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RAINBOW:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_RED:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_GREEN:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_MONOCHROME:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BLUE:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_ALPHA:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_BLUE:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_RED:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_WHITE_TO_GREEN:
				{
					/* do nothing */
				} break;
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_BANDED:
				case CMZN_SPECTRUMCOMPONENT_COLOUR_MAPPING_TYPE_STEP:
				{
#if defined (OPENGL_API)
					glDisable(GL_TEXTURE_1D);
#endif /* defined (OPENGL_API) */
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"cmzn_spectrumcomponent_disable.  Unknown type");
					return_code = 0;
				} break;
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"cmzn_spectrumcomponent_disable.  "
			"Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_spectrumcomponent_disable */

cmzn_spectrumcomponent_id cmzn_spectrumcomponent_access(
	cmzn_spectrumcomponent_id component)
{
	if (component)
		return (ACCESS(cmzn_spectrumcomponent)(component));
	return 0;
}

int cmzn_spectrumcomponent_destroy(cmzn_spectrumcomponent_id *component_address)
{
	if (component_address)
	{
		DEACCESS(cmzn_spectrumcomponent)(component_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

double cmzn_spectrumcomponent_get_banded_ratio(cmzn_spectrumcomponent_id component)
{
	if (component)
	{
		int proportion = cmzn_spectrumcomponent_get_black_band_proportion(component);
		return ((double) proportion / (double)1000.0);
	}
	return 0.0;
}

int cmzn_spectrumcomponent_set_banded_ratio(cmzn_spectrumcomponent_id component,
	double value)
{
	if (component)
	{
		if (((value > 0.0) || (value <= 1.0)))
		{
			return cmzn_spectrumcomponent_set_black_band_proportion(component,
				(int)(value * 1000.0+0.5));
		}
	}
	return CMZN_ERROR_ARGUMENT;
}
