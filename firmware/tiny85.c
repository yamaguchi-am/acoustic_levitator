#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>

#define IOCLK 1000000  // CKSEL[1:0]=10, CKDIV8=0
#define SOUND_FREQUENCY 40000

#define BIT(x) (1 << (x))

const int DIV = IOCLK / SOUND_FREQUENCY - 1;

volatile int g_count = 0;

inline void StartCh0() { TCCR0B |= BIT(CS00); }

inline void StopCh0() { TCCR0B &= ~BIT(CS00); }

inline void StartCh1() {
  TCCR1 |= 0 << CS13 | 0 << CS12 | 0 << CS11 | 1 << CS10;
}

inline void StopCh1() {
  TCCR1 &= ~(1 << CS13 | 1 << CS12 | 1 << CS11 | 1 << CS10);
}

inline void EnableInterrupt() { TIMSK |= BIT(OCIE1B); }

inline void DisableInterrupt() { TIMSK &= ~BIT(OCIE1B); }

inline void SetTimerDiff(int diff) {
  DisableInterrupt();
  TCNT0 = 0;
  TCNT1 = diff;
  EnableInterrupt();
}

ISR(TIM1_COMPB_vect) { g_count++; }

void InitTimer() {
  // Timer 0
  // WGM0: fast PWM mode
  // clear OC0B on compare match
  TCCR0A = 0 << COM0A1 | 0 << COM0A0 | 1 << COM0B1 | 0 << COM0B0 | 1 << WGM01 |
           1 << WGM00;
  // CKDIV=0; CLKIO = 1/8 CLK
  // no prescaler
  TCCR0B = 1 << CS00 | 1 << WGM02;
  OCR0A = DIV;
  OCR0B = DIV / 2;
  StartCh0();

  // Timer 1
  // Clear on compare match
  // PWM mode
  // OC1B set on compare match, cleared when count=0.
  // Clock source = CK
  TCCR1 = 1 << CTC1 | 0 << PWM1A | 0 << COM1A1 | 0 << COM1A0 | 0 << CS13 |
          0 << CS12 | 0 << CS11 | 1 << CS10;
  // Output both OC1B(PB4) and ~OC1B(PB3).
  // The PCB only uses ~OC1B.
  GTCCR = 0 << TSM | 1 << PWM1B | 0 << COM1B1 | 1 << COM1B0;
  OCR1C = DIV;
  OCR1B = DIV / 2;

  EnableInterrupt();
}

inline int button1() { return (PINB & 0b1) == 0; }

inline int button2() { return (PINB & 0b100) == 0; }

int main(void) {
  DDRB = 0b01010;
  PORTB = 0b00101;

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
    while (g_count - prev_count < 800) {
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
