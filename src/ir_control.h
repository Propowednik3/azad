#ifndef _IR_CONTROL_H_
#define _IR_CONTROL_H_

#include <stdint.h>

char ir_init(unsigned int uiPinNum);
char ir_close(void);
int ir_read(uint16_t *cdata, unsigned int ilen);

#endif
