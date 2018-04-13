/********************************************************************\

  Name:         RootLock.cxx
  Created by:   Konstantin Olchanski

  Contents:     Helper functions for locking ROOT access in multithreaded network servers

  $Id$

\********************************************************************/

#include "RootLock.h"

#include <TSemaphore.h>
#include <TTimer.h>
#include <TThread.h>

static TSemaphore gRootSema(0); // wait by server, post by timer
static TSemaphore gWaitSema(0); // post by server, wait by timer
static TSemaphore gDoneSema(0); // post by server, wait by timer

bool gDebugLockRoot = false;

void LockRoot()
{
  if (gDebugLockRoot)
    printf("Try Lock ROOT!\n");
  gWaitSema.Post();
  gRootSema.Wait();
  if (gDebugLockRoot)
    printf("Lock ROOT!\n");
}

void UnlockRoot()
{
  if (gDebugLockRoot)
    printf("Unlock ROOT!\n");
  gDoneSema.Post();
}

LockRootGuard::LockRootGuard() // ctor
{
  fLocked = true;
  LockRoot();
}

LockRootGuard::~LockRootGuard() // dtor
{
  if (fLocked)
    Unlock();
}

void LockRootGuard::Unlock()
{
  UnlockRoot();
  fLocked = false;
}

class ServerTimer : public TTimer
{
public:

  static ServerTimer* fgTimer;

  static void StartServerTimer(int period_msec = 100)
  {
    if (!fgTimer)
      {
        fgTimer = new ServerTimer();
        fgTimer->Start(period_msec,kTRUE);
      }
  }

  Bool_t Notify()
  {
    if (gDebugLockRoot)
      fprintf(stderr, "ServerTimer::Notify!!\n");

    int notWaiting = gWaitSema.TryWait();
    if (gDebugLockRoot)
      printf("NotWaiting %d!\n", notWaiting);
    if (!notWaiting)
      {
        if (gDebugLockRoot)
          printf("Yeld root sema!\n");
        gRootSema.Post();
        TThread::Self()->Sleep(0, 1000000); // sleep in ns
        gDoneSema.Wait();
        if (gDebugLockRoot)
          printf("Recapture root sema!\n");
      }

    Reset();
    return kTRUE;
  }

private:
  // ServerTimer is a singleton class,
  // thus ctor and dtor are private

  ~ServerTimer() // dtor
  {
    //TurnOff();
  }

  ServerTimer() // ctor
  {
    //int period_msec = 100;
    //Start(period_msec,kTRUE);
  }
};

ServerTimer* ServerTimer::fgTimer;

void StartLockRootTimer(int period_msec)
{
  ServerTimer::StartServerTimer(period_msec);
}

// end file
