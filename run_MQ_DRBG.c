// To build:
// gcc -o run_MQ_DRBG MQ_DRBG.c run_MQ_DRBG.c MQ_Param.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQ_DRBG.h"
#include "MQ_Param.h"

#define CEILING(x,y) (((x) / (y)) + ((x) % (y) ? 1 : 0))

int write_drbg_output(unsigned char *drbg_output, char *drbg_outfile, int nbits);

// Reads command line arguments for nbits and outfile and run MQ_DRBG. 
// Writing the requested number of output bit to the file provided
int main(int argc, char **argv) {
    int requested_no_of_bytes = 0,requested_no_of_bits = 0, nbits_produced = 0, nbits_written = 0;
    unsigned char *output_bits=NULL;
    
    mq_drbg_t *mq_drbg_handle = NULL;

    char *drbg_outfile;
    
    if(3 != argc) {
        fprintf(stderr, "usage: MQ_DRBG <nbytes> <outfile>.\n");
        fprintf(stderr, "   nbytes    = required: number of bytes required in output stream.\n");
        fprintf(stderr, "   outfile   = required: file to which output stream will be written.\n");
        exit(FAILURE);
    } else {
        requested_no_of_bytes = atoi(argv[1]);
        requested_no_of_bits = 8 * requested_no_of_bytes;
        drbg_outfile = argv[2];
    }

    mq_drbg_handle = Instantiate_MQ_DRBG(STATE_LENGTH_BITS, BLOCK_LENGTH_BITS, SYSTEM_LENGTH, STRENGTH_BITS,
                                         P_BL_256_Sec_256_F2);
    if(NULL == mq_drbg_handle) {
        printf("ERROR: Fail to instantiate MQ_DRBG. \n");
        return FAILURE;
    }
    output_bits=malloc(requested_no_of_bits);
    if(NULL == output_bits) {
        printf("ERROR: Fail to instantiate MQ_DRBG output buffer.\n");
        return FAILURE;
    }
    memset(output_bits, 0, requested_no_of_bits);

    printf("Running MQ_DRBG to make %d bytes...\n", requested_no_of_bytes);

    nbits_produced = MQ_DRBG(mq_drbg_handle, requested_no_of_bits, output_bits);

    if(nbits_produced != requested_no_of_bits) {
        fprintf(stderr, "ERROR: nbits_produced (%d) does not match requested_no_of_bits (%d)\n", nbits_produced, requested_no_of_bits);
        exit(FAILURE);
    }
    printf("Writing %d bits to %s...\n", nbits_produced, drbg_outfile);
    nbits_written = write_drbg_output(output_bits, drbg_outfile, nbits_produced);

    if(nbits_written != requested_no_of_bits) {
        fprintf(stderr, "ERROR: Fail to write to file, nbits_written to file (%d) does not match requested_no_of_bits (%d)\n", nbits_written, requested_no_of_bits);
        exit(FAILURE);
    }
    
    return SUCCESS;
}

// Writes out the requested number of bits produced by mq_drbg
int write_drbg_output(unsigned char *drbg_output, char *drbg_outfile, int nbits) {
    FILE *OUT_handle = NULL;
    int nbits_written = 0, i = 0;
    int nbytes_output = nbits/8;
    
    if(NULL == (OUT_handle = fopen(drbg_outfile, "w"))) {
        fprintf(stderr, "ERROR: Couldnt open %s\n", drbg_outfile);
        return FAILURE;
    }
    
    // loop over the bytes of output
    for(i = 0; i < nbytes_output; i++) {
        fprintf(OUT_handle, "%02x", drbg_output[i]);
        nbits_written += 8;

        if(0 == (nbits_written%256)) {
            fprintf(OUT_handle, "\n");
        }
    }

    if(0 != nbits_written%256) {
        fprintf(OUT_handle, "\n");
    }
    
    fclose(OUT_handle);

    return nbits_written;
}
