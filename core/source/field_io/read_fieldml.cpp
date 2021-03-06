/**
 * FILE : read_fieldml.cpp
 * 
 * FieldML 0.5 model reader implementation.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "zinc/element.h"
#include "zinc/element.hpp"
#include "zinc/field.h"
#include "zinc/fieldcache.h"
#include "zinc/fieldmodule.h"
#include "zinc/fieldfiniteelement.h"
#include "zinc/node.h"
#include "zinc/region.h"
#include "zinc/status.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_finite_element.h"
#include "field_io/fieldml_common.hpp"
#include "field_io/read_fieldml.hpp"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "general/refcounted.hpp"
#include "mesh/cmiss_node_private.hpp"
#include "FieldmlIoApi.h"

using namespace OpenCMISS;

namespace {

const char *libraryChartArgumentNames[] =
{
	0,
	"chart.1d.argument",
	"chart.2d.argument",
	"chart.3d.argument"
};

struct ElementFieldComponent
{
	cmzn_elementbasis_id elementBasis;
	HDsMapInt local_point_to_node;
	HDsMapIndexing indexing;
	int local_point_count;
	const int *swizzle;
	int *local_point_indexes;
	int *swizzled_local_point_indexes;
	int *node_identifiers;
	cmzn_node_value_label constantNodeDerivative;
	HDsMapInt nodeDerivativesMap;
	int constantNodeVersion;
	HDsMapInt nodeVersionsMap;


	ElementFieldComponent(cmzn_elementbasis_id elementBasisIn,
			HDsMapInt local_point_to_nodeIn,
			HDsMapIndexing indexingIn, int local_point_countIn,
			const int *swizzleIn) :
		elementBasis(elementBasisIn),
		local_point_to_node(local_point_to_nodeIn),
		indexing(indexingIn),
		local_point_count(local_point_countIn),
		swizzle(swizzleIn),
		local_point_indexes(new int[local_point_count]),
		swizzled_local_point_indexes(new int[local_point_count]),
		node_identifiers(new int[local_point_count]),
		constantNodeDerivative(CMZN_NODE_VALUE_LABEL_VALUE),
		constantNodeVersion(1)
	{
	}

	~ElementFieldComponent()
	{
		cmzn_elementbasis_destroy(&elementBasis);
		delete[] local_point_indexes;
		delete[] swizzled_local_point_indexes;
		delete[] node_identifiers;
	}

	void setNodeDerivativesMap(DsMap<int> *nodeDerivativesMapIn)
	{
		cmzn::SetImpl(this->nodeDerivativesMap, cmzn::Access(nodeDerivativesMapIn));
	}

	DsMap<int> *getNodeDerivativesMap() const
	{
		return cmzn::GetImpl(this->nodeDerivativesMap);
	}

	void setConstantNodeDerivative(cmzn_node_value_label constantNodeDerivativeIn)
	{
		this->constantNodeDerivative = constantNodeDerivativeIn;
	}

	cmzn_node_value_label getConstantNodeDerivative() const
	{
		return this->constantNodeDerivative;
	}

	void setNodeVersionsMap(DsMap<int> *nodeVersionsMapIn)
	{
		cmzn::SetImpl(this->nodeVersionsMap, cmzn::Access(nodeVersionsMapIn));
	}

	DsMap<int> *getNodeVersionsMap() const
	{
		return cmzn::GetImpl(this->nodeVersionsMap);
	}

	void setConstantNodeVersion(int constantNodeVersionIn)
	{
		this->constantNodeVersion = constantNodeVersionIn;
	}

	int getConstantNodeVersion() const
	{
		return this->constantNodeVersion;
	}

};

typedef std::map<FmlObjectHandle,ElementFieldComponent*> EvaluatorElementFieldComponentMap;
typedef std::map<FmlObjectHandle,HDsLabels> FmlObjectLabelsMap;
typedef std::map<FmlObjectHandle,HDsMapInt> FmlObjectIntParametersMap;
typedef std::map<FmlObjectHandle,HDsMapDouble> FmlObjectDoubleParametersMap;

class FieldMLReader
{
	cmzn_region *region;
	cmzn_fieldmodule_id field_module;
	const char *filename;
	FmlSessionHandle fmlSession;
	int meshDimension;
	FmlObjectHandle fmlNodesType;
	FmlObjectHandle fmlNodesArgument;
	FmlObjectHandle fmlElementsType;
	FmlObjectHandle fmlNodeDerivativesType;
	FmlObjectHandle fmlNodeDerivativesArgument;
	FmlObjectHandle fmlNodeVersionsType;
	FmlObjectHandle fmlNodeVersionsArgument;
	EvaluatorElementFieldComponentMap componentMap;
	FmlObjectLabelsMap labelsMap;
	FmlObjectIntParametersMap intParametersMap;
	FmlObjectDoubleParametersMap doubleParametersMap;
	bool verbose;
	int nameBufferLength;
	char *nameBuffer; // buffer for reading object names into
	std::set<FmlObjectHandle> processedObjects;

public:
	FieldMLReader(struct cmzn_region *region, const char *filename) :
		region(cmzn_region_access(region)),
		field_module(cmzn_region_get_fieldmodule(region)),
		filename(filename),
		fmlSession(Fieldml_CreateFromFile(filename)),
		meshDimension(0),
		fmlNodesType(FML_INVALID_OBJECT_HANDLE),
		fmlNodesArgument(FML_INVALID_OBJECT_HANDLE),
		fmlElementsType(FML_INVALID_OBJECT_HANDLE),
		fmlNodeDerivativesType(FML_INVALID_OBJECT_HANDLE),
		fmlNodeDerivativesArgument(FML_INVALID_OBJECT_HANDLE),
		fmlNodeVersionsType(FML_INVALID_OBJECT_HANDLE),
		fmlNodeVersionsArgument(FML_INVALID_OBJECT_HANDLE),
		verbose(false),
		nameBufferLength(50),
		nameBuffer(new char[nameBufferLength])
	{
		Fieldml_SetDebug(fmlSession, /*debug*/verbose);
	}

	~FieldMLReader()
	{
		for (EvaluatorElementFieldComponentMap::iterator iter = componentMap.begin(); iter != componentMap.end(); iter++)
		{
			delete (iter->second);
		}
		Fieldml_Destroy(fmlSession);
		cmzn_fieldmodule_destroy(&field_module);
		cmzn_region_destroy(&region);
		delete[] nameBuffer;
	}

	/** @return  1 on success, 0 on failure */
	int parse();

private:

	std::string getName(FmlObjectHandle fmlObjectHandle)
	{
		if (FML_INVALID_HANDLE == fmlObjectHandle)
			return std::string("INVALID");
		nameBuffer[0] = 0;
		while (true)
		{
			int length = Fieldml_CopyObjectName(fmlSession, fmlObjectHandle, nameBuffer, nameBufferLength);
			if (length < nameBufferLength - 1)
				break;
			nameBufferLength *= 2;
			delete[] nameBuffer;
			nameBuffer = new char[nameBufferLength];
		}
		return std::string(nameBuffer);
	}

	std::string getDeclaredName(FmlObjectHandle fmlObjectHandle)
	{
		if (FML_INVALID_HANDLE == fmlObjectHandle)
			return std::string("INVALID");
		nameBuffer[0] = 0;
		while (true)
		{
			int length = Fieldml_CopyObjectDeclaredName(fmlSession, fmlObjectHandle, nameBuffer, nameBufferLength);
			if (length < nameBufferLength - 1)
				break;
			nameBufferLength *= 2;
			delete[] nameBuffer;
			nameBuffer = new char[nameBufferLength];
		}
		return std::string(nameBuffer);
	}

	DsLabels *getLabelsForEnsemble(FmlObjectHandle fmlEnsemble);

	template <typename VALUETYPE> int readParametersArray(FmlObjectHandle fmlParameters,
		DsMap<VALUETYPE>& parameters);

	DsMap<int> *getEnsembleParameters(FmlObjectHandle fmlParameters);
	DsMap<double> *getContinuousParameters(FmlObjectHandle fmlParameters);

	int readNodes(FmlObjectHandle fmlNodesArgument);

	/** find and read global nodes, derivatives and versions types */
	int readGlobals();

	int readMeshes();

	ElementFieldComponent *getElementFieldComponent(cmzn_mesh_id mesh,
		FmlObjectHandle fmlEvaluator, FmlObjectHandle fmlNodeParametersArgument,
		FmlObjectHandle fmlNodesArgument, FmlObjectHandle fmlElementArgument);

	int readField(FmlObjectHandle fmlFieldEvaluator, FmlObjectHandle fmlComponentsType,
		std::vector<FmlObjectHandle> &fmlComponentEvaluators, FmlObjectHandle fmlNodeParameters,
		FmlObjectHandle fmlNodeParametersArgument, FmlObjectHandle fmlNodesArgument,
		FmlObjectHandle fmlElementArgument);

	bool evaluatorIsScalarContinuousPiecewiseOverElements(FmlObjectHandle fmlEvaluator,
		FmlObjectHandle &fmlElementArgument);

	bool evaluatorIsNodeParameters(FmlObjectHandle fmlNodeParameters, FmlObjectHandle fmlComponentsArgument);

	int readAggregateFields();

	int readReferenceFields();

	bool isProcessed(FmlObjectHandle fmlObjectHandle)
	{
		return (processedObjects.find(fmlObjectHandle) != processedObjects.end());
	}

	void setProcessed(FmlObjectHandle fmlObjectHandle)
	{
		processedObjects.insert(fmlObjectHandle);
	}

};

/**
 * Gets handle to DsLabels matching fmlEnsembleType. Definition is read from
 * FieldML document only when first requested.
 * ???GRC Assumes there is only one argument for each type; consider having a
 * separate class to represent each argument.
 *
 * @param fmlEnsembleType  Handle of type FHT_ENSEMBLE_TYPE.
 * @return  Accessed pointer to labels, or 0 on failure */
DsLabels *FieldMLReader::getLabelsForEnsemble(FmlObjectHandle fmlEnsembleType)
{
	FmlObjectLabelsMap::iterator iterator = this->labelsMap.find(fmlEnsembleType);
	if (iterator != labelsMap.end())
		return cmzn::Access(cmzn::GetImpl(iterator->second));

	std::string name = this->getName(fmlEnsembleType);
	if (name.length()==0)
	{
		// GRC workaround for ensemble types that have not been imported
		name = "NONIMPORTED_";
		name.append(getDeclaredName(fmlEnsembleType));
	}
	if (Fieldml_GetObjectType(fmlSession, fmlEnsembleType) != FHT_ENSEMBLE_TYPE)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getLabelsForEnsemble:  Argument %s is not ensemble type", name.c_str());
		return 0;
	}

	FieldmlEnsembleMembersType fmlEnsembleMembersType = Fieldml_GetEnsembleMembersType(fmlSession, fmlEnsembleType);
	int recordSize = 0;
	switch (fmlEnsembleMembersType)
	{
	case FML_ENSEMBLE_MEMBER_RANGE:
		break;
	case FML_ENSEMBLE_MEMBER_LIST_DATA:
		recordSize = 1;
		break;
	case FML_ENSEMBLE_MEMBER_RANGE_DATA:
		recordSize = 2;
		break;
	case FML_ENSEMBLE_MEMBER_STRIDE_RANGE_DATA:
		recordSize = 3;
		break;
	case FML_ENSEMBLE_MEMBER_UNKNOWN:
	default:
		display_message(ERROR_MESSAGE, "Read FieldML:  Unsupported members type %d for ensemble type %s",
			fmlEnsembleMembersType, name.c_str());
		return 0;
		break;
	}
	if (verbose)
	{
		display_message(INFORMATION_MESSAGE, "Reading ensemble type %s\n", name.c_str());
	}
	HDsLabels labels(new DsLabels());
	labels->setName(name);
	this->setProcessed(fmlEnsembleType);
	this->labelsMap[fmlEnsembleType] = labels;

	int return_code = 1;
	if (FML_ENSEMBLE_MEMBER_RANGE == fmlEnsembleMembersType)
	{
		FmlEnsembleValue min = Fieldml_GetEnsembleMembersMin(fmlSession, fmlEnsembleType);
		FmlEnsembleValue max = Fieldml_GetEnsembleMembersMax(fmlSession, fmlEnsembleType);
		int stride = Fieldml_GetEnsembleMembersStride(fmlSession, fmlEnsembleType);
		int result = labels->addLabelsRange(min, max, stride);
		if (result != CMZN_OK)
			return 0;
	}
	else
	{
		const int memberCount = Fieldml_GetMemberCount(fmlSession, fmlEnsembleType);
		FmlObjectHandle fmlDataSource = Fieldml_GetDataSource(fmlSession, fmlEnsembleType);
		int arrayRank = 0;
		int arraySizes[2];
		if (fmlDataSource == FML_INVALID_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Could not get data source for ensemble type %s", name.c_str());
			return_code = 0;
		}
		else if (FML_DATA_SOURCE_ARRAY != Fieldml_GetDataSourceType(fmlSession, fmlDataSource))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Can only define ensemble types from array data source; processing %s", name.c_str());
			return_code = 0;
		}
		else if (2 != (arrayRank = Fieldml_GetArrayDataSourceRank(fmlSession, fmlDataSource)))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Expected array data source of rank 2; processing %s", name.c_str());
			return_code = 0;
		}
		else if ((FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceSizes(fmlSession, fmlDataSource, arraySizes)) ||
			(arraySizes[0] < 1) || (arraySizes[1] != recordSize))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Invalid data source sizes; processing %s", name.c_str());
			return_code = 0;
		}
		else
		{
			FmlReaderHandle fmlReader = Fieldml_OpenReader(fmlSession, fmlDataSource);
			int *rangeData = new int[arraySizes[0]*arraySizes[1]];
			const int arrayOffsets[2] = { 0, 0 };
			FmlIoErrorNumber ioResult = FML_IOERR_NO_ERROR;
			if (fmlReader == FML_INVALID_HANDLE)
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Could not open reader for ensemble type %s", name.c_str());
				return_code = 0;
			}
			else if (FML_IOERR_NO_ERROR !=
				(ioResult = Fieldml_ReadIntSlab(fmlReader, arrayOffsets, arraySizes, rangeData)))
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Error reading array data source %s", getName(fmlDataSource).c_str());
				return_code = 0;
			}
			else
			{
				const int recordCount = arraySizes[0];
				for (int i = 0; i < recordCount; i++)
				{
					switch (fmlEnsembleMembersType)
					{
					case FML_ENSEMBLE_MEMBER_LIST_DATA:
					{
						int result = labels->findOrCreateLabel(rangeData[i]);
						if (result != CMZN_OK)
							return_code = 0;
					} break;
					case FML_ENSEMBLE_MEMBER_RANGE_DATA:
					{
						int result = labels->addLabelsRange(/*min*/rangeData[i*2], /*max*/rangeData[i*2 + 1]);
						if (result != CMZN_OK)
							return_code = 0;
					} break;
					case FML_ENSEMBLE_MEMBER_STRIDE_RANGE_DATA:
					{
						int result = labels->addLabelsRange(/*min*/rangeData[i*3], /*max*/rangeData[i*3 + 1], /*stride*/rangeData[i*3 + 2]);
						if (result != CMZN_OK)
							return_code = 0;
					} break;
					default:
						// should never happen - see switch above
						display_message(ERROR_MESSAGE, "Read FieldML:  Unexpected ensemble members type");
						return_code = 0;
						break;
					}
					if (!return_code)
					{
						break;
					}
				}
				if (return_code && (labels->getSize() != memberCount))
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Ensemble type %s lists member count %d, actual number in data source is %d",
						name.c_str(), memberCount, labels->getSize());
					return_code = 0;
				}
			}
			delete[] rangeData;
			Fieldml_CloseReader(fmlReader);
		}
	}
	if (!return_code)
		return 0;
	return cmzn::Access(cmzn::GetImpl(labels));
}

// template and full specialisations to read different types with template
template <typename VALUETYPE> FmlIoErrorNumber FieldML_ReadSlab(
	FmlReaderHandle readerHandle, const int *offsets, const int *sizes, VALUETYPE *valueBuffer);

template <> inline FmlIoErrorNumber FieldML_ReadSlab(
	FmlReaderHandle readerHandle, const int *offsets, const int *sizes, double *valueBuffer)
{
	return Fieldml_ReadDoubleSlab(readerHandle, offsets, sizes, valueBuffer);
}

template <> inline FmlIoErrorNumber FieldML_ReadSlab(
	FmlReaderHandle readerHandle, const int *offsets, const int *sizes, int *valueBuffer)
{
	return Fieldml_ReadIntSlab(readerHandle, offsets, sizes, valueBuffer);
}

// TODO : Support order
// ???GRC can order cover subset of ensemble?
template <typename VALUETYPE> int FieldMLReader::readParametersArray(FmlObjectHandle fmlParameters,
	DsMap<VALUETYPE>& parameters)
{
	std::string name = this->getName(fmlParameters);
	FieldmlDataDescriptionType dataDescription = Fieldml_GetParameterDataDescription(fmlSession, fmlParameters);
	if ((dataDescription != FML_DATA_DESCRIPTION_DENSE_ARRAY) &&
		(dataDescription != FML_DATA_DESCRIPTION_DOK_ARRAY))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Unknown data description for parameters %s; must be dense array or DOK array", name.c_str());
		return 0;
	}

	int return_code = 1;
	const int recordIndexCount = (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY) ? 1 : 0;

	const int denseIndexCount = Fieldml_GetParameterIndexCount(fmlSession, fmlParameters, /*isSparse*/0);

	std::vector<HDsLabels> denseIndexLabels(denseIndexCount);
	const int expectedArrayRank = (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY) ? 2 : denseIndexCount;
	int *arrayRawSizes = new int[expectedArrayRank];
	int *arrayOffsets = new int[expectedArrayRank];
	int *arraySizes = new int[expectedArrayRank];
	int arrayRank = 0;

	FmlObjectHandle fmlDataSource = Fieldml_GetDataSource(fmlSession, fmlParameters);
	if (fmlDataSource == FML_INVALID_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Could not get data source for parameters %s", name.c_str());
		return_code = 0;
	}
	else if (FML_DATA_SOURCE_ARRAY != Fieldml_GetDataSourceType(fmlSession, fmlDataSource))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Only supports ArrayDataSource for parameters %s", name.c_str());
		return_code = 0;
	}
	else if ((arrayRank = Fieldml_GetArrayDataSourceRank(fmlSession, fmlDataSource)) != expectedArrayRank)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Data source %s has invalid rank for parameters %s",
			getName(fmlDataSource).c_str(), name.c_str());
		return_code = 0;
	}
	else if ((arrayRank > 0) && (
		(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceRawSizes(fmlSession, fmlDataSource, arrayRawSizes)) ||
		(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceOffsets(fmlSession, fmlDataSource, arrayOffsets)) ||
		(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceSizes(fmlSession, fmlDataSource, arraySizes))))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Failed to get array sizes of data source %s for parameters %s",
			getName(fmlDataSource).c_str(), name.c_str());
		return_code = 0;
	}
	else
	{
		// array size of 0 means 'all of raw size after offset', so calculate effective size.
		for (int r = 0; r < arrayRank; r++)
		{
			if (arraySizes[r] == 0)
				arraySizes[r] = arrayRawSizes[r] - arrayOffsets[r];
		}
		for (int i = 0; i < denseIndexCount; i++)
		{
			FmlObjectHandle fmlDenseIndexEvaluator = Fieldml_GetParameterIndexEvaluator(fmlSession, fmlParameters, i + 1, /*isSparse*/0);
			FmlObjectHandle fmlDenseIndexType = Fieldml_GetValueType(fmlSession, fmlDenseIndexEvaluator);
			int count = Fieldml_GetMemberCount(fmlSession, fmlDenseIndexType);
			if (count != arraySizes[recordIndexCount + i])
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Data source %s size[%d]=%d, differs from size of dense index %s for parameters %s",
					getName(fmlDataSource).c_str(), recordIndexCount + i, arraySizes[recordIndexCount + i],
					getName(fmlDenseIndexEvaluator).c_str(), name.c_str());
				return_code = 0;
				break;
			}
			denseIndexLabels[i] = HDsLabels(this->getLabelsForEnsemble(fmlDenseIndexType));
			if (!denseIndexLabels[i])
			{
				return_code = 0;
				break;
			}
			FmlObjectHandle fmlOrderDataSource = Fieldml_GetParameterIndexOrder(fmlSession, fmlParameters, i + 1);
			if (fmlOrderDataSource != FML_INVALID_HANDLE)
			{
				display_message(WARNING_MESSAGE, "Read FieldML:  Parameters %s dense index %s specifies order. This is not yet supported; results will be incorrect.",
					name.c_str(), getName(fmlDenseIndexEvaluator).c_str());
			}
		}
	}

	int sparseIndexCount = 0;
	std::vector<HDsLabels> sparseIndexLabels;
	FmlObjectHandle fmlKeyDataSource = FML_INVALID_HANDLE;
	int keyArrayRawSizes[2];
	int keyArraySizes[2];
	int keyArrayOffsets[2];
	if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
	{
		sparseIndexCount = Fieldml_GetParameterIndexCount(fmlSession, fmlParameters, /*isSparse*/1);
		fmlKeyDataSource = Fieldml_GetKeyDataSource(fmlSession, fmlParameters);
		if (fmlKeyDataSource == FML_INVALID_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Could not get key data source for parameters %s", name.c_str());
			return_code = 0;
		}
		else if (FML_DATA_SOURCE_ARRAY != Fieldml_GetDataSourceType(fmlSession, fmlKeyDataSource))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Only supports ArrayDataSource for keys for parameters %s", name.c_str());
			return_code = 0;
		}
		else if (Fieldml_GetArrayDataSourceRank(fmlSession, fmlKeyDataSource) != 2)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Key data source %s for parameters %s must be rank 2",
				getName(fmlKeyDataSource).c_str(), name.c_str());
			return_code = 0;
		}
		else if ((FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceRawSizes(fmlSession, fmlKeyDataSource, keyArrayRawSizes)) ||
			(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceSizes(fmlSession, fmlKeyDataSource, keyArraySizes)) ||
			(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceOffsets(fmlSession, fmlKeyDataSource, keyArrayOffsets)))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Failed to get array sizes for key data source %s for parameters %s",
				getName(fmlKeyDataSource).c_str(), name.c_str());
			return_code = 0;
		}
		else
		{
			// zero array size means use raw size less offset
			for (int r = 0; r < 2; ++r)
			{
				if (keyArraySizes[r] == 0)
					keyArraySizes[r] = keyArrayRawSizes[r] - keyArrayOffsets[r];
			}
			if ((keyArraySizes[0] != arraySizes[0]) || (keyArraySizes[1] != sparseIndexCount))
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Invalid array sizes for key data source %s for parameters %s",
					getName(fmlKeyDataSource).c_str(), name.c_str());
				return_code = 0;
			}
		}
		if (return_code)
		{
			for (int i = 0; i < sparseIndexCount; i++)
			{
				FmlObjectHandle fmlSparseIndexEvaluator = Fieldml_GetParameterIndexEvaluator(fmlSession, fmlParameters, i + 1, /*isSparse*/1);
				FmlObjectHandle fmlSparseIndexType = Fieldml_GetValueType(fmlSession, fmlSparseIndexEvaluator);
				sparseIndexLabels.push_back(HDsLabels(getLabelsForEnsemble(fmlSparseIndexType)));
				if (!sparseIndexLabels[i])
				{
					return_code = 0;
					break;
				}
			}
		}
	}

	HDsMapIndexing indexing(parameters.createIndexing());

	int valueBufferSize = 1;
	int totalDenseSize = 1;
	if (arraySizes && arrayOffsets)
	{
		for (int r = 0; r < arrayRank; r++)
		{
			valueBufferSize *= arraySizes[r];
			arrayOffsets[r] = 0;
			if (r >= recordIndexCount)
				totalDenseSize *= arraySizes[r];
		}
	}
	VALUETYPE *valueBuffer = new VALUETYPE[valueBufferSize];
	int *keyBuffer = 0;
	if (return_code)
	{
		if (0 == valueBuffer)
		{
			return_code = 0;
		}
		if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
		{
			keyBuffer = new int [keyArraySizes[0]*keyArraySizes[1]];
			if (0 == keyBuffer)
			{
				return_code = 0;
			}
		}
	}

	FmlReaderHandle fmlReader = FML_INVALID_HANDLE;
	FmlReaderHandle fmlKeyReader = FML_INVALID_HANDLE;
	if (return_code)
	{
		fmlReader = Fieldml_OpenReader(fmlSession, fmlDataSource);
		if (fmlReader == FML_INVALID_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Could not open reader for parameters %s data source %s",
				name.c_str(), getName(fmlDataSource).c_str());
			return_code = 0;
		}
		if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
		{
			fmlKeyReader = Fieldml_OpenReader(fmlSession, fmlKeyDataSource);
			if (fmlKeyReader == FML_INVALID_HANDLE)
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Could not open reader for parameters %s key data source %s",
					name.c_str(), getName(fmlKeyDataSource).c_str());
				return_code = 0;
			}
		}
	}

	if (return_code)
	{
		FmlIoErrorNumber ioResult = FML_IOERR_NO_ERROR;
		ioResult = FieldML_ReadSlab(fmlReader, arrayOffsets, arraySizes, valueBuffer);
		if (ioResult != FML_IOERR_NO_ERROR)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Failed to read data source %s for parameters %s",
				getName(fmlDataSource).c_str(), name.c_str());
			return_code = 0;
		}
		if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
		{
			int keyArrayReadOffsets[2] = { 0, 0 };
			ioResult = Fieldml_ReadIntSlab(fmlKeyReader, keyArrayReadOffsets, keyArraySizes, keyBuffer);
			if (ioResult != FML_IOERR_NO_ERROR)
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Failed to read key data source %s for parameters %s",
					getName(fmlKeyDataSource).c_str(), name.c_str());
				return_code = 0;
			}
		}
	}

	if (return_code)
	{
		const int recordCount = (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY) ? arraySizes[0] : 1;
		for (int record = 0; (record < recordCount) && return_code; ++record)
		{
			for (int i = 0; i < sparseIndexCount; i++)
				indexing->setEntryIdentifier(*(sparseIndexLabels[i]), keyBuffer[record*sparseIndexCount + i]);
			return_code = parameters.setValues(*indexing, totalDenseSize, valueBuffer + record*totalDenseSize);
		}
	}

	if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
		Fieldml_CloseReader(fmlKeyReader);
	Fieldml_CloseReader(fmlReader);
	delete[] valueBuffer;
	delete[] keyBuffer;
	delete[] arraySizes;
	delete[] arrayOffsets;
	delete[] arrayRawSizes;
	return return_code;
}

/**
 * Returns integer map for supplied parameters, reading it from the data source
 * if encountered for the first time.
 *
 * @param fmlParameters  Handle of type FHT_PARAMETER_EVALUATOR.
 * @return  Accessed pointer to integer map, or 0 on failure */
DsMap<int> *FieldMLReader::getEnsembleParameters(FmlObjectHandle fmlParameters)
{
	FmlObjectIntParametersMap::iterator iter = this->intParametersMap.find(fmlParameters);
	if (iter != this->intParametersMap.end())
		return cmzn::Access(cmzn::GetImpl(iter->second));

	std::string name = getName(fmlParameters);
	if (Fieldml_GetObjectType(this->fmlSession, fmlParameters) != FHT_PARAMETER_EVALUATOR)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getEnsembleParameters.  %s is not a parameter evaluator", name.c_str());
		return 0;
	}
	FmlObjectHandle fmlValueType = Fieldml_GetValueType(fmlSession, fmlParameters);
	FieldmlHandleType valueClass = Fieldml_GetObjectType(fmlSession, fmlValueType);
	if (valueClass != FHT_ENSEMBLE_TYPE)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getEnsembleParameters.  %s is not ensemble-valued", name.c_str());
		return 0;
	}

	if (verbose)
		display_message(INFORMATION_MESSAGE, "Reading ensemble parameters %s\n", name.c_str());
	int indexCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlParameters);
	std::vector<HDsLabels> indexingLabelsVector;
	int return_code = 1;
	for (int indexNumber = 1; indexNumber <= indexCount; ++indexNumber)
	{
		FmlObjectHandle fmlIndexEvaluator = Fieldml_GetIndexEvaluator(this->fmlSession, fmlParameters, indexNumber);
		FmlObjectHandle fmlEnsembleType = Fieldml_GetValueType(this->fmlSession, fmlIndexEvaluator);
		if ((FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlIndexEvaluator)) ||
			(FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlEnsembleType)))
		{
			display_message(WARNING_MESSAGE, "FieldMLReader::getEnsembleParameters:  Index %d (%s) of parameters %s is not an ensemble-valued argument evaluator",
				indexNumber, getName(fmlIndexEvaluator).c_str(), name.c_str());
			return_code = 0;
		}
		if (verbose)
			display_message(INFORMATION_MESSAGE, "  Index %d = %s\n", indexNumber, getName(fmlIndexEvaluator).c_str());
		indexingLabelsVector.push_back(HDsLabels(this->getLabelsForEnsemble(fmlEnsembleType)));
	}
	if (!return_code)
		return 0;
	HDsMapInt parameters(DsMap<int>::create(indexingLabelsVector));
	parameters->setName(name);
	return_code = this->readParametersArray(fmlParameters, *parameters);
	if (!return_code)
		return 0;
	this->setProcessed(fmlParameters);
	this->intParametersMap[fmlParameters] = parameters;
	return cmzn::Access(cmzn::GetImpl(parameters));
}

/**
 * Returns double map for supplied parameters, reading it from the data source
 * if encountered for the first time.
 *
 * @param fmlParameters  Handle of type FHT_PARAMETER_EVALUATOR.
 * @return  Accessed pointer to double map, or 0 on failure */
DsMap<double> *FieldMLReader::getContinuousParameters(FmlObjectHandle fmlParameters)
{
	FmlObjectDoubleParametersMap::iterator iter = this->doubleParametersMap.find(fmlParameters);
	if (iter != this->doubleParametersMap.end())
		return cmzn::Access(cmzn::GetImpl(iter->second));

	std::string name = getName(fmlParameters);
	if (Fieldml_GetObjectType(this->fmlSession, fmlParameters) != FHT_PARAMETER_EVALUATOR)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getContinuousParameters.  %s is not a parameter evaluator", name.c_str());
		return 0;
	}
	FmlObjectHandle fmlValueType = Fieldml_GetValueType(fmlSession, fmlParameters);
	FieldmlHandleType valueClass = Fieldml_GetObjectType(fmlSession, fmlValueType);
	if (valueClass != FHT_CONTINUOUS_TYPE)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getContinuousParameters.  %s is not continuous-valued", name.c_str());
		return 0;
	}
	FmlObjectHandle fmlComponentsType = Fieldml_GetTypeComponentEnsemble(fmlSession, fmlValueType);
	if (fmlComponentsType != FML_INVALID_OBJECT_HANDLE)
	{
		display_message(WARNING_MESSAGE, "FieldMLReader::getContinuousParameters:  Cannot read non-scalar parameters %s", name.c_str());
		return 0;
	}

	if (verbose)
		display_message(INFORMATION_MESSAGE, "Reading continuous parameters %s\n", name.c_str());
	int indexCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlParameters);
	std::vector<HDsLabels> indexingLabelsVector;
	int return_code = 1;
	for (int indexNumber = 1; indexNumber <= indexCount; ++indexNumber)
	{
		FmlObjectHandle fmlIndexEvaluator = Fieldml_GetIndexEvaluator(this->fmlSession, fmlParameters, indexNumber);
		FmlObjectHandle fmlEnsembleType = Fieldml_GetValueType(this->fmlSession, fmlIndexEvaluator);
		if ((FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlIndexEvaluator)) ||
			(FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlEnsembleType)))
		{
			display_message(WARNING_MESSAGE, "FieldMLReader::getContinuousParameters:  Index %d (%s) of parameters %s is not an ensemble-valued argument evaluator",
				indexNumber, getName(fmlIndexEvaluator).c_str(), name.c_str());
			return_code = 0;
		}
		if (verbose)
			display_message(INFORMATION_MESSAGE, "  Index %d = %s\n", indexNumber, getName(fmlIndexEvaluator).c_str());
		indexingLabelsVector.push_back(HDsLabels(this->getLabelsForEnsemble(fmlEnsembleType)));
	}
	if (!return_code)
		return 0;
	HDsMapDouble parameters(DsMap<double>::create(indexingLabelsVector));
	parameters->setName(name);
	return_code = this->readParametersArray(fmlParameters, *parameters);
	if (!return_code)
		return 0;
	this->setProcessed(fmlParameters);
	this->doubleParametersMap[fmlParameters] = parameters;
	return cmzn::Access(cmzn::GetImpl(parameters));
}

int FieldMLReader::readNodes(FmlObjectHandle fmlNodesArgumentIn)
{
	if (FML_INVALID_OBJECT_HANDLE != this->fmlNodesType)
		return CMZN_ERROR_ALREADY_EXISTS;
	FmlObjectHandle fmlNodesTypeIn = Fieldml_GetValueType(this->fmlSession, fmlNodesArgumentIn);
	HDsLabels nodesLabels(this->getLabelsForEnsemble(fmlNodesTypeIn));
	if (!nodesLabels)
		return CMZN_ERROR_GENERAL;
	int return_code = CMZN_OK;
	cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(this->field_module, CMZN_FIELD_DOMAIN_TYPE_NODES);
	cmzn_nodetemplate_id nodetemplate = cmzn_nodeset_create_nodetemplate(nodeset);
	DsLabelIterator *nodesLabelIterator = nodesLabels->createLabelIterator();
	if (!nodesLabelIterator)
		return_code = CMZN_ERROR_MEMORY;
	else
	{
		while (nodesLabelIterator->increment())
		{
			DsLabelIdentifier nodeIdentifier = nodesLabelIterator->getIdentifier();
			cmzn_node_id node = cmzn_nodeset_create_node(nodeset, nodeIdentifier, nodetemplate);
			if (!node)
			{
				return_code = CMZN_ERROR_MEMORY;
				break;
			}
			cmzn_node_destroy(&node);
		}
	}
	cmzn::Deaccess(nodesLabelIterator);
	cmzn_nodetemplate_destroy(&nodetemplate);
	cmzn_nodeset_destroy(&nodeset);
	if (CMZN_OK == return_code)
	{
		this->fmlNodesType = fmlNodesTypeIn;
		this->fmlNodesArgument = fmlNodesArgumentIn;
	}
	return return_code;
}

int FieldMLReader::readGlobals()
{
	// if nodes not found, use legacy behaviour i.e. nodes are free parameter index
	std::string nodesTypeName("nodes");
	std::string nodesArgumentName = nodesTypeName + ".argument";
	FmlObjectHandle fmlPossibleNodesType = Fieldml_GetObjectByName(this->fmlSession, nodesTypeName.c_str());
	FmlObjectHandle fmlPossibleNodesArgument = Fieldml_GetObjectByName(this->fmlSession, nodesArgumentName.c_str());
	if ((FML_INVALID_OBJECT_HANDLE != fmlPossibleNodesType) &&
		(FML_INVALID_OBJECT_HANDLE != fmlPossibleNodesArgument))
	{
		if ((FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlPossibleNodesType)) ||
			(FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlPossibleNodesArgument)) ||
			(Fieldml_GetValueType(this->fmlSession, fmlPossibleNodesArgument) != fmlPossibleNodesType))
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  Non-standard definition of %s or %s. Ignoring.",
				nodesTypeName.c_str(), nodesArgumentName.c_str());
		}
		else
		{
			int return_code = this->readNodes(fmlPossibleNodesArgument);
			if (CMZN_OK != return_code)
				return 0;
		}
	}
	// if node_derivatives not found, use legacy behaviour i.e. no node derivatives
	std::string nodeDerivativesTypeName("node_derivatives");
	std::string nodeDerivativesArgumentName = nodeDerivativesTypeName + ".argument";
	this->fmlNodeDerivativesType = Fieldml_GetObjectByName(this->fmlSession, nodeDerivativesTypeName.c_str());
	this->fmlNodeDerivativesArgument = Fieldml_GetObjectByName(this->fmlSession, nodeDerivativesArgumentName.c_str());
	if ((FML_INVALID_OBJECT_HANDLE != this->fmlNodeDerivativesType) ||
		(FML_INVALID_OBJECT_HANDLE != this->fmlNodeDerivativesArgument))
	{
		if ((FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, this->fmlNodeDerivativesType)) ||
			(FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, this->fmlNodeDerivativesArgument)) ||
			(Fieldml_GetValueType(this->fmlSession, this->fmlNodeDerivativesArgument) != this->fmlNodeDerivativesType))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Non-standard definition of %s or %s.",
				nodeDerivativesTypeName.c_str(), nodeDerivativesArgumentName.c_str());
			return 0;
		}
		HDsLabels tmpNodeDerivatives(this->getLabelsForEnsemble(this->fmlNodeDerivativesType));
		if (!tmpNodeDerivatives)
			return 0;
	}
	// if node_versions not found, use legacy behaviour i.e. no node versions
	std::string nodeVersionsTypeName("node_versions");
	std::string nodeVersionsArgumentName = nodeVersionsTypeName + ".argument";
	this->fmlNodeVersionsType = Fieldml_GetObjectByName(this->fmlSession, nodeVersionsTypeName.c_str());
	this->fmlNodeVersionsArgument = Fieldml_GetObjectByName(this->fmlSession, nodeVersionsArgumentName.c_str());
	if ((FML_INVALID_OBJECT_HANDLE != this->fmlNodeVersionsType) ||
		(FML_INVALID_OBJECT_HANDLE != this->fmlNodeVersionsArgument))
	{
		if ((FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, this->fmlNodeVersionsType)) ||
			(FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, this->fmlNodeVersionsArgument)) ||
			(Fieldml_GetValueType(this->fmlSession, this->fmlNodeVersionsArgument) != this->fmlNodeVersionsType))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Non-standard definition of %s or %s.",
				nodeVersionsTypeName.c_str(), nodeVersionsArgumentName.c_str());
			return 0;
		}
		HDsLabels tmpNodeVersions(this->getLabelsForEnsemble(this->fmlNodeVersionsType));
		if (!tmpNodeVersions)
			return 0;
	}
	return 1;
}

int FieldMLReader::readMeshes()
{
	const int meshCount = Fieldml_GetObjectCount(fmlSession, FHT_MESH_TYPE);
	if (meshCount != 1)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Require 1 mesh type, %d found", meshCount);
		return 0;
	}
	int return_code = 1;
	for (int meshIndex = 1; (meshIndex <= meshCount) && return_code; meshIndex++)
	{
		FmlObjectHandle fmlMeshType = Fieldml_GetObject(fmlSession, FHT_MESH_TYPE, meshIndex);
		std::string name = getName(fmlMeshType);

		FmlObjectHandle fmlMeshChartType = Fieldml_GetMeshChartType(fmlSession, fmlMeshType);
		FmlObjectHandle fmlMeshChartComponentType = Fieldml_GetTypeComponentEnsemble(fmlSession, fmlMeshChartType);
		meshDimension = Fieldml_GetMemberCount(fmlSession, fmlMeshChartComponentType);
		if ((meshDimension < 1) || (meshDimension > 3))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Invalid dimension %d for mesh type %s", meshDimension, name.c_str());
			return_code = 0;
			break;
		}

		if (verbose)
		{
			display_message(INFORMATION_MESSAGE, "Reading mesh '%s' dimension %d\n", name.c_str(), meshDimension);
		}

		fmlElementsType = Fieldml_GetMeshElementsType(fmlSession, fmlMeshType);
		HDsLabels elementsLabels(getLabelsForEnsemble(fmlElementsType));
		if (verbose)
		{
			display_message(INFORMATION_MESSAGE, "Defining %d elements from %s\n",
				elementsLabels->getSize(), getName(fmlElementsType).c_str());
		}

		// determine element shape mapping

		FmlObjectHandle fmlShapeEvaluator = Fieldml_GetMeshShapes(fmlSession, fmlMeshType);
		cmzn_element_shape_type const_shape_type = CMZN_ELEMENT_SHAPE_TYPE_INVALID;
		HDsMapInt elementShapeParameters; // used only if shape evaluator uses indirect map
		if (fmlShapeEvaluator == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Missing shape evaluator for mesh type %s", name.c_str());
			return_code = 0;
		}
		else if (FHT_BOOLEAN_TYPE != Fieldml_GetObjectType(fmlSession, Fieldml_GetValueType(fmlSession, fmlShapeEvaluator)))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Non-boolean-valued shape evaluator for mesh type %s", name.c_str());
			return_code = 0;
		}
		else
		{
			// Note: external evaluator arguments are assumed to be 'used'
			int argumentCount = Fieldml_GetArgumentCount(fmlSession, fmlShapeEvaluator, /*isBound*/0, /*isUsed*/1);
			FmlObjectHandle fmlChartArgument = FML_INVALID_OBJECT_HANDLE;
			FmlObjectHandle fmlChartArgumentValue = FML_INVALID_OBJECT_HANDLE;
			FmlObjectHandle fmlElementsArgument = FML_INVALID_OBJECT_HANDLE;
			FmlObjectHandle fmlElementsArgumentValue = FML_INVALID_OBJECT_HANDLE;
			if (argumentCount > 0)
			{
				fmlChartArgument = Fieldml_GetArgument(fmlSession, fmlShapeEvaluator, 1, /*isBound*/0, /*isUsed*/1);
				fmlChartArgumentValue = Fieldml_GetValueType(fmlSession, fmlChartArgument);
			}
			if (argumentCount == 2)
			{
				fmlElementsArgument = Fieldml_GetArgument(fmlSession, fmlShapeEvaluator, 2, /*isBound*/0, /*isUsed*/1);
				fmlElementsArgumentValue = Fieldml_GetValueType(fmlSession, fmlElementsArgument);
				if (fmlElementsArgumentValue != fmlElementsType)
				{
					FmlObjectHandle tmp = fmlElementsArgument;
					fmlElementsArgument = fmlChartArgument;
					fmlChartArgument = tmp;
					tmp = fmlElementsArgumentValue;
					fmlElementsArgumentValue = fmlChartArgumentValue;
					fmlChartArgumentValue = tmp;
				}
			}
			FieldmlHandleType shapeEvaluatorValueType = Fieldml_GetObjectType(fmlSession, Fieldml_GetValueType(fmlSession, fmlShapeEvaluator));
			FieldmlHandleType chartArgumentValueType = (argumentCount > 0) ? Fieldml_GetObjectType(fmlSession, fmlChartArgumentValue) : FHT_UNKNOWN;
			if (((argumentCount != 1) && (argumentCount != 2)) ||
				(FHT_BOOLEAN_TYPE != shapeEvaluatorValueType) ||
				(FHT_CONTINUOUS_TYPE != chartArgumentValueType) ||
				(Fieldml_GetTypeComponentCount(fmlSession, fmlChartArgumentValue) != meshDimension) ||
				((argumentCount == 2) && (fmlElementsArgumentValue != fmlElementsType)))
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Shape evaluator %s for mesh type %s must be a boolean evaluator with chart argument, plus optionally mesh elements argument.",
					getName(fmlShapeEvaluator).c_str(), name.c_str());
				return_code = 0;
			}
			else
			{
				FieldmlHandleType shapeEvaluatorType = Fieldml_GetObjectType(fmlSession, fmlShapeEvaluator);
				switch (shapeEvaluatorType)
				{
				case FHT_EXTERNAL_EVALUATOR:
					{
						// Case 1. single recognised shape external evaluator = all elements same shape
						const_shape_type = getElementShapeFromFieldmlName(getName(fmlShapeEvaluator).c_str());
						if (const_shape_type == CMZN_ELEMENT_SHAPE_TYPE_INVALID)
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Unrecognised element shape evaluator %s for mesh type %s.",
								getName(fmlShapeEvaluator).c_str(), name.c_str());
							return_code = 0;
							break;
						}
					} break;
				case FHT_PIECEWISE_EVALUATOR:
					{
						FmlObjectHandle fmlIndexArgument = Fieldml_GetIndexEvaluator(fmlSession, fmlShapeEvaluator, 1);
						FmlObjectHandle fmlElementToShapeParameter = FML_INVALID_OBJECT_HANDLE;
						if (fmlIndexArgument == fmlElementsArgument)
						{
							// Case 2. piecewise over elements, directly mapping to recognised shape external evaluators
							// nothing more to do
						}
						else if ((1 != Fieldml_GetBindCount(fmlSession, fmlShapeEvaluator)) ||
							(Fieldml_GetBindArgument(fmlSession, fmlShapeEvaluator, 1) != fmlIndexArgument) ||
							(FML_INVALID_OBJECT_HANDLE == (fmlElementToShapeParameter = Fieldml_GetBindEvaluator(fmlSession, fmlShapeEvaluator, 1))) ||
							(FHT_PARAMETER_EVALUATOR != Fieldml_GetObjectType(fmlSession, fmlElementToShapeParameter)) ||
							(Fieldml_GetValueType(fmlSession, fmlElementToShapeParameter) !=
								Fieldml_GetValueType(fmlSession, fmlIndexArgument)))
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Shape evaluator %s for mesh type %s has unrecognised piecewise form.",
								getName(fmlShapeEvaluator).c_str(), name.c_str());
							return_code = 0;
						}
						else
						{
							// Case 3. piecewise over 'shape ensemble', indirectly mapping from parameters mapping from element
							elementShapeParameters = HDsMapInt(this->getEnsembleParameters(fmlElementToShapeParameter));
							if (!elementShapeParameters)
							{
								display_message(ERROR_MESSAGE, "Read FieldML:  Invalid element to shape parameters %s for shape evaluator %s of mesh type %s.",
									getName(fmlElementToShapeParameter).c_str(), getName(fmlShapeEvaluator).c_str(), name.c_str());
							}
						}
					} break;
				default:
					display_message(ERROR_MESSAGE, "Read FieldML:  Shape evaluator %s for mesh type %s has unrecognised form.",
						getName(fmlShapeEvaluator).c_str(), name.c_str());
					return_code = 0;
					break;
				}
			}
		}

		// create elements in the mesh of given dimension

		cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(field_module, meshDimension);
		cmzn_elementtemplate_id elementtemplate = cmzn_mesh_create_elementtemplate(mesh);

		FmlObjectHandle fmlLastElementShapeEvaluator = FML_INVALID_OBJECT_HANDLE;
		cmzn_element_shape_type last_shape_type = CMZN_ELEMENT_SHAPE_TYPE_INVALID;

		HDsMapIndexing elementShapeParametersIndexing(elementShapeParameters ? elementShapeParameters->createIndexing() : 0);

		DsLabelIterator *elementsLabelIterator = elementsLabels->createLabelIterator();
		if (!elementsLabelIterator)
			return_code = 0;
		else
		{
			while (elementsLabelIterator->increment())
			{
				int elementIdentifier = elementsLabelIterator->getIdentifier();
				cmzn_element_shape_type shape_type = const_shape_type;
				if (const_shape_type == CMZN_ELEMENT_SHAPE_TYPE_INVALID)
				{
					int shapeIdentifier = elementIdentifier;
					if (elementShapeParameters && (
						(CMZN_OK != elementShapeParametersIndexing->setEntry(*elementsLabelIterator)) ||
						(CMZN_OK != elementShapeParameters->getValues(*elementShapeParametersIndexing, 1, &shapeIdentifier))))
					{
						display_message(ERROR_MESSAGE, "Read FieldML:  Failed to map shape of element %d in mesh type %s.",
							elementIdentifier, name.c_str());
						return_code = 0;
						break;
					}
					else
					{
						FmlObjectHandle fmlElementShapeEvaluator =
							Fieldml_GetElementEvaluator(fmlSession, fmlShapeEvaluator, shapeIdentifier, /*allowDefault*/1);
						if (fmlElementShapeEvaluator == fmlLastElementShapeEvaluator)
						{
							shape_type = last_shape_type;
						}
						else
						{
							shape_type = getElementShapeFromFieldmlName(getName(fmlElementShapeEvaluator).c_str());
							fmlLastElementShapeEvaluator = fmlElementShapeEvaluator;
						}
						if (shape_type == CMZN_ELEMENT_SHAPE_TYPE_INVALID)
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Could not get shape of element %d in mesh type %s.",
								elementIdentifier, name.c_str());
							return_code = 0;
							break;
						}
					}
				}
				if (shape_type != last_shape_type)
				{
					if (!(cmzn_elementtemplate_set_element_shape_type(elementtemplate, shape_type)))
					{
						return_code = 0;
						break;
					}
					last_shape_type = shape_type;
				}
				if (!cmzn_mesh_define_element(mesh, elementIdentifier, elementtemplate))
				{
					return_code = 0;
					break;
				}
			}
		}
		cmzn::Deaccess(elementsLabelIterator);
		cmzn_elementtemplate_destroy(&elementtemplate);
		cmzn_mesh_destroy(&mesh);
	}
	return return_code;
}

ElementFieldComponent *FieldMLReader::getElementFieldComponent(cmzn_mesh_id mesh,
	FmlObjectHandle fmlEvaluator, FmlObjectHandle fmlNodeParametersArgument,
	FmlObjectHandle fmlNodesArgument, FmlObjectHandle fmlElementArgument)
{
	EvaluatorElementFieldComponentMap::iterator iter = componentMap.find(fmlEvaluator);
	if (iter != componentMap.end())
	{
		return iter->second;
	}

	USE_PARAMETER(mesh); // GRC should remove altogether
	std::string evaluatorName = getName(fmlEvaluator);
	FmlObjectHandle fmlEvaluatorType = Fieldml_GetValueType(fmlSession, fmlEvaluator);
	if ((FHT_REFERENCE_EVALUATOR != Fieldml_GetObjectType(fmlSession, fmlEvaluator)) ||
		(FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(fmlSession, fmlEvaluatorType)))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  getElementFieldComponent argument %s is not a real-valued reference evaluator.",
			evaluatorName.c_str());
		return 0;
	}
	FmlObjectHandle fmlInterpolator = Fieldml_GetReferenceSourceEvaluator(fmlSession, fmlEvaluator);
	std::string interpolatorName = getDeclaredName(fmlInterpolator);
	std::string interpolatorLocalName = getName(fmlInterpolator);
	const char *interpolator_name = interpolatorName.c_str();
	int basis_index = -1;
	for (int i = 0; i < numLibraryBases; i++)
	{
		if (0 == strcmp(interpolator_name, libraryBases[i].fieldmlBasisEvaluatorName))
		{
			basis_index = i;
			break;
		}
	}
	if (basis_index < 0)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Reference evaluator %s references unknown basis interpolator %s (local name %s).",
			evaluatorName.c_str(), interpolator_name, interpolatorLocalName.c_str());
		return 0;
	}

	// Note: external evaluator arguments are assumed to be 'used'
	int interpolatorArgumentCount = Fieldml_GetArgumentCount(fmlSession, fmlInterpolator, /*isBound*/0, /*isUsed*/1);
	// GRC Hermite scale factors change here
	if (interpolatorArgumentCount != 2)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Reference evaluator %s source %s (local name %s) has %d argument(s); 2 are expected.",
			evaluatorName.c_str(), interpolator_name, interpolatorLocalName.c_str(), interpolatorArgumentCount);
		return 0;
	}
	FmlObjectHandle chartArgument = FML_INVALID_OBJECT_HANDLE;
	FmlObjectHandle parametersArgument = FML_INVALID_OBJECT_HANDLE;
	for (int i = 1; i <= interpolatorArgumentCount; i++)
	{
		FmlObjectHandle arg = Fieldml_GetArgument(fmlSession, fmlInterpolator, i, /*isBound*/0, /*isUsed*/1);
		std::string argName = getDeclaredName(arg);
		if (0 == argName.compare(libraryChartArgumentNames[meshDimension]))
		{
			if (chartArgument != FML_INVALID_OBJECT_HANDLE)
			{
				chartArgument = FML_INVALID_OBJECT_HANDLE;
				break;
			}
			chartArgument = arg;
		}
		else
		{
			// GRC Hermite scale factors change here
			parametersArgument = arg;
		}
	}
	if ((FML_INVALID_OBJECT_HANDLE == chartArgument) ||
		(FML_INVALID_OBJECT_HANDLE == parametersArgument))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Reference evaluator %s source %s (local name %s) is not a regular basis interpolator over %s.",
			evaluatorName.c_str(), interpolator_name, interpolatorLocalName.c_str(), libraryChartArgumentNames[meshDimension]);
		return 0;
	}

	FmlObjectHandle fmlLocalChartEvaluator = Fieldml_GetBindByArgument(fmlSession, fmlEvaluator, chartArgument);
	FmlObjectHandle fmlElementParametersEvaluator = Fieldml_GetBindByArgument(fmlSession, fmlEvaluator, parametersArgument);
	// exactly one mesh expected
	if (1 != Fieldml_GetObjectCount(fmlSession, FHT_MESH_TYPE))
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getElementFieldComponent:  Only supports 1 mesh type");
		return 0;
	}
	int return_code = 1;
	FmlObjectHandle fmlMeshType = Fieldml_GetObject(fmlSession, FHT_MESH_TYPE, /*meshIndex*/1);
	FmlObjectHandle fmlMeshChartType = Fieldml_GetMeshChartType(fmlSession, fmlMeshType);
	if ((fmlLocalChartEvaluator == FML_INVALID_OBJECT_HANDLE) ||
		(Fieldml_GetObjectType(fmlSession, fmlLocalChartEvaluator) != FHT_ARGUMENT_EVALUATOR) ||
		(Fieldml_GetValueType(fmlSession, fmlLocalChartEvaluator) != fmlMeshChartType))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Evaluator %s does not bind local mesh chart argument to generic chart argument %s.",
			evaluatorName.c_str(), libraryChartArgumentNames[meshDimension]);
		return_code = 0;
	}
	if (fmlElementParametersEvaluator == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Evaluator %s does not bind to parameters argument %s for basis interpolator %s.",
			evaluatorName.c_str(), getDeclaredName(parametersArgument).c_str(), interpolator_name);
		return_code = 0;
	}
	int evaluatorBindCount = Fieldml_GetBindCount(fmlSession, fmlEvaluator);
	// GRC Hermite scale factors change here
	if (2 != evaluatorBindCount)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Evaluator %s has %d bindings; interpolator %s requires 2 bindings to chart and parameters.",
			evaluatorName.c_str(), evaluatorBindCount, interpolator_name);
		return_code = 0;
	}
	if (!return_code)
		return 0;

	std::string elementParametersName = getName(fmlElementParametersEvaluator);

	//FmlObjectHandle fmlParametersType = Fieldml_GetValueType(fmlSession, parametersArgument);
	//int parametersComponentCount = Fieldml_GetTypeComponentCount(fmlSession, fmlParametersType);
	if ((Fieldml_GetObjectType(fmlSession, fmlElementParametersEvaluator) != FHT_AGGREGATE_EVALUATOR) ||
		(1 != Fieldml_GetIndexEvaluatorCount(fmlSession, fmlElementParametersEvaluator)))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Expect evaluator %s parameter source %s to be an AggregateEvaluator with 1 index",
			evaluatorName.c_str(), elementParametersName.c_str());
		return 0;
	}
	FmlObjectHandle fmlElementParametersIndexArgument = Fieldml_GetIndexEvaluator(fmlSession, fmlElementParametersEvaluator, 1);
	FmlObjectHandle fmlElementParametersIndexType = Fieldml_GetValueType(this->fmlSession, fmlElementParametersIndexArgument);
	if ((Fieldml_GetObjectType(this->fmlSession, fmlElementParametersIndexArgument) != FHT_ARGUMENT_EVALUATOR) ||
		(Fieldml_GetObjectType(this->fmlSession, fmlElementParametersIndexType) != FHT_ENSEMBLE_TYPE))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s index %s must be an ensemble-valued ArgumentEvaluator",
			elementParametersName.c_str(), getName(fmlElementParametersIndexArgument).c_str());
		return 0;
	}
	FmlObjectHandle fmlTempNodeParameters = Fieldml_GetDefaultEvaluator(fmlSession, fmlElementParametersEvaluator);
	if ((fmlTempNodeParameters == FML_INVALID_OBJECT_HANDLE) ||
		(0 != Fieldml_GetEvaluatorCount(fmlSession, fmlElementParametersEvaluator)))
	{
		display_message(ERROR_MESSAGE, "Read FieldML (Current Limitation):  Evaluator %s element parameter source %s must use only default component evaluator",
			evaluatorName.c_str(), elementParametersName.c_str());
		return 0;
	}
	if (fmlTempNodeParameters != fmlNodeParametersArgument)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Evaluator %s element parameter source %s default evaluator %s does not match nodal parameters argument %s",
			evaluatorName.c_str(), elementParametersName.c_str(), getName(fmlTempNodeParameters).c_str(), getName(fmlNodeParametersArgument).c_str());
		return 0;
	}

	cmzn_elementbasis_id elementBasis = cmzn_fieldmodule_create_elementbasis(field_module, meshDimension, libraryBases[basis_index].functionType[0]);
	if (!libraryBases[basis_index].homogeneous)
	{
		for (int dimension = 2; dimension <= meshDimension; dimension++)
		{
			cmzn_elementbasis_set_function_type(elementBasis, dimension,
				libraryBases[basis_index].functionType[dimension - 1]);
		}
	}
	int basisNodeCount = cmzn_elementbasis_get_number_of_nodes(elementBasis);
	int basisFunctionCount = cmzn_elementbasis_get_number_of_functions(elementBasis);

	// Handle bindings to element parameters
	int usedBindCount = 0;

	// 1. Local-to-global node map
	FmlObjectHandle fmlLocalToGlobalNodeEvaluator =
		Fieldml_GetBindByArgument(this->fmlSession, fmlElementParametersEvaluator, fmlNodesArgument);
	FmlObjectHandle fmlLocalNodesArgument = FML_INVALID_OBJECT_HANDLE;
	HDsMapInt localToGlobalNodeMap;
	if (FML_INVALID_OBJECT_HANDLE == fmlLocalToGlobalNodeEvaluator)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s does not bind local-to-global node map to nodes argument %s.",
			elementParametersName.c_str(), getName(fmlNodesArgument).c_str());
		return_code = 0;
	}
	else
	{
		++usedBindCount;
		if ((FHT_PARAMETER_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlLocalToGlobalNodeEvaluator)) ||
			(2 != Fieldml_GetIndexEvaluatorCount(fmlSession, fmlLocalToGlobalNodeEvaluator)))
		{
			return_code = 0;
		}
		else
		{
			// element & localnode indexes could be in any order
			FmlObjectHandle fmlIndexEvaluator1 = Fieldml_GetIndexEvaluator(fmlSession, fmlLocalToGlobalNodeEvaluator, 1);
			FmlObjectHandle fmlIndexEvaluator2 = Fieldml_GetIndexEvaluator(fmlSession, fmlLocalToGlobalNodeEvaluator, 2);
			if (fmlIndexEvaluator1 == fmlElementArgument)
				fmlLocalNodesArgument = fmlIndexEvaluator2;
			else if (fmlIndexEvaluator2 == fmlElementArgument)
				fmlLocalNodesArgument = fmlIndexEvaluator1;
			if ((fmlLocalNodesArgument == FML_INVALID_OBJECT_HANDLE) ||
				((fmlLocalNodesArgument != fmlElementParametersIndexArgument) && (basisNodeCount == basisFunctionCount)))
			{
				return_code = 0;
			}
			else
			{
				FmlObjectHandle fmlLocalNodesType = Fieldml_GetValueType(fmlSession, fmlLocalNodesArgument);
				int localNodeCount = Fieldml_GetMemberCount(fmlSession, fmlLocalNodesType);
				if (localNodeCount != basisNodeCount)
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s local-to-global node map %s "
						"uses %d local nodes, not %d as expected for basis %s",
						elementParametersName.c_str(), getName(fmlLocalToGlobalNodeEvaluator).c_str(),
						localNodeCount, basisNodeCount, interpolator_name);
					return_code = 0;
				}
			}
		}
		if (return_code)
		{
			cmzn::SetImpl(localToGlobalNodeMap, this->getEnsembleParameters(fmlLocalToGlobalNodeEvaluator));
			if (!localToGlobalNodeMap)
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s local-to-global node map %s could not be read.",
					elementParametersName.c_str(), getName(fmlLocalToGlobalNodeEvaluator).c_str());
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s bound evaluator %s is not a valid local-to-global node map.",
				elementParametersName.c_str(), getName(fmlLocalToGlobalNodeEvaluator).c_str());
		}
	}

	// 2. Optional element parameter index to local node map (Hermite only)
	FmlObjectHandle fmlParameterIndexToLocalNodeEvaluator = FML_INVALID_OBJECT_HANDLE;
	HDsMapInt parameterIndexToLocalNodeMap;
	if ((fmlLocalNodesArgument != FML_INVALID_OBJECT_HANDLE) && 
		(fmlLocalNodesArgument != fmlElementParametersIndexArgument))
	{
		fmlParameterIndexToLocalNodeEvaluator =
			Fieldml_GetBindByArgument(this->fmlSession, fmlElementParametersEvaluator, fmlLocalNodesArgument);
		if (FML_INVALID_OBJECT_HANDLE == fmlParameterIndexToLocalNodeEvaluator)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s does not bind parameter-to-local-node map to argument %s.",
				elementParametersName.c_str(), getName(fmlLocalNodesArgument).c_str());
			return_code = 0;
		}
		else
		{
			++usedBindCount;
			if ((FHT_PARAMETER_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlParameterIndexToLocalNodeEvaluator)) ||
				(1 != Fieldml_GetIndexEvaluatorCount(fmlSession, fmlParameterIndexToLocalNodeEvaluator)) ||
				(Fieldml_GetIndexEvaluator(fmlSession, fmlParameterIndexToLocalNodeEvaluator, 1) != fmlElementParametersIndexArgument))
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s bound evaluator %s is not a valid parameter-to-local-node map.",
					elementParametersName.c_str(), getName(fmlParameterIndexToLocalNodeEvaluator).c_str());
				return_code = 0;
			}
			else
			{
				cmzn::SetImpl(parameterIndexToLocalNodeMap, this->getEnsembleParameters(fmlParameterIndexToLocalNodeEvaluator));
				if (!parameterIndexToLocalNodeMap)
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s parameter-to-local-node map %s could not be read.",
					elementParametersName.c_str(), getName(fmlParameterIndexToLocalNodeEvaluator).c_str());
					return_code = 0;
				}
				else
				{
					// Hermite: check parameter-to-local-node map is in order 1 1 1 .. 2 2 2 .. 3 3 3 .. for
					// Could later add support for different orderings if required
					int *nodeIdentifiers = new int[basisFunctionCount];
					HDsMapIndexing mapIndexing(parameterIndexToLocalNodeMap->createIndexing());
					if (!parameterIndexToLocalNodeMap->getValues(*mapIndexing, basisFunctionCount, nodeIdentifiers))
					{
						display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s parameter-to-local-node map %s contents are sparse.",
							elementParametersName.c_str(), getName(fmlParameterIndexToLocalNodeEvaluator).c_str());
						return_code = 0;
					}
					else
					{
						// expect same number of parameters per node; not valid for future quadratic Hermite and other bases
						// GRC to fix
						const int basisFunctionsPerNode = basisFunctionCount / basisNodeCount;
						int expectedNodeIdentifier = 0;
						for (int d = 0; d < basisFunctionCount; ++d)
						{
							if ((d % basisFunctionsPerNode) == 0)
								++expectedNodeIdentifier;
							if (nodeIdentifiers[d] != expectedNodeIdentifier)
							{
								display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s parameter-to-local-node map %s has unsupported sequence.",
									elementParametersName.c_str(), getName(fmlParameterIndexToLocalNodeEvaluator).c_str());
								return_code = 0;
								break;
							}
						}
					}
					delete[] nodeIdentifiers;
				}
			}
		}
	}

	// 3. Optional node derivative constant or map
	cmzn_node_value_label constantNodeDerivative = CMZN_NODE_VALUE_LABEL_VALUE;
	HDsMapInt nodeDerivativesMap;
	FmlObjectHandle fmlNodeDerivativesEvaluator = FML_INVALID_OBJECT_HANDLE;
	if ((FML_INVALID_OBJECT_HANDLE != this->fmlNodeDerivativesArgument) &&
		(FML_INVALID_OBJECT_HANDLE != (fmlNodeDerivativesEvaluator =
			Fieldml_GetBindByArgument(this->fmlSession, fmlElementParametersEvaluator, this->fmlNodeDerivativesArgument))))
	{
		++usedBindCount;
		FieldmlHandleType objectType = Fieldml_GetObjectType(this->fmlSession, fmlNodeDerivativesEvaluator);
		if (FHT_CONSTANT_EVALUATOR == objectType)
		{
			char *valueString = Fieldml_GetConstantEvaluatorValueString(this->fmlSession, fmlNodeDerivativesEvaluator);
			if (valueString)
			{
				int temp;
				if ((1 == sscanf(valueString, " %d", &temp)) &&
					(CMZN_NODE_VALUE_LABEL_VALUE <= temp) && (temp <= CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3))
				{
					constantNodeDerivative = static_cast<cmzn_node_value_label>(temp);
				}
				else
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Invalid node derivative '%s' in constant evaluator %s",
						valueString, this->getName(fmlNodeDerivativesEvaluator).c_str());
					return_code = 0;
				}
				Fieldml_FreeString(valueString);
			}
			else
				return_code = 0;
		}
		else if (FHT_PARAMETER_EVALUATOR == objectType)
		{
			if ((1 == Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlNodeDerivativesEvaluator) &&
				(Fieldml_GetIndexEvaluator(fmlSession, fmlNodeDerivativesEvaluator, 1) == fmlElementParametersIndexArgument)))
			{
				cmzn::SetImpl(nodeDerivativesMap, this->getEnsembleParameters(fmlNodeDerivativesEvaluator));
				if (!nodeDerivativesMap)
					return_code = 0;
			}
			else
				return_code = 0;
		}
		else
			return_code = 0;
		if (!return_code)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s binds invalid node derivatives evaluator %s.",
				elementParametersName.c_str(), getName(fmlNodeDerivativesEvaluator).c_str());
		}
	}

	// 4. Optional node version constant or map
	int constantNodeVersion = 1;
	HDsMapInt nodeVersionsMap;
	FmlObjectHandle fmlNodeVersionsEvaluator = FML_INVALID_OBJECT_HANDLE;
	if ((FML_INVALID_OBJECT_HANDLE != this->fmlNodeVersionsArgument) &&
		(FML_INVALID_OBJECT_HANDLE != (fmlNodeVersionsEvaluator =
			Fieldml_GetBindByArgument(this->fmlSession, fmlElementParametersEvaluator, this->fmlNodeVersionsArgument))))
	{
		++usedBindCount;
		FieldmlHandleType objectType = Fieldml_GetObjectType(this->fmlSession, fmlNodeVersionsEvaluator);
		if (FHT_CONSTANT_EVALUATOR == objectType)
		{
			char *valueString = Fieldml_GetConstantEvaluatorValueString(this->fmlSession, fmlNodeVersionsEvaluator);
			if (valueString)
			{
				if ((1 != sscanf(valueString, " %d", &constantNodeVersion)) || (constantNodeVersion < 1))
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Invalid node version '%s' in constant evaluator %s",
						valueString, this->getName(fmlNodeVersionsEvaluator).c_str());
					return_code = 0;
				}
				Fieldml_FreeString(valueString);
			}
			else
				return_code = 0;
		}
		else if (FHT_PARAMETER_EVALUATOR == objectType)
		{
			if ((1 == Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlNodeVersionsEvaluator) &&
				(Fieldml_GetIndexEvaluator(fmlSession, fmlNodeVersionsEvaluator, 1) == fmlElementParametersIndexArgument)))
			{
				cmzn::SetImpl(nodeVersionsMap, this->getEnsembleParameters(fmlNodeVersionsEvaluator));
				if (!nodeVersionsMap)
					return_code = 0;
			}
			else
				return_code = 0;
		}
		else
			return_code = 0;
		if (!return_code)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s binds invalid node versions evaluator %s.",
				elementParametersName.c_str(), getName(fmlNodeVersionsEvaluator).c_str());
		}
	}

	const int elementParametersBindCount = Fieldml_GetBindCount(fmlSession, fmlElementParametersEvaluator);
	if (usedBindCount != elementParametersBindCount)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Element parameters evaluator %s used only %d of %d bindings.",
			elementParametersName.c_str(), usedBindCount, elementParametersBindCount);
		return_code = 0;
	}
	if (!return_code)
	{
		cmzn_elementbasis_destroy(&elementBasis);
		return 0;
	}

	// element template now validated

	if (verbose)
	{
		display_message(INFORMATION_MESSAGE, "Read FieldML:  Interpreting evaluator %s as nodal/element interpolator using basis %s.\n",
			evaluatorName.c_str(), interpolator_name);
	}
	HDsMapIndexing indexing(localToGlobalNodeMap->createIndexing());
	ElementFieldComponent *component = new ElementFieldComponent(elementBasis, localToGlobalNodeMap, indexing, basisNodeCount, libraryBases[basis_index].swizzle);
	if (component)
	{
		if (nodeDerivativesMap)
			component->setNodeDerivativesMap(cmzn::GetImpl(nodeDerivativesMap));
		else
			component->setConstantNodeDerivative(constantNodeDerivative);
		if (nodeVersionsMap)
			component->setNodeVersionsMap(cmzn::GetImpl(nodeVersionsMap));
		else
			component->setConstantNodeVersion(constantNodeVersion);
		componentMap[fmlEvaluator] = component;
	}
	return component;
}

// @param fmlComponentType  Only provide if multi-component, otherwise pass FML_INVALID_OBJECT_HANDLE
int FieldMLReader::readField(FmlObjectHandle fmlFieldEvaluator, FmlObjectHandle fmlComponentsType,
	std::vector<FmlObjectHandle> &fmlComponentEvaluators, FmlObjectHandle fmlNodeParameters,
	FmlObjectHandle fmlNodeParametersArgument, FmlObjectHandle fmlNodesArgument,
	FmlObjectHandle fmlElementArgument)
{
	int return_code = 1;
	const int componentCount = static_cast<int>(fmlComponentEvaluators.size());

	std::string fieldName = getName(fmlFieldEvaluator);

	if (verbose)
	{
		display_message(INFORMATION_MESSAGE, "\n==> Defining field from evaluator %s, %d components\n",
			fieldName.c_str(), componentCount);
	}

	cmzn_field_id field = cmzn_fieldmodule_create_field_finite_element(field_module, componentCount);
	FE_field *feField = 0;
	Computed_field_get_type_finite_element(field, &feField);
	cmzn_field_set_name(field, fieldName.c_str());
	cmzn_field_set_managed(field, true);
	if ((componentCount >= meshDimension) && (componentCount <= 3))
	{
		// if field value type is RC coordinates, set field 'type coordinate' flag.
		// Needed to define faces, and by cmgui to find default coordinate field.
		FmlObjectHandle fmlValueType = Fieldml_GetValueType(fmlSession, fmlFieldEvaluator);
		std::string valueTypeName = this->getDeclaredName(fmlValueType);
		if ((valueTypeName == "coordinates.rc.3d") ||
			(valueTypeName == "coordinates.rc.2d") ||
			(valueTypeName == "coordinates.rc.1d"))
		{
			cmzn_field_set_type_coordinate(field, true);
		}
	}

	// set node parameters (nodes have already been created)
	cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(field_module, CMZN_FIELD_DOMAIN_TYPE_NODES);
	HDsMapDouble nodeParameters(this->getContinuousParameters(fmlNodeParameters));
	if (!nodeParameters)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Field %s node parameters %s could not be read.",
			fieldName.c_str(), getName(fmlNodeParameters).c_str());
		return_code = 0;
	}
	if (return_code)
	{
		HDsLabels nodesLabels(getLabelsForEnsemble(this->fmlNodesType));
		HDsLabels componentsLabels;
		int componentsIndex = -1, componentsSize = 1;
		if (FML_INVALID_OBJECT_HANDLE != fmlComponentsType)
		{
			cmzn::SetImpl(componentsLabels, this->getLabelsForEnsemble(fmlComponentsType));
			if (!(componentsLabels &&
				(0 <= (componentsIndex = nodeParameters->getLabelsIndex(*componentsLabels))) &&
				(0 < (componentsSize = componentsLabels->getSize()))))
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Field %s node parameters %s is missing components index.",
					fieldName.c_str(), getName(fmlNodeParameters).c_str());
				return_code = 0;
			}
		}
		HDsLabels nodeDerivativesLabels;
		int nodeDerivativesIndex = -1, nodeDerivativesSize = 1;
		if (FML_INVALID_OBJECT_HANDLE != this->fmlNodeDerivativesArgument)
		{
			cmzn::SetImpl(nodeDerivativesLabels, this->getLabelsForEnsemble(this->fmlNodeDerivativesType));
			if (nodeDerivativesLabels)
			{
				nodeDerivativesIndex = nodeParameters->getLabelsIndex(*nodeDerivativesLabels);
				if (0 <= nodeDerivativesIndex)
					nodeDerivativesSize = nodeDerivativesLabels->getSize();
			}
			else
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Missing node derivatives labels");
				return_code = 0;
			}
		}
		HDsLabels nodeVersionsLabels;
		int nodeVersionsIndex = -1, nodeVersionsSize = 1;
		if (FML_INVALID_OBJECT_HANDLE != this->fmlNodeVersionsArgument)
		{
			cmzn::SetImpl(nodeVersionsLabels, this->getLabelsForEnsemble(this->fmlNodeVersionsType));
			if (nodeVersionsLabels)
			{
				nodeVersionsIndex = nodeParameters->getLabelsIndex(*nodeVersionsLabels);
				if (0 <= nodeVersionsIndex)
					nodeVersionsSize = nodeVersionsLabels->getSize();
			}
			else
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Missing node versions labels");
				return_code = 0;
			}
		}
		const int valuesSize = componentsSize*nodeDerivativesSize*nodeVersionsSize;
		int componentsOffset = 1;
		int nodeDerivativesOffset = 1;
		int nodeVersionsOffset = 1;
		int index[3] = { componentsIndex, nodeDerivativesIndex, nodeVersionsIndex };
		int size[3] = { componentCount, nodeDerivativesSize, nodeVersionsSize };
		int *offset[3] = { &componentsOffset, &nodeDerivativesOffset, &nodeVersionsOffset };
		// get offsets for components, derivatives and versions parameters in any order:
		int highestIndex = 0;
		for (int j = 0; j < 3; ++j)
			if (index[j] > highestIndex)
				highestIndex = index[j];
		for (int i = 0; i <= highestIndex; ++i)
			for (int j = 0; j < 3; ++j)
				if (index[j] == i)
				{
					for (int k = 0; k < 3; ++k)
						if (index[k] > i)
							*offset[j] *= size[k];
					break;
				}
		double *values = new double[valuesSize];
		// store both current and last value exists in same array and swap pointers
		int *allocatedValueExists = new int[2*valuesSize];
		int *valueExists = allocatedValueExists;
		valueExists[0] = 0;
		int *lastValueExists = allocatedValueExists + valuesSize;
		lastValueExists[0] = -5;
		cmzn_nodetemplate_id nodetemplate = 0;
		HDsMapIndexing nodeParametersIndexing(nodeParameters->createIndexing());
		DsLabelIterator *nodesLabelIterator = nodesLabels->createLabelIterator();
		FE_nodeset *feNodeset = cmzn_nodeset_get_FE_nodeset_internal(nodeset);
		if (!nodesLabelIterator)
			return_code = CMZN_ERROR_MEMORY;
		else
		{
			while (nodesLabelIterator->increment())
			{
				nodeParametersIndexing->setEntry(*nodesLabelIterator);
				int valuesRead = 0;
				if (nodeParameters->getValuesSparse(*nodeParametersIndexing, valuesSize, values, valueExists, valuesRead))
				{
					if (0 < valuesRead)
					{
						for (int i = 0; i < valuesSize; ++i)
							if (valueExists[i] != lastValueExists[i])
							{
								cmzn_nodetemplate_destroy(&nodetemplate);
								nodetemplate = cmzn_nodeset_create_nodetemplate(nodeset);
								cmzn_nodetemplate_define_field(nodetemplate, field);
								for (int c = 0; c < componentCount; ++c)
								{
									int *exists = valueExists + c*componentsOffset;
									for (int d = 0; d < nodeDerivativesSize; ++d)
									{
										for (int v = nodeVersionsSize - 1; 0 <= v; --v)
											if (exists[v*nodeVersionsOffset])
											{
												cmzn_nodetemplate_set_value_number_of_versions(nodetemplate, field, c + 1,
													static_cast<cmzn_node_value_label>(d + CMZN_NODE_VALUE_LABEL_VALUE), v + 1);
												break;
											}
										exists += nodeDerivativesOffset;
									}
								}
								break;
							}
						const int nodeIdentifier = nodesLabelIterator->getIdentifier();
						cmzn_node *node = feNodeset->findNodeByIdentifier(nodeIdentifier);
						if (!cmzn_node_merge(node, nodetemplate) ||
							(CMZN_OK != FE_field_assign_node_parameters_sparse_FE_value(feField, node,
								valuesSize, values, valueExists, valuesRead, componentCount, componentsOffset,
								nodeDerivativesSize, nodeDerivativesOffset, nodeVersionsSize, nodeVersionsOffset)))
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Failed to set field %s parameters at node %d",
								fieldName.c_str(), nodeIdentifier);
							return_code = 0;
							break;
						}
						int *tmp = lastValueExists;
						lastValueExists = valueExists;
						valueExists = tmp;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Failed to get field %s node parameters at node %d",
						fieldName.c_str(), nodesLabelIterator->getIdentifier());
					return_code = 0;
					break;
				}
			}
		}
		cmzn::Deaccess(nodesLabelIterator);
		delete[] allocatedValueExists;
		delete[] values;
		cmzn_nodetemplate_destroy(&nodetemplate);
	}

	// define element fields

	cmzn_mesh_id mesh =
		cmzn_fieldmodule_find_mesh_by_dimension(field_module, meshDimension);
	cmzn_elementtemplate_id elementtemplate = 0;
	HDsLabels elementsLabels(getLabelsForEnsemble(fmlElementsType));
	DsLabelIterator *elementsLabelIterator = elementsLabels->createLabelIterator();
	if (!elementsLabelIterator)
		return_code = 0;
	std::vector<FmlObjectHandle> fmlElementEvaluators(componentCount, FML_INVALID_OBJECT_HANDLE);
	std::vector<ElementFieldComponent*> components(componentCount, (ElementFieldComponent*)0);
	std::vector<HDsMapInt> componentFunctionMap(componentCount);
	std::vector<HDsMapIndexing> componentFunctionIndex(componentCount);
	for (int ic = 0; ic < componentCount; ic++)
	{
		int bindCount = Fieldml_GetBindCount(fmlSession, fmlComponentEvaluators[ic]);
		if (1 == bindCount)
		{
			// recognised indirect form: a parameter mapping element to piecewise index
			FmlObjectHandle fmlComponentFunctionMapEvaluator =
				Fieldml_GetBindEvaluator(fmlSession, fmlComponentEvaluators[ic], 1);
			HDsMapInt parameterMap(this->getEnsembleParameters(fmlComponentFunctionMapEvaluator));
			if (parameterMap)
			{
				componentFunctionMap[ic] = parameterMap;
				componentFunctionIndex[ic] = HDsMapIndexing(parameterMap->createIndexing());
			}
			else
				return_code = 0;
		}
	}
	while (return_code && elementsLabelIterator->increment())
	{
		DsLabelIdentifier elementIdentifier = elementsLabelIterator->getIdentifier();
		bool newElementtemplate = (elementtemplate == 0);
		bool definedOnAllComponents = true;
		for (int ic = 0; ic < componentCount; ic++)
		{
			int functionId = elementIdentifier;
			if (componentFunctionMap[ic])
			{
				// handle indirect element to function map
				if (!(componentFunctionIndex[ic]->setEntry(*elementsLabelIterator) &&
					componentFunctionMap[ic]->getValues(*(componentFunctionIndex[ic]), 1, &functionId)))
				{
					definedOnAllComponents = false;
					break;
				}
			}
			FmlObjectHandle fmlElementEvaluator = Fieldml_GetElementEvaluator(fmlSession,
				fmlComponentEvaluators[ic], functionId, /*allowDefault*/1);
			if (fmlElementEvaluator != fmlElementEvaluators[ic])
			{
				fmlElementEvaluators[ic] = fmlElementEvaluator;
				newElementtemplate = true;
			}
			if (fmlElementEvaluator == FML_INVALID_OBJECT_HANDLE)
			{
				definedOnAllComponents = false;
				break;
			}
		}
		if (definedOnAllComponents)
		{
			if (newElementtemplate)
			{
				if (elementtemplate)
					cmzn_elementtemplate_destroy(&elementtemplate);
				elementtemplate = cmzn_mesh_create_elementtemplate(mesh);
				// do not want to override shape of existing elements:
				cmzn_elementtemplate_set_element_shape_type(elementtemplate, CMZN_ELEMENT_SHAPE_TYPE_INVALID);
				int total_local_point_count = 0;
				for (int ic = 0; ic < componentCount; ic++)
				{
					components[ic] = getElementFieldComponent(mesh, fmlElementEvaluators[ic],
						fmlNodeParametersArgument, fmlNodesArgument, fmlElementArgument);
					if (!components[ic])
					{
						display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s component %d element %d evaluator %s does not reference a supported basis function or mapping",
							fieldName.c_str(), ic + 1, elementIdentifier, getName(fmlElementEvaluators[ic]).c_str());
						return_code = 0;
						break;
					}
					bool new_local_point_to_node = true;
					for (int jc = 0; jc < ic; jc++)
					{
						if (components[jc]->local_point_to_node == components[ic]->local_point_to_node)
						{
							new_local_point_to_node = false;
							break;
						}
					}
					if (new_local_point_to_node)
					{
						const int *swizzle = components[ic]->swizzle;
						for (int i = 0; i < components[ic]->local_point_count; i++)
						{
							components[ic]->local_point_indexes[i] = total_local_point_count + i + 1;
							if (swizzle)
							{
								components[ic]->swizzled_local_point_indexes[i] = total_local_point_count + swizzle[i];
							}
							else
							{
								components[ic]->swizzled_local_point_indexes[i] = components[ic]->local_point_indexes[i];
							}
						}
						total_local_point_count += components[ic]->local_point_count;
						cmzn_elementtemplate_set_number_of_nodes(elementtemplate, total_local_point_count);
					}
					if (!cmzn_elementtemplate_define_field_simple_nodal(elementtemplate, field,
						/*component*/ic + 1, components[ic]->elementBasis, components[ic]->local_point_count,
						components[ic]->local_point_indexes))
					{
						return_code = 0;
						break;
					}
					// set derivative/version mappings
					DsMap<int> *nodeDerivativesMap = components[ic]->getNodeDerivativesMap();
					HDsMapIndexing nodeDerivativesMapIndexing;
					if (nodeDerivativesMap)
						cmzn::SetImpl(nodeDerivativesMapIndexing, nodeDerivativesMap->createIndexing());
					cmzn_node_value_label nodeDerivative = components[ic]->getConstantNodeDerivative();
					DsMap<int> *nodeVersionsMap = components[ic]->getNodeVersionsMap();
					HDsMapIndexing nodeVersionsMapIndexing;
					if (nodeVersionsMap)
						cmzn::SetImpl(nodeVersionsMapIndexing, nodeVersionsMap->createIndexing());
					int nodeVersion = components[ic]->getConstantNodeVersion();
					const int basisNodeCount = cmzn_elementbasis_get_number_of_nodes(components[ic]->elementBasis);
					// should be a more convenient way to get the basis dof labels
					DsLabels *dofLabels = 0;
					if (nodeDerivativesMap)
						dofLabels = nodeDerivativesMap->getLabels(0);
					else if (nodeVersionsMap)
						dofLabels = nodeVersionsMap->getLabels(0);
					int dofIndex = 0;
					for (int n = 1; n <= basisNodeCount; ++n)
					{
						const int functionCount = cmzn_elementbasis_get_number_of_functions_per_node(components[ic]->elementBasis, n);
						for (int f = 1; f <= functionCount; ++f)
						{
							if (nodeDerivativesMap)
							{
								int value;
								if (!(nodeDerivativesMapIndexing->setEntryIndex(*dofLabels, dofIndex) &&
									nodeDerivativesMap->getValues(*nodeDerivativesMapIndexing, 1, &value)))
								{
									display_message(ERROR_MESSAGE, "Read FieldML:  Invalid derivative mapping for field %s in element %d",
										fieldName.c_str(), elementIdentifier);
									goto fail;
								}
								nodeDerivative = static_cast<cmzn_node_value_label>(CMZN_NODE_VALUE_LABEL_VALUE + value - 1);
							}
							if (CMZN_OK != cmzn_elementtemplate_set_map_node_value_label(elementtemplate, field,
								/*component*/ic + 1, n, f, nodeDerivative))
							{
								display_message(ERROR_MESSAGE, "Read FieldML:  Invalid derivative %d for field %s in element %d",
									nodeDerivative, fieldName.c_str(), elementIdentifier);
								goto fail;
							}
							if (nodeVersionsMap)
							{
								if (!(nodeVersionsMapIndexing->setEntryIndex(*dofLabels, dofIndex) &&
									nodeVersionsMap->getValues(*nodeVersionsMapIndexing, 1, &nodeVersion)))
								{
									display_message(ERROR_MESSAGE, "Read FieldML:  Invalid version mapping for field %s in element %d",
										fieldName.c_str(), elementIdentifier);
									goto fail;
								}
							}
							if (CMZN_OK != cmzn_elementtemplate_set_map_node_version(elementtemplate, field,
								/*component*/ic + 1, n, f, nodeVersion))
							{
								display_message(ERROR_MESSAGE, "Read FieldML:  Invalid version %d for field %s in element %d",
									nodeVersion, fieldName.c_str(), elementIdentifier);
								goto fail;
							}
							++dofIndex;
						}
					}
				}
			}
			int total_local_point_count = 0;
			for (int ic = 0; (ic < componentCount) && return_code; ic++)
			{
				ElementFieldComponent *component = components[ic];
				if ((total_local_point_count + 1) == component->local_point_indexes[0])
				{
					total_local_point_count += component->local_point_count;
					component->indexing->setEntry(*elementsLabelIterator);
					if (!component->local_point_to_node->getValues(*(component->indexing),
						component->local_point_count, component->node_identifiers))
					{
						display_message(ERROR_MESSAGE, "Read FieldML:  Incomplete local to global map for field %s", fieldName.c_str());
						return_code = 0;
						break;
					}
					for (int i = 0; i < component->local_point_count; i++)
					{
						cmzn_node_id node = cmzn_nodeset_find_node_by_identifier(nodeset, component->node_identifiers[i]);
						if (!node)
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Cannot find node %d for element %d local point %d in local point to node map %s",
								component->node_identifiers[i], elementIdentifier, i + 1, component->local_point_to_node->getName().c_str());
							return_code = 0;
							break;
						}
						cmzn_elementtemplate_set_node(elementtemplate, component->swizzled_local_point_indexes[i], node);
						cmzn_node_destroy(&node);
					}
				}
			}
			if (return_code)
			{
				cmzn_element_id element = cmzn_mesh_find_element_by_identifier(mesh, elementIdentifier);
				if (!cmzn_element_merge(element, elementtemplate))
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Could not merge element %d", elementIdentifier);
					return_code = 0;
				}
				cmzn_element_destroy(&element);
			}
		}
	}
fail:
	cmzn_elementtemplate_destroy(&elementtemplate);
	cmzn::Deaccess(elementsLabelIterator);
	cmzn_mesh_destroy(&mesh);
	cmzn_nodeset_destroy(&nodeset);
	cmzn_field_destroy(&field);

	return return_code;
}

/**
 * Test whether the evaluator is scalar, continuous and piecewise over elements
 * of the mesh, directly or indirectly via a map to an intermediate ensemble,
 * and a function of the same element argument evaluator.
 * @param fmlEvaluator  The evaluator to check.
 * @param fmlElementArgument  On true result, set to the element argument the
 * piecewise evaluator is ultimately indexed over.
 * @return  Boolean true if in recognised form, false if not.
 */
bool FieldMLReader::evaluatorIsScalarContinuousPiecewiseOverElements(FmlObjectHandle fmlEvaluator,
	FmlObjectHandle &fmlElementArgument)
{
	if (FHT_PIECEWISE_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlEvaluator))
		return false;
	FmlObjectHandle fmlValueType = Fieldml_GetValueType(this->fmlSession, fmlEvaluator);
	if (FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlValueType))
		return false;
	if (1 != Fieldml_GetTypeComponentCount(this->fmlSession, fmlValueType))
		return false;
	FmlObjectHandle fmlPiecewiseIndex = Fieldml_GetIndexEvaluator(this->fmlSession, fmlEvaluator, /*evaluatorIndex*/1);
	if (fmlPiecewiseIndex == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Piecewise Evaluator %s has no index evaluator",
			this->getName(fmlEvaluator).c_str());
		return false;
	}
	// can either be directly indexed by elements, or indirectly by map
	// from elements to intermediate ensemble (i.e. a function ID)
	int bindCount = Fieldml_GetBindCount(fmlSession, fmlEvaluator);
	if (1 == bindCount)
	{
		// test for recognised indirect form: a parameter mapping element to piecewise index
		FmlObjectHandle fmlBindArgument = Fieldml_GetBindArgument(fmlSession, fmlEvaluator, 1);
		FmlObjectHandle fmlBindEvaluator = Fieldml_GetBindEvaluator(fmlSession, fmlEvaluator, 1);
		if (fmlBindArgument != fmlPiecewiseIndex)
			return false;
		if (FHT_PARAMETER_EVALUATOR != Fieldml_GetObjectType(fmlSession, fmlBindEvaluator))
			return false;
		int parameterIndexCount = Fieldml_GetIndexEvaluatorCount(fmlSession, fmlBindEvaluator);
		if (1 != parameterIndexCount)
			return false;
		fmlPiecewiseIndex = Fieldml_GetIndexEvaluator(this->fmlSession, fmlBindEvaluator, /*evaluatorIndex*/1);
		if (fmlPiecewiseIndex == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Parameter Evaluator %s has no index evaluator",
				this->getName(fmlBindEvaluator).c_str());
			return false;
		}
	}
	else if (1 < bindCount)
		return false;
	FmlObjectHandle fmlIndexType = Fieldml_GetValueType(this->fmlSession, fmlPiecewiseIndex);
	if (fmlIndexType != this->fmlElementsType)
		return false;
	fmlElementArgument = fmlPiecewiseIndex;
	return true;
}

/**
 * @param fmlComponentsArgument  Optional, for multi-component fields only.
 */
bool FieldMLReader::evaluatorIsNodeParameters(FmlObjectHandle fmlNodeParameters,
	FmlObjectHandle fmlComponentsArgument)
{
	if ((Fieldml_GetObjectType(this->fmlSession, fmlNodeParameters) != FHT_PARAMETER_EVALUATOR) ||
		(Fieldml_GetObjectType(this->fmlSession, Fieldml_GetValueType(fmlSession, fmlNodeParameters)) != FHT_CONTINUOUS_TYPE))
	{
		if (verbose)
			display_message(WARNING_MESSAGE, "Read FieldML:  %s is not continuous parameters type so can't be node parameters",
				this->getName(fmlNodeParameters).c_str());
		return false;
	}
	// check arguments are nodes, components (if any) and optionally derivatives, versions
	FmlObjectHandle fmlPossibleNodesArgument = FML_INVALID_OBJECT_HANDLE;
	bool hasComponents = false;
	bool hasDerivatives = false;
	bool hasVersions = false;
	int usedIndexCount = 0;
	int nodeParametersIndexCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlNodeParameters);
	for (int i = 1; i <= nodeParametersIndexCount; ++i)
	{
		FmlObjectHandle fmlIndexEvaluator = Fieldml_GetIndexEvaluator(fmlSession, fmlNodeParameters, i);
		if (FML_INVALID_OBJECT_HANDLE == fmlIndexEvaluator)
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  %s is missing index %d", this->getName(fmlNodeParameters).c_str(), i);
			return false;
		}
		if (fmlIndexEvaluator == fmlComponentsArgument)
		{
			if (hasComponents)
			{
				display_message(WARNING_MESSAGE, "Read FieldML:  %s binds to components more than once", this->getName(fmlNodeParameters).c_str());
				return false;
			}
			hasComponents = true;
			++usedIndexCount;
		}
		else if (fmlIndexEvaluator == this->fmlNodeDerivativesArgument)
		{
			if (hasDerivatives)
			{
				display_message(WARNING_MESSAGE, "Read FieldML:  %s binds to derivatives more than once", this->getName(fmlNodeParameters).c_str());
				return false;
			}
			hasDerivatives = true;
			++usedIndexCount;
		}
		else if (fmlIndexEvaluator == this->fmlNodeVersionsArgument)
		{
			if (hasVersions)
			{
				display_message(WARNING_MESSAGE, "Read FieldML:  %s binds to versions more than once", this->getName(fmlNodeParameters).c_str());
				return false;
			}
			hasVersions = true;
			++usedIndexCount;
		}
		else if (FML_INVALID_OBJECT_HANDLE == fmlPossibleNodesArgument)
		{
			fmlPossibleNodesArgument = fmlIndexEvaluator;
			++usedIndexCount;
		}
	}
	if (usedIndexCount != nodeParametersIndexCount)
	{
		display_message(WARNING_MESSAGE, "Read FieldML:  %s has unexpected extra index evaluators", this->getName(fmlNodeParameters).c_str());
		return false;
	}
	if ((fmlComponentsArgument != FML_INVALID_OBJECT_HANDLE) && !hasComponents)
	{
		display_message(WARNING_MESSAGE, "Read FieldML:  %s has unexpected extra index evaluators", this->getName(fmlNodeParameters).c_str());
		return false;
	}
	if (FML_INVALID_OBJECT_HANDLE == fmlPossibleNodesArgument)
	{
		display_message(WARNING_MESSAGE, "Read FieldML:  %s is not indexed by nodes", this->getName(fmlNodeParameters).c_str());
		return false;
	}
	if (fmlPossibleNodesArgument != this->fmlNodesArgument)
	{
		if (this->fmlNodesArgument == FML_INVALID_OBJECT_HANDLE)
		{
			if (CMZN_OK != this->readNodes(fmlPossibleNodesArgument))
				return false;
		}
		else
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  %s is not indexed by standard nodes argument", this->getName(fmlNodeParameters).c_str());
			return false;
		}
	}
	return true;
}

/** continuous-valued aggregates of piecewise varying with mesh elements are
 * interpreted as vector-valued finite element fields */
int FieldMLReader::readAggregateFields()
{
	const int aggregateCount = Fieldml_GetObjectCount(this->fmlSession, FHT_AGGREGATE_EVALUATOR);
	int return_code = 1;
	for (int aggregateIndex = 1; (aggregateIndex <= aggregateCount) && return_code; aggregateIndex++)
	{
		FmlObjectHandle fmlAggregate = Fieldml_GetObject(this->fmlSession, FHT_AGGREGATE_EVALUATOR, aggregateIndex);
		std::string fieldName = getName(fmlAggregate);

		//int indexCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlAggregate);
		FmlObjectHandle fmlComponentsArgument = Fieldml_GetIndexEvaluator(this->fmlSession, fmlAggregate, 1);
		if (FML_INVALID_OBJECT_HANDLE == fmlComponentsArgument)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s has missing index. Skipping.", fieldName.c_str());
			continue;
		}
		if (FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlComponentsArgument))
		{
			if (this->verbose)
				display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s index is not an argument evaluator. Skipping.", fieldName.c_str());
			continue;
		}
		FmlObjectHandle fmlValueType = Fieldml_GetValueType(this->fmlSession, fmlAggregate);
		if (FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlValueType))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s is not continuous type. Skipping.", fieldName.c_str());
			continue;
		}
		FmlObjectHandle fmlComponentsType = Fieldml_GetTypeComponentEnsemble(this->fmlSession, fmlValueType);
		if (Fieldml_GetValueType(this->fmlSession, fmlComponentsArgument) != fmlComponentsType)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s index evaluator and component types do not match. Skipping.", fieldName.c_str());
			continue;
		}
		const int componentCount = Fieldml_GetMemberCount(this->fmlSession, fmlComponentsType);

		// check components are scalar, piecewise over elements
		bool validComponents = true;
		std::vector<FmlObjectHandle> fmlComponentEvaluators(componentCount, FML_INVALID_OBJECT_HANDLE);
		FmlObjectHandle fmlElementArgument = FML_INVALID_OBJECT_HANDLE;
		for (int componentIndex = 1; componentIndex <= componentCount; componentIndex++)
		{
			// Component ensemble identifiers are always 1..N:
			int componentIdentifier = componentIndex;
			int ic = componentIndex - 1;
			fmlComponentEvaluators[ic] = Fieldml_GetElementEvaluator(fmlSession, fmlAggregate, componentIdentifier, /*allowDefault*/1);
			if (fmlComponentEvaluators[ic] == FML_INVALID_OBJECT_HANDLE)
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s component %d evaluator is missing", fieldName.c_str(), componentIndex);
				return_code = 0;
				break;
			}
			FmlObjectHandle fmlComponentElementArgument = FML_INVALID_OBJECT_HANDLE;
			if (!this->evaluatorIsScalarContinuousPiecewiseOverElements(
				fmlComponentEvaluators[ic], fmlComponentElementArgument))
			{
				if (verbose)
					display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s component %d is not piecewise over elements",
						fieldName.c_str(), componentIndex);
				validComponents = false;
				break;
			}
			if (fmlElementArgument == FML_INVALID_OBJECT_HANDLE)
				fmlElementArgument = fmlComponentElementArgument;
			else if (fmlComponentElementArgument != fmlElementArgument)
			{
				if (verbose)
					display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s components must use same element argument",
						fieldName.c_str());
				validComponents = false;
				break;
			}
		}
		if (!return_code)
			break;
		if (!validComponents)
		{
			if (verbose)
				display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s cannot be interpreted as a field defined over a mesh. Skipping.",
					fieldName.c_str());
			continue;
		}

		// determine if exactly one binding of node parameters
		int bindCount = Fieldml_GetBindCount(fmlSession, fmlAggregate);
		if (1 != bindCount)
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s does not have exactly 1 binding (for node parameters). Skipping.",
				fieldName.c_str());
			continue;
		}
		FmlObjectHandle fmlNodeParametersArgument = Fieldml_GetBindArgument(fmlSession, fmlAggregate, 1);
		FmlObjectHandle fmlNodeParameters = Fieldml_GetBindEvaluator(fmlSession, fmlAggregate, 1);
		if (!this->evaluatorIsNodeParameters(fmlNodeParameters, fmlComponentsArgument))
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s bound argument %s is not valid node parameters. Skipping.",
				fieldName.c_str(), this->getName(fmlNodeParameters).c_str());
			continue;
		}

		return_code = readField(fmlAggregate, fmlComponentsType, fmlComponentEvaluators, fmlNodeParameters,
			fmlNodeParametersArgument, fmlNodesArgument, fmlElementArgument);
	}
	return return_code;
}

/** continuous-valued references to piecewise varying with mesh elements are
 * interpreted as scalar-valued finite element fields */
int FieldMLReader::readReferenceFields()
{
	const int referenceCount = Fieldml_GetObjectCount(fmlSession, FHT_REFERENCE_EVALUATOR);
	int return_code = 1;
	for (int referenceIndex = 1; (referenceIndex <= referenceCount) && return_code; referenceIndex++)
	{
		FmlObjectHandle fmlReference = Fieldml_GetObject(fmlSession, FHT_REFERENCE_EVALUATOR, referenceIndex);
		std::string fieldName = getName(fmlReference);

		FmlObjectHandle fmlValueType = Fieldml_GetValueType(fmlSession, fmlReference);
		if (FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(fmlSession, fmlValueType))
		{
			//display_message(WARNING_MESSAGE, "Read FieldML:  Ignore reference %s as not continuous type\n", fieldName.c_str());
			continue;
		}

		// check reference evaluator is scalar, piecewise over elements
		FmlObjectHandle fmlComponentEvaluator = Fieldml_GetReferenceSourceEvaluator(fmlSession, fmlReference);
		if (fmlComponentEvaluator == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Reference %s source evaluator is missing", fieldName.c_str());
			return_code = 0;
			break;
		}
		FmlObjectHandle fmlElementArgument = FML_INVALID_OBJECT_HANDLE;
		if (!this->evaluatorIsScalarContinuousPiecewiseOverElements(
			fmlComponentEvaluator, fmlElementArgument))
		{
			if (verbose)
				display_message(WARNING_MESSAGE,
					"Read FieldML:  Reference %s cannot be interpreted as a field defined over a mesh. Skipping.",
					fieldName.c_str());
			continue;
		}

		// determine if exactly one binding of node parameters
		int bindCount = Fieldml_GetBindCount(fmlSession, fmlReference);
		if (1 != bindCount)
		{
			if (verbose)
			{
				display_message(WARNING_MESSAGE,
					"Read FieldML:  Reference %s does not have exactly 1 binding (for nodal parameters). Skipping.",
					fieldName.c_str());
			}
			continue;
		}
		FmlObjectHandle fmlNodeParametersArgument = Fieldml_GetBindArgument(fmlSession, fmlReference, 1);
		FmlObjectHandle fmlNodeParameters = Fieldml_GetBindEvaluator(fmlSession, fmlReference, 1);
		if (!this->evaluatorIsNodeParameters(fmlNodeParameters, /*fmlComponentsArgument*/FML_INVALID_OBJECT_HANDLE))
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  Reference %s bound argument %s is not valid node parameters. Skipping.",
				fieldName.c_str(), this->getName(fmlNodeParameters).c_str());
			continue;
		}

		std::vector<FmlObjectHandle> fmlComponentEvaluators(1, fmlComponentEvaluator);
		return_code = readField(fmlReference, /*fmlComponentsType*/FML_INVALID_OBJECT_HANDLE,
			fmlComponentEvaluators, fmlNodeParameters, fmlNodeParametersArgument, fmlNodesArgument, fmlElementArgument);
	}
	return return_code;
}

int FieldMLReader::parse()
{
	int return_code = 1;
	if ((!region) || (!filename) || (!field_module))
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::parse.  Invalid construction arguments");
		return_code = 0;
	}
	if (fmlSession == (const FmlSessionHandle)FML_INVALID_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML: could not parse file %s", filename);
		return_code = 0;
	}
	else
	{
		int parseErrorCount = Fieldml_GetErrorCount(fmlSession);
		return_code = (parseErrorCount == 0);
		for (int i = 1; i <= parseErrorCount; i++)
		{
			char *error_string = Fieldml_GetError(fmlSession, i);
			display_message(ERROR_MESSAGE, "FieldML Parse error: %s", error_string);
			Fieldml_FreeString(error_string);
		}
	}
	if (!return_code)
		return 0;
	cmzn_region_begin_change(region);
	return_code = return_code && this->readGlobals();
	return_code = return_code && this->readMeshes();
	return_code = return_code && this->readAggregateFields();
	return_code = return_code && this->readReferenceFields();
	cmzn_region_end_change(region);
	return return_code;
}

bool string_contains_FieldML_tag(const char *text)
{
	return (0 != strstr(text, "<Fieldml"));
}

} // anonymous namespace

bool is_FieldML_file(const char *filename)
{
	bool result = false;
	FILE *stream = fopen(filename, "r");
	if (stream)
	{
		char block[200];
		size_t size = fread((void *)block, sizeof(char), sizeof(block), stream);
		if (size > 0)
		{
			block[size - 1] = '\0';
			result = string_contains_FieldML_tag(block);
		}
		fclose(stream);
	}
	return result;
}

bool is_FieldML_memory_block(unsigned int memory_buffer_size, const void *memory_buffer)
{
	if ((0 == memory_buffer_size) || (!memory_buffer))
		return false;
	unsigned int size = memory_buffer_size;
	if (size > 200)
		size = 200;
	char block[200];
	memcpy(block, memory_buffer, size);
	block[size - 1] = '\0';
	return string_contains_FieldML_tag(block);
}

int parse_fieldml_file(struct cmzn_region *region, const char *filename)
{
	FieldMLReader fmlReader(region, filename);
	return fmlReader.parse();
}
