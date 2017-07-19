/*
 * SimulationModes.hpp
 *
 *  Created on: May 31, 2017
 *      Author: manu_
 */

#ifndef SIMULATIONMODES_HPP_
#define SIMULATIONMODES_HPP_



/* Simulation modes*/
/* If running in Windows OS, use debug mode*/

#define HOMOGENEOUS_DEPTH
//#define RADIATED_CHEMICAL
//#define PROPORTIONAL_LIGHT
//#define ADJUST_SALINITY_GRAZERS
//#define IBM_MODEL_TEMPERATURE
//#define STABLE_CHLOROPHYLL
//#define USE_PHOTOPERIOD
//#define LINEAR_LIGHT
#define AQUATOX_LIGHT_ALLOWANCE

#define LIMITATION_MINIMUM

#define USE_LITERATURE_AND_DATA_CONSTANTS

//#define ADDITIVE_TURBIDITY

#define ADD_CONSTANT_BIOMASS_DIFFERENTIAL
//#define ADD_VARIABLE_BIOMASS_DIFFERENTIAL

#define GRAZING_EFFECT_ON_ALGAE_BIOMASS

//#define NUTRIENT_LIMITATION_GLM

#define NUTRIENT_LIMITATION_QUOTIENT

#define TIME_VARIABLE_PHOSPHORUS_CONCENTRATION_AT_BOTTOM

//#define MIGRATE_ZOOPLANKTON_AT_HOUR

//#define LOCALE_SEARCH_ZOOPLANKTON_MIGRATION

//#define ADD_GRAZER_PREDATORY_PRESSURE

#define CHECK_ASSERTIONS

#define GRAZER_CARRYING_CAPACITY_MORTALITY

#define ALGAL_CARRYING_CAPACITY_MORTALITY

#define INDIVIDUAL_BASED_ANIMALS
//#define CHECK_GRAZER_LOWER_LIMIT

//#define SATURATION_GRAZING
//#define ZOOPLANKTON_ACCUMULATION
/* End simulation modes*/


#endif /* SIMULATIONMODES_HPP_ */
