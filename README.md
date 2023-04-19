########################## Multivarate Quadratic DRBG ##########################

    * MQ_DRBG is based on iteration of a random multivarate quadratic system.

    * Initialize state by hashing seed to get input vector of integers
      - Apply equations from parameter file to random state
        - Get two vectors of values from this
          - Vector y is the new state vector
          - Vector z is the output containing one block of bit to output
      - If not enough bits produced, run again with the state just returned
        - This will make another block of output

    * The seed needs to be as large as the block length. Pad with 0s if it is not

    * My code uses the strongest parameter set. This parameter set has field size 1, so there is only code for 
      multiplication with field size 1.

    * My code is base on the experimental python code in the python directory 
      [https://github.com/zain-95/CSPRBG/blob/master/README.md]. There are mistakes in the
      standard. I have changed my code to make the test vectors work. I explain the problems and the changes I make
      in the README for the python code [https://github.com/zain-95/CSPRBG/blob/master/mqdrbgfunctions.py].


## Building the code

    * Run the line below to build the code:
        ```
        gcc -o run_MQ_DRBG MQ_DRBG.c run_MQ_DRBG.c MQ_Param.c
        ```

## Running the code

    * Run the code with the detail below:
        ```
        usage: MQ_DRBG <nbits> <outfile>
            nbits     = required: number of bits required in output stream.
            outfile   = required: file to which output stream will be written.
        ```
