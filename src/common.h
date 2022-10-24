//
// Created by Litong Deng on 2022/10/4.
//

#ifndef CMC_STC89_COMMON_H
#define CMC_STC89_COMMON_H

#include <8051.h>

#define ADDR_0 P1_0
#define ADDR_1 P1_1
#define ADDR_2 P1_2
#define ADDR_3 P1_3
#define EN_LED P1_4

#define LED_SIZE 8
#define LED_LINE P0
#define LED_SINGLE P0_0

#define LED_0 P0_0
#define LED_1 P0_1
#define LED_2 P0_2
#define LED_3 P0_3

#define KEY_IN_4 P2_7
#define KEY_IN_3 P2_6
#define KEY_IN_2 P2_5
#define KEY_IN_1 P2_4

#define KEY_OUT_1 P2_3
#define KEY_OUT_2 P2_2
#define KEY_OUT_3 P2_1
#define KEY_OUT_4 P2_0

typedef void(VoidFun)();

void enable_u3_74hc138();

void config_t0(unsigned int interval_ms);

void config_t1(unsigned int interval_ms);

void delay_ms_t0(unsigned int ms);

void delay_ms_t1(unsigned int ms);

void run_in_every_ms_t1(unsigned int total_millis, VoidFun *fun);

#endif  // CMC_STC89_COMMON_H
