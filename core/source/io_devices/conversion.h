/*******************************************************************************
FILE : conversion.h

LAST MODIFIED : 19 June 1996

DESCRIPTION :
Converts between different coordinate systems.

NOTE :
Conversion functions use the same prototype - two arrays of data values of the
same precision.  Two typedefs are used -
	type_std: calling
	type_ext: extended for calculations.
An array of procedures is created to convert between coordinates.  A typical
calling example would be to go from RECT to CYL_POLARC -, just call
(*(conversion_position[RECTANGULAR_CARTESIAN][CYLINDRICAL_POLAR]))
	(old_values,new_values);
Incremental functions have an extra parameter - initial values in the
destination coord system.  Therefore, to add a RECT increment to a CYL_POLAR
value and get the answer in CYL_POLAR, just call
(*(conversion_increment_position[RECTANGULAR_CARTESIAN][CYLINDRICAL_POLAR]))
	(increment,old_values,new_values);
???NOTE :
	At the moment, the only type of increment you can add is a RECTANGULAR one,
as this is probably the only type of position data value we will receive from
external devices.  At some stage all other types of increments should be
implemented.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CONVERSION_H)
#define CONVERSION_H

/*
Constants
---------
*/
/*???DB.  What about graphics/geometry.h ? */
#if !defined (PI)
#define PI 3.1415926535898
#endif
#define PI_180 (PI/180.0)
/*???GH. how many coordinate systems do we have? */
#define DOF3_PRECISION double
#define DOF3_PRECISION_STRING "lf"
#define DOF3_NUM_FORMAT "%14.6" DOF3_PRECISION_STRING
#define CONVERSION_NUM_POSITION 6
#define CONVERSION_NUM_DIRECTION 1
#define CONVERSION_NUM_VECTOR 2
#define CONVERSION_INCREMENT_NUM_POSITION 1
#define CONVERSION_INCREMENT_NUM_VECTOR 1
/* we will only need to add a RECTANGULAR_CARTESIAN increment to most
	coord systems (so far)... */


/*
Global Types
------------
*/
struct Dof3_data
/*******************************************************************************
LAST MODIFIED : 28 December 1995

DESCRIPTION :
Contains the three data elements.
==============================================================================*/
{
	DOF3_PRECISION data[3];
}; /* Dof3_data */

enum Conv_coordinate_system
/*******************************************************************************
LAST MODIFIED : 8 September 1994

DESCRIPTION :
The possible coordinate systems.
==============================================================================*/
{
	CONV_RECTANGULAR_CARTESIAN=0,
	CONV_CYLINDRICAL_POLAR=1,
	CONV_SPHERICAL_POLAR=2,
	CONV_PROLATE_SPHEROIDAL=3,
	CONV_OBLATE_SPHEROIDAL=4,
	CONV_FIBRE=5
}; /* enum Conv_coordinate_system */

enum Conv_dir_coordinate_system
/*******************************************************************************
LAST MODIFIED : 8 September 1994

DESCRIPTION :
The possible direction coordinate systems.
==============================================================================*/
{
	CONV_DIR_EULER=0
}; /* enum Conv_dir_coordinate_system */

enum Conv_vec_coordinate_system
/*******************************************************************************
LAST MODIFIED : 8 September 1994

DESCRIPTION :
The possible vector coordinate systems.
==============================================================================*/
{
	CONV_VEC_COMPONENT=0,
	CONV_VEC_SPHERICAL_POLAR=1
}; /* enum Conv_vec_coordinate_system */

typedef int (conversion_proc)(struct Dof3_data *,struct Dof3_data *);
typedef int (conversion_increment_proc)(struct Dof3_data *,struct Dof3_data *,
	struct Dof3_data *);

/*
Global variables
----------------
*/
extern conversion_proc *conversion_position
	[CONVERSION_NUM_POSITION][CONVERSION_NUM_POSITION];
extern conversion_proc *conversion_direction
	[CONVERSION_NUM_DIRECTION][CONVERSION_NUM_DIRECTION];
extern conversion_proc *conversion_vector
	[CONVERSION_NUM_VECTOR][CONVERSION_NUM_VECTOR];
extern conversion_increment_proc *conversion_increment_position
	[CONVERSION_INCREMENT_NUM_POSITION][CONVERSION_NUM_POSITION];
extern conversion_increment_proc *conversion_increment_vector
	[CONVERSION_INCREMENT_NUM_VECTOR][CONVERSION_NUM_VECTOR];


/*
Global Functions
----------------
*/
int conversion_init(void);
/*****************************************************************************
LAST MODIFIED : 16 May 1994

DESCRIPTION :
Sets up the array of conversion routines.
============================================================================*/

#endif
