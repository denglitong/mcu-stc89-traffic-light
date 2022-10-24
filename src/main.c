#include "common.h"

#define BUZZ P1_6

const unsigned char LED_RED = 0xff & ~(0x01 << 6 | 0x01 << 7);
const unsigned char LED_YELLOW = 0xff & ~(0x01 << 3 | 0x01 << 4);
const unsigned char LED_GREEN = 0xff & ~(0x01 << 0 | 0x01 << 1);

unsigned char __idata LIGHT_INDEX = 0;
// LED_RED, LED_YELLOW, LED_GREEN
unsigned char SECONDS[] = {15, 10, 5};
short DIGIT = 0;

unsigned char T0_INTERRUPT_FLAG = 0;
// 受精度所限，T0 的 1s 间隔需要拆分为多次来实现
unsigned short T0_INTERVAL_MS = 1;
unsigned short T0_INTERVAL_CNT = 1000;
unsigned short T0_CNT = 0;

unsigned short BUZZ_FLAG = 0;
unsigned short BUZZ_FREQUENCY_CNT = 1000;
unsigned short BUZZ_CNT = 0;

unsigned char T1_INTERRUPT_FLAG = 0;
unsigned short T1_INTERVAL_MS = 1;
unsigned short T1_INTERVAL_CNT = 1000;
unsigned short T1_CNT = 0;

unsigned char LED_CHAR[] = {
    0xff ^ 0b111111,  0xff ^ 0b110,     0xff ^ 0b1011011, 0xff ^ 0b1001111,
    0xff ^ 0b1100110, 0xff ^ 0b1101101, 0xff ^ 0b1111101, 0xff ^ 0b111,
    0xff ^ 0b1111111, 0xff ^ 0b1101111,
};
unsigned char LED_BUFF[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

void turn_on_led_master_switch();
void turn_off_all_leds();
void turn_on_red();
void turn_on_yellow();
void turn_on_green();

void ConfigT0();
void interrupt_timer0() __interrupt(1);

void enable_buzzer();
void switch_buzzer();

void update_led_buffer(unsigned long digit);
void flush_led_buffer();

void ConfigT1();
void interrupt_timer1() __interrupt(3);
void flush_traffic_light();

int main() {
  enable_u3_74hc138();
  enable_buzzer();

  ConfigT0();
  ConfigT1();

  DIGIT = SECONDS[LIGHT_INDEX];

  update_led_buffer(DIGIT);
  flush_led_buffer();

  while (1) {
    // 中断从用户视角可以理解为两个"并行"运行的事件
    // 有中断处理区就得去响应，间隔的两个中断交替响应，在用户看来就是同时"并行"运行的
    if (BUZZ_FLAG) {
      switch_buzzer();
      BUZZ_FLAG = 0;
    }
    if (T0_INTERRUPT_FLAG) {
      update_led_buffer(DIGIT);
      T0_INTERRUPT_FLAG = 0;
    }
    if (T1_INTERRUPT_FLAG) {
      T1_INTERRUPT_FLAG = 0;
    }
  }
}

void turn_on_led_master_switch() {
  // 110 LEDS6 为低电平，三极管导通，LED 总开关打开
  ADDR_2 = 1;
  ADDR_1 = 1;
  ADDR_0 = 0;
}

void turn_off_all_leds() { LED_LINE = 0xff; }

void turn_on_red() { LED_LINE = LED_RED; }

void turn_on_yellow() { LED_LINE = LED_YELLOW; }

void turn_on_green() { LED_LINE = LED_GREEN; }

void ConfigT0() {
  unsigned long total_time_beats = (11059200 / 12 / 1000) * T0_INTERVAL_MS;
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

void ConfigT1() {
  unsigned long total_time_beats = (11059200 / 12 / 1000) * T1_INTERVAL_MS;
  unsigned long init_time_beats = 65536 - total_time_beats;

  TMOD &= 0x0F;  // 清除 T1 的控制位
  TMOD |= 0x10;  // set Timer1 mode TH1-TL1 16 bits timer
  // setup TH1 TL1 initial value
  TH1 = (unsigned char)(init_time_beats >> 8);
  TL1 = (unsigned char)(init_time_beats);
  ET1 = 1;  // enable Timer1 interrupt
  TR1 = 1;  // start Timer1
}

void reload_timer0() {
  // reset Timer0 1ms
  TH0 = 0xFC;
  TL0 = 0x67;
}

void interrupt_timer0() __interrupt(1) {
  reload_timer0();
  T0_CNT++;
  BUZZ_CNT++;

  if (T0_CNT == T0_INTERVAL_CNT) {
    T0_CNT = 0;
    T0_INTERRUPT_FLAG = 1;
    DIGIT--;
    if (DIGIT < 0) {
      LIGHT_INDEX++;
      LIGHT_INDEX %= 3;
      DIGIT = SECONDS[LIGHT_INDEX];
      if (LIGHT_INDEX == 2) {
        BUZZ_FREQUENCY_CNT = 100;
        BUZZ_CNT = 0;
      } else {
        BUZZ_FREQUENCY_CNT = 1000;
      }
    }
  }

  if (BUZZ_CNT == BUZZ_FREQUENCY_CNT) {
    BUZZ_CNT = 0;
    BUZZ_FLAG = 1;
  }

  flush_led_buffer();
}

void enable_buzzer() { BUZZ = 0; }

void switch_buzzer() { BUZZ ^= 0x01; }

void update_led_buffer(unsigned long digit) {
  signed char i = 0;
  unsigned char buf[6];
  for (i = 0; i < 6; ++i) {
    buf[i] = digit % 10;
    digit /= 10;
  }

  for (i = 5; i >= 1; i--) {
    if (buf[i] == 0) {
      LED_BUFF[i] = 0xFF;
    } else {
      break;
    }
  }
  for (; i >= 0; i--) {
    LED_BUFF[i] = LED_CHAR[buf[i]];
  }

  // adjust LEF_BUFF with LIGHT_INDEX position
  if (LIGHT_INDEX == 0) {
    // left shift 4 position
    LED_BUFF[5] = LED_BUFF[1];
    LED_BUFF[4] = LED_BUFF[0];
    LED_BUFF[3] = 0xFF;
    LED_BUFF[2] = 0xFF;
    LED_BUFF[1] = 0xFF;
    LED_BUFF[0] = 0xFF;
  } else if (LIGHT_INDEX == 1) {
    // left shift 2 position
    LED_BUFF[5] = 0xFF;
    LED_BUFF[4] = 0xFF;
    LED_BUFF[3] = LED_BUFF[1];
    LED_BUFF[2] = LED_BUFF[0];
    LED_BUFF[1] = 0xFF;
    LED_BUFF[0] = 0xFF;
  }
}

void turn_off_all_segs() { P0 = 0xff; }

// i: 0 - (TUBE_SIZE-1)
void enable_tube(unsigned char i) {
  // P1_2 P1_1 P1_0
  // TUBE 0 000
  // TUBE 1 001
  // TUBE 2 010
  // TUBE 3 011
  // TUBE 4 100
  // TUBE 5 101
  P1 &= 1 << 3;
  P1 |= i;
}

void flush_led_buffer() {
  turn_off_all_segs();
  static unsigned char TUBE_IDX = 0;
  switch (TUBE_IDX) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      enable_tube(TUBE_IDX);
      P0 = LED_BUFF[TUBE_IDX];
      TUBE_IDX++;
      break;
    case 5:
      enable_tube(TUBE_IDX);
      P0 = LED_BUFF[TUBE_IDX];
      TUBE_IDX = 0;
      break;
    default:
      break;
  }
}

void reload_timer1() {
  // reset Timer1 1ms
  TH1 = 0xFC;
  TL1 = 0x67;
}

void interrupt_timer1() __interrupt(3) {
  reload_timer1();
  T1_CNT++;
  if (T1_CNT == T1_INTERVAL_CNT) {
    T1_CNT = 0;
    T1_INTERRUPT_FLAG = 1;
  }
  flush_traffic_light();
}

void flush_traffic_light() {
  turn_off_all_leds();
  turn_on_led_master_switch();
  switch (LIGHT_INDEX) {
    case 0:
      turn_on_red();
      break;
    case 1:
      turn_on_yellow();
      break;
    case 2:
      turn_on_green();
      break;
    default:
      break;
  }
}
