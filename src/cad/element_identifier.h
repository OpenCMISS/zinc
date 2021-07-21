/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(ELEMENT_IDENTIFIER_H)
#define ELEMENT_IDENTIFIER_H

#include <map>

#include <Quantity_Color.hxx>

#include "api/cmiss_field_cad.h"
#include "cad/cad_element.h"

struct cmzn_cad_identifier
{
	cmzn_field_cad_topology_id cad_topology;
	Cad_primitive_identifier identifier;

	cmzn_cad_identifier(cmzn_field_cad_topology_id cad_topology, Cad_primitive_identifier identifier);
	~cmzn_cad_identifier();

	cmzn_cad_identifier(const cmzn_cad_identifier& cad_identifier);
	cmzn_cad_identifier& operator=(const cmzn_cad_identifier& source);
	bool operator==(const cmzn_cad_identifier& other) const;
	bool operator!=(const cmzn_cad_identifier& other) const;
};

struct Cad_topology_primitive_identifier
{
private:
	int surface_index;
	int curve_index;
	int point_index;

public:
	Cad_topology_primitive_identifier(int surface_index = -1, int curve_index = -1, int point_index = -1)
		: surface_index(surface_index)
		, curve_index(curve_index)
		, point_index(point_index)
	{
	}

	~Cad_topology_primitive_identifier()
	{
	}

	inline int getSurfaceIndex() const {return surface_index;}
	inline int getCurveIndex() const {return curve_index;}
	inline int getPointIndex() const {return point_index;}
};

class Cad_topology_primitive_identifier_compare
{
public:
	bool operator() (const Cad_topology_primitive_identifier &id1, const Cad_topology_primitive_identifier &id2) const
	{
		if (id1.getSurfaceIndex() < id2.getSurfaceIndex())
			return true;
		else if (id1.getSurfaceIndex() == id2.getSurfaceIndex())
		{
			if (id1.getCurveIndex() < id2.getCurveIndex())
				return true;
			else if (id1.getCurveIndex() == id2.getCurveIndex())
			{
				if (id1.getPointIndex() < id2.getPointIndex())
					return true;
			}
		}
		return false;
	}
};



struct cmzn_cad_colour
{
	enum cmzn_cad_colour_type {
		CMZN_CAD_COLOUR_NOT_DEFINED = -1,
		CMZN_CAD_COLOUR_GENERIC = 0,
		CMZN_CAD_COLOUR_SURFACE = 1,
		CMZN_CAD_COLOUR_CURVE = 2
	};

private:
	cmzn_cad_colour_type colour_type;
	Quantity_Color colour;

public:
	cmzn_cad_colour(cmzn_cad_colour_type colour_type = CMZN_CAD_COLOUR_NOT_DEFINED
		, Quantity_Color colour = Quantity_NOC_WHITE)
		: colour_type(colour_type)
		, colour(colour)
	{
	}

	cmzn_cad_colour_type getColourType() const {return colour_type;}
	Quantity_Color getColour() const {return colour;}
};

typedef std::map<Cad_topology_primitive_identifier,cmzn_cad_colour, Cad_topology_primitive_identifier_compare> Cad_colour_map;
typedef std::map<Cad_topology_primitive_identifier,cmzn_cad_colour, Cad_topology_primitive_identifier_compare>::iterator Cad_colour_map_iterator;
typedef std::map<Cad_topology_primitive_identifier,cmzn_cad_colour, Cad_topology_primitive_identifier_compare>::const_iterator Cad_colour_map_const_iterator;


#endif
