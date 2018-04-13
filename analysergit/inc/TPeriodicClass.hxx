#ifndef TPeriodicClass_hxx_seen
#define TPeriodicClass_hxx_seen

#include <TTimer.h>

#include <stdio.h>

double GetTimeSec();


class TPeriodicClass : public TTimer
{
public:
  typedef void (*TimerHandler)(void);

  int          fPeriod_msec;
  TimerHandler fHandler;
  double       fLastTime;

  TPeriodicClass(int period_msec,TimerHandler handler);

  Bool_t Notify();

  ~TPeriodicClass()
  {
    TurnOff();
  }
};


#endif

