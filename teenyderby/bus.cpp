#include <stdio.h>
#include "bus.h"
#include "utils.h"
#include "teenyat.h"

void derby_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
    if (delay) *delay = 0;
    if (!t || !data) return;

    DerbyState *self = (DerbyState *)t->ex_data;
    data->s = 0;

    if (!self || !g_derby_state || !g_cars) return;

    int self_index = self->id;
    if (self_index < 0 || (size_t)self_index >= g_derby_state_count)
        return;

    auto get_target_index = [&]() -> int {
        uint8_t id = self->sensor_target;
        return id;
    };

    switch (addr) {

    case DERBY_SENSOR_TARGET_ADDR:
        data->s = self->sensor_target;
        break;

    case DERBY_SENSOR_REL_X_ADDR: {
        int ti = get_target_index();
        if (ti >= 0)
            data->s = (*g_cars)[ti].x - (*g_cars)[self_index].x;
        break;
    }

    case DERBY_SENSOR_REL_Y_ADDR: {
        int ti = get_target_index();
        if (ti >= 0)
            data->s = (*g_cars)[ti].y - (*g_cars)[self_index].y;
        break;
    }

    case DERBY_SENSOR_X_ADDR: {
        int ti = get_target_index();
        if (ti >= 0) data->s = (*g_cars)[ti].x;
        break;
    }

    case DERBY_SENSOR_Y_ADDR: {
        int ti = get_target_index();
        if (ti >= 0) data->s = (*g_cars)[ti].y;
        break;
    }

    case DERBY_SENSOR_SPEED_ADDR: {
        int ti = get_target_index();
        if (ti >= 0) data->s = g_derby_state[ti].speed;
        break;
    }

    case DERBY_SENSOR_DIR_ADDR: {
        int ti = get_target_index();
        if (ti >= 0) data->s = g_derby_state[ti].direction & 7;
        break;
    }

    case DERBY_SENSOR_HEALTH_ADDR: {
        int ti = get_target_index();
        if (ti >= 0) data->s = g_derby_state[ti].health;
        break;
    }

    case DERBY_SELF_ID_ADDR:
        data->s = self->id;
        break;

    case DERBY_SELF_SPEED_ADDR:
        data->s = self->speed;
        break;

    case DERBY_SELF_DIR_ADDR:
        data->s = self->direction;
        break;

    case DERBY_SELF_HEALTH_ADDR:
        data->s = self->health;
        break;

    case DERBY_SELF_X_ADDR:
        data->s = (*g_cars)[self_index].x;
        break;

    case DERBY_SELF_Y_ADDR:
        data->s = (*g_cars)[self_index].y;
        break;
    default:
        break;
    }
}

void derby_bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
    if (delay) *delay = 0;
    DerbyState *state = (DerbyState *)t->ex_data;
    if (!state) return;

    switch (addr) {

    case DERBY_THROTTLE_ADDR:
        if (data.s < -100) data.s = -100;
        if (data.s >  100) data.s = 100;
        state->throttle = data.s;
        break;

    case DERBY_SENSOR_TARGET_ADDR: {
        if (g_derby_state_count == 0) {
            state->sensor_target = 0;
            break;
        }

        int raw = data.u;                       
        uint8_t wrapped = raw % g_derby_state_count;

        state->sensor_target = wrapped;
        break;
    }

    default:
        if (addr >= DERBY_DIR_BASE_ADDR && addr <= DERBY_DIR_MAX_ADDR) {
            state->direction = addr - DERBY_DIR_BASE_ADDR;
        }
        break;

    }
}
