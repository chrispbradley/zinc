/**
 * FILE : sceneviewerinput.h
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_SCENEVIEWERINPUT_H__
#define CMZN_SCENEVIEWERINPUT_H__

#include "sceneviewer.h"
#include "types/sceneviewerinputid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a scene viewer input object for manually setting mouse or other input
 * event data.
 *
 * @param sceneviewer  Handle to cmzn_sceneviewer object.
 * @return  Handle to cmzn_sceneviewerinput on success, or 0 on failure.
 */
ZINC_API cmzn_sceneviewerinput_id cmzn_sceneviewer_create_sceneviewerinput(cmzn_sceneviewer_id sceneviewer);

/**
 * Returns a new reference to the cmzn_sceneviewerinput with reference count incremented.
 * Caller is responsible for destroying the new reference.
 *
 * @param input  Handle to cmzn_sceneviewerinput object.
 * @return  Accessed handle to cmzn_sceneviewerinput.
 */
ZINC_API cmzn_sceneviewerinput_id cmzn_sceneviewerinput_access(cmzn_sceneviewerinput_id input);

/**
 * Destroys this reference to the cmzn_sceneviewerinput (and sets it to 0).
 * Internally this just decrements the reference count.
 *
 * @param address_input  Address of handle to cmzn_sceneviewerinput object.
 * @return  CMZN_OK if successfully deaccess cmzn_sceneviewerinput, any other value on
 * failure.
 */
ZINC_API int cmzn_sceneviewerinput_destroy(cmzn_sceneviewerinput_id *address_input);

/**
 * Set the position of the input
 *
 * @param input  Handle to cmzn_sceneviewerinput object.
 * @param x The x-coordinate of the input position.
 * @param y The y-coordinate of the input position.
 * @return  CMZN_OK if successful, any other value on failure.
 */
ZINC_API int cmzn_sceneviewerinput_set_position(cmzn_sceneviewerinput_id input, int x, int y);

/**
 * Set modifiers for the input, e.g. shift, ctrl, alt.
 *
 * @param input  Handle to cmzn_sceneviewerinput object.
 * @param modifiers  The input modifiers to set, logical OR of
 * enum cmzn_sceneviewerinput_modifier_flags.
 * @return  CMZN_OK if successful, any other value on failure.
 */
ZINC_API int cmzn_sceneviewerinput_set_modifiers(cmzn_sceneviewerinput_id input,
	cmzn_sceneviewerinput_modifiers_type modifiers);

/**
 * Set the button for the scene viewer input.
 *
 * @param input zinc scene viewer input.
 * @param button_type zinc scene viewer input button enumeration value.
 * @return CMZN_OK on success, any other value on failure.
 */
ZINC_API int cmzn_sceneviewerinput_set_button_type(cmzn_sceneviewerinput_id input,
	enum cmzn_sceneviewerinput_button_type button_type);

/**
 * Set the scene viewer input event type: button press, motion, etc.
 *
 * @param input  Handle to cmzn_sceneviewerinput object.
 * @param type  Enumerator for the input event type.
 * @return  CMZN_OK if successful, any other value on failure.
 */
ZINC_API int cmzn_sceneviewerinput_set_event_type(cmzn_sceneviewerinput_id input,
	enum cmzn_sceneviewerinput_event_type event_type);

#ifdef __cplusplus
}
#endif

#endif
