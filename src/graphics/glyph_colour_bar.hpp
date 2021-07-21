/**
 * glyph_colour_bar.hpp
 *
 * Derived glyph type which creates a spectrum colour_bar which automatically
 * updates when the spectrum changes.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GLYPH_COLOUR_BAR_HPP
#define GLYPH_COLOUR_BAR_HPP

#include "opencmiss/zinc/material.h"
#include "graphics/glyph.hpp"

struct cmzn_spectrum;

struct cmzn_glyph_colour_bar : public cmzn_glyph
{
private:
	cmzn_spectrum *spectrum;
	GT_object *graphicsObject;
	double axis[3], centre[3], sideAxis[3];
	double extendLength;
	cmzn_material *labelMaterial;
	char *numberFormat;
	int labelDivisions;
	double tickLength;

	cmzn_glyph_colour_bar(cmzn_spectrum *spectrumIn);
	virtual ~cmzn_glyph_colour_bar();

	void invalidate()
	{
		if (this->graphicsObject)
		{
			DEACCESS(GT_object)(&this->graphicsObject);
		}
		this->changed();
	}

public:
	static cmzn_glyph_colour_bar *create(cmzn_spectrum *spectrumIn)
	{
		if (spectrumIn)
			return new cmzn_glyph_colour_bar(spectrumIn);
		return 0;
	}

	virtual GT_object *getGraphicsObject(cmzn_tessellation *tessellation,
		cmzn_material *material, cmzn_font *font);

	virtual bool usesFont()
	{
		return true;
	}

	int getAxis(int valuesCount, double *valuesOut) const;

	int setAxis(int valuesCount, const double *valuesIn);

	int getCentre(int valuesCount, double *valuesOut) const;

	int setCentre(int valuesCount, const double *valuesIn);

	double getExtendLength() const
	{
		return this->extendLength;
	}

	int setExtendLength(double extendLengthIn);

	int getLabelDivisions() const
	{
		return this->labelDivisions;
	}

	int setLabelDivisions(int labelDivisionsIn);

	cmzn_material_id getLabelMaterial() const
	{
		return labelMaterial ? cmzn_material_access(labelMaterial) : 0;
	}

	int setLabelMaterial(cmzn_material_id material);

	char *getNumberFormat() const;

	int setNumberFormat(const char *numberFormatIn);

	int getSideAxis(int valuesCount, double *valuesOut) const;

	int setSideAxis(int valuesCount, const double *valuesIn);

	cmzn_spectrum *getSpectrum()
	{
		return this->spectrum;
	}

	double getTickLength() const
	{
		return this->tickLength;
	}

	int setTickLength(double tickLengthIn);

	virtual void fontChange();

	virtual void materialChange(struct MANAGER_MESSAGE(cmzn_material) *message);

	virtual void spectrumChange(struct MANAGER_MESSAGE(cmzn_spectrum) *message);

};

#endif // GLYPH_COLOUR_BAR_HPP
