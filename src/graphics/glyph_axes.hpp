/**
 * glyph_axes.hpp
 *
 * Derived glyph type which creates 2 or 3 dimensional axes with optional
 * per axis labels and materials.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GLYPH_AXES_HPP
#define GLYPH_AXES_HPP

#include "opencmiss/zinc/material.h"
#include "graphics/glyph.hpp"

struct cmzn_glyph_axes : public cmzn_glyph
{
private:
	cmzn_glyph *axisGlyph;
	double axisWidth;
	char *axisLabels[3];
	cmzn_material *axisMaterials[3];
	GT_object *graphicsObject;

	cmzn_glyph_axes(cmzn_glyph *axisGlyphIn, double axisWidthIn);
	virtual ~cmzn_glyph_axes();
	void invalidate();

public:
	static cmzn_glyph_axes *create(cmzn_glyph *axisGlyphIn, double axisWidthIn)
	{
		if (axisGlyphIn && (axisWidthIn >= 0.0))
			return new cmzn_glyph_axes(axisGlyphIn, axisWidthIn);
		return 0;
	}

	virtual GT_object *getGraphicsObject(cmzn_tessellation *tessellation,
		cmzn_material *material, cmzn_font *font);

	virtual bool usesFont()
	{
		for (int i = 0; i < 3; ++i)
		{
			if (axisLabels[i])
				return true;
		}
		return false;
	}

	double getAxisWidth() const
	{
		return this->axisWidth;
	}

	int setAxisWidth(double axisWidthIn);

	/** @return  Allocated string or 0 */
	char *getAxisLabel(int axisNumber);

	int setAxisLabel(int axisNumber, const char *label);

	/** @return  ACCESSed material or 0 */
	cmzn_material *getAxisMaterial(int axisNumber);

	int setAxisMaterial(int axisNumber, cmzn_material *material);

	virtual void fontChange();

	virtual void materialChange(struct MANAGER_MESSAGE(cmzn_material) *message);

	virtual bool usesCircleDivisions()
	{
		return this->axisGlyph->usesCircleDivisions();
	}

};

#endif // GLYPH_AXES_HPP
