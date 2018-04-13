#include "TV1190Histogram.h"
#include "TV1190Data.hxx"
#include "TDirectory.h"

const int Nchannels = 64;

/// Reset the histograms for this canvas
TV1190Histograms::TV1190Histograms(){  
  
  CreateHistograms();
}


void TV1190Histograms::CreateHistograms(){
  

  // Otherwise make histograms
  clear();
  
  std::cout << "Create Histos" << std::endl;
  for(int i = 0; i < Nchannels; i++){ // loop over channels    

    char name[100];
    char title[100];
    sprintf(name,"V1190_%i_%i",0,i);

    // Delete old histograms, if we already have them
    TH1D *old = (TH1D*)gDirectory->Get(name);
    if (old){
      delete old;
    }


    // Create new histograms
    
    sprintf(title,"V1190 histogram for channel=%i",i);	
    
    TH1D *tmp = new TH1D(name,title,5000,0,500000);
    tmp->SetXTitle("TDC value");
    tmp->SetYTitle("Number of Entries");
    push_back(tmp);
  }

}



  
/// Update the histograms for this canvas.
void TV1190Histograms::UpdateHistograms(TDataContainer& dataContainer){


  TV1190Data *data = dataContainer.GetEventData<TV1190Data>("TDC0");
  if(!data) return;

  /// Get the Vector of ADC Measurements.
  std::vector<TDCMeasurement> measurements = data->GetMeasurements();
  for(unsigned int i = 0; i < measurements.size(); i++){ // loop over measurements
	
    int chan = measurements[i].GetChannel();
    GetHistogram(chan)->Fill(measurements[i].GetMeasurement());
  }

}



/// Take actions at begin run
void TV1190Histograms::BeginRun(int transition,int run,int time){

  CreateHistograms();

}

/// Take actions at end run  
void TV1190Histograms::EndRun(int transition,int run,int time){

}
