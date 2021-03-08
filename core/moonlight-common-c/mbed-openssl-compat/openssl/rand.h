#pragma once

int mbed_RAND_bytes(unsigned char *buf, int num);

#define RAND_bytes(buf, num) mbed_RAND_bytes(buf, num)