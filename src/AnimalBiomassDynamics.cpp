/*
 * AnimalBiomassDynamics.cpp
 *
 *  Created on: Jul 6, 2017
 *      Author: manu_
 */
#include <algorithm> // for remove_if
#include <functional> // for unary_function
#include <iterator>
#include "../headers/AnimalBiomassDynamics.hpp"

using namespace std;

std::ostream& operator<<(std::ostream& os, const AnimalCohort& cohort){
	os<<"(Animal cohort. ID: "<<cohort.cohortID<<", x: "<<cohort.x<<", y: "<<cohort.y<<", number of indviduals: "<<cohort.numberOfIndividuals<<", body biomass: "<<cohort.bodyBiomass<<", gonad biomass: "<<cohort.gonadBiomass;
	os<<", stage: "<<cohort.stage;
	os<<", is bottom animal: "<<(cohort.isBottomAnimal?1:0)<<")";
	return os;

}


std::ostream& operator<<(std::ostream& os, const EggCohort& cohort){
	os<<"(Egg cohort. ID: "<<cohort.cohortID<<", x: "<<cohort.x<<", y: "<<cohort.y<<", age: "<<cohort.ageInHours<<", number of eggs: "<<cohort.numberOfEggs<<", biomass: "<<cohort.biomass;
	os<<", is bottom egg: "<<(cohort.isBottomAnimal?1:0)<<")";
	return os;
}
void operator+=(AnimalCohort& cohort1, const AnimalCohort& cohort2){
	cohort1.bodyBiomass+=cohort2.bodyBiomass;
	cohort1.numberOfIndividuals+=cohort2.numberOfIndividuals;
	cohort1.gonadBiomass+=cohort2.gonadBiomass;
	cohort1.ageInHours=0;

}

void operator-=(AnimalCohort& cohort1, const AnimalCohort& cohort2){
	cohort1.bodyBiomass -= cohort2.bodyBiomass;
	cohort1.numberOfIndividuals -= cohort2.numberOfIndividuals;
	cohort1.gonadBiomass -= cohort2.gonadBiomass;
	cohort1.ageInHours=0;

}


void operator+=(EggCohort& cohort1, const EggCohort& cohort2){
	cohort1.biomass += cohort2.biomass;
	cohort1.numberOfEggs += cohort2.numberOfEggs;
}

void operator+=(AnimalCohort& cohort1, const EggCohort& cohort2){
	cohort1.bodyBiomass += cohort2.biomass;
	cohort1.numberOfIndividuals += cohort2.numberOfEggs;
	cohort1.ageInHours=0;
	 if(cohort1.numberOfIndividuals<0){
		 cout<<"Error."<<endl;
	 }

}

void operator*=(AnimalCohort& cohort1, const double number){
	cohort1.bodyBiomass*=number;
	cohort1.gonadBiomass*=number;
	cohort1.numberOfIndividuals*=number;
	 if(cohort1.numberOfIndividuals<0){
		 cout<<"Error."<<endl;
	 }

}


namespace FoodWebModel {

AnimalBiomassDynamics::AnimalBiomassDynamics():randomGenerator(NULL) {
	// TODO Auto-generated constructor stub

}

AnimalBiomassDynamics::~AnimalBiomassDynamics() {
	delete randomGenerator;
}

void AnimalBiomassDynamics::reportAssertionError(int depthIndex, int columnIndex, biomassType biomass, biomassType previousBiomass, biomassType differential, bool isBottom) {
	unsigned int isBottomAsInt=(isBottom?1:0);
	if (isnan((float)biomass)||isinf((float)biomass)) {
		(*assertionViolationBuffer) << "NanInfAnimal; Biomass: " << "Biomass: "
				<< biomass << ", Depth: "<<depthIndex<<", Column: "
				<< columnIndex << ", Hour: " << (*current_hour) << ", IsBottom: "<<isBottomAsInt << endl;
	}
	if (biomass < 0.0f) {
		(*assertionViolationBuffer) << "NegativeAnimal; Biomass: "
				<< biomass << ", Depth: "<<depthIndex<<", Column: "
				<< columnIndex << ", Hour: " << (*current_hour) << ", IsBottom: "<<isBottomAsInt << endl;
	}
	/*if (biomass
			!= previousBiomass + differential) {
		(*assertionViolationBuffer) << "UpdateAnimal; Biomass: "
				<< biomass << ", PreviousBiomass: "
				<< previousBiomass << ", BiomassDiff: "
				<< differential << ", BiomassError: "
				<< biomass
						- (previousBiomass
								+ differential)
				<< ", Depth: "<<depthIndex<<", Column: " << columnIndex << ", Hour: " << (*current_hour)
				<< ", IsBottom: "<<isBottomAsInt << endl;
	}*/
}

void AnimalBiomassDynamics::updateCohortBiomassForAnimals(std::map<pair<int,int>,AnimalCohort> *animals) {//

	for (std::map<pair<int,int>,AnimalCohort>::iterator it = animals->begin();
			it != animals->end(); ++it) {
		updateCohortBiomass(it->second);
		/* Remove cohort if the number of animals or its biomass is 0*/
		if(it->second.numberOfIndividuals<=0||it->second.bodyBiomass<=0.0f){
			animals->erase(it);
		}

	}

}

/* Calculation of grazer biomass (AquaTox Documentation, page 100, equation 90)*/

void AnimalBiomassDynamics::updateAnimalBiomass(){
/* Clear string buffers */
	animalBiomassBuffer.str("");
#ifdef INDIVIDUAL_BASED_ANIMALS
	animalTraceBuffer.str("");
#endif
	/* Clear zooplankton count summing. This will be compared with threshold upper number as a halting condition*/
	this->floating_animal_count_summing=0;
	/*Matrix to store the decision of biomass must be saved. It will be read when registering slough to output slough file*/
	bool registerZooplanktonBiomass[MAX_DEPTH_INDEX][MAX_COLUMN_INDEX];
	/* Clear vertical migrated phyto biomass*/
#ifndef INDIVIDUAL_BASED_ANIMALS
	for(int columnIndex=0; columnIndex<MAX_COLUMN_INDEX; columnIndex++){
		bottomAnimalBiomass[columnIndex]=0.0f;
		for(int depthIndex=0; depthIndex<MAX_DEPTH_INDEX; depthIndex++){
			floatingAnimalBiomass[depthIndex][columnIndex]=0.0f;
		}
	}
#endif
	/*Migrate verically zooplankton according to current time */

#ifdef INDIVIDUAL_BASED_ANIMALS
	/* Update biomass in bottom and floating grazer cohorts*/



	updateCohortBiomassForAnimals(bottomAnimals);
	updateCohortBiomassForAnimals(floatingAnimals);

	/* Increase egg age*/

#ifdef CREATE_NEW_COHORTS
#ifdef MATURE_JUVENILES
	matureEggs(bottomEggs, bottomJuveniles);
	matureEggs(floatingEggs, floatingJuveniles);
	matureJuveniles(bottomJuveniles, bottomAnimals);
	matureJuveniles(floatingJuveniles, floatingAnimals);
#else
	matureEggs(bottomEggs, bottomAnimals);
	matureEggs(floatingEggs, floatingAnimals);
#endif
#endif
#ifdef PRINT_ANIMAL_SUMMATORY
	cout<<"Summatory of individuals. Original: "<<this->floating_animal_count_summing<<", Normalized: "<<this->floating_animal_count_summing/(41*254)<<"."<<endl;
	cout<<"Floating adults: "<<this->floatingAnimals->size()<<", floating juveniles: "<<this->floatingJuveniles.size()<<", floating eggs: "<<floatingEggs.size()<<"."<<endl;
	cout<<"Combined animals size: "<<this->floatingAnimals->size()+this->floatingJuveniles.size()+floatingEggs.size()<<"."<<endl;
#endif
	/* Remove all dead animals*/
//	removeDeadAnimals();

#else
	/*Calculate phytoplankton and periphyton biomass on the current step*/
	for(int columnIndex=0; columnIndex<MAX_COLUMN_INDEX; columnIndex++){
		lineBuffer.str("");
		lineBuffer.clear();
		bool registerBottomAnimalBiomass=*current_time%TIME_OUTPUT_RESOLUTION==0;
		/*Transform grazer count to biomass*/
		bottomAnimalBiomass[columnIndex]=((biomassType)bottomAnimalCount[columnIndex])*DAPHNIA_WEIGHT_IN_MICROGRAMS;
		biomassType previousBottomAnimalBiomass = bottomAnimalBiomass[columnIndex];
		/*Update biomass and output new biomass*/
		biomassType bottomAnimalBiomassDifferential = animalBiomassDifferential(maxDepthIndex[columnIndex], columnIndex, true, bottomAnimalCount[columnIndex], previousBottomAnimalBiomass);
		if(bottomAnimalBiomassDifferential>0.0f){
			biomassType mortalityBiomass=previousBottomAnimalBiomass;
			calculateGrazerCarryingCapacityMortality(mortalityBiomass);
			bottomAnimalBiomassDifferential-=animal_carrying_capacity*bottomAnimalBiomassDifferential;
		}

		bottomAnimalBiomass[columnIndex]+=bottomAnimalBiomassDifferential;
#ifdef CHECK_ASSERTIONS
		reportAssertionError(maxDepthIndex[columnIndex], columnIndex, bottomAnimalBiomass[columnIndex], previousBottomAnimalBiomass,
				bottomAnimalBiomassDifferential, true);
#endif
		/* From biomass to discrete count*/
		bottomAnimalCount[columnIndex]=ceil(bottomAnimalBiomass[columnIndex]/DAPHNIA_WEIGHT_IN_MICROGRAMS);
		bottomAnimalCount[columnIndex]=max<animalCountType>((animalCountType)0.0f, bottomAnimalCount[columnIndex]);
		/*If biomass must be registered, register standard and slough periphyton biomass*/
		if(registerBottomAnimalBiomass&&bottomAnimalBiomass[columnIndex]>0){
			animalBiomassBuffer<<lineBuffer.str()<<commaString<<bottomAnimalCount[columnIndex]<<commaString<<bottomAnimalBiomass[columnIndex]<<endl;
		}
		for(int depthIndex=0; depthIndex<MAX_DEPTH_INDEX; depthIndex++){
			lineBuffer.str("");
			lineBuffer.clear();
			/* Do not register biomass for empty cells. Since this is the bottom of the lake, do not register biomass below it*/
			if(depthIndex>maxDepthIndex[columnIndex]){
				floatingAnimalBiomass[depthIndex][columnIndex]=0.0f;
				registerZooplanktonBiomass[depthIndex][columnIndex]=false;
				break;
			} else{
				/*Set if biomass must be registered*/
				registerZooplanktonBiomass[depthIndex][columnIndex]=depthIndex%DEPTH_OUTPUT_RESOLUTION==0&&columnIndex%COLUMN_OUTPUT_RESOLUTION==0;
				/*Transform zooplankton count to biomass*/
				floatingAnimalBiomass[depthIndex][columnIndex]=((biomassType)floatingAnimalCount[depthIndex][columnIndex])*DAPHNIA_WEIGHT_IN_MICROGRAMS;
				biomassType previousFloatingAnimalBiomass = floatingAnimalBiomass[depthIndex][columnIndex];
				/*Update biomass and output new biomass*/
				biomassType floatingAnimalBiomassDifferential = animalBiomassDifferential(depthIndex, columnIndex, false, floatingAnimalCount[depthIndex][columnIndex], previousFloatingAnimalBiomass);
				if(floatingAnimalBiomassDifferential>0.0f){
					biomassType mortalityBiomass=animalBiomass;
					calculateGrazerCarryingCapacityMortality(mortalityBiomass);
					floatingAnimalBiomassDifferential-=animal_carrying_capacity*floatingAnimalBiomassDifferential;
				}

				floatingAnimalBiomass[depthIndex][columnIndex]+=floatingAnimalBiomassDifferential;
#ifdef CHECK_ASSERTIONS
				reportAssertionError(depthIndex, columnIndex, floatingAnimalBiomass[depthIndex][columnIndex], previousFloatingAnimalBiomass,
						floatingAnimalBiomassDifferential, false);
#endif
				/* From biomass to discrete count*/
				floatingAnimalCount[depthIndex][columnIndex]=ceil(floatingAnimalBiomass[depthIndex][columnIndex]/DAPHNIA_WEIGHT_IN_MICROGRAMS);
				floatingAnimalCount[depthIndex][columnIndex]=max<animalCountType>((animalCountType)0.0f, floatingAnimalCount[depthIndex][columnIndex]);
				this->floating_animal_count_summing+=floatingAnimalCount[depthIndex][columnIndex];
				/*If biomass must be registered, register standard phytoplankton biomass*/
				if(registerZooplanktonBiomass[depthIndex][columnIndex]&&floatingAnimalBiomass[depthIndex][columnIndex]>0){
					animalBiomassBuffer<<lineBuffer.str()<<commaString<<floatingAnimalCount[depthIndex][columnIndex]<<commaString<<floatingAnimalBiomass[depthIndex][columnIndex]<<endl;
				}

			}
		}
	}
#endif
}

#ifdef INDIVIDUAL_BASED_ANIMALS

/* Function for updating biomass in grazer cohorts*/
void AnimalBiomassDynamics::updateCohortBiomass(AnimalCohort& cohort){
	int depthIndex=cohort.x, columnIndex=cohort.y;

	/*Assign cohort ID if it does not exist*/
	if(cohort.cohortID==-1){
		cohort.cohortID=(*this->cohortID)++;
	}

	/* Check if biomass must be registered*/
	bool registerBiomass=*current_hour%TIME_OUTPUT_RESOLUTION==0;
	lineBuffer.str("");
	lineBuffer.clear();
	biomassType initialFoodBiomass= getFoodBiomass(cohort.isBottomAnimal, cohort.y, cohort.x);
	biomassType initialAnimalBiomass = cohort.bodyBiomass;
	animalCountType animalCount=cohort.numberOfIndividuals;
#if defined (INDIVIDUAL_BASED_ANIMALS)&& defined (CREATE_NEW_COHORTS)
	if(*current_hour>=20*24){
//		cout<<"Potential negative biomass."<<endl;
	}
	biomassType biomassDifferential = animalBiomassDifferential(depthIndex, columnIndex, cohort.isBottomAnimal, animalCount, initialAnimalBiomass, cohort.stage);
#else
	biomassType biomassDifferential = animalBiomassDifferential(depthIndex, columnIndex, cohort.isBottomAnimal, animalCount, initialAnimalBiomass);
#endif
	if(biomassDifferential>0.0f){
#if defined(INDIVIDUAL_BASED_ANIMALS)&&defined(CREATE_NEW_COHORTS)
		biomassType mortalityBiomass=min<biomassType>(animalCount*this->initial_grazer_weight[cohort.stage], initialAnimalBiomass);
#else
		biomassType mortalityBiomass=previousBottomAnimalBiomass;
#endif
		calculateGrazerCarryingCapacityMortality(mortalityBiomass);
		biomassDifferential-=animal_carrying_capacity*biomassDifferential;
	}
#ifdef CREATE_NEW_COHORTS
	/* If biomass differential is negative, do not invest in eggs*/
	if(cohort.stage==AnimalStage::Egg){
		cout<<"Error."<<endl;
	}

	//if(cohort.stage!=AnimalStage::Juvenile&&biomassDifferential>0.0f){
		if(biomassDifferential>0.0f){

		biomassType biomassDifferential=cohort.isBottomAnimal?this->bottomFoodBiomassDifferential[cohort.y]:this->floatingFoodBiomassDifferential[cohort.x][cohort.y];
		calculateReproductionProportionInvestment(initialFoodBiomass, biomassDifferential);
	} else{
		reproduction_proportion_investment=0.0f;
	}
	if(cohort.x>maxDepthIndex[cohort.y]){
		cout<<"Lower overflow."<<endl;
	}
#else
	reproduction_proportion_investment=0.0f;
#endif
	/* Amount of biomass invested in body and gonad weight*/
//	reproduction_proportion_investment=0.0f;
	biomassType bodyBiomassInvestment=biomassDifferential*(1-reproduction_proportion_investment);
 biomassType bodyLostBiomass=consumed_biomass*(1-reproduction_proportion_investment);
	biomassType gonadBiomassInvestment=biomassDifferential*reproduction_proportion_investment;
	cohort.gonadBiomass+=gonadBiomassInvestment;
#ifdef ANIMAL_STARVATION_HOURS_WITHOUT_FOOD
	animalStarvationMortality(cohort, getFoodBiomass(cohort));
#elif defined(ANIMAL_STARVATION_PROPORTION_LOST_BIOMASS)
	animalStarvationMortality(cohort);
#endif
#ifdef CREATE_NEW_COHORTS
	/* If there exists gonad investment, create new eggs*/
	if((*(this->current_hour)%this->ovipositing_period)==0){
		if(cohort.gonadBiomass>0.0f){
			/* If the biomass is enough to generate at least one egg, create it.*/
			if(cohort.gonadBiomass>=(biomassType)initial_grazer_weight[AnimalStage::Egg]){
	#ifdef REPORT_COHORT_INFO
				cout<<"Hour: "<<(*(this->current_hour))<<". Creating new cohort with ID: "<<(*this->cohortID)<<", biomass: "<<cohort.gonadBiomass<<", x: "<<cohort.x<<", y: "<<cohort.y<<"."<<endl;
	#endif
				biomassType newCohortBiomass = createNewCohort(cohort, cohort.gonadBiomass, cohort.isBottomAnimal?&(this->bottomJuveniles):&(this->floatingJuveniles));
	#ifdef REPORT_COHORT_INFO
				cout<<"Cohort created."<<endl;
	#endif
				/* Store biomass remnant as body investment in the parent cohort*/
				cohort.gonadBiomass-=newCohortBiomass;
			}
		}
	}
#endif
#ifdef ANIMAL_AGING
	animalAging(cohort);
#endif
	cohort.bodyBiomass+=bodyBiomassInvestment;
//	cohort.numberOfIndividuals=cohort.bodyBiomass/(biomassType)initial_grazer_weight[cohort.stage];
	/*Remove dead animals in the cohort*/
	if(animal_mortality>0){
		biomassType biomassAfterEating = getFoodBiomass(cohort);
		animalCountType deadIndividuals;
		if(biomassAfterEating==0){
			/* If there is no food, all individuals die*/
			deadIndividuals=cohort.numberOfIndividuals;
//			deadIndividuals=animal_mortality/this->initial_grazer_weight[cohort.stage];
		} else{
			/* Otherwise, only a number die*/
			biomassType starvationProportion = (cohort.numberOfIndividuals/biomassAfterEating);
			starvationProportion*=this->starvation_factor;
			deadIndividuals=animal_mortality/this->initial_grazer_weight[cohort.stage]*starvationProportion;
		}
//		deadIndividuals=animal_mortality/this->initial_grazer_weight[cohort.stage];
		cohort.numberOfIndividuals=max<animalCountType>(0,cohort.numberOfIndividuals-deadIndividuals);
		if(cohort.numberOfIndividuals<0){
			cout<<"Error."<<endl;
		}
	}

	/*Remove starved animals in the cohort*/
	/*if(consumed_biomass>0){
		biomassType biomassDeficit=consumed_biomass;
		animalCountType starvedIndividuals=this->starvation_factor*biomassDeficit/this->initial_grazer_weight[cohort.stage];
   cout<<"Starved individuals: "<<starvedIndividuals<<", starvation factor: "<<this->starvation_factor<<endl;
		cohort.numberOfIndividuals=max<animalCountType>(0,cohort.numberOfIndividuals-starvedIndividuals);
	}*/
#ifdef CHECK_ASSERTIONS
	reportAssertionError(maxDepthIndex[columnIndex], columnIndex, cohort.bodyBiomass+cohort.gonadBiomass, initialAnimalBiomass,
			biomassDifferential, cohort.isBottomAnimal);
#endif
	/* Update number of individuals based on cohort biomass*/
#ifndef CREATE_NEW_COHORTS
	cohort.numberOfIndividuals=ceil(cohort.bodyBiomass/initial_grazer_weight[cohort.stage]);
	cohort.numberOfIndividuals=max<animalCountType>(0, cohort.numberOfIndividuals);
#endif
  if(bodyLostBiomass>0.0f){

    //cohort.numberOfIndividuals=ceil(cohort.bodyBiomass/initial_grazer_weight[cohort.stage]);
	  // Include removal of starved grazers multiplied by a weight
	cohort.numberOfIndividuals-=ceil(this->dead_animal_proportion*bodyLostBiomass/initial_grazer_weight[cohort.stage]);
	cohort.numberOfIndividuals=max<animalCountType>(0,cohort.numberOfIndividuals);
	if(cohort.numberOfIndividuals<0){
		cout<<"Error."<<endl;
	}
  }
	this->floating_animal_count_summing+=cohort.numberOfIndividuals;
	/* If the number of individuals or total biomass in the cohort is 0, consider it dead of starvation */
//	if((cohort.bodyBiomass<=0||cohort.numberOfIndividuals<=0)&&cohort.death==None){
//		cohort.death=causeOfDeath::Starvation;
//	}
	/*If biomass must be registered, register standard phytoplankton biomass*/
	if(registerBiomass&&cohort.numberOfIndividuals>0){
		animalBiomassBuffer<<lineBuffer.str()<<commaString<<cohort.numberOfIndividuals<<commaString<<cohort.bodyBiomass<<commaString<<cohort.gonadBiomass<<commaString<<locale_algae_biomass_before_eating<<commaString<<locale_algae_biomass_after_eating<<commaString<<used_consumption<<commaString<<animal_carrying_capacity<<commaString<<reproduction_proportion_investment<<commaString<<cohort.stage<<commaString<<cohort.cohortID<<endl;
	}

	/* If the cohort is dead, register its death*/
//	if(cohort.death!=None){
//		unsigned int isBottomAnimal=cohort.isBottomAnimal?1:0;
//		this->animalDeadBuffer<<cohort.x<<commaString<<cohort.y<<commaString<<(*current_hour)<<commaString<<isBottomAnimal<<commaString<<cohort.ageInHours<<commaString<<cohort.hoursWithoutFood<<commaString<<cohort.stage<<commaString<<cohort.numberOfIndividuals<<commaString<<cohort.bodyBiomass<<endl;
//	}
}
#endif
//void AnimalBiomassDynamics::verticalMigrateAnimalsNoPreference(){
//
//
//
//
//
//	/* Migrate biomass with respect to the previous hour*/
//
//	/* First, calculate depth movement with respect to previous hour*/
//	int depth_dependent_hour_shift =(*current_hour)==0?INITIAL_ZOOPLANKTON_SHIFT:floatingAnimalBiomassCenterDifferencePerDepth[(*current_hour)%HOURS_PER_DAY];
//
//	/*If there exists a movement in zooplankton across depth*/
//	if(depth_dependent_hour_shift!=0){
//		/* Temporary store zooplankton count in a buffer*/
//		for(int depthIndex=0; depthIndex<MAX_DEPTH_INDEX; depthIndex++){
//			for(int columnIndex=0; columnIndex<MAX_COLUMN_INDEX; columnIndex++){
//				verticalMigrationAnimalBiomassBuffer[depthIndex][columnIndex] =floatingAnimalCount[depthIndex][columnIndex];
//				floatingAnimalCount[depthIndex][columnIndex] = 0.0f;
//			}
//		}
//		int initial_depth_index=depth_dependent_hour_shift>0?0:-depth_dependent_hour_shift,
//				final_depth_index=depth_dependent_hour_shift>0?MAX_DEPTH_INDEX-depth_dependent_hour_shift:MAX_DEPTH_INDEX;
//
//		for(int depthIndex=initial_depth_index; depthIndex<final_depth_index; depthIndex++){
//
//			/*Adjust depth movement per depth. If it is within depth range, update zooplankton count*/
//			int depth_adjustment=depthIndex+depth_dependent_hour_shift;
//			for(int columnIndex=0; columnIndex<MAX_COLUMN_INDEX; columnIndex++){
//				/* Move zooplankton individuals to the adjusted depth*/
//				if(depth_adjustment<=maxDepthIndex[columnIndex]&&depthIndex<=maxDepthIndex[columnIndex]){
//					floatingAnimalCount[depth_adjustment][columnIndex] =verticalMigrationAnimalBiomassBuffer[depthIndex][columnIndex];
//				}
//			}
//		}
//
//		/* Accumulate unmoved biomass to extreme shallow or deep index*/
//#ifdef ZOOPLANKTON_ACCUMULATION
//		initial_depth_index=depth_dependent_hour_shift>0?MAX_DEPTH_INDEX-depth_dependent_hour_shift:0,
//			final_depth_index=depth_dependent_hour_shift>0?MAX_DEPTH_INDEX:-depth_dependent_hour_shift;
//
//		for(int depthIndex=initial_depth_index; depthIndex<final_depth_index; depthIndex++){
//			for(int columnIndex=0; columnIndex<MAX_COLUMN_INDEX; columnIndex++){
//				unsigned int receivingIndex=depth_dependent_hour_shift>0?maxDepthIndex[columnIndex]:0;
//				floatingAnimalCount[receivingIndex][columnIndex]+=verticalMigrationZooplanktonBiomassBuffer[depthIndex][columnIndex];
//
//			}
//		}
//#endif
//	}
////	for(int depthIndex=0; depthIndex<MAX_DEPTH_INDEX; depthIndex++){
////
////		/*Adjust depth movement per depth. If it is within depth range, update zooplankton count*/
////		int depth_adjustment=depthIndex+depth_dependent_hour_shift;
////
////		for(int columnIndex=0; columnIndex<MAX_COLUMN_INDEX; columnIndex++){
////			if(depth_adjustment<0||depth_adjustment>=MAX_DEPTH_INDEX||depth_adjustment>maxDepthIndex[columnIndex]){
////				/* Move zooplankton individuals to the adjusted depth*/
////				unsigned int receivingIndex=depth_dependent_hour_shift>0?maxDepthIndex[columnIndex]:0;
////#ifdef ZOOPLANKTON_ACCUMULATION
////				bool zooplanktionAccumulationCondition = depthIndex>maxDepthIndex[columnIndex]||;
////#else
////				bool zooplanktionAccumulationCondition = depthIndex<=maxDepthIndex[columnIndex];
////#endif
////				if(zooplanktionAccumulationCondition){
////					zooplanktonCount[receivingIndex][columnIndex]+=verticalMigrationZooplanktonBiomassBuffer[depthIndex][columnIndex];
////				}
////			}
////		}
////	}
//}
//
//
//
//
///* Distribute preference score and daphnia across neighboring cells. Initially, it will be distributed in the same column.*/
//void AnimalBiomassDynamics::verticalMigrateAnimalsPreference(){
//	calculateLocalPreferenceScore();
//	biomassType *neighboringPreferences = new biomassType[this->vertical_migration_buffer_size];
//	for (int depthIndex = 0; depthIndex < MAX_DEPTH_INDEX; depthIndex++) {
//		int candidateLowerRowIndex = depthIndex-this->maximum_distance_daphnia_swum_in_rows_per_hour;
//		int candidateUpperRowIndex = depthIndex+this->maximum_distance_daphnia_swum_in_rows_per_hour;
//		int localeLowerRowIndex=candidateLowerRowIndex<0?0:candidateLowerRowIndex;
//		for (int columnIndex = 0; columnIndex < MAX_COLUMN_INDEX;columnIndex++) {
//			/* Normalize daphnia preference in neighboring cells from the same column. First, sum preference at neighboring cells*/
//			biomassType neighboringPreferencesSum=0.0f;
//			int localeUpperRowIndex=candidateUpperRowIndex>maxDepthIndex[columnIndex]?maxDepthIndex[columnIndex]:candidateUpperRowIndex;
//			int usedMigrationBufferSize=localeUpperRowIndex-localeLowerRowIndex;
//			for (int localeDepthIndex = localeLowerRowIndex; localeDepthIndex < localeUpperRowIndex; localeDepthIndex++) {
//				neighboringPreferencesSum+=this->foodPreferenceScore[localeDepthIndex][columnIndex];
//				neighboringPreferences[localeDepthIndex-localeLowerRowIndex] = foodPreferenceScore[localeDepthIndex][columnIndex];
//			}
//			/*Find shift towards 0-based*/
//			biomassType minimumLocalePreference=neighboringPreferences[0];
//			for (int localeDepthIndex = 0; localeDepthIndex < usedMigrationBufferSize; localeDepthIndex++) {
//				minimumLocalePreference=min(neighboringPreferences[localeDepthIndex],minimumLocalePreference);
//			}
//			for (int localeDepthIndex = 0; localeDepthIndex < usedMigrationBufferSize; localeDepthIndex++) {
//				neighboringPreferences[localeDepthIndex]-=minimumLocalePreference;
//			}
//			/* Then, normalize each cell by this summing*/
//			for (int localeDepthIndex = 0; localeDepthIndex < usedMigrationBufferSize; localeDepthIndex++) {
//				neighboringPreferences[localeDepthIndex]/=neighboringPreferencesSum;
//			}
//			/* Finally, distribute grazers using this distribution*/
//			biomassType bufferedZooplanktonCount=floatingAnimalCount[depthIndex][columnIndex];
//			floatingAnimalCount[depthIndex][columnIndex]=0.0f;
//			for (int localeDepthIndex = localeLowerRowIndex; localeDepthIndex < localeUpperRowIndex; localeDepthIndex++) {
//				this->floatingAnimalCount[localeDepthIndex][columnIndex]+=bufferedZooplanktonCount*neighboringPreferences[localeDepthIndex-localeLowerRowIndex];
//			}
//		}
//	}
//}

/* Calculate neighboring preference score for daphnia migration. Inspired by ([1] B.-P. Han and M. Straï¿½kraba, ï¿½Modeling patterns of zooplankton diel vertical migration,ï¿½ J. Plankton Res., vol. 20, no. 8, pp. 1463ï¿½1487, 1998., Eqn. 15)*/
void AnimalBiomassDynamics::calculateLocalPreferenceScore() {
	for (int depthIndex = 0; depthIndex < MAX_DEPTH_INDEX; depthIndex++) {
		for (int columnIndex = 0; columnIndex < MAX_COLUMN_INDEX;
				columnIndex++) {
			/*Daphnia prefer descents in light (to avoid predators) and increases in algae*/
			this->foodPreferenceScore[depthIndex][columnIndex] =
					((previousLakeLightAtDepth[depthIndex][columnIndex] - lakeLightAtDepth[depthIndex][columnIndex])
							/ lakeLightAtDepth[depthIndex][columnIndex])
							* (floatingFoodBiomassDifferential[depthIndex][columnIndex]/floatingFoodBiomass[depthIndex][columnIndex]);
		}
	}
}

/* Get food biomass for the modeled species */
biomassType AnimalBiomassDynamics::getFoodBiomass(bool bottom, int columnIndex,
		int depthIndex) {
	return bottom ?
			this->bottomFoodBiomass[columnIndex] :
			this->floatingFoodBiomass[depthIndex][columnIndex];
}
#if defined(INDIVIDUAL_BASED_ANIMALS)&&defined(CREATE_NEW_COHORTS)
biomassType AnimalBiomassDynamics::animalBiomassDifferential(int depthIndex, int columnIndex, bool bottom, animalCountType animalCount, biomassType animalBiomass, AnimalStage stage){
#else
biomassType AnimalBiomassDynamics::animalBiomassDifferential(int depthIndex, int columnIndex, bool bottom, animalCountType animalCount, biomassType animalBiomass){
#endif
	physicalType localeTemperature = temperature[depthIndex][columnIndex];

		/* Get zooplankton count and biomass*/
	locale_algae_biomass_before_eating = getFoodBiomass(bottom, columnIndex, depthIndex);
	biomassType localeFoodBiomassInMicrograms = locale_algae_biomass_before_eating*this->food_conversion_factor;
	stroganovApproximation(localeTemperature);
#ifdef INDIVIDUAL_BASED_ANIMALS
	biomassType usedWeight= this->initial_grazer_weight[stage];
#else
	biomassType usedWeight= 15;
#endif
	foodConsumptionRate(depthIndex,columnIndex,bottom, animalCount, localeFoodBiomassInMicrograms, usedWeight, localeTemperature);
	locale_algae_biomass_after_eating =  getFoodBiomass(bottom,	columnIndex, depthIndex);
	defecation();
	animalRespiration(animalBiomass, localeTemperature, salinity_effect_matrix[depthIndex][columnIndex]);
	animalExcretion(salinity_corrected_animal_respiration);
	/* Calculate mortality based on animal biomass or animal count*/
#if defined(INDIVIDUAL_BASED_ANIMALS)&&defined(CREATE_NEW_COHORTS)

	biomassType mortalityBiomass=min<biomassType>(animalCount*this->initial_grazer_weight[stage], animalBiomass);
#else
	biomassType mortalityBiomass=animalBiomass;
#endif
	animalMortality(mortalityBiomass, localeTemperature, salinity_effect_matrix[depthIndex][columnIndex]);
#ifdef STARVATION_MORTALITY
//	animal_mortality/=min<biomassType>(1.0f,locale_algae_biomass_before_eating/142.0f);
//	animal_mortality=min<biomassType>(animal_mortality, animalBiomass);
#endif
#ifdef ADD_DEAD_BIOMASS_NUTRIENTS
	biomassType reabsorbedAlgalNutrients=reabsorbed_animal_nutrients_proportion*animal_mortality;
#ifdef CHECK_LOST_BIOMASS_ADDITION
	if(deadFloatingBiomass[depthIndex][columnIndex]<0.0f){
		cout<<"Lower than 0 dead biomass: "<<deadFloatingBiomass[depthIndex][columnIndex]<<"."<<endl;
	}
#endif
	/* Add dead animal biomass*/
	if(bottom){
		deadBottomBiomass[columnIndex]+=reabsorbedAlgalNutrients;
	} else{
		deadFloatingBiomass[depthIndex][columnIndex]+=reabsorbedAlgalNutrients;
	}
#ifdef CHECK_LOST_BIOMASS_ADDITION
	if(deadFloatingBiomass[depthIndex][columnIndex]<0.0f){
		cout<<"Lower than 0 dead biomass: "<<deadFloatingBiomass[depthIndex][columnIndex]<<"."<<endl;
	}
#endif
#endif

	consumed_biomass=locale_defecation+salinity_corrected_animal_respiration+animal_excretion_loss;
//	consumed_biomass=0.0f;
	biomassType localeBiomassDifferential=used_consumption-consumed_biomass-animal_mortality-animal_predatory_pressure;
	calculatePredationPressure(animalCount);
		if(*current_hour>=20*24&&bottom){
//			cout<<"Condition found."<<endl;
		}
	/* Plot grazer biomass differential*/
	lineBuffer.str("");
	lineBuffer.clear();
	lineBuffer<<depthIndex;
	lineBuffer<<commaString<<columnIndex;
	lineBuffer<<commaString<<(*current_hour);
	lineBuffer<<commaString<<bottom?1:0;
#ifdef EXTENDED_OUTPUT
	lineBuffer<<commaString<<consumption_per_individual;
	lineBuffer<<commaString<<consumption_per_individual/localeFoodBiomassBeforeEating;
	lineBuffer<<commaString<<used_consumption/localeFoodBiomassBeforeEating;
	lineBuffer<<commaString<<stroganov_adjustment;
	lineBuffer<<commaString<<locale_consumption;
	lineBuffer<<commaString<<locale_consumption_salt_adjusted;
	lineBuffer<<commaString<<locale_defecation;
	lineBuffer<<commaString<<basal_animal_respiration;
	lineBuffer<<commaString<<active_respiration_exponent;
	lineBuffer<<commaString<<active_respiration_factor;
	lineBuffer<<commaString<<active_respiration;
	lineBuffer<<commaString<<metabolic_respiration;
	lineBuffer<<commaString<<base_animal_respiration;
	lineBuffer<<commaString<<salinity_corrected_animal_respiration;
	lineBuffer<<commaString<<animal_excretion_loss;
	lineBuffer<<commaString<<animal_temperature_mortality;
	lineBuffer<<commaString<<animal_temp_independent_mortality;
	lineBuffer<<commaString<<animal_base_mortality;
	lineBuffer<<commaString<<salinity_mortality;
	lineBuffer<<commaString<<low_oxigen_animal_mortality;
	lineBuffer<<commaString<<animal_mortality;
	lineBuffer<<commaString<<animal_predatory_pressure;
	lineBuffer<<commaString<<animal_carrying_capacity;
	lineBuffer<<commaString<<locale_algae_biomass_before_eating;
	lineBuffer<<commaString<<locale_algae_biomass_after_eating;
	lineBuffer<<commaString<<reproduction_investment_subtraction;
	lineBuffer<<commaString<<reproduction_investment_exponent;
	lineBuffer<<commaString<<reproduction_investment_power;
	lineBuffer<<commaString<<reproduction_proportion_investment;
#endif
	lineBuffer<<commaString<<localeBiomassDifferential;
	return localeBiomassDifferential;
}



/* Food consumption (AquaTox Documentation, page 105, equation 98)*/
void AnimalBiomassDynamics::foodConsumptionRate(int depthIndex, int columnIndex, bool bottom, animalCountType animalCount, biomassType foodBiomassInMicrograms, biomassType individualWeight, physicalType localeTemperature){

#ifdef GRAZING_DEPENDING_ON_WEIGHT
	/* Grazing constant taken from [1] C. W. Burns, “Relation between filtering rate, temperature, and body size in four species of Daphnia,” Limnol. Oceanogr., vol. 14, no. 5, pp. 693–700, Sep. 1969.*/
	biomassType bodyLength = pow((individualWeight*MICROGRAM_TO_GRAM)/this->filtering_length_coefficient, this->filtering_length_exponent);
	biomassType usedFiltering = this->filtering_coefficient*pow(bodyLength, this->filtering_exponent)*MILLILITER_TO_VOLUME_PER_CELL;
#else
	biomassType usedFiltering = this->filtering_rate_per_individual_in_cell_volume;
#endif
#ifdef TEMPERATURE_MEDIATED_GRAZING
	usedFiltering*=localeTemperature*this->consumption_temperature_factor;
#endif
//	usedFiltering=0.0f;
	consumption_per_individual = usedFiltering*foodBiomassInMicrograms*stroganov_adjustment;
#ifdef SATURATION_GRAZING
	consumption_per_individual = min<biomassType>(FEEDING_SATURATION,MAXIMUM_GRAZING_ABSORBED);
//	grazing_per_individual = min<biomassType>(FEEDING_SATURATION,grazing_per_individual);
#endif
	locale_consumption= consumption_per_individual*animalCount;
	locale_consumption_salt_adjusted=locale_consumption*salinity_effect_matrix[depthIndex][columnIndex];
	/* Grazing can be adjusted according to water salinity*/
#ifdef ADJUST_SALINITY_GRAZERS
	used_consumption=locale_consumption_salt_adjusted;
#else
	used_consumption=locale_consumption;
#endif
	 used_consumption=min<biomassType>(foodBiomassInMicrograms, used_consumption);
	 biomassType updatedAlgaeBiomassInMicrograms = foodBiomassInMicrograms - used_consumption;
	 if(used_consumption<0){
		 cout<<"Error."<<endl;
	 }
	 biomassType updatedConcentration = updatedAlgaeBiomassInMicrograms/CELL_VOLUME_IN_LITER;
#ifdef GRAZING_EFFECT_ON_ALGAE_BIOMASS
	if(bottom){
		bottomFoodBiomass[columnIndex]= updatedConcentration;
	} else{
		floatingFoodBiomass[depthIndex][columnIndex] = updatedConcentration;
	}
#endif
}

/* Food consumption (AquaTox Documentation, page 105, equation 97)*/
void AnimalBiomassDynamics::defecation(){
	locale_defecation = DEFECATION_COEFFICIENT*used_consumption;
}

/* Zooplankton respiration (AquaTox Documentation, page 106, equation 100)*/
void AnimalBiomassDynamics::animalRespiration(biomassType zooBiomass, physicalType localeTemperature, physicalType localeSalinityEffect){
	base_animal_respiration = RESPIRATION_ADJUSTMENT*(basalRespiration(zooBiomass) + metabolicFoodConsumption());
	salinity_corrected_animal_respiration = base_animal_respiration*localeSalinityEffect;
}


/* Basal respiration (AquaTox Documentation, page 106, equation 101)*/
biomassType AnimalBiomassDynamics::basalRespiration(biomassType zooBiomass){
	basal_animal_respiration =zooBiomass*BASAL_RESPIRATION_RATE*this->basal_respiration_weight*stroganov_adjustment;
	return basal_animal_respiration;

}

/* An approximation of Stroganov function between 0 and 15 degrees (AquaTox Documentation, page 84, figure 59)*/
void AnimalBiomassDynamics::stroganovApproximation(physicalType localeTemperature){
	stroganov_adjustment= (0.1f+0.011f*localeTemperature)*STROGANOV_ADJUSTMENT;
}

/* Active respiration (AquaTox Documentation, page 107, equation 104)*/
biomassType AnimalBiomassDynamics::activeRespiration(biomassType zooBiomass, physicalType localeTemperature){
	active_respiration_exponent = RATE_RESPIRATION_CHANGE_10DEG*localeTemperature;
	active_respiration_factor=exp(active_respiration_exponent);
	active_respiration = zooBiomass*active_respiration_factor;
	return active_respiration;
}


/* Specific dynamic action respiration (AquaTox Documentation, page 109, equation 110)*/
biomassType AnimalBiomassDynamics::metabolicFoodConsumption(){
	metabolic_respiration= this->k_value_respiration*(used_consumption-locale_defecation);
	return metabolic_respiration;
}

/* Grazer excretion biomass loss (AquaTox Documentation, page 109, equation 111)*/
void AnimalBiomassDynamics::animalExcretion(biomassType localeRespiration){
	animal_excretion_loss= EXCRETION_RESPIRATION_PROPORTION*localeRespiration;
}


/* Grazer mortality (AquaTox Documentation, page 110, equation 112)*/
void AnimalBiomassDynamics::animalMortality(biomassType localeBiomass, physicalType localeTemperature, physicalType localeSalinityConcentration){
	animalBaseMortality(localeTemperature, localeBiomass);
	salinityMortality(localeBiomass);
	calculateLowOxigenMortality(localeBiomass);
	animal_mortality = animal_base_mortality+salinity_mortality+low_oxigen_animal_mortality;


}

/* Grazer base mortality (AquaTox Documentation, page 110, equation 113)*/
void AnimalBiomassDynamics::animalBaseMortality(physicalType
		localeTemperature, biomassType localeBiomass){
	animalTemperatureMortality(localeTemperature, localeBiomass);
	//
#ifdef GRAZER_CARRYING_CAPACITY_MORTALITY
	calculateGrazerCarryingCapacityMortality(localeBiomass);
	animal_temp_independent_mortality = this->animal_carrying_capacity*localeBiomass;
#else
	animal_temp_independent_mortality = this->animal_base_mortality_proportion*localeBiomass;
#endif
	animal_base_mortality= animal_temperature_mortality+animal_temp_independent_mortality;
}

/* Grazer base mortality (AquaTox Documentation, page 110, equation 114)*/
void AnimalBiomassDynamics::animalTemperatureMortality(physicalType localeTemperature, biomassType localeBiomass){
	if(localeTemperature<=MAXIMUM_TOLERABLE_TEMPERATURE){
		animal_temperature_mortality = 0.0f;
	} else{
		animal_temperature_mortality = localeBiomass*exp(MAXIMUM_TOLERABLE_TEMPERATURE-localeTemperature)/2.0f;
	}

}

void AnimalBiomassDynamics::calculateGrazerCarryingCapacityMortality(biomassType inputBiomass){
//	biomassType animalBiomassProduct = (this->maximum_found_animal_biomass*this->animal_carrying_capacity_coefficient);
//	biomassType carryingCapacityExponent = -inputBiomass/animalBiomassProduct+this->animal_carrying_capacity_intercept;
	biomassType animalBiomassProduct = inputBiomass*this->animal_carrying_capacity_coefficient;
	biomassType carryingCapacityExponent = -animalBiomassProduct+this->animal_carrying_capacity_intercept;
	biomassType carryingCapacityExponentiation = (1/(1+exp(carryingCapacityExponent)));
	animal_carrying_capacity =  max<biomassType>(0.0f,min<biomassType>(1.0f,this->animal_carrying_capacity_amplitude*(carryingCapacityExponentiation+this->animal_carrying_capacity_constant)));
//	animal_carrying_capacity=0.0f;
}



/* Salinity mortality (AquaTox Documentation, page 110, equation 112)*/
void AnimalBiomassDynamics::salinityMortality(biomassType localeBiomass){
		/* Salinity mortality can be present or not*/
#ifdef ADJUST_SALINITY_GRAZERS
	salinity_mortality=localeBiomass*(1-salinity_effect_matrix);
#else
	salinity_mortality = 0.0f;
#endif
}

/* The levels of oxygen concentration found in the lake do not reflect significant D. pulex mortality according to: (L. J. Weider and W. Lampert, ï¿½Differential response of Daphnia genotypes to oxygen stress: respiration rates, hemoglobin content and low-oxygen tolerance,ï¿½ Oecologia, vol. 65, no. 4, pp. 487ï¿½491, Mar. 1985.)*/
void AnimalBiomassDynamics::calculateLowOxigenMortality(biomassType inputBiomass){
	low_oxigen_animal_mortality=0.0f;
}

/* Include predation pressure to control grazer values*/
void AnimalBiomassDynamics::calculatePredationPressure(animalCountType zooplanktonLocaleCount){
#ifdef ADD_GRAZER_PREDATORY_PRESSURE
	/* Use a sigmoid function to model predatory pressure*/
	animal_predatory_pressure = 1/(1+exp(-(biomassType)zooplanktonLocaleCount+INITIAL_PREDATORY_PRESSURE));
#else
	animal_predatory_pressure = 0.0f;
#endif
}

#ifdef INDIVIDUAL_BASED_ANIMALS

#ifdef ANIMAL_STARVATION_HOURS_WITHOUT_FOOD
/* Starvation mortality ([1] Z. M. Gliwicz and C. Guisande, â€œFamily planning inDaphnia: resistance to starvation in offspring born to mothers grown at different food levels,â€ Oecologia, vol. 91, no. 4, pp. 463â€“467, Oct. 1992., page 465, Fig. 1)*/
void AnimalBiomassDynamics::animalStarvationMortality(AnimalCohort& cohort, biomassType foodBiomass){
	if(foodBiomass<this->food_starvation_threshold){
			/* If starving, increment the number of hours without food*/
			cohort.hoursWithoutFood++;
		if(cohort.hoursWithoutFood>=this->max_hours_without_food){
			/* If starving above the maximum, kill the cohort*/
			cohort.numberOfIndividuals=0;
			cohort.death=Starvation;
		}
	} else{
		/* IF there is enough food, reset the number of hours without food*/
		cohort.hoursWithoutFood=0;
	}

}
#elif defined(ANIMAL_STARVATION_PROPORTION_LOST_BIOMASS)

/* Starvation mortality ([1] Z. M. Gliwicz and C. Guisande, â€œFamily planning inDaphnia: resistance to starvation in offspring born to mothers grown at different food levels,â€ Oecologia, vol. 91, no. 4, pp. 463â€“467, Oct. 1992., page 465, Fig. 1)*/
void AnimalBiomassDynamics::animalStarvationMortality(AnimalCohort& cohort){
#ifdef ACCUMULATIVE_HOUR_STARVATION
	biomassType biomassPerIndividual = cohort.bodyBiomass/cohort.numberOfIndividuals;
	cohort.starvationBiomass +=consumed_biomass/biomassPerIndividual*this->dead_animals_per_lost_biomass_and_concentration;
	animalCountType animalsDeadByStarvation = cohort.starvationBiomass/this->initial_grazer_weight[cohort.stage];

	if(animalsDeadByStarvation>0){

		cohort.starvationBiomass=0.0f;
		cohort.numberOfIndividuals-=animalsDeadByStarvation;
		cohort.numberOfIndividuals=max<animalCountType>(cohort.numberOfIndividuals, 0);
//			if(animalsDeadByStarvation>0){
//				cout<<"The actual number of animals dead by starvation is "<<animalsDeadByStarvation<<" for "<<cohort<<endl;
//			}

	}
#else
	animalCountType maxAnimalsDeadByStarvation = consumed_biomass/this->initial_grazer_weight[cohort.stage];
	biomassType biomassPerIndividual = cohort.bodyBiomass/cohort.numberOfIndividuals;
	if(maxAnimalsDeadByStarvation>0){
		animalCountType actualAnimalsDeadByStarvation = (maxAnimalsDeadByStarvation/biomassPerIndividual)*this->dead_animals_per_lost_biomass_and_concentration;
		cohort.numberOfIndividuals-=actualAnimalsDeadByStarvation;
//		if(actualAnimalsDeadByStarvation>0){
//			cout<<"The actual number of animals dead by starvation is "<<actualAnimalsDeadByStarvation<<" for "<<cohort<<endl;
//		}
	}
#endif
}
#endif

/* Get food biomass depending of the animal cohort*/
biomassType  AnimalBiomassDynamics::getFoodBiomass(AnimalCohort& cohort){
	if(cohort.isBottomAnimal)
		return bottomFoodBiomass[cohort.y];
	else
		return floatingFoodBiomass[cohort.x][cohort.y];
}

#ifdef ANIMAL_AGING

/* Animal aging*/
void AnimalBiomassDynamics::animalAging(AnimalCohort& cohort){
	if(cohort.ageInHours++>this->maximum_age_in_hours){
		cohort.death=Senescence;
#ifdef REPORT_COHORT_INFO
		cout<<"Hour: "<<(*this->current_hour)<<". Senescence reached for cohort: "<<cohort<<"."<<endl;
#endif
	}
}

#endif



/* Remove dead animals*/


/* Iterate over animal collections and remove those that are dead*/
//void AnimalBiomassDynamics::removeDeadCohorts(set<AnimalCohort> *animalCohorts){
//	animalCohorts->erase(std::remove_if(animalCohorts->begin(), animalCohorts->end(), removeDeadCohort()), animalCohorts->end());
//}
//
//void AnimalBiomassDynamics::removeDeadAnimals(){
//	removeDeadCohorts(bottomAnimals);
//	removeDeadCohorts(floatingAnimals);
//
//}

#ifdef CREATE_NEW_COHORTS

void AnimalBiomassDynamics::calculateReproductionProportionInvestment(biomassType foodBiomass, biomassType foodBiomassDifferential){
	/* Proportion of investment to eggs taken from ([1] M. Lynch, â€œThe Life History Consequences of Resource Depression in Daphnia Pulex,â€ Ecology, vol. 70, no. 1, pp. 246â€“256, Feb. 1989.)*/
#ifdef EXPONENTIAL_GONAD_ALLOCATION
	reproduction_investment_subtraction=foodBiomass*MILLILITER_TO_LITER - this->reproduction_proportion_investment_intercept;
	reproduction_investment_exponent=-this->reproduction_proportion_investment_coefficient*(reproduction_investment_subtraction);
	reproduction_investment_power=exp((biomassType)reproduction_investment_exponent);
	reproduction_proportion_investment = this->reproduction_proportion_investment_amplitude*(1 - reproduction_investment_power);
#elif defined(SIGMOIDAL_GONAD_ALLOCATION)
	reproduction_investment_subtraction=-reproduction_proportion_investment_coefficient*(foodBiomass*MILLILITER_TO_LITER);
	reproduction_investment_exponent=reproduction_investment_subtraction+this->reproduction_proportion_investment_amplitude;
	reproduction_investment_power=exp((biomassType)reproduction_investment_exponent);
	reproduction_proportion_investment=max<biomassType>(0.0f,min<biomassType>(1.0f,this->reproduction_proportion_investment_multiplier*this->reproduction_proportion_investment_intercept/(this->reproduction_proportion_investment_intercept+reproduction_investment_power))+this->reproduction_proportion_investment_constant);
#elif defined(DIFFERENTIAL_ALLOCATION)
	reproduction_proportion_investment=foodBiomassDifferential/0.012f;
	#else
	reproduction_investment_subtraction=0;
	reproduction_investment_exponent=reproduction_proportion_investment_coefficient*(foodBiomass*MILLILITER_TO_LITER);
	reproduction_investment_power=reproduction_investment_exponent+this->reproduction_proportion_investment_amplitude;
	reproduction_proportion_investment=max<biomassType>(0,min<biomassType>(0.4f,reproduction_investment_power));
#endif

}

biomassType AnimalBiomassDynamics::createNewCohort(AnimalCohort& parentCohort, biomassType initialBiomass, vector<AnimalCohort> *juveniles){
	/* The attributes from the child cohort are inherited from the parent cohort*/
	EggCohort eggCohort;
	eggCohort.x=parentCohort.x;
	eggCohort.y=parentCohort.y;
	if(eggCohort.x<0||eggCohort.y<0){
		cout<<"Error."<<endl;
	}
	eggCohort.isBottomAnimal=parentCohort.isBottomAnimal;
	/*The others are attributes for newborn cohorts*/
	eggCohort.ageInHours=0;
	eggCohort.numberOfEggs=(animalCountType)(initialBiomass/initial_grazer_weight[AnimalStage::Egg]);
	eggCohort.biomass=(biomassType)eggCohort.numberOfEggs*initial_grazer_weight[AnimalStage::Egg];
	eggCohort.cohortID=(*this->cohortID)++;
	eggCohort.hasHatched=false;
	/*Add to the correct map of eggs*/
	if(eggCohort.isBottomAnimal){
		bottomEggs.push_back(eggCohort);
	} else {
		floatingEggs.push_back(eggCohort);
	}
	AnimalCohort juvenileCohort=parentCohort;
	pair<int,int> cohortCoordinates(parentCohort.x, parentCohort.y);
	juvenileCohort.bodyBiomass=initialBiomass;
	juvenileCohort.stage=AnimalStage::Juvenile;
	juvenileCohort.gonadBiomass=0.0f;
	juvenileCohort.numberOfIndividuals=(animalCountType)(initialBiomass/initial_grazer_weight[AnimalStage::Juvenile]);
	juvenileCohort.cohortID=(*this->cohortID)++;
//	juveniles->push_back(juvenileCohort);
	return eggCohort.biomass;
}
#ifdef MATURE_JUVENILES
void AnimalBiomassDynamics::matureEggs(vector<EggCohort>& eggs, vector<AnimalCohort>& juveniles){
#else
void AnimalBiomassDynamics::matureEggs(vector<EggCohort>& eggs, map<pair<int,int>,AnimalCohort> *juveniles){
#endif
	for (std::vector<EggCohort>::iterator it=eggs.begin(); it!=eggs.end(); ++it){
		/* Increase egg age and check if the egg has hatched*/
		it->hasHatched=++it->ageInHours>incubation_hours;
		const EggCohort& eggCohort=*it;
		/* If eggs are counted, report number of eggs found*/
#ifdef COUNT_EGGS
		lineBuffer.str("");
		lineBuffer.clear();
		lineBuffer<<eggCohort.x;
		lineBuffer<<commaString<<eggCohort.y;
		lineBuffer<<commaString<<(*current_hour);
		lineBuffer<<commaString<<eggCohort.isBottomAnimal?1:0;
#ifdef EXTENDED_OUTPUT
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
		lineBuffer<<commaString<<0;
#endif
		lineBuffer<<commaString<<0<<commaString<<eggCohort.numberOfEggs<<commaString<<eggCohort.biomass<<commaString<<AnimalStage::Egg<<commaString<<eggCohort.cohortID<<endl;
		animalBiomassBuffer<<lineBuffer.str();
#endif
		/*If the egg has hatched, add to the adult cohort*/
	    if(eggCohort.hasHatched){
#ifdef REPORT_COHORT_INFO
	    	cout<<"Hour: "<<*(this->current_hour)<<". Maturing egg cohort: "<<eggCohort<<"."<<endl;
#endif

#ifndef MATURE_JUVENILES
	    	pair<int,int> cohortCoordinates(eggCohort.x, eggCohort.y);
	    	if ( juveniles->find(cohortCoordinates) == juveniles->end() ) {
#endif
	    		AnimalCohort animalCohort;
	    		animalCohort.x=eggCohort.x;
	    		animalCohort.y=eggCohort.y;
	    		animalCohort.bodyBiomass=eggCohort.biomass;
	    		animalCohort.numberOfIndividuals=eggCohort.numberOfEggs;
	    		animalCohort.cohortID=eggCohort.cohortID;
	    		animalCohort.isBottomAnimal=eggCohort.isBottomAnimal;
	    		animalCohort.gonadBiomass=0.0f;
	    		animalCohort.ageInHours=0;
#ifdef MATURE_JUVENILES
	    		animalCohort.stage=AnimalStage::Juvenile;
	    		juveniles.push_back(animalCohort);
#else
	    		animalCohort.stage=AnimalStage::Mature;
	    		/* If the adult cohort exists, increase biomass and number of eggs*/
	    		(*juveniles)[cohortCoordinates]=animalCohort;
	    	} else{
	    		/* Otherwise, add number of individuals and biomass to the existing cohort*/
	    		(*juveniles)[cohortCoordinates]+=animalCohort;
	    	}
//	    	animalCohort.death=causeOfDeath::None;
//	    	animalCohort.hoursWithoutFood=animalCohort.ageInHours=0;
	    	/*Register newly hatched individuals as adults*/
#endif


#ifdef REPORT_COHORT_INFO
	    	cout<<"Egg cohort matured."<<endl;
#endif
	    }

	}
	/* Remove eggs that have hatched*/
	int size_before_erase=eggs.size();
	eggs.erase(std::remove_if(eggs.begin(), eggs.end(), removeOldEggs()),
			eggs.end());
	int size_after_erase=eggs.size();

}

#ifdef MATURE_JUVENILES

/* Mature juveniles and update their biomass */
void AnimalBiomassDynamics::matureJuveniles(vector<AnimalCohort>& juveniles, map<pair<int,int>,AnimalCohort> *adultAnimals){
	for (std::vector<AnimalCohort>::iterator it = juveniles.begin();
			it != juveniles.end(); ++it) {
		updateCohortBiomass(*it);
		/* Remove cohort if the number of animals or its biomass is 0*/
		/*Mature age*/
		if(++it->ageInHours>=this->maturation_hours){
			/* Create a copy of the cohort to store in adult animals. After erasing it from the juveniles collection, the original cohort will be deleted*/
			it->stage=AnimalStage::Mature;
			AnimalCohort cohortCopy;
			cohortCopy.ageInHours=it->ageInHours;
			cohortCopy.bodyBiomass=it->bodyBiomass;
			cohortCopy.cohortID=it->cohortID;
			cohortCopy.gonadBiomass=it->gonadBiomass;
			cohortCopy.isBottomAnimal=it->isBottomAnimal;
			cohortCopy.numberOfIndividuals=it->numberOfIndividuals;
			cohortCopy.stage=it->stage;
			cohortCopy.x=it->x;
			cohortCopy.y=it->y;
			pair<int,int> cohortCoordinates(cohortCopy.x, cohortCopy.y);
			if ( adultAnimals->find(cohortCoordinates) == adultAnimals->end() ) {
				(*adultAnimals)[cohortCoordinates]=cohortCopy;
			} else{
				(*adultAnimals)[cohortCoordinates]+=cohortCopy;
			}
		}


	}

	/*Remove starved or mature juveniles*/
	juveniles.erase(std::remove_if(juveniles.begin(), juveniles.end(), removeStarvedOrMaturedJuveniles()), juveniles.end());
}
#endif

#endif

/* If the migration index is greater than 0, migrate adult and juvenile cohorts*/

void AnimalBiomassDynamics::migrateAnimalCohorts(){
	int migrationStep = zooplanktonBiomassCenterDifferencePerDepth[*current_hour%HOURS_PER_DAY];
	if(migrationStep!=0){
		migrateAdultCohortsStructurally(floatingAnimals,migrationStep);
		migrateJuvenileCohortsStructurally(floatingJuveniles,migrationStep);

//
//		if(migrationStep>0){
//			for(int i=1; i<=migrationStep; i++){
//				migrateAdultCohortsStructurally(floatingAnimals,i);
//						//migrateJuvenileCohortsStructurally(floatingJuveniles,i);
//			}
//		} else{
//			for(int i=migrationStep; i<0; i++){
//							migrateAdultCohortsStructurally(floatingAnimals,i+migrationStep);
//									//migrateJuvenileCohortsStructurally(floatingJuveniles,i+migrationStep);
//						}
//		}

	}
}

/* Migate the center of mass of the adult cohorts*/
void AnimalBiomassDynamics::migrateAdultCohortsStructurally(std::map<pair<int,int>,AnimalCohort> *animals, int migrationStep){
	vector<AnimalCohort> modificatedCohorts;
	AnimalCohort& cohortAlias=(*animals)[pair<int,int>(11,0)];
	for (std::map<pair<int,int>,AnimalCohort>::iterator it = animals->begin();
				it != animals->end(); ++it) {
		bool migratedBiomass = migrateAdultCohortStructurally(animals, it->second, migrationStep, modificatedCohorts);
		/*If the cohort has migrated, remove it from its previous location*/
		if(migratedBiomass){
//			it->second.bodyBiomass*=0.1f;
//			it->second.gonadBiomass*=0.1f;
//			it->second.numberOfIndividuals*=0.1f;
//			animals->erase(it);
//			modificatedCohorts.push_back(it->second);
		}
	}
	for (std::vector<AnimalCohort>::iterator it = modificatedCohorts.begin();
					it != modificatedCohorts.end(); ++it) {
		/* Store original coordinates */
		pair<int,int> cohortCoordinates(it->x, it->y), originalCoordinates(it->originalX, it->originalY);
		if(animals->find(cohortCoordinates)==animals->end()){
			if(it->stage!=Mature){
				cout<<"Error."<<endl;
			}
			if(it->x<0||it->y<0){
				cout<<"Error."<<endl;
			}
//			(*animals)[cohortCoordinates] = *it;
		} else{
			(*animals)[cohortCoordinates] += *it;
			if((*animals)[cohortCoordinates].numberOfIndividuals<0){
						cout<<"Error."<<endl;
					}
		}
		AnimalCohort originalCohort = (*animals)[originalCoordinates];
		/*Remove migrated individuals*/
		animalCountType originalCount = it->numberOfIndividuals;
		(*animals)[originalCoordinates]-=*it;
		if((*animals)[originalCoordinates].numberOfIndividuals<0){
			cout<<"Error."<<endl;
		}
	}
	/* Remove empty cohorts */
	for (std::map<pair<int,int>,AnimalCohort>::iterator it = animals->begin();
					it != animals->end(); ++it) {
		if(it->second.numberOfIndividuals<=0){
			animals->erase(it);
		}
	}
}


void AnimalBiomassDynamics::migrateJuvenileCohortsStructurally(vector<AnimalCohort>& juveniles, int migrationStep){
	for (std::vector<AnimalCohort>::iterator it = juveniles.begin();
			it != juveniles.end(); ++it) {
		migrateJuvenileCohortStructurally(juveniles, *it, migrationStep);
		//(*it)*=0.9f;
	}

}

bool AnimalBiomassDynamics::migrateAdultCohortStructurally(std::map<pair<int,int>,AnimalCohort> *animals, AnimalCohort& cohort, int migrationStep, vector<AnimalCohort>& modificatedCohort){
	/*Migrate the center of biomass of the animal cohorts*/
	int destinationX =cohort.x+migrationStep;
	destinationX=max<int>(0,min<int>(maxDepthIndex[cohort.y], destinationX));
	/*Check the limits of the new depth of the cohort*/
	bool verticalMigrationOccurred = destinationX>=0&&destinationX<MAX_DEPTH_INDEX&&destinationX<=maxDepthIndex[cohort.y]&&destinationX!=cohort.x;
	if(verticalMigrationOccurred){
		pair<int,int> cohortCoordinates(destinationX, cohort.y);
		AnimalCohort migratedCohort=cohort;
		int previousDepth = migratedCohort.x;
		migratedCohort.originalX=cohort.x;
		migratedCohort.originalY=cohort.y;
		migratedCohort.x=destinationX;

		if(migratedCohort.stage!=Mature){
						cout<<"Error."<<endl;
					}
					if(migratedCohort.x<0||migratedCohort.y<0){
						cout<<"Error."<<endl;
					}
		/*Update the cohort numbers*/
//		if(animals->find(cohortCoordinates)==animals->end()){
//			(*animals)[cohortCoordinates] = migratedCohort;
//		} else{
//			(*animals)[cohortCoordinates] += migratedCohort;
//		}
		modificatedCohort.push_back(migratedCohort);
	}
	return verticalMigrationOccurred;
}

void AnimalBiomassDynamics::migrateJuvenileCohortStructurally(vector<AnimalCohort>& juveniles, AnimalCohort& cohort, int migrationStep){
	/*Migrate the center of biomass of the animal cohorts*/
	int destinationX =cohort.x+migrationStep;
	destinationX=max<int>(0,min<int>(maxDepthIndex[cohort.y], destinationX));
	/*Check the limits of the new depth of the cohort*/
	if(destinationX>=0&&destinationX<MAX_DEPTH_INDEX&&destinationX<=maxDepthIndex[cohort.y]&&destinationX!=cohort.x){
		cohort.x=destinationX;
	}
}


#endif


/* Set the maximum allocation biomass to 0.5 and set that the grazers left behind do not eat nor reproduce*/


} /* namespace FoodWebModel */
