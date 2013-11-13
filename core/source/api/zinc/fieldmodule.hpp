/***************************************************************************//**
 * FILE : fieldmodule.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_FIELDMODULE_HPP__
#define CMZN_FIELDMODULE_HPP__

#include "zinc/fieldmodule.h"
#include "zinc/field.hpp"
#include "zinc/fieldcache.hpp"
#include "zinc/element.hpp"
#include "zinc/node.hpp"
#include "zinc/optimisation.hpp"
#include "zinc/timesequence.hpp"
#include "zinc/types/scenecoordinatesystem.hpp"

namespace OpenCMISS
{
namespace Zinc
{

class Fieldmodulenotifier;
class FieldAlias;
class FieldAdd;
class FieldPower;
class FieldMultiply;
class FieldDivide;
class FieldSubtract;
class FieldSumComponents;
class FieldLog;
class FieldSqrt;
class FieldExp;
class FieldAbs;
class FieldIdentity;
class FieldComponent;
class FieldConcatenate;
class FieldIf;
class FieldConstant;
class FieldStringConstant;
class FieldCoordinateTransformation;
class FieldVectorCoordinateTransformation;
class FieldFibreAxes;
class FieldFiniteElement;
class FieldEmbedded;
class FieldFindMeshLocation;
class FieldNodeValue;
class FieldStoredMeshLocation;
class FieldStoredString;
class FieldGroup;
class FieldImage;
class FieldAnd;
class FieldEqualTo;
class FieldGreaterThan;
class FieldLessThan;
class FieldOr;
class FieldNot;
class FieldXor;
class FieldDeterminant;
class FieldEigenvalues;
class FieldEigenvectors;
class FieldMatrixInvert;
class FieldMatrixMultiply;
class FieldProjection;
class FieldTranspose;
class FieldNodesetSum;
class FieldNodesetMean;
class FieldNodesetSumSquares;
class FieldNodesetMeanSquares;
class FieldNodesetMinimum;
class FieldNodesetMaximum;
class FieldElementGroup;
class FieldNodeGroup;
class FieldTimeLookup;
class FieldTimeValue;
class FieldDerivative;
class FieldCurl;
class FieldDivergence;
class FieldGradient;
class FieldSin;
class FieldCos;
class FieldTan;
class FieldAsin;
class FieldAcos;
class FieldAtan;
class FieldAtan2;
class FieldCrossProduct;
class FieldCrossProduct3D;
class FieldDotProduct;
class FieldMagnitude;
class FieldNormalise;
class FieldImagefilterBinaryDilate;
class FieldImagefilterBinaryErode;
class FieldImagefilterBinaryThreshold;
class FieldImagefilterCannyEdgeDetection;
class FieldImagefilterConnectedThreshold;
class FieldImagefilterCurvatureAnisotropicDiffusion;
class FieldImagefilterDiscreteGaussian;
class FieldImagefilterHistogram;
class FieldImagefilterMean;
class FieldImagefilterGradientMagnitudeRecursiveGaussian;
class FieldImagefilterRescaleIntensity;
class FieldImagefilterSigmoid;
class FieldImagefilterThreshold;
class FieldSceneviewerProjection;
class Timekeeper;
class Optimisation;
class Sceneviewer;

class Fieldmodule
{
private:

	cmzn_fieldmodule_id id;

public:

	Fieldmodule() : id(0)
	{	}

	// takes ownership of C handle, responsibility for destroying it
	explicit Fieldmodule(cmzn_fieldmodule_id field_module_id) :
		id(field_module_id)
	{ }

	Fieldmodule(const Fieldmodule& fieldModule) :
		id(cmzn_fieldmodule_access(fieldModule.id))
	{ }

	Fieldmodule(Field& field) :
		id(cmzn_field_get_fieldmodule(field.getId()))
	{ }

	Fieldmodule& operator=(const Fieldmodule& fieldModule)
	{
		cmzn_fieldmodule_id temp_id = cmzn_fieldmodule_access(fieldModule.id);
		if (0 != id)
		{
			cmzn_fieldmodule_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Fieldmodule()
	{
		if (0 != id)
		{
			cmzn_fieldmodule_destroy(&id);
		}
	}

	bool isValid()
	{
		return (0 != id);
	}

	cmzn_fieldmodule_id getId()
	{
		return id;
	}

	int beginChange()
	{
		return cmzn_fieldmodule_begin_change(id);
	}

	int endChange()
	{
		return cmzn_fieldmodule_end_change(id);
	}

	int defineAllFaces()
	{
		return cmzn_fieldmodule_define_all_faces(id);
	}

	Field findFieldByName(const char *fieldName)
	{
		return Field(cmzn_fieldmodule_find_field_by_name(id, fieldName));
	}

	Fieldcache createFieldcache()
	{
		return Fieldcache(cmzn_fieldmodule_create_fieldcache(id));
	}

	Fielditerator createFielditerator()
	{
		return Fielditerator(cmzn_fieldmodule_create_fielditerator(id));
	}

	Fieldmodulenotifier createNotifier();

	Elementbasis createElementbasis(int dimension, enum Elementbasis::FunctionType functionType)
	{
		return Elementbasis(cmzn_fieldmodule_create_elementbasis(
			id, dimension, static_cast<cmzn_elementbasis_function_type>(functionType)));
	}

	Nodeset findNodesetByDomainType(Field::DomainType domainType)
	{
		return Nodeset(cmzn_fieldmodule_find_nodeset_by_domain_type(id,
			static_cast<cmzn_field_domain_type>(domainType)));
	}

	Nodeset findNodesetByName(const char *nodeset_name)
	{
		return Nodeset(cmzn_fieldmodule_find_nodeset_by_name(id,
			nodeset_name));
	}

	Mesh findMeshByDimension(int dimension)
	{
		return Mesh(cmzn_fieldmodule_find_mesh_by_dimension(id, dimension));
	}

	Mesh findMeshByName(const char *meshName)
	{
		return Mesh(cmzn_fieldmodule_find_mesh_by_name(id, meshName));
	}

	Timesequence getMatchingTimesequence(int timesCount, const double *timesIn)
	{
		return Timesequence(cmzn_fieldmodule_get_matching_timesequence(
			id, timesCount, timesIn));
	}

	Optimisation createOptimisation()
	{
		return Optimisation(cmzn_fieldmodule_create_optimisation(id));
	}

	FieldAlias createFieldAlias(Field& sourceField);

	FieldAdd createFieldAdd(Field& sourceField1, Field& sourceField2);

	FieldPower createFieldPower(Field& sourceField1, Field& sourceField2);

	FieldMultiply createFieldMultiply(Field& sourceField1, Field& sourceField2);

	FieldDivide createFieldDivide(Field& sourceField1, Field& sourceField2);

	FieldSubtract createFieldSubtract(Field& sourceField1, Field& sourceField2);

	FieldSumComponents createFieldSumComponents(Field& sourceField);

	FieldLog createFieldLog(Field& sourceField);

	FieldSqrt createFieldSqrt(Field& sourceField);

	FieldExp createFieldExp(Field& sourceField);

	FieldAbs createFieldAbs(Field& sourceField);

	FieldIdentity createFieldIdentity(Field& sourceField);

	FieldComponent createFieldComponent(Field& sourceField, int componentIndex);

	FieldConcatenate createFieldConcatenate(int fieldsCount, Field *sourceFields);

	FieldIf createFieldIf(Field& sourceField1, Field& sourceField2, Field& sourceField3);

	FieldConstant createFieldConstant(int valuesCount, const double *valuesIn);

	FieldStringConstant createFieldStringConstant(const char *stringConstant);

	FieldCoordinateTransformation createFieldCoordinateTransformation(Field& sourceField);

	FieldVectorCoordinateTransformation createFieldVectorCoordinateTransformation(
		Field& vectorField, Field& coordinateField);

	FieldFibreAxes createFieldFibreAxes(Field& fibreField, Field& coordinateField);

	FieldFiniteElement createFieldFiniteElement(int numberOfComponents);

	FieldEmbedded createFieldEmbedded(Field& sourceField, Field& embeddedLocationField);

	FieldFindMeshLocation createFieldFindMeshLocation(
		Field& sourceField, Field& meshField, Mesh& mesh);

	FieldNodeValue createFieldNodeValue(Field& sourceField,
		Node::ValueLabel nodeValueLabel, int versionNumber);

	FieldStoredMeshLocation createFieldStoredMeshLocation(Mesh& mesh);

	FieldStoredString createFieldStoredString();

	FieldGroup createFieldGroup();

	FieldImage createFieldImage();

	FieldImage createFieldImageFromSource(Field& sourceField);

	FieldAnd createFieldAnd(Field& sourceField1, Field& sourceField2);

	FieldEqualTo createFieldEqualTo(Field& sourceField1, Field& sourceField2);

	FieldGreaterThan createFieldGreaterThan(Field& sourceField1, Field& sourceField2);

	FieldLessThan createFieldLessThan(Field& sourceField1, Field& sourceField2);

	FieldOr createFieldOr(Field& sourceField1, Field& sourceField2);

	FieldNot createFieldNot(Field& sourceField);

	FieldXor createFieldXor(Field& sourceField1, Field& sourceField2);

	FieldDeterminant createFieldDeterminant(Field& sourceField);

	FieldEigenvalues createFieldEigenvalues(Field& sourceField);

	FieldEigenvectors createFieldEigenvectors(FieldEigenvalues& eigenValuesField);

	FieldMatrixInvert createFieldMatrixInvert(Field& sourceField);

	FieldMatrixMultiply createFieldMatrixMultiply(int numberOfRows,
		Field sourceField1, Field& sourceField2);

	FieldProjection createFieldProjection(Field& sourceField,	Field& projectionMatrixField);

	FieldTranspose createFieldTranspose(int sourceNumberOfRows, Field& sourceField);

	FieldNodesetSum createFieldNodesetSum(Field& sourceField, Nodeset& nodeset);

	FieldNodesetMean createFieldNodesetMean(Field& sourceField, Nodeset& nodeset);

	FieldNodesetSumSquares createFieldNodesetSumSquares(Field& sourceField, Nodeset& nodeset);

	FieldNodesetMeanSquares createFieldNodesetMeanSquares(Field& sourceField, Nodeset& nodeset);

	FieldNodesetMinimum createFieldNodesetMinimum(Field& sourceField, Nodeset& nodeset);

	FieldNodesetMaximum createFieldNodesetMaximum(Field& sourceField, Nodeset& nodeset);

	FieldNodeGroup createFieldNodeGroup(Nodeset& nodeset);

	FieldElementGroup createFieldElementGroup(Mesh& mesh);

	FieldTimeLookup createFieldTimeLookup(Field& sourceField, Field& timeField);

	FieldTimeValue createFieldTimeValue(Timekeeper& timeKeeper);

	FieldDerivative createFieldDerivative(Field& sourceField, int xi_index);

	FieldCurl createFieldCurl(Field& vectorField, Field& coordinateField);

	FieldDivergence createFieldDivergence(Field& vectorField, Field& coordinateField);

	FieldGradient createFieldGradient(Field& sourceField, Field& coordinateField);

	FieldSin createFieldSin(Field& sourceField);

	FieldCos createFieldCos(Field& sourceField);

	FieldTan createFieldTan(Field& sourceField);

	FieldAsin createFieldAsin(Field& sourceField);

	FieldAcos createFieldAcos(Field& sourceField);

	FieldAtan createFieldAtan(Field& sourceField);

	FieldAtan2 createFieldAtan2(Field& sourceField1, Field& sourceField2);

	FieldCrossProduct createFieldCrossProduct(int fieldsCount, Field *sourceFields);

	FieldCrossProduct createFieldCrossProduct(Field& sourceField1, Field& sourceField2);

	FieldDotProduct createFieldDotProduct(Field& sourceField1, Field& sourceField2);

	FieldMagnitude createFieldMagnitude(Field& sourceField);

	FieldNormalise createFieldNormalise(Field& sourceField);

	FieldImagefilterBinaryDilate createFieldImagefilterBinaryDilate(Field& sourceField,
		int radius, double dilate_value);

	FieldImagefilterBinaryErode createFieldImagefilterBinaryErode(Field& sourceField,
		int radius, double erode_value);

	FieldImagefilterBinaryThreshold createFieldImagefilterBinaryThreshold(Field& sourceField);

	FieldImagefilterCannyEdgeDetection createFieldImagefilterCannyEdgeDetection(Field& sourceField,
			double variance, double maximumError, double upperThreshold, double lowerThreshold);

	FieldImagefilterConnectedThreshold createFieldImagefilterConnectedThreshold(Field& sourceField,
		double lowerThreshold, double upperThreshold, double replaceValue,
		int dimension, int seedPointsCount, const double *seedPoints);

	FieldImagefilterCurvatureAnisotropicDiffusion createFieldImagefilterCurvatureAnisotropicDiffusion(
		Field& sourceField, double timeStep, double conductance, int numIterations);

	FieldImagefilterDiscreteGaussian createFieldImagefilterDiscreteGaussian(Field& sourceField);

	FieldImagefilterHistogram createFieldImagefilterHistogram(Field& sourceField);

	FieldImagefilterMean createFieldImagefilterMean(Field& sourceField, int valuesCount,
		const int *radiusSizesIn);

	FieldImagefilterGradientMagnitudeRecursiveGaussian
		createFieldImagefilterGradientMagnitudeRecursiveGaussian(Field& sourceField,
			double sigma);

	FieldImagefilterRescaleIntensity createFieldImagefilterRescaleIntensity(Field& sourceField,
		double outputMin, double outputMax);

	FieldImagefilterSigmoid createFieldImagefilterSigmoid(Field& sourceField,
		double min, double max,	double alpha, double beta);

	FieldImagefilterThreshold createFieldImagefilterThreshold(Field& sourceField);

	FieldSceneviewerProjection createFieldSceneviewerProjection(
		Sceneviewer& sceneviewer, Scenecoordinatesystem fromCoordinateSystem,
		Scenecoordinatesystem toCoordinateSystem);
};

class Fieldmoduleevent
{
protected:
	cmzn_fieldmoduleevent_id id;

public:

	Fieldmoduleevent() : id(0)
	{  }

	// takes ownership of C handle, responsibility for destroying it
	explicit Fieldmoduleevent(cmzn_fieldmoduleevent_id in_fieldmodule_event_id) :
		id(in_fieldmodule_event_id)
	{  }

	Fieldmoduleevent(const Fieldmoduleevent& fieldmoduleEvent) :
		id(cmzn_fieldmoduleevent_access(fieldmoduleEvent.id))
	{  }

	enum ChangeFlag
	{
		CHANGE_FLAG_NONE = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_NONE,
		CHANGE_FLAG_ADD = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_ADD,
		CHANGE_FLAG_REMOVE = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_REMOVE,
		CHANGE_FLAG_IDENTIFIER = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_IDENTIFIER,
		CHANGE_FLAG_DEFINITION = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_DEFINITION,
		CHANGE_FLAG_DEPENDENCY = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_DEPENDENCY,
		CHANGE_FLAG_METADATA = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_METADATA,
		CHANGE_FLAG_FINAL = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_FINAL,
		CHANGE_FLAG_RESULT = CMZN_FIELDMODULEEVENT_CHANGE_FLAG_RESULT
	};

	/**
	 * Type for passing logical OR of #ChangeFlag
	 * @see getChangeFlags
	 */
	typedef int ChangeFlags;

	Fieldmoduleevent& operator=(const Fieldmoduleevent& fieldmoduleEvent)
	{
		cmzn_fieldmoduleevent_id temp_id = cmzn_fieldmoduleevent_access(fieldmoduleEvent.id);
		if (0 != id)
			cmzn_fieldmoduleevent_destroy(&id);
		id = temp_id;
		return *this;
	}

	~Fieldmoduleevent()
	{
		if (0 != id)
		{
			cmzn_fieldmoduleevent_destroy(&id);
		}
	}

	bool isValid()
	{
		return (0 != id);
	}

	cmzn_fieldmoduleevent_id getId()
	{
		return id;
	}

	ChangeFlags getChangeFlags() const
	{
		return static_cast<ChangeFlag>(cmzn_fieldmoduleevent_get_change_flags(id));
	}

};

/**
 * Base class functor for field module notifier callbacks:
 * - Derive from this class adding any user data required.
 * - Implement virtual operator()(const Fieldmoduleevent&) to handle callback.
 * @see Fieldmodulenotifier::setCallback()
 */
class Fieldmodulecallback
{
friend class Fieldmodulenotifier;
private:
	Fieldmodulecallback(Fieldmodulecallback&); // not implemented
	Fieldmodulecallback& operator=(Fieldmodulecallback&); // not implemented

	static void C_callback(cmzn_fieldmoduleevent_id fieldmoduleevent_id, void *callbackVoid)
	{
		Fieldmoduleevent fieldmoduleevent(cmzn_fieldmoduleevent_access(fieldmoduleevent_id));
		Fieldmodulecallback *callback = reinterpret_cast<Fieldmodulecallback *>(callbackVoid);
		(*callback)(fieldmoduleevent);
	}

  virtual void operator()(const Fieldmoduleevent &fieldmoduleevent) = 0;

protected:
	Fieldmodulecallback()
	{ }

public:
	virtual ~Fieldmodulecallback()
	{ }
};

class Fieldmodulenotifier
{
protected:
	cmzn_fieldmodulenotifier_id id;

public:

	Fieldmodulenotifier() : id(0)
	{  }

	// takes ownership of C handle, responsibility for destroying it
	explicit Fieldmodulenotifier(cmzn_fieldmodulenotifier_id in_fieldmodulenotifier_id) :
		id(in_fieldmodulenotifier_id)
	{  }

	Fieldmodulenotifier(const Fieldmodulenotifier& fieldmoduleNotifier) :
		id(cmzn_fieldmodulenotifier_access(fieldmoduleNotifier.id))
	{  }

	Fieldmodulenotifier& operator=(const Fieldmodulenotifier& fieldmoduleNotifier)
	{
		cmzn_fieldmodulenotifier_id temp_id = cmzn_fieldmodulenotifier_access(fieldmoduleNotifier.id);
		if (0 != id)
		{
			cmzn_fieldmodulenotifier_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Fieldmodulenotifier()
	{
		if (0 != id)
		{
			cmzn_fieldmodulenotifier_destroy(&id);
		}
	}

	bool isValid()
	{
		return (0 != id);
	}

	cmzn_fieldmodulenotifier_id getId()
	{
		return id;
	}

	int setCallback(Fieldmodulecallback& callback)
	{
		return cmzn_fieldmodulenotifier_set_callback(id, callback.C_callback, static_cast<void*>(&callback));
	}

	int clearCallback()
	{
		return cmzn_fieldmodulenotifier_clear_callback(id);
	}
};

inline Fieldmodule Field::getFieldmodule()
{
	return Fieldmodule(*this);
}

inline Fieldmodulenotifier Fieldmodule::createNotifier()
{
	return Fieldmodulenotifier(cmzn_fieldmodule_create_notifier(id));
}

}  // namespace Zinc
}

#endif
