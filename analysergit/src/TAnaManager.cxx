#include "TAnaManager.hxx"


TAnaManager::TAnaManager(){	//initialise
/* uncomment Line 16
//For generating default histograms
	fV792Histogram = 0;	//ADC
	fV1190Histogram = 0;	//TDC
#ifdef USE_V792
	fV792Histogram = new TV792Histograms();	//ADC
	fV792Histogram->DisableAutoUpdate();	//disable auto-update (update histo in AnaManager)
#endif
#ifdef USE_V1190
	fV1190Histogram = new TV1190Histograms();	//TDC
	fV1190Histogram->DisableAutoUpdate();	//disable auto-update (update histo in AnaManager)
#endif
*/
};

int TAnaManager::ProcessMidasEvent(TDataContainer& dataContainer){	//analyse
/*	if(fV792Histogram)	fV792Histogram->UpdateHistograms(dataContainer);
	if(fV1190Histogram)	fV1190Histogram->UpdateHistograms(dataContainer);*/
	return 1;
}

bool TAnaManager::HaveV792Histograms(){	//check for ADC histos
	if(fV792Histogram)	return true;
	return false;
}

bool TAnaManager::HaveV1190Histograms(){	//check for TDC histos
	if(fV1190Histogram)	return true;
	return false;
};

TV792Histograms*	TAnaManager::GetV792Histograms()	{return fV792Histogram;}	//ADC
TV1190Histograms*	TAnaManager::GetV1190Histograms()	{return fV1190Histogram;}	//TDC
