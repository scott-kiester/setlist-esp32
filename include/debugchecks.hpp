#ifndef __DEBUG_CHECKS_HPP___
#define __DEBUG_CHECKS_HPP___

#include <stdlib.h>

#define DEBUG_CHECKS_ENABLED

#ifdef DEBUG_CHECKS_ENABLED 

#define DC_VAR_DECLARE(type, var) type var;
#define DC_VAR_SET(var, value) var = value;
#define DC_ASSERT(expression) assert(expression)

#else

#define DC_VAR_DECLARE(type, var)
#define DC_VAR_SET(var, value)
#define DC_ASSERT(expression)

#endif

#endif
