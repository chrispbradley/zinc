/***************************************************************************//**
 * FILE : computed_field_matrix_operators.c
 *
 * Implements a number of basic matrix operations on computed fields.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <cmath>
#include "zinc/fieldmatrixoperators.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_matrix_operators.hpp"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "general/message.h"
#include "graphics/quaternion.hpp"

class Computed_field_matrix_operators_package : public Computed_field_type_package
{
};

int Computed_field_get_square_matrix_size(struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If <field> can represent a square matrix with numerical components, the number
of rows = number of columns is returned.
==============================================================================*/
{
	int n, return_code, size;

	ENTER(Computed_field_get_square_matrix_size);
	return_code = 0;
	if (field)
	{
		size = field->number_of_components;
		n = 1;
		while (n * n < size)
		{
			n++;
		}
		if (n * n == size)
		{
			return_code = n;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_square_matrix_size.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_square_matrix_size */

int Computed_field_is_square_matrix(struct Computed_field *field,
	void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns true if <field> can represent a square matrix, on account of having n*n
components, where n is a positive integer. If matrix is square, n is returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_is_square_matrix);
	USE_PARAMETER(dummy_void);
	if (field)
	{
		return_code =
			Computed_field_has_numerical_components(field, (void *)NULL) &&
			(0 != Computed_field_get_square_matrix_size(field));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_is_square_matrix.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_is_square_matrix */

namespace {

const char computed_field_determinant_type_string[] = "determinant";

class Computed_field_determinant : public Computed_field_core
{
public:
	Computed_field_determinant() : Computed_field_core()
	{
	};

	virtual ~Computed_field_determinant() { }

private:
	Computed_field_core *copy()
	{
		return new Computed_field_determinant();
	}

	const char *get_type_string()
	{
		return (computed_field_determinant_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		return (0 != dynamic_cast<Computed_field_determinant*>(other_field));
	}

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_determinant::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		FE_value *source_values = sourceCache->values;
		switch (getSourceField(0)->number_of_components)
		{
		case 1:
			valueCache.values[0] = source_values[0];
			break;
		case 4:
			valueCache.values[0] = source_values[0]*source_values[3] - source_values[1]*source_values[2];
			break;
		case 9:
			valueCache.values[0] =
				source_values[0]*(source_values[4]*source_values[8] - source_values[5]*source_values[7]) +
				source_values[1]*(source_values[5]*source_values[6] - source_values[3]*source_values[8]) +
				source_values[2]*(source_values[3]*source_values[7] - source_values[4]*source_values[6]);
			break;
		default:
			return 0;
			break;
		}
		return 1;
	}
	return 0;
}

int Computed_field_determinant::list()
{
	int return_code = 0;
	if (field)
	{
		display_message(INFORMATION_MESSAGE,"    source field : %s\n",
			field->source_fields[0]->name);
		return_code = 1;
	}
	return (return_code);
}

/** Returns allocated command string for reproducing field. Includes type. */
char *Computed_field_determinant::get_command_string()
{
	char *command_string = 0;
	if (field)
	{
		int error = 0;
		append_string(&command_string, computed_field_determinant_type_string, &error);
		append_string(&command_string, " field ", &error);
		append_string(&command_string, field->source_fields[0]->name, &error);
	}
	return (command_string);
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_determinant(
	cmzn_fieldmodule_id field_module, cmzn_field_id source_field)
{
	cmzn_field_id field = 0;
	if (field_module && source_field &&
		Computed_field_is_square_matrix(source_field, (void *)NULL) &&
		(cmzn_field_get_number_of_components(source_field) <= 9))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/1,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_determinant());
	}
	return (field);
}

namespace {

class EigenvalueFieldValueCache : public RealFieldValueCache
{
public:
	/* cache for matrix, eigenvectors. eigenvalues go in values member of base class */
	double *a, *v;

	EigenvalueFieldValueCache(int componentCount) :
		RealFieldValueCache(componentCount),
		a(new double[componentCount*componentCount]),
		v(new double[componentCount*componentCount])
	{
	}

	virtual ~EigenvalueFieldValueCache()
	{
		delete[] a;
		delete[] v;
	}

	static EigenvalueFieldValueCache* cast(FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<EigenvalueFieldValueCache*>(valueCache);
	}

	static EigenvalueFieldValueCache& cast(FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<EigenvalueFieldValueCache&>(valueCache);
	}
};

const char computed_field_eigenvalues_type_string[] = "eigenvalues";

class Computed_field_eigenvalues : public Computed_field_core
{
public:

	Computed_field_eigenvalues() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_eigenvalues();
	}

	const char *get_type_string()
	{
		return(computed_field_eigenvalues_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_eigenvalues*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& /*parentCache*/)
	{
		return new EigenvalueFieldValueCache(field->number_of_components);
	}

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_eigenvalues::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	EigenvalueFieldValueCache &valueCache = EigenvalueFieldValueCache::cast(inValueCache);
	RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		const int n = field->number_of_components;
		const int matrix_size = n * n;
		cmzn_field_id source_field = getSourceField(0);
		for (int i = 0; i < matrix_size; i++)
		{
			valueCache.a[i] = (double)(sourceCache->values[i]);
		}
		if (!matrix_is_symmetric(n, valueCache.a, 1.0E-6))
		{
			display_message(WARNING_MESSAGE,
				"Eigenanalysis of field %s may be wrong as matrix not symmetric",
				source_field->name);
		}
		/* get eigenvalues and eigenvectors sorted from largest to smallest */
		int nrot;
		if (Jacobi_eigenanalysis(n, valueCache.a, valueCache.values, valueCache.v, &nrot) &&
			eigensort(n, valueCache.values, valueCache.v))
		{
			/* values now contains the eigenvalues, v the eigenvectors in columns, while
				 values of a above the main diagonal are destroyed */
			return 1;
		}
	}
	return 0;
}

int Computed_field_eigenvalues::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_eigenvalues);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,"    source field : %s\n",
			field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_eigenvalues.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_eigenvalues */

char *Computed_field_eigenvalues::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_eigenvalues::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_eigenvalues_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_eigenvalues::get_command_string.  "
			"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_eigenvalues::get_command_string */

int Computed_field_is_type_eigenvalues(struct Computed_field *field)
/*******************************************************************************
LAST MODIFIED : 14 August 2006

DESCRIPTION :
Returns true if <field> has the appropriate static type string.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_is_type_eigenvalues);
	if (field)
	{
		if (dynamic_cast<Computed_field_eigenvalues*>(field->core))
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_is_type_eigenvalues.  Missing field");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_is_type_eigenvalues */

} //namespace

int Computed_field_is_type_eigenvalues_conditional(struct Computed_field *field,
	void *dummy_void)
/*******************************************************************************
LAST MODIFIED : 14 August 2006

DESCRIPTION :
List conditional function version of Computed_field_is_type_eigenvalues.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_is_type_eigenvalues_conditional);
	USE_PARAMETER(dummy_void);
	return_code = Computed_field_is_type_eigenvalues(field);
	LEAVE;

	return (return_code);
} /* Computed_field_is_type_eigenvalues_conditional */

Computed_field *cmzn_fieldmodule_create_field_eigenvalues(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field)
{
	struct Computed_field *field = NULL;
	if (field_module && source_field &&
		Computed_field_is_square_matrix(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/Computed_field_get_square_matrix_size(source_field),
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_eigenvalues());
	}
	return (field);
}

int Computed_field_get_type_eigenvalues(struct Computed_field *field,
	struct Computed_field **source_field)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type 'eigenvalues', the <source_field> it calculates the
eigenvalues of is returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_eigenvalues);
	if (field && (dynamic_cast<Computed_field_eigenvalues*>(field->core)) &&
		source_field)
	{
		*source_field = field->source_fields[0];
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_eigenvalues.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_eigenvalues */

namespace {

const char computed_field_eigenvectors_type_string[] = "eigenvectors";

class Computed_field_eigenvectors : public Computed_field_core
{
public:
	Computed_field_eigenvectors() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_eigenvectors();
	}

	const char *get_type_string()
	{
		return(computed_field_eigenvectors_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_eigenvectors*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_eigenvectors::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	EigenvalueFieldValueCache *eigenvalueCache = EigenvalueFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (eigenvalueCache)
	{
		double *v = eigenvalueCache->v;
		const int n = getSourceField(0)->number_of_components;
		/* return the vectors across the rows of the field values */
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
			{
				valueCache.values[i*n + j] = (FE_value)(v[j*n + i]);
			}
		}
		return 1;
	}
	return 0;
}

int Computed_field_eigenvectors::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_eigenvectors);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    eigenvalues field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_eigenvectors.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_eigenvectors */

char *Computed_field_eigenvectors::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_eigenvectors::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_eigenvectors_type_string, &error);
		append_string(&command_string, " eigenvalues ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_eigenvectors::get_command_string.  "
			"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_eigenvectors::get_command_string */

} //namespace

Computed_field *cmzn_fieldmodule_create_field_eigenvectors(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *eigenvalues_field)
{
	struct Computed_field *field = NULL;
	if (field_module && eigenvalues_field &&
		Computed_field_is_type_eigenvalues(eigenvalues_field))
	{
		int n = eigenvalues_field->number_of_components;
		int number_of_components = n * n;
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			number_of_components,
			/*number_of_source_fields*/1, &eigenvalues_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_eigenvectors());
	}
	return (field);
}

int Computed_field_get_type_eigenvectors(struct Computed_field *field,
	struct Computed_field **eigenvalues_field)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type 'eigenvectors', the <eigenvalues_field> used by it is
returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_eigenvectors);
	if (field && (dynamic_cast<Computed_field_eigenvectors*>(field->core)) &&
		eigenvalues_field)
	{
		*eigenvalues_field = field->source_fields[0];
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_eigenvectors.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_eigenvectors */

namespace {

class MatrixInvertFieldValueCache : public RealFieldValueCache
{
public:
	// cache stores intermediate LU-decomposed matrix and RHS vector in double
	// precision, as well as the integer pivot indx
	int n;
	double *a, *b;
	int *indx;

	MatrixInvertFieldValueCache(int componentCount, int n) :
		RealFieldValueCache(componentCount),
		n(n),
		a(new double[n*n]),
		b(new double[n]),
		indx(new int[n])
	{
	}

	virtual ~MatrixInvertFieldValueCache()
	{
		delete[] a;
		delete[] b;
		delete[] indx;
	}

	static MatrixInvertFieldValueCache* cast(FieldValueCache* valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<MatrixInvertFieldValueCache*>(valueCache);
	}

	static MatrixInvertFieldValueCache& cast(FieldValueCache& valueCache)
	{
		return FIELD_VALUE_CACHE_CAST<MatrixInvertFieldValueCache&>(valueCache);
	}
};

const char computed_field_matrix_invert_type_string[] = "matrix_invert";

class Computed_field_matrix_invert : public Computed_field_core
{
public:

	Computed_field_matrix_invert() : Computed_field_core()
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_matrix_invert();
	}

	const char *get_type_string()
	{
		return(computed_field_matrix_invert_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_matrix_invert*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	virtual FieldValueCache *createValueCache(cmzn_fieldcache& /*parentCache*/)
	{
		return new MatrixInvertFieldValueCache(field->number_of_components, Computed_field_get_square_matrix_size(getSourceField(0)));
	}

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_matrix_invert::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	MatrixInvertFieldValueCache &valueCache = MatrixInvertFieldValueCache::cast(inValueCache);
	RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		const int n = valueCache.n;
		const int matrix_size = n * n;
		for (int i = 0; i < matrix_size; i++)
		{
			valueCache.a[i] = (double)(sourceCache->values[i]);
		}
		double d;
		if (LU_decompose(n, valueCache.a, valueCache.indx, &d,/*singular_tolerance*/1.0e-12))
		{
			for (int i = 0; i < n; i++)
			{
				/* take a column of the identity matrix */
				for (int j = 0; j < n; j++)
				{
					valueCache.b[j] = 0.0;
				}
				valueCache.b[i] = 1.0;
				if (LU_backsubstitute(n, valueCache.a, valueCache.indx, valueCache.b))
				{
					/* extract a column of the inverse matrix */
					for (int j = 0; j < n; j++)
					{
						valueCache.values[j*n + i] = (FE_value)valueCache.b[j];
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"Computed_field_matrix_invert::evaluate.  "
						"Could not LU backsubstitute matrix");
					return 0;
				}
			}
			return 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Computed_field_matrix_invert::evaluate.  "
				"Could not LU decompose matrix");
		}
	}
	return 0;
}

int Computed_field_matrix_invert::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_matrix_invert);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,"    source field : %s\n",
			field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_matrix_invert.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_matrix_invert */

char *Computed_field_matrix_invert::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_matrix_invert::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_matrix_invert_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_matrix_invert::get_command_string.  "
			"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_matrix_invert::get_command_string */

} //namespace

Computed_field *cmzn_fieldmodule_create_field_matrix_invert(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field)
{
	struct Computed_field *field = NULL;
	if (field_module && source_field &&
		Computed_field_is_square_matrix(source_field, (void *)NULL))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			Computed_field_get_number_of_components(source_field),
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_matrix_invert());
	}
	LEAVE;

	return (field);
}

int Computed_field_get_type_matrix_invert(struct Computed_field *field,
	struct Computed_field **source_field)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type 'matrix_invert', the <source_field> it calculates the
matrix_invert of is returned.
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_get_type_matrix_invert);
	if (field && (dynamic_cast<Computed_field_matrix_invert*>(field->core)) &&
		source_field)
	{
		*source_field = field->source_fields[0];
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_matrix_invert.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_matrix_invert */

namespace {

const char computed_field_matrix_multiply_type_string[] = "matrix_multiply";

class Computed_field_matrix_multiply : public Computed_field_core
{
public:
	int number_of_rows;

	Computed_field_matrix_multiply(
		int number_of_rows) :
		Computed_field_core(), number_of_rows(number_of_rows)

	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_matrix_multiply(number_of_rows);
	}

	const char *get_type_string()
	{
		return(computed_field_matrix_multiply_type_string);
	}

	int compare(Computed_field_core* other_field);

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_matrix_multiply::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Compare the type specific data
==============================================================================*/
{
	Computed_field_matrix_multiply* other;
	int return_code;

	ENTER(Computed_field_matrix_multiply::compare);
	if (field && (other = dynamic_cast<Computed_field_matrix_multiply*>(other_core)))
	{
		if (number_of_rows == other->number_of_rows)
		{
			return_code = 1;
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_matrix_multiply::compare */

int Computed_field_matrix_multiply::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (source1Cache && source2Cache)
	{
		const int m = this->number_of_rows;
		const int s = getSourceField(0)->number_of_components / m;
		const int n = getSourceField(1)->number_of_components / s;
		FE_value *a=source1Cache->values;
		FE_value *b=source2Cache->values;
		for (int i=0;i<m;i++)
		{
			for (int j=0;j<n;j++)
			{
				FE_value sum=0.0;
				for (int k=0;k<s;k++)
				{
					sum += a[i*s+k] * b[k*n+j];
				}
				valueCache.values[i*n+j]=sum;
			}
		}
		int number_of_xi = cache.getRequestedDerivatives();
		if (number_of_xi && source1Cache->derivatives_valid && source2Cache->derivatives_valid)
		{
			for (int d=0;d<number_of_xi;d++)
			{
				/* use the product rule */
				a = source1Cache->values;
				FE_value *ad = source1Cache->derivatives+d;
				b = source2Cache->values;
				FE_value *bd = source2Cache->derivatives+d;
				for (int i=0;i<m;i++)
				{
					for (int j=0;j<n;j++)
					{
						FE_value sum=0.0;
						for (int k=0;k<s;k++)
						{
							sum += a[i*s+k] * bd[number_of_xi*(k*n+j)] +
								ad[number_of_xi*(i*s+k)] * b[k*n+j];
						}
						valueCache.derivatives[number_of_xi*(i*n+j)+d]=sum;
					}
				}
			}
			valueCache.derivatives_valid=1;
		}
		else
		{
			valueCache.derivatives_valid=0;
		}
		return 1;
	}
	return 0;
}

int Computed_field_matrix_multiply::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_matrix_multiply);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    number of rows : %d\n",number_of_rows);
		display_message(INFORMATION_MESSAGE,"    source fields : %s %s\n",
			field->source_fields[0]->name,field->source_fields[1]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_matrix_multiply.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_matrix_multiply */

char *Computed_field_matrix_multiply::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_matrix_multiply::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_matrix_multiply_type_string, &error);
		sprintf(temp_string, " number_of_rows %d", number_of_rows);
		append_string(&command_string, temp_string, &error);
		append_string(&command_string, " fields ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_matrix_multiply::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_matrix_multiply::get_command_string */

} //namespace

Computed_field *cmzn_fieldmodule_create_field_matrix_multiply(
	struct cmzn_fieldmodule *field_module,
	int number_of_rows, struct Computed_field *source_field1,
	struct Computed_field *source_field2)
{
	Computed_field *field = NULL;
	if (field_module && (0 < number_of_rows) && source_field1 && source_field1->isNumerical() &&
		source_field2 && source_field2->isNumerical())
	{
		int nc1 = source_field1->number_of_components;
		int nc2 = source_field2->number_of_components;
		int s = 0;
		if ((0 == (nc1 % number_of_rows)) &&
			(0 < (s = nc1/number_of_rows)) &&
			(0 == (nc2 % s)) &&
			(0 < (nc2 / s)))
		{
			int result_number_of_columns = nc2 / s;
			Computed_field *source_fields[2];
			source_fields[0] = source_field1;
			source_fields[1] = source_field2;
			field = Computed_field_create_generic(field_module,
				/*check_source_field_regions*/true,
				/*number_of_components*/number_of_rows * result_number_of_columns,
				/*number_of_source_fields*/2, source_fields,
				/*number_of_source_values*/0, NULL,
				new Computed_field_matrix_multiply(number_of_rows));
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_fieldmodule_create_field_matrix_multiply.  "
				"Fields are of invalid size for multiplication");
		}
	}
	return (field);
}

int Computed_field_get_type_matrix_multiply(struct Computed_field *field,
	int *number_of_rows, struct Computed_field **source_field1,
	struct Computed_field **source_field2)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_MATRIX_MULTIPLY, the
<number_of_rows> and <source_fields> used by it are returned.
==============================================================================*/
{
	Computed_field_matrix_multiply* core;
	int return_code;

	ENTER(Computed_field_get_type_matrix_multiply);
	if (field && (core=dynamic_cast<Computed_field_matrix_multiply*>(field->core)) &&
		source_field1 && source_field2)
	{
		*number_of_rows = core->number_of_rows;
		*source_field1 = field->source_fields[0];
		*source_field2 = field->source_fields[1];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_matrix_multiply.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_matrix_multiply */

namespace {

const char computed_field_projection_type_string[] = "projection";

class Computed_field_projection : public Computed_field_core
{
public:
	int matrix_rows, matrix_columns;

	Computed_field_projection(int matrix_columns, int matrix_rows) :
		Computed_field_core(),
		matrix_rows(matrix_rows),
		matrix_columns(matrix_columns)
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_projection(matrix_rows, matrix_columns);
	}

	const char *get_type_string()
	{
		return(computed_field_projection_type_string);
	}

	int compare(Computed_field_core* other_field)
	{
		if (dynamic_cast<Computed_field_projection*>(other_field))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();

	virtual enum FieldAssignmentResult assign(cmzn_fieldcache& cache, RealFieldValueCache& valueCache);
};

int Computed_field_projection::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	RealFieldValueCache *source1Cache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	RealFieldValueCache *source2Cache = RealFieldValueCache::cast(getSourceField(1)->evaluate(cache));
	if (source1Cache && source2Cache)
	{
		/* Calculate the transformed coordinates */
		int coordinate_components = getSourceField(0)->number_of_components;
		FE_value *projection_matrix = source2Cache->values;
		for (int i = 0 ; i < field->number_of_components ; i++)
		{
			valueCache.values[i] = 0.0;
			for (int j = 0 ; j < coordinate_components ; j++)
			{
				valueCache.values[i] +=
					projection_matrix[i * (coordinate_components + 1) + j] *
					source1Cache->values[j];
			}
			/* The last source value is fixed at 1 */
			valueCache.values[i] += projection_matrix[
				i * (coordinate_components + 1) + coordinate_components];
		}
		/* The last calculated value is the perspective value which divides through
			all the other components */
		FE_value perspective = 0.0;
		for (int j = 0 ; j < coordinate_components ; j++)
		{
			perspective += projection_matrix[field->number_of_components
				* (coordinate_components + 1) + j] * source1Cache->values[j];
		}
		perspective += projection_matrix[field->number_of_components
			* (coordinate_components + 1) + coordinate_components];

		int number_of_xi = cache.getRequestedDerivatives();
		if (number_of_xi && source1Cache->derivatives_valid && source2Cache->derivatives_valid)
		{
			for (int k=0;k<number_of_xi;k++)
			{
				/* Calculate the coordinate derivatives without perspective */
				for (int i = 0 ; i < field->number_of_components ; i++)
				{
					valueCache.derivatives[i * number_of_xi + k] = 0.0;
					for (int j = 0 ; j < coordinate_components ; j++)
					{
						valueCache.derivatives[i * number_of_xi + k] +=
							projection_matrix[i * (coordinate_components + 1) + j]
							* source1Cache->derivatives[j * number_of_xi + k];
					}
				}
				/* Calculate the perspective derivative */
				FE_value dhdxi = 0.0;
				for (int j = 0 ; j < coordinate_components ; j++)
				{
					dhdxi += projection_matrix[field->number_of_components
						* (coordinate_components + 1) + j]
						* source1Cache->derivatives[j *number_of_xi + k];
				}

				/* Calculate the perspective reciprocal derivative using chain rule */
				FE_value dh1dxi = (-1.0) / (perspective * perspective) * dhdxi;

				/* Calculate the derivatives of the perspective scaled transformed
					 coordinates, which is ultimately what we want */
				for (int i = 0 ; i < field->number_of_components ; i++)
				{
					valueCache.derivatives[i * number_of_xi + k] =
						valueCache.derivatives[i * number_of_xi + k] / perspective
						+ valueCache.values[i] * dh1dxi;
				}
			}
			valueCache.derivatives_valid = 1;
		}
		else
		{
			valueCache.derivatives_valid = 0;
		}
		/* Now apply the perspective scaling to the non derivative transformed
			 coordinates */
		for (int i = 0 ; i < field->number_of_components ; i++)
		{
			valueCache.values[i] /= perspective;
		}
		return 1;
	}
	return 0;
}

int Computed_field_projection::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_projection);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,"    source field : %s\n",
			field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,"    projection_matrix field : %s\n",
			field->source_fields[1]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_projection.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_projection */

char *Computed_field_projection::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_projection::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_projection_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		append_string(&command_string, " projection_matrix ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[1], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_projection::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_projection::get_command_string */

/** inverts the common 4x4 homogeneous transformation only */
enum FieldAssignmentResult Computed_field_projection::assign(cmzn_fieldcache& cache, RealFieldValueCache& valueCache)
{
	if ((field->number_of_components == 3) && (matrix_rows == 4) && (matrix_columns == 4))
	{
		cmzn_field_id coordinateField = getSourceField(0);
		cmzn_field_id projectionField = getSourceField(1);
		RealFieldValueCache *projectionCache = RealFieldValueCache::cast(projectionField->evaluate(cache));
		if (projectionCache)
		{
			// Inefficient; inverse matrix could be cached
			valueCache.derivatives_valid = 0;
			double *projection_matrix = projectionCache->values;
			double lu_matrix[16];
			for (int i = 0; i < 16; ++i)
			{
				lu_matrix[i] = projection_matrix[i];
			}
			double d, result[4];
			int indx[4];
			result[0] = (double)valueCache.values[0];
			result[1] = (double)valueCache.values[1];
			result[2] = (double)valueCache.values[2];
			result[3] = 1.0;
			if (LU_decompose(4,lu_matrix,indx,&d,/*singular_tolerance*/1.0e-12) &&
				LU_backsubstitute(4,lu_matrix,indx,result) &&
				(0.0 != result[3]))
			{
				RealFieldValueCache *coordinateCache = RealFieldValueCache::cast(coordinateField->getValueCache(cache));
				coordinateCache->values[0] = (result[0] / result[3]);
				coordinateCache->values[1] = (result[1] / result[3]);
				coordinateCache->values[2] = (result[2] / result[3]);
				return coordinateField->assign(cache, *coordinateCache);
			}
		}
	}
	return FIELD_ASSIGNMENT_RESULT_FAIL;
}

} //namespace

cmzn_field_id cmzn_fieldmodule_create_field_projection(
	cmzn_fieldmodule_id field_module,
	cmzn_field_id source_field, cmzn_field_id projection_matrix_field)
{
	Computed_field *field = NULL;
	if (field_module && source_field && source_field->isNumerical() &&
		projection_matrix_field && projection_matrix_field->isNumerical())
	{
		int number_of_components = (projection_matrix_field->number_of_components /
			(source_field->number_of_components + 1)) - 1;
		if ((number_of_components + 1)*(source_field->number_of_components + 1) ==
			projection_matrix_field->number_of_components)
		{
			Computed_field *source_fields[2];
			source_fields[0] = source_field;
			source_fields[1] = projection_matrix_field;
			field = Computed_field_create_generic(field_module,
				/*check_source_field_regions*/true,
				number_of_components,
				/*number_of_source_fields*/2, source_fields,
				/*number_of_source_values*/0, NULL,
				new Computed_field_projection(
					/*matrix_rows*/(number_of_components + 1),
					/*matrix_columns*/(source_field->number_of_components + 1)));
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"cmzn_fieldmodule_create_field_projection.  Projection matrix field %s has invalid number of components",
				projection_matrix_field->name);
		}
	}
	return (field);
}

/***************************************************************************//**
 * If the field is of type COMPUTED_FIELD_PROJECTION, the source_field and
 * projection matrix_field used by it are returned.
 */
int Computed_field_get_type_projection(struct Computed_field *field,
	struct Computed_field **source_field, struct Computed_field **projection_matrix_field)
{
	Computed_field_projection* core;
	int return_code;

	ENTER(Computed_field_get_type_projection);
	if (field && (core = dynamic_cast<Computed_field_projection*>(field->core)) &&
		source_field && projection_matrix_field)
	{
		*source_field = field->source_fields[0];
		*projection_matrix_field = field->source_fields[1];
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_projection.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
}

namespace {

const char computed_field_transpose_type_string[] = "transpose";

class Computed_field_transpose : public Computed_field_core
{
public:
	int source_number_of_rows;

	Computed_field_transpose(int source_number_of_rows) :
		Computed_field_core(), source_number_of_rows(source_number_of_rows)
	{
	};

private:
	Computed_field_core *copy()
	{
		return new Computed_field_transpose(source_number_of_rows);
	}

	const char *get_type_string()
	{
		return(computed_field_transpose_type_string);
	}

	int compare(Computed_field_core* other_field);

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_transpose::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Compare the type specific data
==============================================================================*/
{
	Computed_field_transpose *other;
	int return_code;

	ENTER(Computed_field_transpose::compare);
	if (field && (other = dynamic_cast<Computed_field_transpose*>(other_core)))
	{
		if (source_number_of_rows == other->source_number_of_rows)
		{
			return_code = 1;
		}
		else
		{
			return_code=0;
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_transpose::compare */

int Computed_field_transpose::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		/* returns n row x m column tranpose of m row x n column source field,
			 where values always change along rows fastest */
		const int m = this->source_number_of_rows;
		const int n = getSourceField(0)->number_of_components / m;
		FE_value *source_values = sourceCache->values;
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < m; j++)
			{
				valueCache.values[i*m + j] = source_values[j*n + i];
			}
		}
		int number_of_xi = cache.getRequestedDerivatives();
		if (number_of_xi && sourceCache->derivatives_valid)
		{
			/* transpose derivatives in same way as values */
			for (int i = 0; i < n; i++)
			{
				for (int j = 0; j < m; j++)
				{
					FE_value *source_derivatives = sourceCache->derivatives +
						number_of_xi*(j*n + i);
					FE_value *destination_derivatives = valueCache.derivatives +
						number_of_xi*(i*m + j);
					for (int d = 0; d < number_of_xi; d++)
					{
						destination_derivatives[d] = source_derivatives[d];
					}
				}
			}
			valueCache.derivatives_valid=1;
		}
		else
		{
			valueCache.derivatives_valid=0;
		}
		return 1;
	}
	return 0;
}

int Computed_field_transpose::list()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_transpose);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source number of rows : %d\n",source_number_of_rows);
		display_message(INFORMATION_MESSAGE,"    source field : %s\n",
			field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_transpose.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_transpose */

char *Computed_field_transpose::get_command_string()
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_transpose::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_transpose_type_string, &error);
		sprintf(temp_string, " source_number_of_rows %d",
			source_number_of_rows);
		append_string(&command_string, temp_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_transpose::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_transpose::get_command_string */

} //namespace

Computed_field *cmzn_fieldmodule_create_field_transpose(
	struct cmzn_fieldmodule *field_module,
	int source_number_of_rows, struct Computed_field *source_field)
{
	struct Computed_field *field = NULL;
	if (field_module && (0 < source_number_of_rows) && source_field && source_field->isNumerical() &&
		(0 == (source_field->number_of_components % source_number_of_rows)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			source_field->number_of_components,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_transpose(source_number_of_rows));
	}
	return (field);
}

int Computed_field_get_type_transpose(struct Computed_field *field,
	int *source_number_of_rows, struct Computed_field **source_field)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_TRANSPOSE, the
<source_number_of_rows> and <source_field> used by it are returned.
==============================================================================*/
{
	Computed_field_transpose* core;
	int return_code;

	ENTER(Computed_field_get_type_transpose);
	if (field && (core = dynamic_cast<Computed_field_transpose*>(field->core)) &&
		source_field)
	{
		*source_number_of_rows = core->source_number_of_rows;
		*source_field = field->source_fields[0];
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_transpose.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_transpose */

namespace {

const char computed_field_quaternion_to_matrix_type_string[] = "quaternion_to_matrix";

class Computed_field_quaternion_to_matrix : public Computed_field_core
{
public:

	 Computed_field_quaternion_to_matrix() :
			Computed_field_core()
	 {
	 };

	 ~Computed_field_quaternion_to_matrix()
	 {
	 };

private:
	 Computed_field_core *copy()
	 {
			return new Computed_field_quaternion_to_matrix();
	 }

	 const char *get_type_string()
	 {
			return(computed_field_quaternion_to_matrix_type_string);
	 }

	 int compare(Computed_field_core* other_field)
	 {
			if (dynamic_cast<Computed_field_quaternion_to_matrix*>(other_field))
			{
				 return 1;
			}
			else
			{
				 return 0;
			}
	 }

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_quaternion_to_matrix::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		Quaternion quat(
			/*w*/sourceCache->values[0],
			/*x*/sourceCache->values[1],
			/*y*/sourceCache->values[2],
			/*z*/sourceCache->values[3]);
		return quat.quaternion_to_matrix(valueCache.values);
	}
	return 0;
}

int Computed_field_quaternion_to_matrix::list()
/*******************************************************************************
LAST MODIFIED: 18 Jun 2008

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_quaternion_to_matrix);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_quaternion_to_matrix.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_quaternion_to_matrix */

char *Computed_field_quaternion_to_matrix::get_command_string()
/*******************************************************************************
LAST MODIFIED : 18 Jun 2008

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_quaternion_to_matrix::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_quaternion_to_matrix_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_quaternion_to_matrix::get_command_string.  "
			"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_quaternion_to_matrix::get_command_string
		 */
} //namespace

/***************************************************************************//**
 * Creates a 4x4 (= 16 component) transformation matrix from a 4 component
 * quaternion valued source field.
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  4 component field giving source quaternion value.
 * @return Newly created field.
 */
Computed_field *Computed_field_create_quaternion_to_matrix(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field)
{
	struct Computed_field *field = NULL;
	if (field_module && source_field && source_field->isNumerical() &&
		(source_field->number_of_components == 4))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/16,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_quaternion_to_matrix());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_create_quaternion_to_matrix.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_quaternion_to_matrix(struct Computed_field *field,
	struct Computed_field **quaternion_to_matrix_field)
/*******************************************************************************
LAST MODIFIED : 18 Jun 2008

DESCRIPTION :
If the field is of type 'transformation', the <source_field> it calculates the
transformation of is returned.
==============================================================================*/
{
	 int return_code;

	ENTER(Computed_field_get_type_quatenions_to_transformation);
	if (field && (dynamic_cast<Computed_field_quaternion_to_matrix*>(field->core)))
	{
		*quaternion_to_matrix_field = field->source_fields[0];
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_quaternion_to_matrix.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_quaternion */

namespace {

const char computed_field_matrix_to_quaternion_type_string[] = "matrix_to_quaternion";

class Computed_field_matrix_to_quaternion : public Computed_field_core
{
public:

	 Computed_field_matrix_to_quaternion() :
			Computed_field_core()
	 {
	 };

	 ~Computed_field_matrix_to_quaternion()
	 {
	 };

private:
	 Computed_field_core *copy()
	 {
			return new Computed_field_matrix_to_quaternion();
	 }

	 const char *get_type_string()
	 {
			return(computed_field_matrix_to_quaternion_type_string);
	 }

	int compare(Computed_field_core* other_field)
	 {
			if (dynamic_cast<Computed_field_matrix_to_quaternion*>(other_field))
			{
				 return 1;
			}
			else
			{
				 return 0;
			}
	 }

	int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_matrix_to_quaternion::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
	RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		return Quaternion::matrix_to_quaternion(
			/*source*/sourceCache->values, /*destination*/valueCache.values);
	}
	return 0;
}

int Computed_field_matrix_to_quaternion::list()
/*******************************************************************************
LAST MODIFIED : 18 Jun 2008

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_matrix_to_quaternion);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    field : %s\n",field->source_fields[0]->name);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_matrix_to_quaternion.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_matrix_to_quaternion */

char *Computed_field_matrix_to_quaternion::get_command_string()
/*******************************************************************************
LAST MODIFIED : 18 Jun 2008

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name;
	int error;

	ENTER(Computed_field_matrix_to_quaternion::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string,
			computed_field_matrix_to_quaternion_type_string, &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_matrix_to_quaternion::get_command_string.  "
			"Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_matrix_to_quaternion::get_command_string
		 */

} //namespace

/***************************************************************************//**
 * Creates a 4 component field returning the nearest quaternion value equivalent
 * to 4x4 matrix source field
 *
 * @param field_module  Region field module which will own new field.
 * @param source_field  4x4 component source field.
 * @return Newly created field.
 */
Computed_field *Computed_field_create_matrix_to_quaternion(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *source_field)
{
	struct Computed_field *field = NULL;
	if (field_module && source_field && source_field->isNumerical() &&
		(source_field->number_of_components == 16))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/true,
			/*number_of_components*/4,
			/*number_of_source_fields*/1, &source_field,
			/*number_of_source_values*/0, NULL,
			new Computed_field_matrix_to_quaternion());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_create_matrix_to_quaternion.  Invalid argument(s)");
	}

	return (field);
}

int Computed_field_get_type_matrix_to_quaternion(struct Computed_field *field,
	struct Computed_field **matrix_to_quaternion_field)
/*******************************************************************************
LAST MODIFIED : 18 Jun 2008

DESCRIPTION :
If the field is of type 'transformation', the <source_field> it calculates the
transformation of is returned.
==============================================================================*/
{
	 int return_code;

	ENTER(Computed_field_get_type_quatenions_to_transformation);
	if (field && (dynamic_cast<Computed_field_matrix_to_quaternion*>(field->core)))
	{
		 *matrix_to_quaternion_field = field->source_fields[0];
		 return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_matrix_to_quaternion.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_quaternion */

