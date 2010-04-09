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

#ifndef __CMISS_MATERIAL_H__
#define __CMISS_MATERIAL_H__

#include "api/cmiss_texture.h"

struct Graphical_material;
#ifndef CMISS_MATERIAL_ID_DEFINED
/***************************************************************************//**
 * A handle to cmiss material. cmiss material describes the
 * colour, shading and other graphical properties of a material, it is highly 
 * similar to material described by OpenGL.
 * User can get a handle to material either through create new material using 
 * Cmiss_graphics_module_create_material or use existing materials in the 
 * graphics_module provided by the ciss_command_data with 
 * Cmiss_graphics_module_find_material_by_name.
 * Cmgui also provide a number of preset materials in the default
 * graphics_packge.
 * Preset graphical materials are:
 * black, blue, bone, gray50, gold, green, muscle, red, silver, tissue,
 * transparent_gray50 and white. 
 *
 * Please see available Cmiss_graphic_material API functions belong for 
 * configuarble properties.
 */
typedef struct Graphical_material * Cmiss_material_id;
#define CMISS_MATERIAL_ID_DEFINED
#endif /* CMISS_MATERIAL_ID_DEFINED */

/***************************************************************************//**
 * Set/change name for <material>.
 *
 * @param material  The handle to cmiss grpahical material.
 * @param name  name to be set to the material
 * @return  1 if successfully set/change name for material, otherwise 0.
 */
int Cmiss_material_set_name(
	Cmiss_material_id material, const char *name);

/***************************************************************************//**
 * Set the alpha value (opacity) of the material.
 *
 * @param material  The handle to the to be modified cmiss graphic material.
 * @param alpha  Opacity of the material. Transparent or translucent objects has
 *   lower alpha values then an opaque ones. Minimum acceptable value is 0
 *   and maximum acceptable value is 1.
 * @return  1 if successfully modify material, otherwise 0.
 */
int Cmiss_material_set_alpha(
	Cmiss_material_id material, float alpha);

/***************************************************************************//**
 * Set the size and brigtness of the highlight.
 *
 * @param material  The handle to the to be modified cmiss material.
 * @param shininess  Size and brightness of the hightlight, the higher the 
 *   value, the smaller and brighter the highlight. Minimum acceptable value is 0
 *   and maximum acceptable value is 1.
 * @return  1 if successfully modify material, otherwise 0.
 */
int Cmiss_material_set_shininess(
	Cmiss_material_id material, float shininess);

/***************************************************************************//**
 * Set the ambient colour of the material. Ambient colour simulates the colour 
 * of the material when it does not receive direct illumination.
 *
 * @param material  The handle to the to be modified cmiss material.
 * @param red  Value of the red component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @param green  Value of the green component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @param blue  Value of the blue component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @return  1 if successfully modify material, otherwise 0.
 */
int Cmiss_material_set_ambient(
	Cmiss_material_id material, float red, float green, float blue);

/***************************************************************************//**
 * Set the diffuse color of the material. Diffuse colour response to light that 
 * comes from one direction and this colour scattered equally in all directions 
 * once the light hits it.
 *
 * @param material  The handle to the to be modified cmiss material.
 * @param red  Value of the red component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @param green  Value of the green component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @param blue  Value of the blue component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @return  1 if successfully modify material, otherwise 0.
 */
int Cmiss_material_set_diffuse(
	Cmiss_material_id material, float red, float green, float blue);

/***************************************************************************//**
 * Set the emissive colour of the material. Emissive colour simulates colours
 * that is originating from the material itself. It is not affected by any
 * lighting.
 *
 * @param material  The handle to the to be modified cmiss material.
 * @param red  Value of the red component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @param green  Value of the green component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @param blue  Value of the blue component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @return  1 if successfully modify material, otherwise 0.
 */
int Cmiss_material_set_emission(
	Cmiss_material_id material, float red, float green, float blue);

/***************************************************************************//**
 * Set the specular colour of the material. Specular colour produces highlights.
 * Unlike ambient and diffuse reflect, specular colour depends on location of
 * the viewpoint, it is brightest along the direct angle of reflection.
 *
 * @param material  The handle to the to be modified cmiss material.
 * @param red  Value of the red component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @param green  Value of the green component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @param blue  Value of the blue component to be set.
 *   Minimum acceptable value is 0 and maximum acceptable value is 1.
 * @return  1 if successfully modify material, otherwise 0.
 */
int Cmiss_material_set_specular(
	Cmiss_material_id material, float red, float green, float blue);

/***************************************************************************//**
 * Set the texture of the material.
 *
 * @param material  The handle to the to be modified cmiss material.
 * @param texture  Handle to cmiss texture to be used for this material.
 * @return  1 if successfully modify material, otherwise 0.
 */
int Cmiss_material_set_texture(
	Cmiss_material_id material, Cmiss_texture_id texture);

/***************************************************************************//**
* Sets the persistent property flag of the material;.
* Default setting persistent=0 means the material is destroyed when the number of
* external references to it, including use as a source for other objects, drops
* to zero.
* Setting persistent=1 means the material exists in graphics module even if no
* external references to it are held, whence it can be found by name or other
* search criteria.
*
* @param material  The material to set the persistent flag for.
* @param persistent  Non-zero for persistent, 0 for non-persistent.
* @return  1 on success, 0 on failure.
*/ 
int Cmiss_material_set_persistent(Cmiss_material_id material, int persistent_flag);

/***************************************************************************//**
* Returns the persistent property flag of the material.
*
* @see Cmiss_material_set_persistent
* @param material  The material to query.
* @return  1 if material is persistent, 0 if non-persistent.
*/ 
int Cmiss_material_get_persistent(Cmiss_material_id material);

/***************************************************************************//**
 * Access the material, increase the access count of the material by one.
 *
 * @param material  handle to the "to be access" cmiss material.
 * @return  handle to material if successfully access material.
 */
Cmiss_material_id Cmiss_material_access(Cmiss_material_id material);

/***************************************************************************//**
 * Destroy the material.
 *
 * @param material  address to the handle to the "to be destroyed" 
 *   cmiss material.
 * @return  1 if successfully destroy material, otherwise 0.
 */
int Cmiss_material_destroy(Cmiss_material_id *material);

/***************************************************************************//**
 * Return an allocated string containing material name.
 *
 * @param material  handle to the cmiss material.
 * @return  allocated string containing material name, otherwise NULL.
 */
char *Cmiss_material_get_name(Cmiss_material_id material);
#endif
