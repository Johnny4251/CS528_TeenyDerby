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
        case DERBY_FACE_ADDR:{
            int value = data.u % 8;
            switch (value){
                case 0: // north
                    state->move_cmd = 5;
                    break;
                case 1: // northeast
                    state->move_cmd = 6;
                    break;
                case 2: // east
                    state->move_cmd = 7;
                    break;
                case 3: // southeast
                    state->move_cmd = 8;
                    break;
                case 4: // south
                    state->move_cmd = 9;
                    break;
                case 5: // southwest
                    state->move_cmd = 10;
                    break;
                case 6: // west
                    state->move_cmd = 11;
                    break;
                case 7: // northwest
                    state->move_cmd = 12;
                    break;
                default:
                    break;
                }
            break;
            }
        default: 
            state->move_cmd = 0;
            break;
    }

    return;
}