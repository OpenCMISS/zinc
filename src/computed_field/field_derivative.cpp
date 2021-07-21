/**
 * FILE : field_derivative.cpp
 *
 * Field derivative defining order and type of derivative operator to apply
 * to any given field. Each FieldDerivative has a unique index within its
 * owning region for efficient look up in field cache. Object describes how to
 * evaluate derivative, including links to next lower FieldDerivative so
 * can evaluate downstream derivatives using rules.
 */
 /* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/status.h"
#include "computed_field/field_derivative.hpp"
#include "computed_field/field_location.hpp"
#include "computed_field/fieldparametersprivate.hpp"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_region.h"
#include "general/message.h"
#include "region/cmiss_region.hpp"

FieldDerivative::FieldDerivative(cmzn_region *regionIn,
		FE_mesh *meshIn, int meshOrderIn,
		cmzn_fieldparameters *fieldparametersIn, int parameterOrderIn,
		FieldDerivative *lowerDerivativeIn) :
	region(nullptr),  // set by cmzn_region_add_field_derivative
	lowerDerivative((lowerDerivativeIn) ? lowerDerivativeIn->access() : nullptr),
	cacheIndex(-1),
	mesh(meshIn),
	meshDimension((meshIn) ? meshIn->getDimension() : 0),
	meshOrder(meshOrderIn),
	fieldparameters(fieldparametersIn),
	parameterOrder(parameterOrderIn),
	access_count(1)
{
	regionIn->addFieldDerivative(this);
}

FieldDerivative::~FieldDerivative()
{
	if (this->region)
		this->region->removeFieldDerivative(this);
	if (this->lowerDerivative)
		FieldDerivative::deaccess(this->lowerDerivative);
}

FieldDerivative *FieldDerivative::createMeshDerivative(FE_mesh *mesh, FieldDerivative *lowerDerivative)
{
	if (!mesh)
	{
		display_message(ERROR_MESSAGE, "FieldDerivative::createMeshDerivative.  Missing mesh");
		return nullptr;
	}
	int meshOrder = 1;
	if (lowerDerivative)
	{
		if (lowerDerivative->fieldparameters)
		{
			display_message(ERROR_MESSAGE, "FieldDerivative::createMeshDerivative.  Cannot create mesh derivative of field parameters derivative");
			return nullptr;
		}
		if ((lowerDerivative->mesh) && (lowerDerivative->mesh != mesh))
		{
			display_message(ERROR_MESSAGE, "FieldDerivative::createMeshDerivative.  Cannot create derivative w.r.t. multiple meshes");
			return nullptr;
		}
		meshOrder += lowerDerivative->meshOrder;
	}
	cmzn_region *region = FE_region_get_cmzn_region(mesh->get_FE_region());
	return new FieldDerivative(region, mesh, meshOrder, /*fieldparametersIn*/nullptr, /*parameterOrderIn*/0, lowerDerivative);
}

FieldDerivative *FieldDerivative::createParametersDerivative(cmzn_fieldparameters *fieldparameters, FieldDerivative *lowerDerivative)
{
	if (!fieldparameters)
	{
		display_message(ERROR_MESSAGE, "FieldDerivative::createParametersDerivative.  Missing fieldparameters");
		return nullptr;
	}
	int parameterOrder = 1;
	int meshOrder = 0;
	FE_mesh *mesh = nullptr;
	if (lowerDerivative)
	{
		if ((lowerDerivative->fieldparameters) && (lowerDerivative->fieldparameters != fieldparameters))
		{
			display_message(ERROR_MESSAGE, "FieldDerivative::createParametersDerivative.  Cannot create derivative w.r.t. different field parameters");
			return nullptr;
		}
		mesh = lowerDerivative->mesh;
		meshOrder = lowerDerivative->meshOrder;
		parameterOrder += lowerDerivative->parameterOrder;
	}
	cmzn_region *region = Computed_field_get_region(fieldparameters->getField());
	return new FieldDerivative(region, mesh, meshOrder, fieldparameters, parameterOrder, lowerDerivative);
}

int FieldDerivative::deaccess(FieldDerivative* &field_derivative)
{
	if (!field_derivative)
		return CMZN_ERROR_ARGUMENT;
	--(field_derivative->access_count);
	if (field_derivative->access_count <= 0)
		delete field_derivative;
	field_derivative = 0;
	return CMZN_OK;
}

int FieldDerivative::getTermCount(const Field_location& fieldLocation) const
{
	int termCount = this->getMeshTermCount();
	if (this->fieldparameters)
	{
		const Field_location_element_xi *fieldLocationElementXi = fieldLocation.cast_element_xi();
		cmzn_element *element = nullptr;
		if (fieldLocationElementXi)
		{
			element = fieldLocationElementXi->get_element();
		}
		else
		{
			const Field_location_node *fieldLocationNode = fieldLocation.cast_node();
			if (fieldLocationNode)
				element = fieldLocationNode->get_host_element();
		}
		if (element)
		{
			const int elementParametersCount = this->fieldparameters->getNumberOfElementParameters(element);
			for (int d = 0; d < this->parameterOrder; ++d)
				termCount *= elementParametersCount;
		}
		else
		{
			termCount = 0;
		}
	}
	return termCount;
}
