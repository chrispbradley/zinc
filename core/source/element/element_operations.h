/*******************************************************************************
FILE : element_operations.h

LAST MODIFIED : 3 March 2003

DESCRIPTION :
FE_element functions that utilise non finite element data structures and
therefore cannot reside in finite element modules.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (ELEMENT_OPERATIONS_H)
#define ELEMENT_OPERATIONS_H

#include "zinc/fieldfiniteelement.h"
#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_region.h"
#include "general/multi_range.h"
#include "selection/element_point_ranges_selection.h"
#include <vector>

class IntegrationShapePoints
{
	friend class IntegrationPointsCache;

protected:
	FE_element_shape *shape;
	int dimension;
	int numbersOfPoints[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	int numPoints;
	FE_value *points;
	FE_value *weights;

public:
	typedef bool (*InvokeFunction)(void *, FE_value *xi, FE_value weight);

	// takes ownership of points and weights arrays
	IntegrationShapePoints(FE_element_shape *shapeIn, int *numbersOfPointsIn,
		int numPointsIn, FE_value *pointsIn, FE_value *weightsIn) :
		shape(ACCESS(FE_element_shape)(shapeIn)),
		dimension(0),
		numPoints(numPointsIn),
		points(pointsIn),
		weights(weightsIn)
	{
		get_FE_element_shape_dimension(this->shape, &this->dimension);
		for (int i = 0; i < this->dimension; ++i)
			this->numbersOfPoints[i] = numbersOfPointsIn[i];
	}

	virtual ~IntegrationShapePoints()
	{
		DEACCESS(FE_element_shape)(&this->shape);
		delete[] this->points;
		delete[] this->weights;
	}

	int getNumPoints()
	{
		return this->numPoints;
	}

	void getPoint(int index, FE_value *xi, FE_value *weight)
	{
		for (int i = 0; i < this->dimension; ++i)
			xi[i] = this->points[index*dimension + i];
		*weight = this->weights[index];
	}

	template<class IntegralTerm>
		void forEachPoint(IntegralTerm& term)
	{
		if (this->points)
		{
			for (int i = 0; i < this->numPoints; ++i)
				if (!term(points + i*dimension, weights[i]))
					return;
		}
		else
			this->forEachPointVirtual(IntegralTerm::invoke, (void*)&term);
	}

	virtual void forEachPointVirtual(InvokeFunction invokeFunction, void *termVoid)
	{
		for (int i = 0; i < this->numPoints; ++i)
			if (!(invokeFunction)(termVoid, points + i*dimension, weights[i]))
				return;
	}

private:
	IntegrationShapePoints();
	IntegrationShapePoints(const IntegrationShapePoints& source);
	IntegrationShapePoints& operator=(const IntegrationShapePoints& source);
};

class IntegrationPointsCache
{
private:
	std::vector<IntegrationShapePoints*> knownShapePoints;
	cmzn_element_quadrature_rule quadratureRule;
	int numbersOfPoints[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	bool variableNumbersOfPoints;

public:
	IntegrationPointsCache(cmzn_element_quadrature_rule quadratureRuleIn,
		int numbersOfPointsCountIn, const int *numbersOfPointsIn);

	~IntegrationPointsCache();

	/** @return number of points */
	IntegrationShapePoints *getPoints(cmzn_element *element);
};

/*
Global functions
----------------
*/

int FE_region_change_element_identifiers(struct FE_region *fe_region,
	int dimension,	int element_offset,
	struct Computed_field *sort_by_field, FE_value time,
	cmzn_field_element_group_id element_group);
/*******************************************************************************
LAST MODIFIED : 16 January 2003

DESCRIPTION :
Changes the identifiers of all elements of <cm_type> in <fe_region>.
If <sort_by_field> is NULL, adds <element_offset> to the identifiers.
If <sort_by_field> is specified, it is evaluated at the centre of all elements
in the group and the elements are sorted by it - changing fastest with the first
component and keeping the current order where the field has the same values.
Checks for and fails if attempting to give any of the elements in <fe_region> an
identifier already used by an element in the same master FE_region.
Calls to this function should be enclosed in FE_region_begin_change/end_change.
Note function avoids iterating through FE_region element lists as this is not
allowed during identifier changes.
==============================================================================*/

/***************************************************************************//**
 * Create an element list from the elements in mesh optionally restricted to
 * those within the element_ranges or where conditional_field is true in element
 * at time.
 *
 * @param mesh  Handle to the mesh.
 * @param element_ranges  Optional Multi_range of element identifiers.
 * @param conditional_field  Optional field interpreted as a boolean value which
 * must be true for an element from mesh to be included in list.
 * @param time  Time to evaluate the conditional_field at.
 * @return  The element list, or NULL on failure.
 */
struct LIST(FE_element) *cmzn_mesh_get_selected_element_list(cmzn_mesh_id mesh,
	struct Multi_range *element_ranges, struct Computed_field *conditional_field,
	FE_value time);

/***************************************************************************//**
 * Create an element list from those elements of dimension in the supplied
 * region, optionally restricted to any of the following conditions:
 * - element identifier is in the element_ranges;
 * - element is in the group_field;
 * - conditional field is true in the element;
 *
 * @param region  The pointer to a region
 * @param dimension  The dimension of elements to query about
 * @param element_ranges  Multi_range of elements.
 * @param group_field  Group field of the region
 * @param conditional_field  Optional field interpreted as a boolean value which
 * must be true for an element from mesh to be included in list.
 * @param time  Time to evaluate the conditional_field at.
 * @return  The element list, or NULL on failure.
 */
struct LIST(FE_element) *FE_element_list_from_region_and_selection_group(
	struct cmzn_region *region, int dimension,
	struct Multi_range *element_ranges, struct Computed_field *group_field,
	struct Computed_field *conditional_field, FE_value time);

/***************************************************************************//**
 * Create points in gauss_points_nodeset with embedded locations and weights
 * matching the Gauss quadrature points in all elements of mesh.
 * Supports all main element shapes up to order 4.
 *
 * @param mesh  The mesh to create gauss points for.
 * @param order  The 1-D polynomial order from 1 to 4, which for line shapes
 * gives that number of Gauss points per element dimension.
 * @param gauss_points_nodeset  The nodeset to create gauss points in.
 * @param first_identifier  The minimum identifier to use for the gauss points.
 * @param gauss_location_field  Field to define at gauss points for storing
 * gauss point element_xi location.
 * @param gauss_weight_field  Scalar field to define at gauss points for storing
 * real Gauss point weight.
 * @return  1 on success, 0 on failure.
 */
int cmzn_mesh_create_gauss_points(cmzn_mesh_id mesh, int order,
	cmzn_nodeset_id gauss_points_nodeset, int first_identifier,
	cmzn_field_stored_mesh_location_id gauss_location_field,
	cmzn_field_finite_element_id gauss_weight_field);

#endif /* !defined (ELEMENT_OPERATIONS_H) */
