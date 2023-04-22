#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>

// System clock = 20MHz, prescaler=1/6 (the default after reset)
#define IOCLK 3333333
#define SOUND_FREQUENCY 40000
#define LOOP_FREQUENCY 50

const int DIV = IOCLK / SOUND_FREQUENCY - 1;
const int LOOP_WAIT_COUNT = SOUND_FREQUENCY / LOOP_FREQUENCY;

volatile int g_count = 0;

inline void StartCh0() { TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm; }

inline void StopCh0() { TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm; }

inline void StartCh1() { TCB0.CTRLA |= TCB_ENABLE_bm; }

inline void StopCh1() { TCB0.CTRLA &= ~TCB_ENABLE_bm; }

inline void EnableInterrupt() { TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm; }

inline void DisableInterrupt() { TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_OVF_bm; }

inline void SetTimerDiff(int diff) { TCA0.SINGLE.CMP1BUF = diff; }

ISR(TCA0_OVF_vect) { g_count++; }

void InitTimer() {
  TCA0.SINGLE.CTRLA = 0;
  TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
  PORTMUX.CTRLC |= PORTMUX_TCA00_bm;

  TCA0.SINGLE.PER = DIV;
  TCA0.SINGLE.CMP0 = DIV / 2;
  TCA0.SINGLE.CMP1 = DIV / 2;
  StartCh0();

  // single shot mode, triggered by TCA0 compare match 1
  TCB0.CCMP = DIV / 2;
  TCB0.CTRLA = 0;
  TCB0.CTRLB = TCB_CCMPEN_bm | TCB_CNTMODE_SINGLE_gc;
  TCB0.EVCTRL |= TCB_CAPTEI_bm;
  EVSYS.SYNCCH0 = EVSYS_SYNCCH0_TCA0_CMP1_gc;
  EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_SYNCCH0_gc;
  StartCh1();

  EnableInterrupt();
}

inline int button1() { return (PORTA.IN & 0b100) == 0; }

inline int button2() { return (PORTA.IN & 0b10) == 0; }

int main(void) {
  // PA7: WO0 (TCA0)
  // PA6: WO  (TCB0)
  PORTA.DIR = PIN7_bm | PIN6_bm;
  PORTA.PIN1CTRL |= PORT_PULLUPEN_bm;
  PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;

  InitTimer();
  sei();
  int pos = DIV - 1;
  int speed = 1;
  const int kPeriod = DIV * 2;
  char auto_mode = 0;
  int t = 0;

  const char kDurationBeforeRepeat = 10;
  int prev_count = g_count;
  char prev_btn1 = 0;
  char prev_btn2 = 0;
  char btn1_duration = 0;
  char btn2_duration = 0;

  SetTimerDiff(pos);

  for (;;) {
    while (g_count - prev_count < LOOP_WAIT_COUNT) {
    }
    prev_count = g_count;
    if (auto_mode) {
      if (button1() || button2()) {
        auto_mode = 0;
      }
      pos += speed;
      t--;
      if (t == 0) {
        speed = -speed;
        t = kPeriod;
      }
      if (pos < 0) pos = DIV - 1;
      if (pos >= DIV) pos = 0;
      SetTimerDiff(pos);
      continue;
    }

    if (!auto_mode && button1() && button2()) {
      while (button1() || button2()) {
      }
      auto_mode = 1;
      speed = 1;
      // Start from center position.
      t = kPeriod / 2;
    }

    int btn1 = button1();
    int btn2 = button2();
    if (btn1) {
      if (btn1_duration < kDurationBeforeRepeat) {
        btn1_duration++;
      }
    } else {
      btn1_duration = 0;
    }
    if (btn2) {
      if (btn2_duration < kDurationBeforeRepeat) {
        btn2_duration++;
      }
    } else {
      btn2_duration = 0;
    }

    if ((!btn1 && prev_btn1) || btn1_duration == kDurationBeforeRepeat) {
      pos = pos - 1;
      if (pos < 0) pos = DIV - 1;
      SetTimerDiff(pos);
    }
    if ((!btn2 && prev_btn2) || btn2_duration == kDurationBeforeRepeat) {
      pos = pos + 1;
      if (pos >= DIV) pos = 0;
      SetTimerDiff(pos);
    }
    prev_btn1 = btn1;
    prev_btn2 = btn2;
  }
}
