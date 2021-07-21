/**
 * FILE : glyph_circular.hpp
 *
 * Internal header for glyphs with circular features which are drawn with
 * tessellation circle divisions.
 */
 /* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (GLYPH_CIRCULAR_HPP)
#define GLYPH_CIRCULAR_HPP

#include <vector>
#include "graphics/glyph.hpp"

struct cmzn_glyph_circular : public cmzn_glyph
{
private:

	struct GraphicsObjectDivisions
	{
		int circleDivisions;
		GT_object *graphicsObject;

	public:
		GraphicsObjectDivisions() :
			circleDivisions(0),
			graphicsObject(0)
		{
		}

		GraphicsObjectDivisions(int circleDivisionsIn, GT_object *graphicsObjectIn) :
			circleDivisions(circleDivisionsIn),
			graphicsObject(ACCESS(GT_object)(graphicsObjectIn))
		{
		}

		GraphicsObjectDivisions(const GraphicsObjectDivisions& source) :
			circleDivisions(source.circleDivisions),
			graphicsObject(ACCESS(GT_object)(source.graphicsObject))
		{
		}

		~GraphicsObjectDivisions()
		{
			DEACCESS(GT_object)(&graphicsObject);
		}

		GraphicsObjectDivisions& operator=(const GraphicsObjectDivisions& source)
		{
			this->circleDivisions = source.circleDivisions;
			REACCESS(GT_object)(&this->graphicsObject, source.graphicsObject);
			return *this;
		}
	};
	std::vector<GraphicsObjectDivisions> objects;

	// derived class (sphere, cone etc.) must implement, return accessed object for divisions
	virtual GT_object *createGraphicsObject(int circleDivisions) = 0;

protected:

	cmzn_glyph_circular()
	{
	}

	virtual ~cmzn_glyph_circular()
	{
	}

public:

	virtual bool usesCircleDivisions()
	{
		return true;
	}

	virtual GT_object *getGraphicsObject(cmzn_tessellation *, cmzn_material *, cmzn_font *);

};

struct cmzn_glyph_arrow_solid : public cmzn_glyph_circular
{
private:
	double headLength;
	double shaftThickness;

	cmzn_glyph_arrow_solid(double headLengthIn, double shaftThicknessIn) :
		headLength(headLengthIn),
		shaftThickness(shaftThicknessIn)
	{
	}

	virtual GT_object *createGraphicsObject(int circleDivisions);

public:
	static cmzn_glyph_arrow_solid* create(double headLengthIn, double shaftThicknessIn)
	{
		return new cmzn_glyph_arrow_solid(headLengthIn, shaftThicknessIn);
	}
};

struct cmzn_glyph_cone : public cmzn_glyph_circular
{
private:
	virtual GT_object *createGraphicsObject(int circleDivisions);

public:
	static cmzn_glyph_cone* create()
	{
		return new cmzn_glyph_cone();
	}
};

struct cmzn_glyph_cone_solid : public cmzn_glyph_circular
{
private:
	virtual GT_object *createGraphicsObject(int circleDivisions);

public:
	static cmzn_glyph_cone_solid* create()
	{
		return new cmzn_glyph_cone_solid();
	}
};

struct cmzn_glyph_cylinder : public cmzn_glyph_circular
{
private:
	virtual GT_object *createGraphicsObject(int circleDivisions);

public:
	static cmzn_glyph_cylinder* create()
	{
		return new cmzn_glyph_cylinder();
	}
};

struct cmzn_glyph_cylinder_solid : public cmzn_glyph_circular
{
private:
	virtual GT_object *createGraphicsObject(int circleDivisions);

public:
	static cmzn_glyph_cylinder_solid* create()
	{
		return new cmzn_glyph_cylinder_solid();
	}
};

struct cmzn_glyph_sphere : public cmzn_glyph_circular
{
private:
	virtual GT_object *createGraphicsObject(int circleDivisions);

public:
	static cmzn_glyph_sphere* create()
	{
		return new cmzn_glyph_sphere();
	}
};

/**
 * Creates a graphics object named <name> resembling an arrow made from a cone on
 * a cylinder. The base of the arrow is at (0,0,0) while its head lies at (1,0,0).
 * The radius of the cone is <cone_radius>. The cylinder is <shaft_length> long
 * with its radius given by <shaft_radius>. The ends of the arrow and the cone
 * are both closed.  Primary axis is either 1,2 or 3 and indicates the direction
 * the arrow points in.
 */
struct GT_object *create_GT_object_arrow_solid(const char *name, int primary_axis,
	int number_of_segments_around,ZnReal shaft_length,ZnReal shaft_radius,
	ZnReal cone_radius);

/**
 * Creates a graphics object named <name> resembling a sphere with the given
 * <number_of_segments_around> and <number_of_segments_down> from pole to pole.
 * The sphere is centred at (0,0,0) and its poles are on the (1,0,0) line. It
 * fits into the unit cube spanning from -0.5 to +0.5 across all axes. Parameter
 * <number_of_segments_around> should normally be an even number at least 6 and
 * twice <number_of_segments_down> look remotely spherical.
 */
struct GT_object *create_GT_object_sphere(const char *name,int number_of_segments_around,
	int number_of_segments_down);

#endif /* !defined (GLYPH_CIRCULAR_HPP) */
