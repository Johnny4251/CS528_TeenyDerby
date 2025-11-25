#ifndef __BUS_H_
#define __BUS_H_

#include "teenyat.h"

// WRITE
#define DERBY_THROTTLE_ADDR       0x9000
#define DERBY_DIR_BASE_ADDR       0x9010
#define DERBY_DIR_MAX_ADDR        (DERBY_DIR_BASE_ADDR + 7)

// WRITE/READ
#define DERBY_SENSOR_TARGET_ADDR  0x9020

// READ 
#define DERBY_SENSOR_REL_X_ADDR   0x9021
#define DERBY_SENSOR_REL_Y_ADDR   0x9022
#define DERBY_SENSOR_X_ADDR       0x9023
#define DERBY_SENSOR_Y_ADDR       0x9024
#define DERBY_SENSOR_SPEED_ADDR   0x9025
#define DERBY_SENSOR_DIR_ADDR     0x9026
#define DERBY_SENSOR_HEALTH_ADDR  0x9027
#define DERBY_SELF_ID_ADDR        0x9030
#define DERBY_SELF_SPEED_ADDR     0x9031
#define DERBY_SELF_DIR_ADDR       0x9032
#define DERBY_SELF_HEALTH_ADDR    0x9033
#define DERBY_SELF_X_ADDR         0x9034
#define DERBY_SELF_Y_ADDR         0x9035

void derby_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void derby_bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);

#endif
