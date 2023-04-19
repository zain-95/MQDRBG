#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQ_DRBG.h"
#include "MQ_Param.h"

#define CEILING(x,y) (((x) / (y)) + ((x) % (y) ? 1 : 0))

// bitstring struct
typedef struct bitstr_struct {
    unsigned char *bits;
    int nbits;
} bitstr_t;

// state bits
typedef bitstr_t mq_state_t;

// output bits
typedef bitstr_t mq_block_t;

// coefficient of MQ equations
typedef bitstr_t mq_coeffs_t;

// list of parameters (sizes, strength, MQ coefficients)
typedef struct mq_param_struct {
    int system_length;
    int state_length;
    int block_length;
    int strength;
    unsigned char *P_vec;
} mq_param_t;

// instantiation of mq_drbg
struct mq_drbg_struct {
    mq_state_t *state;
    mq_block_t *block;
    mq_param_t *param;
};

// function declaration
static bitstr_t *init_bitstr(int nbits);
static void field_vector(bitstr_t *bstr, unsigned char *bvec);
static void flatten(unsigned char *bvec, bitstr_t *bstr);
static void print_bitstr(bitstr_t *bitstr);
static void reverse_vec(unsigned char *bvec, int nbits);

// Given a mq_drbg_handle, a requested number of bits and a empty array to hold output bits, 
// generate the request number of output bits and write them to the provided array.
int MQ_DRBG(mq_drbg_t *mq_drbg_handle, int requested_no_of_bits, unsigned char *output_bits) {
    int nbits_produced = 0;
    int i = 0;

    printf("MQ_DRBG: initial state/block:\n");
    print_bitstr(mq_drbg_handle->state);
    printf("\n");
    print_bitstr(mq_drbg_handle->block);
    printf("\n");

    if(NULL == mq_drbg_handle) { // Error and exit if no state is available
        fprintf(stderr,"ERROR: MQ_DRBG: mq_drbg_handle provided was NULL.\n");
        return FAILURE;
    }
    if(NULL == output_bits) { 
        fprintf(stderr,"ERROR: MQ_DRBG: output_bits provided was NULL.\n");
        return FAILURE;
    }

    while(nbits_produced < requested_no_of_bits) {  // loop to generate requested bits
        int nbytes_produced = CEILING(nbits_produced, 8);
        
        Evaluate_MQ(mq_drbg_handle); // apply MQ equations to obtain output block and update state 

        printf("MQ_DRBG:    current state/block:\n");
        print_bitstr(mq_drbg_handle->state);
        printf("\n");
        print_bitstr(mq_drbg_handle->block);
        printf("\n");
        
        // copy block bits to pseudorandom output and update nbits_produced.
        for(i = 0; (i < mq_drbg_handle->param->block_length / 8) && (nbits_produced < requested_no_of_bits); i++) {
            if(requested_no_of_bits-nbits_produced >= 8) {
                output_bits[nbytes_produced + i] = mq_drbg_handle->block->bits[i];
                nbits_produced += 8;
            } else {
                output_bits[nbytes_produced + i] = mq_drbg_handle->block->bits[i] & (0xff << (8 - (requested_no_of_bits-nbits_produced)));
                nbits_produced = requested_no_of_bits;
            }
        }
    }

    return nbits_produced;
}

// Given a mq_drbg_handle which contains a state x, forward and output MQ equations in P,
// update input with updated internal state and the output state.
int Evaluate_MQ(mq_drbg_t *mq_drbg_handle) {
    unsigned char *x_vec = NULL;  // input state
    unsigned char *y_vec = NULL;  // updated state
    unsigned char *z_vec = NULL;  // output block
    int n = (mq_drbg_handle->param->state_length);
    int m = (mq_drbg_handle->param->block_length);
    int t = 0, i = 0, j = 0, k = 0;

    // initialize temp vector x_vec
    if(NULL == (x_vec = (unsigned char *)malloc(mq_drbg_handle->state->nbits))) {
        fprintf(stderr,"ERROR: Evaluate_MQ: couldnt allocate x_vec memory.\n");
        return FAILURE;
    }
    // initialize temp vector y_vec
    if(NULL == (y_vec = (unsigned char *)malloc(n))) {
        fprintf(stderr,"ERROR: Evaluate_MQ: couldnt allocate y_vec memory.\n");
        return FAILURE;
    }
    // initialize temp vector z_vec
    if(NULL == (z_vec = (unsigned char *)malloc(m))) {
        fprintf(stderr,"ERROR: Evaluate_MQ: couldnt allocate z_vec memory.\n");
        return FAILURE;
    }


    for(i = 0; i < n; i++) {
        y_vec[i] = 0;
    }
    for(i = 0; i < m; i++) {
        z_vec[i] = 0;
    }

    // convert bitstrings to vectors of field elements
    field_vector(mq_drbg_handle->state, x_vec);
    
    // Reverse input. Coefficient files expect least-significant bits first, but test vectors have
    // least-significant bit last; so reverse here, then reverse output after computations.
    reverse_vec(x_vec, n);

    for(i = 0; i < n; i++) { // compute new state using MQ equations
        for(j = 0; j < n; j++) { // nonlinear terms a_jk * x_j * x_k
            for(k = j+1; k < n; k++) { // start at k = j+1 and do the linear terms below (x_j*x_j = x_j in field size 1)
                // For field size 1 multiplication is &
                y_vec[i] = y_vec[i] ^ (mq_drbg_handle->param->P_vec[t] & x_vec[j] & x_vec[k]);
                t++;
            }
        }
        for(j = 0; j < n; j++) { // linear terms b_j * x_j
            y_vec[i] = y_vec[i] ^ (mq_drbg_handle->param->P_vec[t] & x_vec[j]);
            t++;
        }
        y_vec[i] = y_vec[i] ^ mq_drbg_handle->param->P_vec[t]; // constant term c
        t++;
    }
    for(i = 0; i < m; i++) { // compute new block using MQ equations
        for(j = 0; j < n; j++) { // nonlinear terms a_jk * x_j * x_k
            for(k = j+1; k < n; k++) { // start at k = j+1 and do the linear terms below (x_j*x_j = x_j in field size 1)
                z_vec[i] = z_vec[i] ^ (mq_drbg_handle->param->P_vec[t] & x_vec[j] & x_vec[k]);
                t++;
            }
        }
        for(j = 0; j < n; j++) { // linear terms b_j * x_j
            z_vec[i] = z_vec[i] ^ (mq_drbg_handle->param->P_vec[t] & x_vec[j]);
            t++;
        }   
        z_vec[i] = z_vec[i] ^ mq_drbg_handle->param->P_vec[t]; // constant term c
        t++;
    }
    
    // reverse output
    // coefficient files expect output to have least-significant bits *first*, but test vectors
    // have least-significant *last*; so reverse output here, as mention above
    reverse_vec(y_vec, n);
    
    // convert y_vec back to bitstr
    flatten(y_vec, mq_drbg_handle->state);

    // reverse output
    // coefficient files expect output to have least-significant bits *first*, but test vectors
    // have least-significant *last*; so reverse output here, as mention above
    reverse_vec(z_vec, m);

    // convert z_vec back to bitstr
    flatten(z_vec, mq_drbg_handle->block);
    
    free(x_vec);
    free(y_vec);
    free(z_vec);
    
    return SUCCESS;
}

// Given state length, block length, system length, requested strength and P (Coefficients from 
// include file contains MQ equations) returns a pointer to initialized mq_drbg_struct.
mq_drbg_t *Instantiate_MQ_DRBG(int state_length, int block_length, int system_length, int strength_bits,
                               unsigned char *P) {
    mq_drbg_t *mq_handle = NULL;
    mq_coeffs_t Pcoeffs;
    int i = 0;

    // initialize entire struct
    if(NULL == (mq_handle = (mq_drbg_t *)malloc(sizeof(mq_drbg_t)))) {
        fprintf(stderr,"ERROR: Instantiate_MQ_DRBG: couldnt allocate mq_handle memory.\n");
        return NULL;    
    }
    
    // initialize state
    if(NULL == (mq_handle->state = (mq_state_t *)init_bitstr(state_length))) {
        fprintf(stderr,"ERROR: Instantiate_MQ_DRBG: couldnt allocate state memory.\n");
        return NULL;    
    }
    for(i = 0; i < mq_handle->state->nbits / 8; i++) {
        mq_handle->state->bits[i] = 0;
    }
    mq_handle->state->bits[i - 1] = INIT_STATE;

    // initialize block
    if(NULL == (mq_handle->block = (mq_block_t *)init_bitstr(block_length))) {
        fprintf(stderr,"ERROR: Instantiate_MQ_DRBG: couldnt allocate block memory.\n");
        return NULL;    
    }
    for(i = 0; i < mq_handle->block->nbits / 8; i++) {
        mq_handle->block->bits[i] = 0;
    }
    
    // initialize param
    if(NULL == (mq_handle->param = (mq_param_t *)malloc(sizeof(mq_param_t)))) {
        fprintf(stderr,"ERROR: Instantiate_MQ_DRBG: couldnt allocate param memory.\n");
        return NULL;    
    }
    
    mq_handle->param->system_length = SYSTEM_LENGTH;
    mq_handle->param->state_length  = STATE_LENGTH_BITS;
    mq_handle->param->block_length  = BLOCK_LENGTH_BITS;
    mq_handle->param->strength      = STRENGTH_BITS;

    Pcoeffs.bits  = P;
    Pcoeffs.nbits = SYSTEM_LENGTH;
    if(NULL == ( mq_handle->param->P_vec=(unsigned char *)malloc(Pcoeffs.nbits))) {
        fprintf(stderr,"ERROR: Instantiate_MQ_DRBG: couldnt allocate param P_vec memory.\n");
        return NULL;
    }
    field_vector(&Pcoeffs,mq_handle->param->P_vec);
    
    return mq_handle;
}

// Take an integer number of bits, allocates memory to a new bitstring and returns a pointer to it.
bitstr_t *init_bitstr(int nbits) {
    bitstr_t *bitstr = NULL;
    if(NULL == (bitstr = (bitstr_t *)malloc(sizeof(bitstr_t)))) {
        fprintf(stderr,"ERROR: init_bitstr: couldnt allocate bitstr struct memory.\n");
        return NULL;    
    }
    if(NULL == (bitstr->bits = (unsigned char *)malloc(CEILING(nbits, 8)))) {
        fprintf(stderr,"ERROR: init_bitstr: couldnt allocate bitstr bits memory.\n");
        return NULL;    
    }
    bitstr->nbits = nbits;
    
    return bitstr;
}

// Given a pointer to a bitstring (with length a multiple of field_size), 
// returns a pointer to an array of integers, where each integer is the value of next group of a field_size number of bits.
// That is, bvec[0] = bit_array[0]||bit_array[1]||...bit_array[field_size-1]
//      and bvec[1] = bit_array[field_size]||bit_array[field_size+1]||...bit_array[2*field_size-1] ...
void field_vector(bitstr_t *bstr, unsigned char *bvec) {
    int i = 0;

    for(i = 0; i < bstr->nbits; i++) {
        bvec[i] = (bstr->bits[i / 8] >> (7 - (i%8))) & 0x01;
    }
}

// Given a list of integers of field_size bits, returns an array of the concatenated bits of bvec[0]...bvec[-1].
// Returns original bitstring input into function field_vector(bit_array, field_size) when applied to its output.
void flatten(unsigned char *bvec, bitstr_t *bstr) {
    int i = 0;
    unsigned char temp_byte = 0;
    
    for(i = 0; i < bstr->nbits; i++) {
        temp_byte ^= (bvec[i] << (7 - (i%8)));
        if(7 == (i%8)) {
            bstr->bits[i / 8] = temp_byte;
            temp_byte = 0;
        }
    }
}

// Given a pointer to a bitstring, print it
void print_bitstr(bitstr_t *bitstr) {
    int i = 0;
    
    for(i = 0; i < bitstr->nbits / 8; i++) {
        printf("%02x", bitstr->bits[i]);
    }  
}

// Takes a pointer to a bit vector and reverse it in place
void reverse_vec(unsigned char *bvec, int nbits) {
    int i = 0;
    unsigned char temp_byte = 0;
    
    for(i = 0; i < nbits / 2; i++) {
        temp_byte = bvec[i];
        bvec[i] = bvec[nbits - 1 - i];
        bvec[nbits-1-i] = temp_byte;
    }
}
