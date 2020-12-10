/* 
 * File:   calculate_collisions.c
 * Author: Emanuele Pisano
 *
 */

#include "iccpa.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <tgmath.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define TRUE 0xff
#define FALSE 0
#define BYTE_SPACE 256

#define KEY_SIZE 16     //key size in byte


#define MAKE_FN_NAME(name, type) name ## _ ## type
#define THREAD_ARGS(type) MAKE_FN_NAME(Thread_args, type)
#define CALCULATE_COLLISIONS(type) MAKE_FN_NAME(calculate_collisions, type)
#define READ_TRACES(type) MAKE_FN_NAME(read_traces, type)
#define ANALYZE_TRACES(type) MAKE_FN_NAME(analyze_traces, type)
#define COLLISION(type) MAKE_FN_NAME(collision, type)
#define COMPUTE_ARRAYS(type) MAKE_FN_NAME(compute_arrays, type)
#define FIND_COLLISIONS(type) MAKE_FN_NAME(find_collisions, type)
#define STANDARD_DEVIATION(type) MAKE_FN_NAME(standard_deviation, type)
#define COVARIANCE(type) MAKE_FN_NAME(covariance, type)
#define OPTIMIZED_PEARSON(type) MAKE_FN_NAME(optimized_pearson, type)


typedef struct {
    DATA_TYPE*** T;
}THREAD_ARGS(DATA_TYPE);


void READ_TRACES(DATA_TYPE) (FILE* input, uint8_t plaintexts[M][plaintextlen]);
uint8_t** ANALYZE_TRACES(DATA_TYPE)();
int COLLISION(DATA_TYPE) (DATA_TYPE** T, int theta0, int theta1, DATA_TYPE* sum_array, DATA_TYPE* std_dev_array, DATA_TYPE* max_correlation);
void COMPUTE_ARRAYS(DATA_TYPE) (DATA_TYPE** T, DATA_TYPE* sum_array, DATA_TYPE* std_dev_array);
void* FIND_COLLISIONS(DATA_TYPE) (void* args);
DATA_TYPE STANDARD_DEVIATION(DATA_TYPE) (DATA_TYPE** T, int theta, int t);
DATA_TYPE COVARIANCE(DATA_TYPE) (DATA_TYPE** T, int theta0, int theta1, int t, DATA_TYPE* sum_array);
DATA_TYPE OPTIMIZED_PEARSON(DATA_TYPE) (DATA_TYPE** T, int theta0, int theta1, DATA_TYPE* sum_array, DATA_TYPE* std_dev_array);


/*
 * Calculate COLLISION(DATA_TYPE)s starting from the power traces
 */
uint8_t** CALCULATE_COLLISIONS(DATA_TYPE) (FILE* infile){
    
    uint8_t m[M][plaintextlen];
    
    
    char fcurr_name[50];
    char to_concat[10];   
    FILE* fcurr;
    
    mkdir("./iccpa_fopaes_bi_temp", 0700);
    
    for(int i=0 ; i<plaintextlen ; i++){
        for(int j=0 ; j<BYTE_SPACE ; j++){
            strcpy(fcurr_name, "./iccpa_fopaes_bi_temp/");
            sprintf(to_concat, "%d", i+1);
            strcat(fcurr_name, to_concat);
            strcat(fcurr_name, "_");
            sprintf(to_concat, "%d", j);
            strcat(fcurr_name, to_concat);
            fcurr = fopen(fcurr_name, "w+");
            fclose(fcurr);
        }
    }
    
    READ_TRACES(DATA_TYPE)(infile, m);
    
    fclose(infile);    
    
    return ANALYZE_TRACES(DATA_TYPE)();
    
}

/*
 * Read power traces and create the needed temp files
 */
void READ_TRACES(DATA_TYPE) (FILE* input, uint8_t plaintexts[M][plaintextlen]){
    DATA_TYPE temp1[plaintextlen][l];
    DATA_TYPE temp2[plaintextlen][l];
    
    char fcurr_name[50];
    char to_concat[10];   
    FILE* fcurr;
    
    for(int j=0; j<N; j++){
        for(int i=0; i<plaintextlen; i++){
            //read the first l samples (corresponding to the loading of the plaintext byte) 
            fread( temp1[i], sizeof(DATA_TYPE), l, input);
            //skip the next l samples (corresponding to the processing of the byte) 
            fseek(input, l*sizeof(DATA_TYPE) , SEEK_CUR);
            //read the next l samples (corresponding to the storing of the cyphertext byte) 
            fread( temp2[i], sizeof(DATA_TYPE), l, input);
        }
        fseek(input, (nsamples-(l*plaintextlen*3))*sizeof(DATA_TYPE) , SEEK_CUR);
        fread( plaintexts[j], sizeof(char), plaintextlen, input);
        
        for(int i=0; i<plaintextlen; i++){
            uint8_t byte_value = (uint8_t) plaintexts[j][i];
            strcpy(fcurr_name, "./iccpa_fopaes_bi_temp/");
            sprintf(to_concat, "%d", i+1);
            strcat(fcurr_name, to_concat);
            strcat(fcurr_name, "_");
            sprintf(to_concat, "%d", byte_value);
            strcat(fcurr_name, to_concat);
            fcurr = fopen(fcurr_name, "a");
            fwrite(temp1[i], sizeof(DATA_TYPE), l, fcurr);
            fwrite(temp2[i], sizeof(DATA_TYPE), l, fcurr);
            fclose(fcurr);
        }
    }
}

/*
 * Decision function returning true or false depending on whether the same data was involved in two given instructions theta0 and theta1
 * compare the value of a synthetic criterion with a practically determined threshold
 * as criterion we used the maximum Pearson correlation factor
 * theta0 and theta1 are time instant at which instruction starts (for performance reasons)
 */
int COLLISION(DATA_TYPE) (DATA_TYPE** T, int theta0, int theta1, DATA_TYPE* sum_array, DATA_TYPE* std_dev_array, DATA_TYPE* max_correlation){
    
    DATA_TYPE correlation = OPTIMIZED_PEARSON(DATA_TYPE)(T, theta0, theta1, sum_array, std_dev_array);
    //check if max is greater than threshold
    if(correlation>threshold && correlation>*max_correlation){
        *max_correlation = correlation;
        return TRUE;
    }
    else{
        return FALSE;
    }
}

/*
 * Efficiently compute in one step both sum and std deviation
 */
void COMPUTE_ARRAYS(DATA_TYPE) (DATA_TYPE** T, DATA_TYPE* sum_array, DATA_TYPE* std_dev_array){
    DATA_TYPE sum;
    DATA_TYPE squared_sum;
    for(int i=0; i<(l*2); i++){
        sum = 0;
        squared_sum = 0;
        for(int j=0; j<n; j++){
            sum += T[j][i];
            squared_sum += T[j][i]*T[j][i];
        }
        sum_array[i] = sum;
        std_dev_array[i] = sqrt((n*squared_sum)-(sum*sum));
    }
}

/*
 * Efficiently compute Person correlator
 */
DATA_TYPE OPTIMIZED_PEARSON(DATA_TYPE)(DATA_TYPE** T, int theta0, int theta1, DATA_TYPE* sum_array, DATA_TYPE* std_dev_array){
    int i;
    DATA_TYPE correlation;
    DATA_TYPE max_correlation = 0;
    for(i=0; i<l; i++){
        correlation = COVARIANCE(DATA_TYPE)(T, theta0, theta1, i, sum_array)  /  (std_dev_array[theta0+i] * std_dev_array[theta1+i]);
        if(correlation > max_correlation){
            max_correlation = correlation;
        }
    }
    return max_correlation;
}

/*
 * Compute COVARIANCE(DATA_TYPE) of given samples in the given time sequences
 */
DATA_TYPE COVARIANCE(DATA_TYPE)(DATA_TYPE** T, int theta0, int theta1, int t, DATA_TYPE* sum_array){
    int i;
    DATA_TYPE first_sum=0;
    for(i=0; i<n; i++){
        first_sum += (T[i][theta0+t]*T[i][theta1+t]);
    }
    return (n*first_sum)-(sum_array[theta0+t]*sum_array[theta1+t]);
}

/*
 * Compute standard deviation of given samples in a given time sequence
 */
DATA_TYPE STANDARD_DEVIATION(DATA_TYPE)(DATA_TYPE** T, int theta, int t){
    int i;
    DATA_TYPE first_sum=0, second_sum=0;
    for(i=0; i<n; i++){
        first_sum += (T[i][theta+t])*(T[i][theta+t]);
        second_sum += T[i][theta+t];
    }
    return sqrt((n*first_sum)-(second_sum*second_sum));
}

/*
 * Analyze power traces using the temp files created, spawning a thread for every
 * key byte to guess
 */
uint8_t** ANALYZE_TRACES(DATA_TYPE)(){
    //T[BYTE_SPACE/2][n][l*2]
    DATA_TYPE*** T;
    DATA_TYPE temp[l];
    int byte_value;
    
    pthread_t running_threads[KEY_SIZE];
    
    char fcurr_name[50];
    char to_concat[10];   
    FILE* fcurr;
    
    uint8_t** guesses = malloc(sizeof(uint8_t*)*KEY_SIZE);
    
    for(int i=0; i<KEY_SIZE; i++){
        T = malloc(sizeof(DATA_TYPE**)*BYTE_SPACE/2);
        THREAD_ARGS(DATA_TYPE)* args = malloc(sizeof(THREAD_ARGS(DATA_TYPE)));
        
        for(byte_value=0; byte_value<(BYTE_SPACE/2); byte_value++){
            T[byte_value] = malloc(sizeof(DATA_TYPE*)*n);
            strcpy(fcurr_name, "./iccpa_fopaes_bi_temp/");
            sprintf(to_concat, "%d", i+1);
            strcat(fcurr_name, to_concat);
            strcat(fcurr_name, "_");
            sprintf(to_concat, "%d", byte_value*2);
            strcat(fcurr_name, to_concat);
            fcurr = fopen(fcurr_name, "r");
            for(int j=0; j<n; j++){
                T[byte_value][j] = malloc(sizeof(DATA_TYPE)*l*2);
                for(int k=0; k<2; k++){ 
                    fread(temp, sizeof(DATA_TYPE), l, fcurr);                   
                    for(int q=0; q<l; q++){
                        T[byte_value][j][k*l+q] = temp[q];
                    }
                }
            }
            fclose(fcurr);
        }
        args->T = T;
        pthread_create(&(running_threads[i]), NULL, FIND_COLLISIONS(DATA_TYPE), (void* ) args);
    }
    for(int j=0; j<KEY_SIZE; j++){
        void** rv;
        pthread_join(running_threads[j], rv);
        if(rv == NULL){
            guesses[j]=NULL;
        }
        else{
            guesses[j] = (uint8_t*) *rv;
        }
    }
    return guesses;
}

/*
 * Check COLLISION(DATA_TYPE)s inside a given key byte
 */
void* FIND_COLLISIONS(DATA_TYPE)(void* args){
    THREAD_ARGS(DATA_TYPE)* args_casted = (THREAD_ARGS(DATA_TYPE)*) args;
    DATA_TYPE*** T = args_casted->T;
    DATA_TYPE sum_array[2*l];
    DATA_TYPE std_dev_array[2*l];
    DATA_TYPE* max_correlation = malloc(sizeof(DATA_TYPE));
    *max_correlation = 0;
    int found = FALSE;
    uint8_t* guessed_byte = malloc(sizeof(uint8_t));
    int i;
   
    for(i=0; i<(BYTE_SPACE/2); i++){    
        COMPUTE_ARRAYS(DATA_TYPE)(T[i], sum_array, std_dev_array);
        if(COLLISION(DATA_TYPE)(T[i], 0, l, sum_array, std_dev_array, max_correlation)){
            /* if a collision is detected, the corresponding byte value is returned
             */
            found = TRUE;
            *guessed_byte = i*2;
        }
    }
    
    //free memory
    free(max_correlation);
    for(i=0; i<(BYTE_SPACE/2); i++){
        for(int j=0; j<n; j++){
            free(T[i][j]);
        }
        free(T[i]);
    }
    free(T);
    free(args);
    
    if(found){
        pthread_exit(guessed_byte);
    }
    else{
        pthread_exit(NULL);
    }
}
