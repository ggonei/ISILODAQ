#ifndef TV792Histograms_h
#define TV792Histograms_h

#include <string>
#include "THistogramArrayBase.h"

/// Class for making histograms of V792 ADC data.
class TV792Histograms : public THistogramArrayBase {
 public:
  TV792Histograms();
  virtual ~TV792Histograms(){};
  
  /// Update the histograms for this canvas.
  void UpdateHistograms(TDataContainer& dataContainer);

  /// Take actions at begin run
  void BeginRun(int transition,int run,int time);

  /// Take actions at end run  
  void EndRun(int transition,int run,int time);

private:

  void CreateHistograms();
    
};

#endif


