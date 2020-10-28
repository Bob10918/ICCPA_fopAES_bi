/* 
 * File:   main.c
 * Author: Emanuele Pisano
 *
 * Created on 19 agosto 2020, 10.51
 */

#include "iccpa.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define TRUE 0xff
#define FALSE 0
#define BYTE_SPACE 256

#define KEY_SIZE 16     //key size in byte

void print_guesses(char** guesses);
void recursive_print(char** current_key, int index, char** guesses);

int main(int argc, char** argv) {
    
    FILE* infile = fopen(argv[1], "r");
    fread(&N, sizeof(uint32_t), 1, infile);
    int N_print = N;      //only for debugging
    n = 30;     //could be set to a different value
    fread(&nsamples, sizeof(uint32_t), 1, infile);
    l = 15;      //could be set at nsamples/KEY_SIZE/number_of_rounds
    fread(&sampletype, sizeof(char), 1, infile);
    char st = sampletype;
    uint8_t plaintextlen_temp;
    fread(&plaintextlen_temp, sizeof(uint8_t), 1, infile);
    plaintextlen = (int) plaintextlen_temp;
    int ptl = plaintextlen;     //only for debugging
    
    M = N;      //could be changed
    threshold = 0.9;    //could be changed
    int max_threads = 100;
    
    char** guesses;
    
    switch (sampletype){
        case 'f':
            guesses = calculate_collisions_float(infile);
          break;

        case 'd':
            guesses = calculate_collisions_double(infile);
          break;

        default:
            exit(-1);
    }
    
    print_guesses(guesses);
    
    return (EXIT_SUCCESS);
}

/*
 * Prints all the possible keys according to the guesses obtained from traces
 */
void print_guesses(char** guesses){
    char* current_key = "";
    recursive_print(guesses, 0, guesses);
}

/*
 * Recursive functions that create all the possible key starting from guesses
 */
void recursive_print(char** current_key, int index, char** guesses){
    if(index==KEY_SIZE){
        for(int i=0; i<KEY_SIZE; i++){
            if(current_key[i] == NULL){
                printf("NULL ");
            }
            else{
                printf("%x ", *(current_key[i]));
            }
        }
        printf("\n");
        return;
    }
    else{
        char* next_key[KEY_SIZE];
        for(int i=0; i<index; i++){
            if(current_key[i]==NULL){
                next_key[i] = NULL;
            }
            else{
                next_key[i] = malloc(sizeof(char));
                *(next_key[i]) = *(current_key[i]);
            }
        }
        if(guesses[index]==NULL){
            next_key[index] = NULL;
            recursive_print(next_key, index+1, guesses);
            return;
        }
        else{
            next_key[index] = malloc(sizeof(char));
            *(next_key[index]) = *(guesses[index]);
            recursive_print(next_key, index+1, guesses);
        
            *(next_key[index]) = (*(guesses[index]))+1;
            recursive_print(next_key, index+1, guesses);
            return;
        }
    }
}