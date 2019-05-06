#ifndef AZ_TIMER
#define AZ_TIMER

class AZTimer {
  public:
    AZTimer(unsigned long p);
    bool check();
  private:
    unsigned long start_time;
    unsigned long period;
};

AZTimer::AZTimer(unsigned long p) {
  period = p? p: 1;
  start_time = 0;
}

bool AZTimer::check() {
  unsigned long current = millis();
  if (current >= start_time) {
    if (current < (start_time + period))
      return false;
  } else {
    if (current < (0xFFFFFFFF - start_time + period))
      return false;
  }
  start_time = current/period*period;
  return true;
}

#endif
