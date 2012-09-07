/***************************************************************************//**
 * FILE : computed_field_matrix_operators.c
 *
 * Implements a number of basic matrix operations on computed fields.
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include <cmath>
extern "C" {
#include "api/cmiss_field_matrix_operators.h"
#include "computed_field/computed_field.h"
}
#include "computed_field/computed_field_matrix_operators.hpp"
#include "computed_field/computed_field_private.hpp"
extern "C" {
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/matrix_vector.h"
#include "general/mystring.h"
#include "general/message.h"
#include "graphics/quaternion.hpp"
}

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

char computed_field_determinant_type_string[] = "determinant";

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

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_determinant::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
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

Cmiss_field_id Cmiss_field_module_create_determinant(
	Cmiss_field_module_id field_module, Cmiss_field_id source_field)
{
	Cmiss_field_id field = 0;
	if (field_module && source_field &&
		Computed_field_is_square_matrix(source_field, (void *)NULL) &&
		(Cmiss_field_get_number_of_components(source_field) <= 9))
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

/***************************************************************************//**
 * Command modifier function which converts field into type 'determinant'
 * (if it is not already) and allows its contents to be modified.
 */
int define_Computed_field_type_determinant(struct Parse_state *state,
	void *field_modify_void, void *computed_field_matrix_operators_package_void)
{
	int return_code;

	ENTER(define_Computed_field_type_determinant);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	Computed_field_modify_data *field_modify =
		reinterpret_cast<Computed_field_modify_data*>(field_modify_void);
	if (state && field_modify)
	{
		return_code = 1;
		Cmiss_field_id source_field = 0;
		if (NULL != field_modify->get_field())
		{
			Computed_field_determinant *determinant_core =
				dynamic_cast<Computed_field_determinant*>(field_modify->get_field()->core);
			if (determinant_core)
			{
				source_field = Cmiss_field_get_source_field(field_modify->get_field(), 1);
			}
		}
		Option_table *option_table = CREATE(Option_table)();
		Option_table_add_help(option_table,
			"Creates a field returning the scalar real determinant of a square matrix. "
			"Only supports 1, 4 (2x2) and 9 (3x3) component source fields.");
		struct Set_Computed_field_conditional_data set_source_field_data =
		{
			Computed_field_is_square_matrix,
			/*user_data*/0,
			field_modify->get_field_manager()
		};
		Option_table_add_entry(option_table, "field", &source_field,
			&set_source_field_data, set_Computed_field_conditional);
		return_code = Option_table_multi_parse(option_table, state);
		if (return_code)
		{
			return_code = field_modify->update_field_and_deaccess(
				Cmiss_field_module_create_determinant(field_modify->get_field_module(),
					source_field));
		}
		DESTROY(Option_table)(&option_table);
		if (source_field)
		{
			Cmiss_field_destroy(&source_field);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_determinant.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
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

char computed_field_eigenvalues_type_string[] = "eigenvalues";

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

	virtual FieldValueCache *createValueCache(Cmiss_field_cache& /*parentCache*/)
	{
		return new EigenvalueFieldValueCache(field->number_of_components);
	}

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_eigenvalues::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
{
	EigenvalueFieldValueCache &valueCache = EigenvalueFieldValueCache::cast(inValueCache);
	RealFieldValueCache *sourceCache = RealFieldValueCache::cast(getSourceField(0)->evaluate(cache));
	if (sourceCache)
	{
		const int n = field->number_of_components;
		const int matrix_size = n * n;
		Cmiss_field_id source_field = getSourceField(0);
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

} //namespace

Computed_field *Cmiss_field_module_create_eigenvalues(
	struct Cmiss_field_module *field_module,
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

int define_Computed_field_type_eigenvalues(struct Parse_state *state,
	void *field_modify_void, void *computed_field_matrix_operators_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type 'eigenvalues' (if it is not already) and allows its
contents to be modified.
==============================================================================*/
{
	int return_code;
	struct Computed_field *source_field;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_eigenvalues);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code=1;
		source_field = (struct Computed_field *)NULL;
		if ((NULL != field_modify->get_field()) &&
			(computed_field_eigenvalues_type_string ==
				Computed_field_get_type_string(field_modify->get_field())))
		{
			return_code = Computed_field_get_type_eigenvalues(field_modify->get_field(), &source_field);
		}
		if (return_code)
		{
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}
			option_table = CREATE(Option_table)();
			Option_table_add_help(option_table,
			  "An eigenvalues field returns the n eigenvalues of an (n * n) square matrix field.  Here, a 9 component source field is interpreted as a (3 * 3) matrix with the first 3 components being the first row, the next 3 components being the middle row, and so on.  The related eigenvectors field can extract the corresponding eigenvectors for the eigenvalues. See a/large_strain for an example of using the eigenvalues and eigenvectors fields.");
			set_source_field_data.conditional_function =
				Computed_field_is_square_matrix;
			set_source_field_data.conditional_function_user_data = (void *)NULL;
			set_source_field_data.computed_field_manager =
				field_modify->get_field_manager();
			Option_table_add_entry(option_table, "field", &source_field,
				&set_source_field_data, set_Computed_field_conditional);
			return_code = Option_table_multi_parse(option_table, state);
			if (return_code)
			{
				return_code = field_modify->update_field_and_deaccess(
					Cmiss_field_module_create_eigenvalues(field_modify->get_field_module(),
						source_field));
			}
			DESTROY(Option_table)(&option_table);
			if (source_field)
			{
				DEACCESS(Computed_field)(&source_field);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_eigenvalues.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_eigenvalues */

namespace {

char computed_field_eigenvectors_type_string[] = "eigenvectors";

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

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_eigenvectors::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
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

Computed_field *Cmiss_field_module_create_eigenvectors(
	struct Cmiss_field_module *field_module,
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

int define_Computed_field_type_eigenvectors(struct Parse_state *state,
	void *field_modify_void,void *computed_field_matrix_operators_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type 'eigenvectors' (if it is not  already) and allows
its contents to be modified.
==============================================================================*/
{
	int return_code;
	struct Computed_field *source_field;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_eigenvectors);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code=1;
		/* get valid parameters for projection field */
		source_field = (struct Computed_field *)NULL;
		if ((NULL != field_modify->get_field()) &&
			(computed_field_eigenvectors_type_string ==
				Computed_field_get_type_string(field_modify->get_field())))
		{
			return_code=Computed_field_get_type_eigenvectors(field_modify->get_field(), &source_field);
		}
		if (return_code)
		{
			/* must access objects for set functions */
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}
			option_table = CREATE(Option_table)();
			Option_table_add_help(option_table,
			  "An eigenvectors field returns vectors corresponding to each eigenvalue from a source eigenvalues field.  For example, if 3 eigenvectors have been computed for a (3 * 3) matrix = 9 component field, the eigenvectors will be a 9 component field with the eigenvector corresponding to the first eigenvalue in the first 3 components, the second eigenvector in the next 3 components, and so on.  See a/large_strain for an example of using the eigenvalues and eigenvectors fields.");
			/* field */
			set_source_field_data.computed_field_manager =
				field_modify->get_field_manager();
			set_source_field_data.conditional_function =
				Computed_field_is_type_eigenvalues_conditional;
			set_source_field_data.conditional_function_user_data = (void *)NULL;
			Option_table_add_entry(option_table, "eigenvalues", &source_field,
				&set_source_field_data, set_Computed_field_conditional);
			return_code = Option_table_multi_parse(option_table, state);
			if (return_code)
			{
				return_code = field_modify->update_field_and_deaccess(
					Cmiss_field_module_create_eigenvectors(field_modify->get_field_module(),
						source_field));
			}
			if (source_field)
			{
				DEACCESS(Computed_field)(&source_field);
			}
			DESTROY(Option_table)(&option_table);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_eigenvectors.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_eigenvectors */

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

char computed_field_matrix_invert_type_string[] = "matrix_invert";

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

	virtual FieldValueCache *createValueCache(Cmiss_field_cache& /*parentCache*/)
	{
		return new MatrixInvertFieldValueCache(field->number_of_components, Computed_field_get_square_matrix_size(getSourceField(0)));
	}

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_matrix_invert::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
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

Computed_field *Cmiss_field_module_create_matrix_invert(
	struct Cmiss_field_module *field_module,
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

int define_Computed_field_type_matrix_invert(struct Parse_state *state,
	void *field_modify_void, void *computed_field_matrix_operators_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type 'matrix_invert' (if it is not already) and allows its
contents to be modified.
==============================================================================*/
{
	int return_code;
	struct Computed_field *source_field;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_matrix_invert);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code = 1;
		source_field = (struct Computed_field *)NULL;
		if ((NULL != field_modify->get_field()) &&
			(computed_field_matrix_invert_type_string ==
				Computed_field_get_type_string(field_modify->get_field())))
		{
			return_code = Computed_field_get_type_matrix_invert(field_modify->get_field(), &source_field);
		}
		if (return_code)
		{
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}
			option_table = CREATE(Option_table)();
			Option_table_add_help(option_table,
			  "A matrix_invert field returns the inverse of a square matrix.  Here, a 9 component source field is interpreted as a (3 * 3) matrix with the first 3 components being the first row, the next 3 components being the middle row, and so on.  See a/current_density for an example of using the matrix_invert field.");
			set_source_field_data.conditional_function =
				Computed_field_is_square_matrix;
			set_source_field_data.conditional_function_user_data = (void *)NULL;
			set_source_field_data.computed_field_manager =
				field_modify->get_field_manager();
			Option_table_add_entry(option_table, "field", &source_field,
				&set_source_field_data, set_Computed_field_conditional);
			return_code = Option_table_multi_parse(option_table, state);
			if (return_code)
			{
				return_code = field_modify->update_field_and_deaccess(
					Cmiss_field_module_create_matrix_invert(field_modify->get_field_module(),
						source_field));
			}
			DESTROY(Option_table)(&option_table);
			if (source_field)
			{
				DEACCESS(Computed_field)(&source_field);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_matrix_invert.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_matrix_invert */

namespace {

char computed_field_matrix_multiply_type_string[] = "matrix_multiply";

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

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

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

int Computed_field_matrix_multiply::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
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

Computed_field *Cmiss_field_module_create_matrix_multiply(
	struct Cmiss_field_module *field_module,
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
				"Cmiss_field_module_create_matrix_multiply.  "
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

int define_Computed_field_type_matrix_multiply(struct Parse_state *state,
	void *field_modify_void,void *computed_field_matrix_operators_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_MATRIX_MULTIPLY (if it is not 
already) and allows its contents to be modified.
==============================================================================*/
{
	const char *current_token;
	int i, number_of_rows, return_code;
	struct Computed_field **source_fields;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_array_data set_field_array_data;
	struct Set_Computed_field_conditional_data set_field_data;

	ENTER(define_Computed_field_type_matrix_multiply);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code=1;
		if (ALLOCATE(source_fields,struct Computed_field *,2))
		{
			/* get valid parameters for matrix_multiply field */
			number_of_rows = 1;
			source_fields[0] = (struct Computed_field *)NULL;
			source_fields[1] = (struct Computed_field *)NULL;
			if ((NULL != field_modify->get_field()) &&
				(computed_field_matrix_multiply_type_string ==
					Computed_field_get_type_string(field_modify->get_field())))
			{
				return_code=Computed_field_get_type_matrix_multiply(field_modify->get_field(),
					&number_of_rows,&(source_fields[0]),&(source_fields[1]));
			}
			if (return_code)
			{
				/* ACCESS the source fields for set_Computed_field_array */
				for (i=0;i<2;i++)
				{
					if (source_fields[i])
					{
						ACCESS(Computed_field)(source_fields[i]);
					}
				}
				/* try to handle help first */
				current_token=state->current_token;
				if (current_token != 0)
				{
					if (!(strcmp(PARSER_HELP_STRING,current_token)&&
						strcmp(PARSER_RECURSIVE_HELP_STRING,current_token)))
					{
						option_table = CREATE(Option_table)();
						Option_table_add_help(option_table,
						  "A matrix_mutliply field calculates the product of two matrices, giving a new m by n matrix.  The product is represented as a field with a list of (m * n) components.   The components of the matrix are listed row by row.  The <number_of_rows> m is used to infer the dimensions of the source matrices.  The two source <fields> are multiplied, with the components of the first interpreted as a matrix with dimensions m by s and the second as a matrix with dimensions s by n.  If the matrix dimensions are not consistent then an error is returned.  See a/curvature for an example of using the matrix_multiply field.");
						Option_table_add_entry(option_table,"number_of_rows",
							&number_of_rows,NULL,set_int_positive);
						set_field_data.conditional_function=
							Computed_field_has_numerical_components;
						set_field_data.conditional_function_user_data=(void *)NULL;
						set_field_data.computed_field_manager=
							field_modify->get_field_manager();
						set_field_array_data.number_of_fields=2;
						set_field_array_data.conditional_data= &set_field_data;
						Option_table_add_entry(option_table,"fields",source_fields,
							&set_field_array_data,set_Computed_field_array);
						return_code=Option_table_multi_parse(option_table,state);
						DESTROY(Option_table)(&option_table);
					}
					else
					{
						/* if the "number_of_rows" token is next, read it */
						if (fuzzy_string_compare(current_token,"number_of_rows"))
						{
							option_table = CREATE(Option_table)();
							/* number_of_rows */
							Option_table_add_entry(option_table,"number_of_rows",
								&number_of_rows,NULL,set_int_positive);
							return_code = Option_table_parse(option_table,state);
							DESTROY(Option_table)(&option_table);
						}
						if (return_code)
						{
							option_table = CREATE(Option_table)();
							set_field_data.conditional_function=
								Computed_field_has_numerical_components;
							set_field_data.conditional_function_user_data=(void *)NULL;
							set_field_data.computed_field_manager=
								field_modify->get_field_manager();
							set_field_array_data.number_of_fields=2;
							set_field_array_data.conditional_data= &set_field_data;
							Option_table_add_entry(option_table,"fields",source_fields,
								&set_field_array_data,set_Computed_field_array);
							return_code = Option_table_multi_parse(option_table, state);
							if (return_code)
							{
								return_code = field_modify->update_field_and_deaccess(
									Cmiss_field_module_create_matrix_multiply(field_modify->get_field_module(),
										number_of_rows, source_fields[0], source_fields[1]));
							}
							DESTROY(Option_table)(&option_table);
						}
						if (!return_code)
						{
							/* error */
							display_message(ERROR_MESSAGE,
								"define_Computed_field_type_matrix_multiply.  Failed");
						}
					}
				}
				else
				{
					display_message(ERROR_MESSAGE, "Missing command options");
					return_code = 0;
				}
				for (i=0;i<2;i++)
				{
					if (source_fields[i])
					{
						DEACCESS(Computed_field)(&(source_fields[i]));
					}
				}
			}
			DEALLOCATE(source_fields);
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"define_Computed_field_type_matrix_multiply.  Not enough memory");
			return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_matrix_multiply.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_matrix_multiply */

namespace {

char computed_field_projection_type_string[] = "projection";

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

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();

	virtual enum FieldAssignmentResult assign(Cmiss_field_cache& cache, RealFieldValueCache& valueCache);
};

int Computed_field_projection::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
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
enum FieldAssignmentResult Computed_field_projection::assign(Cmiss_field_cache& cache, RealFieldValueCache& valueCache)
{
	if ((field->number_of_components == 3) && (matrix_rows == 4) && (matrix_columns == 4))
	{
		Cmiss_field_id coordinateField = getSourceField(0);
		Cmiss_field_id projectionField = getSourceField(1);
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

Cmiss_field_id Cmiss_field_module_create_projection(
	Cmiss_field_module_id field_module,
	Cmiss_field_id source_field, Cmiss_field_id projection_matrix_field)
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
				"Cmiss_field_module_create_projection.  Projection matrix field %s has invalid number of components",
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

/***************************************************************************//**
 * Converts <field> into type COMPUTED_FIELD_PROJECTION (if it is not already)
 * and allows its contents to be modified.
 */
int define_Computed_field_type_projection(struct Parse_state *state,
	void *field_modify_void,void *computed_field_matrix_operators_package_void)
{
	int return_code;

	ENTER(define_Computed_field_type_projection);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	Computed_field_modify_data *field_modify =
		reinterpret_cast<Computed_field_modify_data *>(field_modify_void);
	if (state && field_modify)
	{
		return_code = 1;
		Cmiss_field_id source_field = 0;
		Cmiss_field_id projection_matrix_field = 0;
		if ((NULL != field_modify->get_field()) &&
			(NULL != (dynamic_cast<Computed_field_projection*>(field_modify->get_field()->core))))
		{
			Computed_field_get_type_projection(field_modify->get_field(),
				&source_field, &projection_matrix_field);
		}
		if (source_field)
		{
			ACCESS(Computed_field)(source_field);
		}
		if (projection_matrix_field)
		{
			ACCESS(Computed_field)(projection_matrix_field);
		}
		Option_table *option_table = CREATE(Option_table)();
		Option_table_add_help(option_table,
			"Creates a projection field returning the result of a matrix multiplication "
			"with perspective division on the source field vector. The source_field vector "
			"is expanded to a homogeneous coordinate by appending a component of value 1, "
			"which is multiplied by the projection_matrix field, and the extra calculated "
			"value resulting from the unit component is used to divide through each of the "
			"other components to give a perspective projection in the resulting field. "
			"The projection_matrix field must have have a multiple of "
			"(source_field->number_of_components + 1) components forming a matrix with "
			"that many columns and the resulting (number_of_components + 1) rows. The "
			"first values in the projection_matrix are across the first row, followed by "
			"the next row and so on. "
			"Hence a 4x4 matrix transforms a 3-component vector to a 3-component vector.\n");
		/* field */
		struct Set_Computed_field_conditional_data set_field_data;
		set_field_data.conditional_function = Computed_field_has_numerical_components;
		set_field_data.conditional_function_user_data = (void *)NULL;
		set_field_data.computed_field_manager = field_modify->get_field_manager();
		Option_table_add_entry(option_table, "field", &source_field,
			&set_field_data, set_Computed_field_conditional);
		Option_table_add_entry(option_table, "projection_matrix", &projection_matrix_field,
			&set_field_data, set_Computed_field_conditional);
		return_code = Option_table_multi_parse(option_table, state);
		DESTROY(Option_table)(&option_table);
		if (return_code)
		{
			return_code = field_modify->update_field_and_deaccess(
				Cmiss_field_module_create_projection(field_modify->get_field_module(),
					source_field, projection_matrix_field));
		}
		if (source_field)
		{
			DEACCESS(Computed_field)(&source_field);
		}
		if (projection_matrix_field)
		{
			DEACCESS(Computed_field)(&projection_matrix_field);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_projection.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_projection */

namespace {

char computed_field_transpose_type_string[] = "transpose";

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

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

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

int Computed_field_transpose::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
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

Computed_field *Cmiss_field_module_create_transpose(
	struct Cmiss_field_module *field_module,
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

int define_Computed_field_type_transpose(struct Parse_state *state,
	void *field_modify_void,void *computed_field_matrix_operators_package_void)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_TRANSPOSE (if it is not 
already) and allows its contents to be modified.
==============================================================================*/
{
	const char *current_token;
	int source_number_of_rows, return_code;
	struct Computed_field *source_field;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_transpose);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code=1;
		/* get valid parameters for transpose field */
		source_number_of_rows = 1;
		source_field = (struct Computed_field *)NULL;
		if ((NULL != field_modify->get_field()) &&
			(computed_field_transpose_type_string ==
				Computed_field_get_type_string(field_modify->get_field())))
		{
			return_code=Computed_field_get_type_transpose(field_modify->get_field(),
				&source_number_of_rows,&source_field);
		}
		if (return_code)
		{
			/* ACCESS the source fields for set_Computed_field_conditional */
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}
			/* try to handle help first */
			current_token = state->current_token;
			if (current_token != 0)
			{
				set_source_field_data.conditional_function =
					Computed_field_has_numerical_components;
				set_source_field_data.conditional_function_user_data = (void *)NULL;
				set_source_field_data.computed_field_manager =
					field_modify->get_field_manager();
				if (!(strcmp(PARSER_HELP_STRING,current_token)&&
					strcmp(PARSER_RECURSIVE_HELP_STRING,current_token)))
				{
					option_table = CREATE(Option_table)();
					Option_table_add_help(option_table,
					  "A transpose field returns the transpose of a source matrix field.  If the source <field> has (m * n) components where m is specified by <source_number_of_rows> (with the first n components being the first row), the result is its (n * m) transpose.  See a/current_density for an example of using the matrix_invert field.");
					Option_table_add_entry(option_table,"source_number_of_rows",
						&source_number_of_rows,NULL,set_int_positive);
					Option_table_add_entry(option_table,"field",&source_field,
						&set_source_field_data,set_Computed_field_conditional);
					return_code=Option_table_multi_parse(option_table,state);
					DESTROY(Option_table)(&option_table);
				}
				else
				{
					/* if the "source_number_of_rows" token is next, read it */
					if (fuzzy_string_compare(current_token,"source_number_of_rows"))
					{
						option_table = CREATE(Option_table)();
						/* source_number_of_rows */
						Option_table_add_entry(option_table,"source_number_of_rows",
							&source_number_of_rows,NULL,set_int_positive);
						return_code = Option_table_parse(option_table,state);
						DESTROY(Option_table)(&option_table);
					}
					if (return_code)
					{
						option_table = CREATE(Option_table)();
						Option_table_add_entry(option_table,"field",&source_field,
							&set_source_field_data,set_Computed_field_conditional);
						return_code = Option_table_multi_parse(option_table, state);
						if (return_code)
						{
							return_code = field_modify->update_field_and_deaccess(
								Cmiss_field_module_create_transpose(field_modify->get_field_module(),
									source_number_of_rows, source_field));
						}
						DESTROY(Option_table)(&option_table);
					}
					if (!return_code)
					{
						/* error */
						display_message(ERROR_MESSAGE,
							"define_Computed_field_type_transpose.  Failed");
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "Missing command options");
				return_code = 0;
			}
			if (source_field)
			{
				DEACCESS(Computed_field)(&source_field);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_transpose.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_transpose */


namespace {

char computed_field_quaternion_to_matrix_type_string[] = "quaternion_to_matrix";

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

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_quaternion_to_matrix::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
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
	struct Cmiss_field_module *field_module,
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

int define_Computed_field_type_quaternion_to_matrix(struct Parse_state *state,
	void *field_modify_void, void *computed_field_matrix_operators_package_void)
/*******************************************************************************
LAST MODIFIED : 18 Jun 2008

DESCRIPTION :
Converts a "quaternion" to a transformation matrix.
==============================================================================*/
{
	int return_code;
	struct Computed_field **source_fields;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_quaternion_to_matrix);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code=1;
		source_fields = (struct Computed_field **)NULL;
		if (ALLOCATE(source_fields, struct Computed_field *, 1))
		{
			 source_fields[0] = (struct Computed_field *)NULL;
				if ((NULL != field_modify->get_field()) &&
					(computed_field_quaternion_to_matrix_type_string ==
						Computed_field_get_type_string(field_modify->get_field())))
			 {
					return_code = Computed_field_get_type_quaternion_to_matrix(field_modify->get_field(), 
						 source_fields);
			 }
			 if (return_code)
			 {
					if (source_fields[0])
					{
						 ACCESS(Computed_field)(source_fields[0]);
					}
					option_table = CREATE(Option_table)();
					Option_table_add_help(option_table,
						 "A computed field to convert a quaternion (w,x,y,z) to a 4x4 matrix,");
					set_source_field_data.computed_field_manager =
						field_modify->get_field_manager();
					set_source_field_data.conditional_function =
						 Computed_field_has_4_components;
					set_source_field_data.conditional_function_user_data = (void *)NULL;
					Option_table_add_entry(option_table, "field", &source_fields[0],
						 &set_source_field_data, set_Computed_field_conditional);
					return_code = Option_table_multi_parse(option_table, state);
					if (return_code)
					{
						return_code = field_modify->update_field_and_deaccess(
							Computed_field_create_quaternion_to_matrix(
								field_modify->get_field_module(), source_fields[0]));
					}
					else
					{
						 if ((!state->current_token)||
								(strcmp(PARSER_HELP_STRING,state->current_token)&&
									 strcmp(PARSER_RECURSIVE_HELP_STRING,state->current_token)))
						 {
								/* error */
								display_message(ERROR_MESSAGE,
									 "define_Computed_field_type_quaternion_to_matrix.  Failed");
						 }
					}
					if (source_fields[0])
					{
						 DEACCESS(Computed_field)(&source_fields[0]);
					}
					DESTROY(Option_table)(&option_table);
			 }
			 DEALLOCATE(source_fields);
		}
		else
		{
			 display_message(ERROR_MESSAGE,
					"define_Computed_field_type_quaternion_to_matrix. Not enought memory");
			 return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_quaternion_to_matrix. Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_quaternion_to_matrix */


namespace {

char computed_field_matrix_to_quaternion_type_string[] = "matrix_to_quaternion";

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

	int evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache);

	int list();

	char* get_command_string();
};

int Computed_field_matrix_to_quaternion::evaluate(Cmiss_field_cache& cache, FieldValueCache& inValueCache)
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
	struct Cmiss_field_module *field_module,
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

int define_Computed_field_type_matrix_to_quaternion(struct Parse_state *state,
	void *field_modify_void, void *computed_field_matrix_operators_package_void)
/*******************************************************************************
LAST MODIFIED : 18 Jun 2008

DESCRIPTION :
Converts a transformation matrix to  a "quaternion".
==============================================================================*/
{
	int return_code;
	struct Computed_field **source_fields;
	Computed_field_modify_data *field_modify;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_matrix_to_quaternion);
	USE_PARAMETER(computed_field_matrix_operators_package_void);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void))
	{
		return_code=1;
		source_fields = (struct Computed_field **)NULL;
		if (ALLOCATE(source_fields, struct Computed_field *, 1))
		{
			 source_fields[0] = (struct Computed_field *)NULL;
				if ((NULL != field_modify->get_field()) &&
					(computed_field_matrix_to_quaternion_type_string ==
						Computed_field_get_type_string(field_modify->get_field())))
			 {
					return_code = Computed_field_get_type_matrix_to_quaternion(field_modify->get_field(), 
						 source_fields);
			 }
			 if (return_code)
			 {
					if (source_fields[0])
					{
						 ACCESS(Computed_field)(source_fields[0]);
					}
					option_table = CREATE(Option_table)();
					Option_table_add_help(option_table,
						 "A computed field to convert a 4x4 matrix to a quaternion.  "
						 "components of the matrix should be read in as follow       "
						 "    0   1   2   3                                          " 
						 "    4   5   6   7                                          "
						 "    8   9   10  11                                         "
						 "    12  13  14  15                                         \n");
					set_source_field_data.computed_field_manager =
						field_modify->get_field_manager();
					set_source_field_data.conditional_function =
						 Computed_field_has_16_components;
					set_source_field_data.conditional_function_user_data = (void *)NULL;
					Option_table_add_entry(option_table, "field", &source_fields[0],
						 &set_source_field_data, set_Computed_field_conditional);
					/* process the option table */
					return_code = Option_table_multi_parse(option_table, state);
					if (return_code)
					{
						return_code = field_modify->update_field_and_deaccess(
							Computed_field_create_matrix_to_quaternion(
								field_modify->get_field_module(), source_fields[0]));
					}
					else
					{
						 if ((!state->current_token)||
								(strcmp(PARSER_HELP_STRING,state->current_token)&&
									 strcmp(PARSER_RECURSIVE_HELP_STRING,state->current_token)))
						 {
								/* error */
								display_message(ERROR_MESSAGE,
									 "define_Computed_field_type_matrix_to_quaternion.  Failed");
						 }
					}
					/* no errors, not asking for help */
					if (source_fields[0])
					{
						 DEACCESS(Computed_field)(&source_fields[0]);
					}
					DESTROY(Option_table)(&option_table);
			 }
			 DEALLOCATE(source_fields);
		}
		else
		{
			 display_message(ERROR_MESSAGE,
					"define_Computed_field_type_matrix_to_quaternion. Not enought memory");
			 return_code=0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_matrix_to_quaternion. Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_matrix_to_quaternion */

int Computed_field_register_types_matrix_operators(
	struct Computed_field_package *computed_field_package)
/*******************************************************************************
LAST MODIFIED : 25 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	Computed_field_matrix_operators_package
		*computed_field_matrix_operators_package =
		new Computed_field_matrix_operators_package;

	ENTER(Computed_field_register_types_matrix_operators);
	if (computed_field_package)
	{
		return_code =
			Computed_field_package_add_type(computed_field_package,
				computed_field_determinant_type_string,
				define_Computed_field_type_determinant,
				computed_field_matrix_operators_package);
		return_code =
			Computed_field_package_add_type(computed_field_package,
				computed_field_eigenvalues_type_string,
				define_Computed_field_type_eigenvalues,
				computed_field_matrix_operators_package);
		return_code = 
			Computed_field_package_add_type(computed_field_package,
				computed_field_eigenvectors_type_string,
				define_Computed_field_type_eigenvectors,
				computed_field_matrix_operators_package);
		return_code = 
			Computed_field_package_add_type(computed_field_package,
				computed_field_matrix_invert_type_string,
				define_Computed_field_type_matrix_invert,
				computed_field_matrix_operators_package);
		return_code = 
			Computed_field_package_add_type(computed_field_package,
				computed_field_matrix_multiply_type_string,
				define_Computed_field_type_matrix_multiply,
				computed_field_matrix_operators_package);
		return_code = 
			Computed_field_package_add_type(computed_field_package,
				computed_field_projection_type_string,
				define_Computed_field_type_projection,
				computed_field_matrix_operators_package);
		return_code = 
			Computed_field_package_add_type(computed_field_package,
				computed_field_transpose_type_string,
				define_Computed_field_type_transpose,
				computed_field_matrix_operators_package);
		return_code = 
			Computed_field_package_add_type(computed_field_package,
			computed_field_quaternion_to_matrix_type_string,
			define_Computed_field_type_quaternion_to_matrix,
			computed_field_matrix_operators_package);
		return_code = 
			Computed_field_package_add_type(computed_field_package,
			computed_field_matrix_to_quaternion_type_string,
			define_Computed_field_type_matrix_to_quaternion,
			computed_field_matrix_operators_package);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_types_matrix_operators.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_types_matrix_operators */
