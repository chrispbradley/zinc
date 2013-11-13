/***************************************************************************//**
 * FILE : field_module.hpp
 *
 * Internal header of field module api.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FIELD_MODULE_H)
#define FIELD_MODULE_H

#include "zinc/fieldmodule.h"

struct cmzn_fieldmoduleevent
{
	cmzn_fieldmoduleevent_change_flags changeFlags;
	int access_count;

	cmzn_fieldmoduleevent() :
		changeFlags(CMZN_FIELDMODULEEVENT_CHANGE_FLAG_NONE),
		access_count(1)
	{
	}

	~cmzn_fieldmoduleevent()
	{
	}

	cmzn_fieldmoduleevent *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_fieldmoduleevent* &event);

};

struct cmzn_fieldmodulenotifier
{
private:
	cmzn_region_id region; // owning region: not accessed
	cmzn_fieldmodulenotifier_callback_function function;
	void *user_data;
	int access_count;

	cmzn_fieldmodulenotifier(cmzn_fieldmodule *fieldmodule);

	~cmzn_fieldmodulenotifier();

	static void fieldmanager_callback(struct MANAGER_MESSAGE(Computed_field) *message,
		void *fieldmodulenotifier_void);

public:

	/** private: external code must use cmzn_fieldmodule_create_notifier */
	static cmzn_fieldmodulenotifier *create(cmzn_fieldmodule *fieldmodule)
	{
		if (fieldmodule)
			return new cmzn_fieldmodulenotifier(fieldmodule);
		return 0;
	}

	cmzn_fieldmodulenotifier *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_fieldmodulenotifier* &notifier);

	int setCallback(cmzn_fieldmodulenotifier_callback_function function_in,
		void *user_data_in);

	void *getUserData()
	{
		return this->user_data;
	}

	void clearCallback();

	void regionDestroyed();

	void notify(cmzn_fieldmoduleevent *event)
	{
		if (this->function && event)
			(this->function)(event, this->user_data);
	}

};


/***************************************************************************//**
 * Creates field module object needed to create fields in supplied region.
 * Internally: Also used to set new field default arguments prior to create.
 *
 * @param region  The owning region.
 * @return  Field module for the supplied region.
 */
struct cmzn_fieldmodule *cmzn_fieldmodule_create(struct cmzn_region *region);

/***************************************************************************//**
 * Candidate for external API.
 * @return  1 if field is from this field_module, otherwise 0.
 */
int cmzn_fieldmodule_contains_field(cmzn_fieldmodule_id field_module,
	cmzn_field_id field);

/***************************************************************************//**
 * Internal, non-accessing version of cmzn_fieldmodule_get_region.
 *
 * @param field_module  The field module to query.
 * @return  Non-accessed handle to owning region for field_module.
 */
struct cmzn_region *cmzn_fieldmodule_get_region_internal(
	struct cmzn_fieldmodule *field_module);

/***************************************************************************//**
 * Get non-accessed pointer to master region for this field_module.
 *
 * @param field_module  The field module to query.
 * @return  Non-accessed handle to master region for field_module.
 */
struct cmzn_region *cmzn_fieldmodule_get_master_region_internal(
	struct cmzn_fieldmodule *field_module);

/***************************************************************************//**
 * Sets the name (or name stem if non-unique) of the next field to be created
 * with this field_module.
 *
 * @param field_module  The field module to create fields in.
 * @param field_name  Field name or name stem.
 * @return  Non-zero on success, 0 on failure.
 */
int cmzn_fieldmodule_set_field_name(struct cmzn_fieldmodule *field_module,
	const char *field_name);

/***************************************************************************//**
 * Gets a copy of the field name/stem set in this field_module.
 * Up to caller to DEALLOCATE.
 *
 * @param field_module  The field module to create fields in.
 * @return  Allocated copy of the name, or NULL if none.
 */
char *cmzn_fieldmodule_get_field_name(
	struct cmzn_fieldmodule *field_module);

/***************************************************************************//**
 * Sets the coordinate system to be used for subsequent fields created with
 * this field module.
 *
 * @param field_module  The field module to create fields in.
 * @param coordinate_system  The coordinate system to set.
 * @return  1 on success, 0 on failure.
 */
int cmzn_fieldmodule_set_coordinate_system(
	struct cmzn_fieldmodule *field_module,
	struct Coordinate_system coordinate_system);

/***************************************************************************//**
 * Returns the default coordinate system set in the field_module.
 *
 * @param field_module  The field module to create fields in.
 * @return  Copy of default coordinate system.
 */
struct Coordinate_system cmzn_fieldmodule_get_coordinate_system(
	struct cmzn_fieldmodule *field_module);

/***************************************************************************//**
 * Returns true if the default coordinate system has been explicitly set. 
 *
 * @param field_module  The field module to create fields in.
 * @return  1 if coordinate system set, 0 if never set.
 */
int cmzn_fieldmodule_coordinate_system_is_set(
	struct cmzn_fieldmodule *field_module);

/***************************************************************************//**
 * Sets the replace_field that will be redefined by the next field
 * created with the field module. Cleared after next field create call.
 * Field name and coordinate system defaults are taken from supplied field.
 *
 * @param field_module  The field module to create fields in.
 * @param replace_field  Existing field to be overwritten on next create. Can
 * be NULL to clear.
 * @return  1 on success, 0 on failure.
 */
int cmzn_fieldmodule_set_replace_field(
	struct cmzn_fieldmodule *field_module,
	struct Computed_field *replace_field);

/***************************************************************************//**
 * Gets the replace_field, if any, that will be redefined by the next field
 * created with the field module. Cleared after next field create call.
 *
 * @param field_module  The field module to create fields in.
 * @return  Existing field to be replaced which caller must deaccess, or NULL
 * if none.
 */
struct Computed_field *cmzn_fieldmodule_get_replace_field(
	struct cmzn_fieldmodule *field_module);

/**
 * Internal use only.
 * @return  ACCESSed xi field with name "xi", existing or new.
 */
cmzn_field_id cmzn_fieldmodule_get_or_create_xi_field(
	cmzn_fieldmodule_id field_module);

#endif /* !defined (FIELD_MODULE_H) */
