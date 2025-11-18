#include <stdio.h>
#include "bus.h"
#include "utils.h"
#include "teenyat.h"

void derby_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    return;
}

void derby_bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
	DerbyState *state = (DerbyState *)t->ex_data;
    switch(addr) {
        case DERBY_MOVE_UP_ADDR:
            state->move_cmd = 1;
            break;
        case DERBY_MOVE_DOWN_ADDR:
            state->move_cmd = 2;
            break;
        case DERBY_MOVE_LEFT_ADDR:
            state->move_cmd = 3;
            break;
        case DERBY_MOVE_RIGHT_ADDR:
            state->move_cmd = 4;
            break;
        default: 
            state->move_cmd = 0;
            break;
    }

    return;
}