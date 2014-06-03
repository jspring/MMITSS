/**\file
 *
 *      atsc.h
 *
 *      Database variable definitions, constants and macro definitions
 *      for information coming from Actuated Traffic Signal Controllers.
 *      The data format is modeled on NTCIP, but is meant to be used also
 *      to fill in whatever information is available from a 170 using a
 *      custom modification, from AB3418 on either a 170 or 2070, or from
 *      a sniffer installation.
 *
 * Copyright (c) 2005   Regents of the University of California
 *
 */

#ifndef ATSC_H
#define ATSC_H
 
#include "timestamp.h"

#undef DEBUG_SET // define for low-level debugging

/** Each "group" referred to in the following definitions is a set of 8,
 *  within the atsc_typ each of the bits in an unsigned char will correspond
 *  to one of the group. Configuration files for each intersection will
 *  specify the correspondence between the groups and the approach or
 *  the detector location.
 */
#define MAX_VEHICLE_DETECTOR_GROUPS     8
#define MAX_PHASE_STATUS_GROUPS 	4

// assume for now no intersection will have a phase number higher than 16
// most intersections have only 8
#define MAX_PHASES			16

/** These values are for informations source field, since the code that
 *  interprets this information may make different timing assumptions, or
 *  different assumptions about the completeness of the information,
 *  depending on the source of the information
 */
#define ATSC_SOURCE_NTCIP	1
#define ATSC_SOURCE_AB3418	2
#define ATSC_SOURCE_SNIFF	3 
#define ATSC_SOURCE_EDI		4 

/** In the following structure, phase_status_reds, yellow and greens are
 *  set in a straightforward way to one if the phase is that color, 0
 *  otherwise; phase_status_ons corresponds to phases that are either
 *  yellow or green, phase_status_next are phases that will be on at
 *  the next phase change. (At the sniffer meeting Lindy said they could
 *  send us this information, may be helpful in more refined applciations
 *  later, and we may decide to include it in the intersection message set.)
 */  
typedef struct {
        timestamp_t ts; //hour:min:sec.millisec of update
        unsigned char vehicle_detector_status[MAX_VEHICLE_DETECTOR_GROUPS];
        unsigned char phase_status_reds[MAX_PHASE_STATUS_GROUPS];
        unsigned char phase_status_yellows[MAX_PHASE_STATUS_GROUPS];
        unsigned char phase_status_greens[MAX_PHASE_STATUS_GROUPS];
        unsigned char phase_status_flashing[MAX_PHASE_STATUS_GROUPS];
  	unsigned char phase_status_ons[MAX_PHASE_STATUS_GROUPS];
        unsigned char phase_status_next[MAX_PHASE_STATUS_GROUPS];
	int info_source;	// indicate whether NTCIP, AB348, sniffer, etc. 
} atsc_typ;

#define ATSC_ERROR	-1	// for readability in calling code

/** In each of the above cases, we will adopt the same convention as NTCIP
 *  when interpreting the bits, that is, phases are numbered from one to
 *  MAX_PHASE_STATUS_GROUPS * 8, and phase j corresponds to bit (j-1) mod 8
 *  within element (j-1) div 8 of the group. The following in-line functions
 *  can be used to check whether any of these are set. It returns 1 if the
 *  bit is set (status is true), 0 if it is not set (status is false), and
 *  -1 if the phase number is out of bounds. 
 */

static inline int atsc_is_set(unsigned char *status, int num, int max)
{
	int index = (num - 1)/8;
	int bit_position = (num - 1) % 8;
	int max_bits = 8*max;
	if (num < 1 || num > max_bits)
		return (-1);
	if (status[index] & (1 << bit_position))
		return 1;
	return 0;
}

static inline int atsc_phase_is_set(unsigned char *status, int phase_num)
{
	return atsc_is_set(status, phase_num, MAX_PHASE_STATUS_GROUPS);
}

static inline int atsc_detector_is_set(unsigned char *status, int detector_num)
{
	return atsc_is_set(status, detector_num, MAX_VEHICLE_DETECTOR_GROUPS);
}

// Function that sets the bit in the status array corresponding to num
static inline int set_atsc(unsigned char *status, int num, int max)
{
	int index = (num - 1)/8;
	int bit_position = (num - 1) % 8;
	int max_bits = 8*max;
	if (num < 1 || num > max_bits)
		return (-1);	//ATSC_ERROR; never a legal status
	status[index] = status[index] | (1 << bit_position);
#ifdef DEBUG_SET
	printf("set_atsc: index %d bit position %d status 0x%2hhx\n", 
		index, bit_position, status[index]);
#endif
	return (status[index]);	// informational only
}

static inline int set_atsc_detector(unsigned char *status, int detector_num)
{
	return set_atsc(status, detector_num, MAX_VEHICLE_DETECTOR_GROUPS);
}
 
static inline int set_atsc_phase(unsigned char *status, int phase_num)
{
#ifdef DEBUG_SET
	printf("set phase %d\n", phase_num);
#endif
	return set_atsc(status, phase_num, MAX_PHASE_STATUS_GROUPS);
}

// Function that clears the bit in the status array corresponding to num
static inline int clear_atsc(unsigned char *status, int num, int max)
{
	int index = (num - 1)/8;
	int bit_position = (num - 1) % 8;
	int max_bits = 8*max;
	if (num < 1 || num > max_bits)
		return (-1);	//ATSC_ERROR; never a legal status
	status[index] = status[index] & ~(1 << bit_position);
	return (status[index]);	// informational only
}

static inline int clear_atsc_detector(unsigned char *status, int detector_num)
{
	return clear_atsc(status, detector_num, MAX_VEHICLE_DETECTOR_GROUPS);
}
 
static inline int clear_atsc_phase(unsigned char *status, int phase_num)
{
	return clear_atsc(status, phase_num, MAX_PHASE_STATUS_GROUPS);
}

#endif
