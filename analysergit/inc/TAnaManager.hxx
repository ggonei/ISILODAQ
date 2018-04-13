#ifndef TAnaManager_h
#define TAnaManager_h

// Use this list here to decide which type of equipment to use.

#define USE_V792
#define USE_V1190

#include "TV792Histogram.h"
#include "TV1190Histogram.h"

/// This is an example of how to organize a set of different histograms
/// so that we can access the same information in a display or a batch
/// analyzer.
/// Change the set of ifdef's above to define which equipment to use.
class TAnaManager  {
public:
  TAnaManager();
  virtual ~TAnaManager(){};

  /// Processes the midas event, fills histograms, etc.
  int ProcessMidasEvent(TDataContainer& dataContainer);

	/// Methods for determining if we have a particular set of histograms.
	bool HaveV792Histograms();
	bool HaveV1190Histograms();

	/// Methods for getting particular set of histograms.
	TV792Histograms* GetV792Histograms();
	TV1190Histograms* GetV1190Histograms();


private:

	TV792Histograms *fV792Histogram;
	TV1190Histograms *fV1190Histogram;

};



#endif
