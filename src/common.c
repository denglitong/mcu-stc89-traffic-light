//
// Created by Litong Deng on 2022/10/4.
//

#include "common.h"

#include <8051.h>

void enable_u3_74hc138() {
  // 74HC138 芯片的使能引脚，G1 高电平 G2 低电平 才能启动74HC138的 3-8 译码电路
  ADDR_3 = 1;  // G1 高电平
  EN_LED = 0;  // G2低电平（G2A, G2B）
}

/**
 * Config Timer0 interval milliseconds
 * @param interval_ms Timer interval milliseconds, valid number: 1 ~ 71
 */
void config_t0(unsigned int interval_ms) {
  unsigned long total_time_beats = (11059200 / 12 / 1000) * interval_ms;
  unsigned long init_time_beats = 65536 - total_time_beats;

  EA = 1;        // enable global interrupt
  ET0 = 1;       // enable Timer0 interrupt
  TMOD &= 0xF0;  // 清除 T0 的控制位
  TMOD |= 0x01;  // set Timer0 mode TH0-TL0 16 bits timer
  // setup TH0 TL0 initial value
  TH0 = (unsigned char)(init_time_beats >> 8);
  TL0 = (unsigned char)(init_time_beats);
  TR0 = 1;  // start Timer0
}

/**
 * Config Timer1 interval milliseconds
 * @param interval_ms Timer interval milliseconds, valid number: 1 ~ 71
 */
void config_t1(unsigned int interval_ms) {
  unsigned long total_time_beats = (11059200 / 12 / 1000) * interval_ms;
  unsigned long init_time_beats = 65536 - total_time_beats;

  EA = 1;        // enable global interrupt
  ET1 = 1;       // enable Timer0 interrupt
  TMOD &= 0x0F;  // 清除 T1 的控制位
  TMOD |= 0x10;  // set Timer1 mode TH1-TL1 16 bits timer
  // setup TH1 TL1 initial value
  TH1 = (unsigned char)(init_time_beats >> 8);
  TL1 = (unsigned char)(init_time_beats);
  TR1 = 1;  // start Timer1
}

/**
 * 时钟晶振频率：11059200 (11.0592M)
 * 1 机器周期 = 12 时钟周期
 * 1ms 所需机器周期 = 11059200 * 0.001 / 12 = 921
 * 需要让计数器完成 921 次机器周期，以取得 1ms 的系统时钟
 * TH0 TL0 16bits 计数范围 2^16 = 65536
 * 设置 TH0 TL0 初始位位 65536-921=0xFC67，
 * 则当 Timer0 溢出时即完成了一次 0xFC67~65536 的机器周期循环，即是 1ms
 * 所以 TH0 TL0 初始位设置为 TH0=0xFC TL0=0x67
 * 判断 Timer0 是否溢出使用 TF0 寄存器位
 * @param ms
 */
void delay_ms_t0(unsigned int ms) {
  EA = 1;   // enable global interrupt
  ET0 = 1;  // enable Timer0 interrupt

  // setup T0_M1 = 0, T0_M0 = 1 (Timer0 mode TH0-TL0 16 bits timer)
  TMOD &= 0xF0;  // 清除 T0 的控制位
  TMOD |= 0x01;

  // setup TH0 TL0 initial value
  TH0 = 0xFC;
  TL0 = 0x67;

  // start Timer0
  TR0 = 1;

  // delay ms milliseconds
  while (ms--) {
    while (1) {
      // if Timer0 overflow (has passed 1ms)
      if (TF0) {
        // 软件复位
        TF0 = 0;
        TH0 = 0xFC;
        TL0 = 0x67;
        break;
      }
    }
  }

  // stop/disable Timer0
  TR0 = 0;
}

void delay_ms_t1(unsigned int ms) {
  EA = 1;   // enable global interrupt
  ET1 = 1;  // enable Timer1 interrupt

  // setup T1_M1 = 0, T1_M0 = 1 (Timer1 mode TH1-TL1 16 bits timer)
  TMOD &= 0x0F;  // 清除 T1 的控制位
  TMOD |= 0x10;

  // setup TH0 TL0 initial value
  TH1 = 0xFC;
  TL1 = 0x67;

  // start Timer1
  TR1 = 1;

  // delay ms milliseconds
  while (ms--) {
    while (1) {
      // if Timer0 overflow (has passed 1ms)
      if (TF1) {
        // 软件复位
        TF1 = 0;
        TH1 = 0xFC;
        TL1 = 0x67;
        break;
      }
    }
  }

  // stop/disable Timer1
  TR1 = 0;
}

/**
 * Run some fun for total_millis milliseconds, the fun will be called only once
 * in the beginning of each single millisecond .
 * @param total_millis milliseconds to run
 * @param fun the fun to call within these total_millis milliseconds
 */
void run_in_every_ms_t1(unsigned int total_millis, VoidFun *fun) {
  EA = 1;   // enable global interrupt
  ET0 = 1;  // enable Timer0 interrupt

  // setup T1_M1 = 0, T1_M0 = 1 (Timer1 mode TH1-TL1 16 bits timer)
  TMOD &= 0x0F;  // 清除 T1 的控制位
  TMOD |= 0x10;

  // setup TH0 TL0 initial value
  TH1 = 0xFC;
  TL1 = 0x67;

  // start Timer0
  TR1 = 1;

  // delay total_millis milliseconds
  while (total_millis--) {
    // call fun()
    fun();
    while (1) {
      // if Timer0 overflow (has passed 1ms), will also trigger a Timer0
      // interrupt
      if (TF1) {
        // 软件复位
        TF1 = 0;
        TH1 = 0xFC;
        TL1 = 0x67;
        break;
      }
    }
  }

  // stop/disable Timer0
  TR1 = 0;
}
