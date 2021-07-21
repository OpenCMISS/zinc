/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(CAD_FIELD_LOCATION_HPP)
#define CAD_FIELD_LOCATION_HPP

#include "api/cmiss_field_cad.h"
#include "computed_field/field_location.hpp"
#include "cad/computed_field_cad_topology.h"
#include "cad/element_identifier.h"

class Field_cad_geometry_location : public Field_location
{
protected:
	cmzn_field_cad_topology_id id;

public:
	Field_cad_geometry_location(cmzn_field_cad_topology_id id, FE_value time = 0.0, int number_of_derivatives = 0);
	~Field_cad_geometry_location() = 0;

	inline cmzn_field_cad_topology_id get_id() const {return id;}
	

};

class Field_cad_geometry_surface_location : public Field_cad_geometry_location
{
protected:
	cmzn_cad_surface_identifier identifier;
	double u;
	double v;

public:
	Field_cad_geometry_surface_location(cmzn_field_cad_topology_id id, cmzn_cad_surface_identifier identifier, double u, double v, FE_value time = 0.0, int number_of_derivatives = 0);
	~Field_cad_geometry_surface_location();

   virtual Field_location *clone()
   {
   	return new Field_cad_geometry_surface_location(id, identifier, u, v, time);
   }

	inline cmzn_cad_surface_identifier get_identifier() const {return identifier;}
	inline double get_u() const {return u;}
	inline double get_v() const {return v;}

};

class Field_cad_geometry_curve_location : public Field_cad_geometry_location
{
protected:
	cmzn_cad_curve_identifier identifier;
	double s;

public:
	Field_cad_geometry_curve_location(cmzn_field_cad_topology_id id, cmzn_cad_curve_identifier identifier, double s, FE_value time = 0.0, int number_of_derivatives = 0);
	~Field_cad_geometry_curve_location();

   virtual Field_location *clone()
   {
   	return new Field_cad_geometry_curve_location(id, identifier, s, time);
   }

	inline cmzn_cad_curve_identifier get_identifier() const {return identifier;}
	inline double get_s() const {return s;}
};

#endif // !defined(CAD_FIELD_LOCATION_HPP)
