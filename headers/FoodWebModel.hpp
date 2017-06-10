/*
 * FoodWebModel.hpp
 *
 *  Created on: 13 mar. 2017
 *      Author: manu_
 */


#ifndef FOODWEBMODEL_HPP_
#define FOODWEBMODEL_HPP_

/* Define mode constants*/


#include <math.h>
#include <string>
#include <sstream>
#include "TypeDefinitions.hpp"
#include "ModelConstants.hpp"
#include "ReadProcessedData.hpp"



namespace FoodWebModel {



	class FoodWebModel {


		/*Class attributes*/
	protected:
		ReadProcessedData readProcessedData;
		unsigned int current_hour, ZMaxIndex, simulation_cycles;
		physicalType temperature[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], initial_temperature[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], distance_to_focus[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX];
		physicalType depthVector[MAX_COLUMN_INDEX], temperature_range[MAX_DEPTH_INDEX], indexToDepth[MAX_DEPTH_INDEX], hourlyLightAtSurface[HOURS_PER_DAY], *phosphorus_concentration_at_bottom_in_hour;
		unsigned int maxDepthIndex[MAX_COLUMN_INDEX];

		/*Register lake intensity for zooplankton migration*/
		physicalType lakeLightAtDepth[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], previousLakeLightAtDepth[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX];
		/*Phytoplankton biomass, periphyton biomass and temperature*/
		biomassType phytoBiomass[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], periBiomass[MAX_COLUMN_INDEX], phytoDifferential[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], periDifferential[MAX_COLUMN_INDEX], localBiomass[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], verticalMigratedPhytoBiomass[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], verticalMigratedPeriBiomass[MAX_COLUMN_INDEX], sloughPhytoBiomass[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], phytoBiomassDifferential[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], periBiomassDifferential[MAX_COLUMN_INDEX], baseAlgaeBiomassDifferential[MAX_DEPTH_INDEX];

		/*Algae differential growth*/
		biomassType algae_biomass_differential_production_scale;
		/* Grazer biomass*/
		biomassType zooplanktonBiomass[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], bottomFeederBiomass[MAX_COLUMN_INDEX];
		biomassType zooplanktonBiomassCenterDifferencePerDepth[HOURS_PER_DAY];
		biomassType verticalMigrationZooplanktonBiomassBuffer[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX];
		biomassType used_grazing;
		/* Grazer count*/
		zooplanktonCountType zooplanktonCount[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], bottomFeederCount[MAX_COLUMN_INDEX], priorZooplanktonCount[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX], priorBottomFeederCount[MAX_COLUMN_INDEX];
		biomassType grazerPreferenceScore[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX];

		/*Grazer count summing. The simulation halts below a given number*/
		zooplanktonCountType zooplankton_count_summing;
		/*A vector to reference the calculated biomass*/

		physicalType fractionInEuphoticZone, ZEuphotic, ZMean, ZMax, P;

		/*Parameters of daphnia migration*/
		int maximum_distance_daphnia_swum_in_rows_per_hour, vertical_migration_buffer_size;

		/*Max possible column index (X axis)*/
		//int  maxColumn;

		/*Buffer line to write simulation results*/
		std::ostringstream lineBuffer, algaeBuffer, sloughBuffer, grazerBuffer;
		string commaString = string(", ");
		/*Class methods*/
	private:

		/* Physical attributes*/
		physicalType locale_photo_period,light_at_depth, depthInMeters, turbidity_at_depth, light_at_top, resource_limitation_exponent, light_difference, normalized_light_difference, chemical_at_depth_exponent, light_normalizer, light_allowance, light_at_depth_exponent, temperature_angular_frequency, temperature_sine, nutrient_limitation, chemical_concentration, current_phosphorus_concentration_at_bottom;

		/* Algae attributes*/
		biomassType algae_biomass_to_depth, high_temperature_mortality, resource_limitation_stress, weighted_resource_limitation_stress, sedimentation_rate, algae_biomass_differential_scale;

		/* Algae biomass components*/

		biomassType photosynthesis_value, algae_respiration_value, algae_excretion_value, algae_sinking_value, algae_slough_value, algae_natural_mortality;
		/* Zooplankton attributes*/
		biomassType grazing_per_individual, locale_grazing, locale_defecation, base_zooplankton_respiration, salinity_corrected_zooplankton_respiration, basal_respiration, active_respiration_exponent, active_respiration_factor, active_respiration, metabolic_respiration, grazer_excretion_loss, animal_base_mortality, animal_temperature_mortality, animal_temp_independent_mortality, salinity_effect, salinity_mortality, locale_grazing_salt_adjusted, animal_mortality, grazer_predatory_pressure;
		physicalType salinity_exponent, stroganov_adjustment;

		/* Zooplankton parameter weights*/
		biomassType animal_base_mortality_proportion;

	public:
		FoodWebModel(const SimulationArguments& simArguments);
		int simulate(const SimulationArguments& simArguments);
		void initializeParameters();

	protected:

		/* Main functions*/
		void step();
		void initializePointers();
		void printSimulationMode();
		void writeSimulatedParameters(const string& parameterSimulationRoute);

		/* Physical descriptors*/
		void lightAtDepth(int depthIndex, int columnIndex);
		void calculateTemperature(int depthIndex, int columnIndex);
		void lightAllowance(int depthIndex, int columnIndex);
		void photoPeriod();
		void calculateLightAtTop();
		void phosphorusConcentrationAtDepth(int depthIndex, int columnIndex);
		void saltConcentrationAtDepth(int depthIndex, int columnIndex);
		void chemicalConcentrationAtDepth(int depthIndex, int columnIndex, physicalType concentrationAtBottom);
		void calculateNutrientLimitation();
		void calculatePhysicalLakeDescriptors();
		void setBathymetricParameters();
		void calculateDistanceToFocus();

		/* Algae biomass*/
		void updateAlgaeBiomass();
		biomassType algaeBiomassDifferential(int depthIndex, int columnIndex, bool periPhyton);
		biomassType sumPhytoBiomassToDepth(int depthIndex, int columnIndex);
		void photoSynthesis(biomassType localPointBiomass, physicalType localeLimitationProduct, bool periPhyton);
		physicalType productionLimit(physicalType localeLimitationProduct, bool periPhyton);
		void algaeRespiration(biomassType localPointBiomass, physicalType localTemperature);
		void algaeExcretion();
		void algaeNaturalMortality(physicalType localTemperature, physicalType localeLimitationProduct, biomassType localPointBiomass);
		void algaeHighTemperatureMortality(physicalType localeTemperature);
		void resourceLimitationStress(physicalType localeLimitationProduct);
		void algaeSinking(int depthIndex, int columnIndex);
		void algaeSlough(int columnIndex);

		/* Grazers biomass*/
		void updateZooplanktonBiomass();
		void verticalMigrateZooplanktonCount();
		void verticalMigrateZooplanktonAlgae();
		void calculateLocalPreferenceScore();
		biomassType grazerBiomassDifferential(int depthIndex, int columnIndex, bool bottomFeeder);

		void foodConsumptionRate(int depthIndex, int columnIndex, bool bottomFeeder);
		void defecation();
		void animalRespiration(biomassType zooBiomass, physicalType localeTemperature);
		biomassType basalRespiration(biomassType zooBiomass);
		biomassType activeRespiration(biomassType zooBiomass, physicalType localeTemperature);
		biomassType metabolicFoodConsumption();
		void animalExcretion(biomassType localeRespiration);
		void animalMortality(biomassType localeRespiration, physicalType localeTemperature, physicalType localeSalinityConcentration);
		void animalBaseMortality(physicalType localeTemperature, biomassType localeBiomass);
		void animalTemperatureMortality(physicalType localeTemperature, biomassType localeBiomass);
		void salinityEffect();
		void salinityMortality(biomassType localeBiomass);
		void stroganovApproximation(physicalType localeTemperature);
		void calculatePredationPressure(zooplanktonCountType zooplanktonLocaleCount);
};
}



#endif /* FOODWEBMODEL_HPP_ */
