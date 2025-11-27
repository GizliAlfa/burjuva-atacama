#include "stdint.h"

#ifndef MAIN_H
#define MAIN_H

void pilot_systick();

typedef struct {
    long int tv_sec;            /* Seconds.  */
    long int tv_nsec;           /* Nanoseconds.  */
} /* __attribute__((packed)) */ IEC_TIMESPEC;  /* packed is gcc specific! */

/* cycle time */
typedef struct {
uint64_t cycle_start;   // start of cycle. comm/read/program/write are realtive to this number
uint64_t cycle_current; // time for the full cycle
uint64_t cycle_current_comm; //time until comm done
uint64_t cycle_current_read; //time until read
uint64_t cycle_current_program; //time for program
uint64_t cycle_current_write; //time until write
uint64_t cycle_min;
uint64_t cycle_max;
} cycletimes_t;

#endif