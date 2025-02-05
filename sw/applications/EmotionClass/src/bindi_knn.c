/*
 *  Copyright (c) [2024] [Embedded Systems Laboratory (ESL), EPFL]
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Original app:    Bindi from Jose A. Miranda Calero,  Universidad Carlos III de Madrid                //
// Modified by:     Dimitrios Samakovlis, Embedded Systems Laboratory (ESL), EPFL                       //
                                                                //
// Date:            February 2024                                                                       //  
//////////////////////////////////////////////////////////////////////////////////////////////////////////


#define _BINDI_KNN_C_SRC

/****************************************************************************/
/**                                                                        **/
/**                            MODULES USED                                **/
/**                                                                        **/
/****************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "defines.h"
#include "bindi_knn.h"
#include "training_dataset.h"
#include "timer_sdk.h"

/****************************************************************************/
/**                                                                        **/
/**                     PROTOTYPES OF LOCAL FUNCTIONS                      **/
/**                                                                        **/
/****************************************************************************/

/**
 * @brief calculate_euclidean_distance.  ...
 *
 * ...
 *
 * @param ....
 * 
 * @return ...
 */
float calculate_euclidean_distance(Knn_sample_definitionT sample_1, 
                                             Knn_sample_definitionT sample_2);
/**
 * @brief compare_samples.  ...
 *
 * ...
 *
 * @param ....
 * 
 * @return ....
 */
int compare_samples(const void *sample_1, const void *sample_2);

/**
 * @brief perform_single_inference.  Predicts the label of a sample.
 *
 * It includes a mechanism to minimize false negative cases
 *
 * @param none.
 * 
 * @return it returns the expected label 'FEAR'/'NO_FEAR'.
 */
knnState_t perform_single_inference(void);

/****************************************************************************/
/**                                                                        **/
/**                          EXPORTED VARIABLES                            **/
/**                                                                        **/
/****************************************************************************/

/****************************************************************************/
/**                                                                        **/
/**                           GLOBAL VARIABLES                             **/
/**                                                                        **/
/****************************************************************************/

//float* knn_training_dataset;
/* Variable definition: set of samples for the testing dataset. 
 * In a regular case, this set will have only one sample to save memory */
Knn_sample_definitionT   knn_testing_dataset;

/****************************************************************************/
/**                                                                        **/
/**                          EXPORTED FUNCTIONS                            **/
/**                                                                        **/
/****************************************************************************/

knnState_t runKNN(void)
{
  return(perform_single_inference());
}
/*****************************************************************************
*****************************************************************************/

/****************************************************************************/
/**                                                                        **/
/**                           LOCAL FUNCTIONS                              **/
/**                                                                        **/
/****************************************************************************/

float calculate_euclidean_distance(Knn_sample_definitionT sample_1, 
                                             Knn_sample_definitionT sample_2)
{
  float sub_field_1 = (sample_1.field_1 - sample_2.field_1)*
                                (sample_1.field_1 - sample_2.field_1);
  float sub_field_2 = (sample_1.field_2 - sample_2.field_2)*
                                (sample_1.field_2 - sample_2.field_2);
  float sub_field_3 = (sample_1.field_3 - sample_2.field_3)*
                                (sample_1.field_3 - sample_2.field_3);
  return (sub_field_1 + sub_field_2 + sub_field_3);
}

// Calculate all Euclidean distances from test point
// Sort the n closest distances 
void sort_n_Euclidean_distances(int n_closest) {
  uint32_t t1=0, t2=0;

  t1 = timer_get_cycles();
  // Allocate distances array
  float *distances = (float *) malloc(sizeof(float) * TRAINING_DATASET_SIZE);
  t2 = timer_get_cycles();
  #ifdef PRINT_KNN_DETAILS_CYCLES
  printf("Allocating distances array: %d\n", t2-t1);
  #endif

  // Calculate all distances from the test point
  #ifdef PRINTING_DETAILS
  printf("--- kNN Inference -> Euclidean distances\n");
  #endif
  t1 = timer_get_cycles();
  for (int i = 0; i < TRAINING_DATASET_SIZE; i++) {
    distances[i] = calculate_euclidean_distance(knn_training_dataset[i], knn_testing_dataset);
  }
  t2 = timer_get_cycles();
  #ifdef PRINT_KNN_DETAILS_CYCLES
  printf("Calculating all distances: %d\n", t2-t1);
  #endif
  
  // Do a sorting to sort the n_closest distances
  // Traverse the distance array and swap both distance and training arrays
  #ifdef PRINTING_DETAILS
  printf("--- kNN Inference -> Sorting distances\n");
  #endif
  t1 = timer_get_cycles();
  for (int i = 0; i < n_closest; i++) {
    float min_distance = distances[TRAINING_DATASET_SIZE - 1];
    int min_index = TRAINING_DATASET_SIZE - 1;
    for (int j = TRAINING_DATASET_SIZE - 2; j >= i ; j--) {
      // mark minimum distance
      if (distances[j] < min_distance)  {
        min_distance = distances[j];
        min_index = j;
      }
    }
    // Swap min distance with current 1st distance
    float temp_dist;

    temp_dist = distances[i];
    distances[i] = distances[min_index];
    distances[min_index] = temp_dist;

    // Swap closest training point with current 1st training point
    Knn_sample_definitionT temp_training;

    temp_training = knn_training_dataset[i];
    knn_training_dataset[i] = knn_training_dataset[min_index];
    knn_training_dataset[min_index] = temp_training;
  }
  t2 = timer_get_cycles();
  #ifdef PRINT_KNN_DETAILS_CYCLES
  printf("Sorting distances: %d\n", t2-t1);
  #endif


  // Free distances array
  free((void *)distances);

}

knnState_t perform_single_inference(void)
{
  uint32_t t1=0, t2=0;
  float percentage_count_fear = 0;
  float threshold_cost_percent = (((float) COST_MISSCLASIFICATION_NOFEAR /
                                   (float) COST_MISSCLASIFICATION_FEAR) / 2);
  uint32_t i;
  uint32_t count_fear=0, count_no_fear=0;
		
  /* Inference based on Euclidean distance and closest points */
  t1 = timer_get_cycles();
  uint32_t NEIGHBOURHOOD_SIZE_VAR = (ceil(sqrt(TRAINING_DATASET_SIZE)));
  t2 = timer_get_cycles();
  #ifdef PRINT_KNN_DETAILS_CYCLES
  printf("Euclidean distance and closest points: %d\n", t2-t1);
  #endif

  // Sort the NEIGHBOURHOOD_SIZE_VAR minimum distances from the test point
  t1 = timer_get_cycles();
  sort_n_Euclidean_distances(NEIGHBOURHOOD_SIZE_VAR);
  t2 = timer_get_cycles();
  #ifdef PRINT_KNN_DETAILS_CYCLES
  printf("Sorting distances: %d\n", t2-t1);
  #endif

  t1 = timer_get_cycles();
  for (i = 0; i < NEIGHBOURHOOD_SIZE_VAR; i++)
  {
    if(knn_training_dataset[i].label == NO_FEAR)
      count_no_fear ++;
    else
      count_fear ++;
  }
  t2 = timer_get_cycles();
  #ifdef PRINT_KNN_DETAILS_CYCLES
  printf("Counting fear and no fear: %d\n", t2-t1);
  #endif

  #ifdef PRINTING_DETAILS
  printf("--- kNN Inference -> neighbors: Fear=%d, Nofear=%d\n", count_fear, count_no_fear);
  #endif

  /*... 50%*/
  percentage_count_fear = (float)count_fear / (float)(count_fear+count_no_fear);

  /*...*/
  if(percentage_count_fear > threshold_cost_percent)
    return FEAR;
  else 
    return NO_FEAR;
}

/****************************************************************************/
/**                                                                        **/
/**                                EOF                                     **/
/**                                                                        **/
/****************************************************************************/
