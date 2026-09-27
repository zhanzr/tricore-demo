/*
  blockwrite.h - ST7735S address-window helper (blockwrite_default.h port).
*/

#ifndef __BLOCKWRITE_H
#define __BLOCKWRITE_H

#include <stdint.h>

void BlockWrite(uint16_t Xstart, uint16_t Xend, uint16_t Ystart, uint16_t Yend);

#endif /* __BLOCKWRITE_H */