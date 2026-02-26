#ifndef __FLASH_NAV_H
#define __FLASH_NAV_H
#include "include.h"

#define NAV_MAX_MISSION_LEGS	50

typedef struct {
    double targetLat;
    double targetLon;
    float targetAlt;
    float targetRadius;
    float loiterTime;
    float maxHorizSpeed;
    float maxVertSpeed;
    float poiHeading;
    float poiAltitude;
    u8 relativeAlt;
    u8 type;
} navMission_t;

typedef struct {
    navMission_t missionLegs[NAV_MAX_MISSION_LEGS];
    navMission_t homeLeg;
    u8 missionLeg, Leg_num;
} navStruct_t;

extern navStruct_t navData;

#endif
