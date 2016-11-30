#ifndef switches_included
#define switches_included

#include "msp430.h"
#include <shape.h>

unsigned int p2sw_read();
void p2sw_init(unsigned char mask);

extern Vec2 paddleV;

#endif // included
