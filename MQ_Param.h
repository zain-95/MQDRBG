#ifndef _MQ_PARAM_H
#define _MQ_PARAM_H

#include "stdint.h"

#define INIT_STATE (0x01)

// 256 requested block length, 256 requested strength
#define STRENGTH_BITS      (256)
#define STATE_LENGTH_BITS  (272)
#define BLOCK_LENGTH_BITS  (256)
#define FIELD_SIZE         (1)
#define SYSTEM_LENGTH      (19604112)

extern unsigned char P_BL_256_Sec_256_F2[];

#endif
