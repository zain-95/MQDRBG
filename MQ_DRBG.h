#ifndef _MQ_DRBG_H
#define _MQ_DRBG_H

#include "MQ_Param.h"

#define SUCCESS (0)
#define FAILURE (-1)

typedef struct mq_drbg_struct mq_drbg_t;

// mq_drbg Function Headers

// Given state length, block length, system length, requested strength and
// P (Coefficients from include file contains MQ equations), returns a pointer to 
// initialized mq_drbg_struct.
mq_drbg_t *Instantiate_MQ_DRBG(
    int state_length, 
    int block_length, 
    int system_length,
    int strength_bits,
    unsigned char *P);

// Given a mq_drbg_handle, a requested number of bits and a empty array to hold output bits, 
// generate the request number of output bits and write them to the provided array.
int MQ_DRBG(mq_drbg_t *mq_drbg_handle, int requested_no_of_bits, unsigned char *output_bits);

// Given a mq_drbg_handle which contains a state x, forward and output MQ equations in P,
// update input with updated internal state and the output state.
int Evaluate_MQ(mq_drbg_t *mq_drbg_handle);

#endif
