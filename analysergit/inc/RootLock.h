/********************************************************************\

  Name:         RootLock.h
  Created by:   Konstantin Olchanski

  Contents:     Helper functions for locking ROOT access in multithreaded network servers

  $Id$

\********************************************************************/

extern bool gDebugLockRoot;

void LockRoot();
void UnlockRoot();

struct LockRootGuard
{
  bool fLocked;
  LockRootGuard();
  ~LockRootGuard();
  void Unlock();
};

void StartLockRootTimer(int period_msec = 100);

// end file
