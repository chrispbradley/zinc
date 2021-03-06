/*******************************************************************************
FILE : finite_element_to_iges.h

LAST MODIFIED : 4 March 2003

DESCRIPTION :
Functions for building IGES information from finite elements and exporting
to file.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_TO_IGES_H)
#define FINITE_ELEMENT_TO_IGES_H

/*
Global functions
----------------
*/

int export_to_iges(char *file_name, struct FE_region *fe_region,
	char *region_path, struct Computed_field *field);
/******************************************************************************
LAST MODIFIED : 5 August 2003

DESCRIPTION :
Write bicubic elements to an IGES file.
==============================================================================*/

#endif /* !defined (FINITE_ELEMENT_TO_IGES_H) */
