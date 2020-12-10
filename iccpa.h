/* 
 * File:   iccpa.h
 * Author: Emanuele Pisano
 *
 */

#ifndef ICCPA_H
#define ICCPA_H

#include <stdio.h>
#include <stdint.h>


int N;        //number of power traces captured from a device processing N encryptions of the same message
int n;     //number of encryption of the same message
int nsamples;   //total number of samples per power trace
int l;        //number of samples per instruction processing (clock)
char sampletype;    //type of samples, f=float  d=double
int plaintextlen;   //length of plaintext in bytes
int M;        //number of plaintext messages
float threshold;    //threshold for collision determination



uint8_t** calculate_collisions_float(FILE* infile);
uint8_t** calculate_collisions_double(FILE* infile);


#endif

