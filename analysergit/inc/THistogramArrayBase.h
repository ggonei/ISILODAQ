#ifndef THistogramArrayBase_h
#define THistogramArrayBase_h


#include <iostream>
#include <string>
#include <stdlib.h>

#include "TDataContainer.hxx"
#include "TH1F.h"
#include <vector>

/// Base class for user to create an array of histograms.
/// Features of the histogram array
/// i) Histograms are all defined together.
/// ii) Histograms are updated together
/// 
/// Users of this abstract base class should implement a class
/// derived from must THistogramArrayBase that
///  1) define the histograms that you want in the constructor.
///  2) implement the UpdateHistograms(TDataContainer&) method.
///
/// The logic of this histogram array is based on it being 
/// a set of similar histograms; if you use this class for
/// a set of dissimilar histograms (different binning, different quantities)
/// the results will probably be unsatisfactory.
/// The array'ness is actually implemented as a vector of TH1s.
///
/// The default representation is to have a 1D array of histograms. 
/// But the user can also use the class a 2D array of histograms by specifying the functions
/// 
///
class THistogramArrayBase : public std::vector<TH1*> {
 public:
  THistogramArrayBase():fNumberChannelsInGroups(-1),fGroupName(""),fChannelName(""),
    fDisableAutoUpdate(false),fHasAutoUpdate(false){}

  virtual ~THistogramArrayBase(){}

  /// Update the histograms for this canvas.
  virtual void UpdateHistograms(TDataContainer& dataContainer) = 0;

  /// A helper method for accessing each histogram.  Does bounds checking.
  TH1* GetHistogram(unsigned i){
    if(i >= size()){
      std::cerr << "Invalid index (=" << i 
		<< ") requested in THistogramArrayBase::GetHistogram(int i) " << std::endl;
      return 0;
    }
    return (*this)[i];
  }

  /// Take actions at begin run
  virtual void BeginRun(int transition,int run,int time){};

  /// Take actions at end run  
  virtual void EndRun(int transition,int run,int time){};

  /// Function to define the number of channels in group and 
  /// allow user to treat the array as 2D array.
  void SetNumberChannelsInGroup(int numberChannelsInGroups){ fNumberChannelsInGroups = numberChannelsInGroups; }
  const int  GetNumberChannelsInGroup(){ return fNumberChannelsInGroups; }
  
  /// Set name for the 'group'.
  void SetGroupName(std::string name){  fGroupName = name;  }
  const std::string GetGroupName(){ return fGroupName;  }

  /// Set name for the 'channel'.
  void SetChannelName(std::string name){  fChannelName = name;  }
  const std::string GetChannelName(){ return fChannelName;  }
  
  /// Define whether the histogram gets automatically updated by rootana display.
  /// 'True' means that rootana display will NOT call UpdateHistograms automatically.
  void DisableAutoUpdate(bool DisableautoUpdate=true){ fDisableAutoUpdate = DisableautoUpdate; fHasAutoUpdate = true;}  
  const bool GetDisableAutoUpdate(){ return fDisableAutoUpdate; }  
  const bool HasAutoUpdate(){ return fHasAutoUpdate; }  
  
private:

  /// This is the number of channels in a given group.
  /// This is mostly used by rootana display, but could 
  /// also be used to specify histograms as a 2D array of [group][channel.
  int fNumberChannelsInGroups;
  
  /// The name for the 'group'.
  std::string fGroupName;

  /// The name for the 'channel'.
  std::string fChannelName;

  /// Defines whether the histogram should be automatically updated 
  /// by TRootanaDisplay.
  bool fDisableAutoUpdate;
  bool fHasAutoUpdate;

};

#endif
