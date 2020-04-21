/**
 * @file misc.h
 * @ingroup headers
 * @author Simone Bisogno (bissim.github.io)
 * @brief Miscellaneous constants.
 * @version 0.1.0-rc.4+20200421
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef MISC_H
#define MISC_H

#ifdef __cplusplus
extern "C" {
#endif

// Random value generation
static const short SEED = 1; /**< Seed for rand() function */
static const double LOWER_BOUND = 0.0; /**< Lower bound for generated values */
static const double UPPER_BOUND = 99.9; /**< Upper bound for generated values */

// Time management
static const long NS_IN_S = 1E9; /**< How many nanoseconds in a second */
static const int MS_IN_S = 1E3; /**< How many milliseconds in a second */

#ifdef __cplusplus
}
#endif

#endif // MISC_H
