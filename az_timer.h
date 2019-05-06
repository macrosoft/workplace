#ifndef AZ_TIMER
#define AZ_TIMER

class AZTimer {
  public:
    AZTimer(unsigned long p);
    bool check(bool reset = true);
  private:
    unsigned long start_time;
    unsigned long period;
};

AZTimer::AZTimer(unsigned long p): period(p) {
  start_time = 0;
}

bool AZTimer::check(bool reset) {
  unsigned long current = millis();
  if (current >= start_time) {
    if (current < (start_time + period))
      return false;
  } else {
    if (current < (0xFFFFFFFF - start_time + period))
      return false;
  }
  if (reset)
    start_time = current;
  return true;
}

/*
bool isTimer(unsigned long startTime, unsigned long period) {
  unsigned long currentTime;
  currentTime = millis();
  if (currentTime >= startTime) {
    return (currentTime >= (startTime + period));
  } else {
    return (currentTime >= (0xFFFFFFFF - startTime + period));
  }
}
*/

#endif
