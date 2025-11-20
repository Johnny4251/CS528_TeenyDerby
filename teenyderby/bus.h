#ifndef __BUS_H_
#define __BUS_H_

#include "teenyat.h"

void derby_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void derby_bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);

#endif