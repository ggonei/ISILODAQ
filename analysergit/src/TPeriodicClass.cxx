#include "TPeriodicClass.hxx"
#include <sys/time.h>
#include <assert.h>


double GetTimeSec()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}

TPeriodicClass::TPeriodicClass(int period_msec,TimerHandler handler)
{
  assert(handler != NULL);
  fPeriod_msec = period_msec;
  fHandler  = handler;
  fLastTime = GetTimeSec();
  Start(period_msec,kTRUE);
}

Bool_t TPeriodicClass::Notify()
{
  double t = GetTimeSec();
  //printf("timer notify, period %f should be %f!\n",t-fLastTime,fPeriod_msec*0.001);
  
  if (t - fLastTime >= 0.9*fPeriod_msec*0.001)
    {
      //printf("timer: call handler %p\n",fHandler);
      if (fHandler)
	(*fHandler)();
      
      fLastTime = t;
    }
  
  Reset();
  return kTRUE;
}
