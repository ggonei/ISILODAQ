//
//	Main analysis routine
//	George O'Neill, 12 April 2018
//

//	C++ libraries
#include <stdio.h>
#include <iostream>
#include <time.h>

//	Analysis libraries (based on ROOTANA)
#include "TAnaManager.hxx"
#include "TRootanaEventLoop.hxx"
#include "TV792Data.hxx"
#include "TV1190Data.hxx"

//	ROOT Libraries
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>



class Analyzer: public TRootanaEventLoop {


private:

	TTree* evts;	//	Event tree
	unsigned long long int n_evts = 0;	//	Event counter

	TH1F* Echan[32];	//	Energy spectra
	TH1F* Tchan[32];	//	Time spectra
	TH1F* Tevents;	//	Event rate
	TH2F* EdE;	//	E-dE 2-D spectrum

	TH2F* Es1vEs2;
	TH2F* EgvEs1;
	TH2F* EgvEs2;
	TH1F* Ts1vs2;
	TH1F* Ts1vge;
	TH1F* Ts2vge;
	TH1F* TdSivge;

	std::vector<VADCMeasurement> measE;	//	ADC measurements
	std::vector<TDCMeasurement>  measT;	//	TDC measurements
	std::string s;

//Debugging
	Double_t s1en = 0, s2en = 0, trti = 0, geen = 0, s1ti = 0, s2ti = 0, geti = 0;	//	Holding values
	unsigned long long int dtoff = 1242813469;	//	05/20/2009 @ 10:00am (UTC)
//


public:


	TAnaManager *anaManager;	//	Analysis manager needs to be accessible

	Analyzer() {
		//DisableAutoMainWindow();	//	This makes no difference...
		//UseBatchMode();	//	Silent?

		//	Initialize pointers to null
		anaManager = 0;	
	};

	virtual ~Analyzer() {};

	void Initialize() {}

	void InitManager() {
		if(anaManager) delete anaManager;
		anaManager = new TAnaManager();
	}


	void BeginRun(int transition, int run, int time) {	//	Creates objects for analysis
		InitManager();	//	Start analysis manager
		evts = new TTree("evts", "evts");	//	Create event tree
		evts->Branch("n_evts",&n_evts,"Event_number/L");	//	Event number (redundant)
		evts->Branch("s1en",&s1en,"s1en/D");	//	dE
		evts->Branch("s2en",&s2en,"s2en/D");	//	 E
		evts->Branch("geen",&geen,"geen/D");	//	gE
		evts->Branch("s1ti",&s1ti,"s1ti/D");	//	dE time
		evts->Branch("s2ti",&s2ti,"s2ti/D");	//	 E time
		evts->Branch("geti",&geti,"geti/D");	//	gE time
		evts->Branch("trti",&trti,"trti/D");	//	tr time
		Tevents = new TH1F("Tevents",std::string ("Event rate/second since +dtoff").c_str(),2000000,0,1000000);	//	Events per second
/*		EdE = new TH2F("EdE","E-deltaE spectrum",4200,0,4200,4200,0,4200);	//	E-dE 2-D spectrum
		Es1vEs2 = new TH2F("Es1vEs2","s1-s2 spectrum",4200,0,4200,4200,0,4200); // create new histogram
		EgvEs1 = new TH2F("EgvEs1","ge-s1 spectrum",4200,0,4200,4200,0,4200); // create new histogram
		EgvEs2 = new TH2F("EgvEs2","ge-s2 spectrum",4200,0,4200,4200,0,4200); // create new histogram
		Tevents = new TH1F("Tevents","Events",2000000,-1000000,1000000); // create new histogram
		Ts1vs2 = new TH1F("Ts1vs2","s1 v s2",200000,-100000,100000); // create new histogram
		Ts1vge = new TH1F("Ts1vge","s1 v ge",200000,-100000,100000); // create new histogram
		Ts2vge = new TH1F("Ts2vge","s2 v ge",200000,-100000,100000); // create new histogram
		TdSivge = new TH1F("TdSivge","dSi v ge",200000,-100000,100000); // create new histogram*/
		for(unsigned int i = 0; i < 32; i++) {	//	Raw energy, time histograms/channel
			s = std::to_string(i);
			Echan[i] = new TH1F(std::string ("Echan_"+s).c_str(),std::string ("Raw energy chan"+s).c_str(),4096,0,4096);
			Tchan[i] = new TH1F(std::string ("Tchan_"+s).c_str(),std::string ("Time chan"+s).c_str(),4096,0,4096);
		}
	}


	bool ProcessMidasEvent(TDataContainer& dataContainer) {	//	Processes events
		
		TV792Data *dataE = dataContainer.GetEventData<TV792Data>("QDC0");	//	ADC data
		TV1190Data *dataT = dataContainer.GetEventData<TV1190Data>("TDC0");	//	TDC data
		
//Debugging
		++n_evts;	//	Event counter
		Tevents->Fill(dataContainer.GetMidasData().GetTimeStamp() - dtoff);	//	Event rate
		//std::cout << dataContainer.GetMidasData().GetEventId() << std::endl;	//	Event ID
		//dataE->Print();
		//dataT->Print();
//

		if(dataE){	//	Does the ADC data exist?
			measE = dataE->GetMeasurements();	//	Fetch array from ADC
			for(unsigned int i = 0; i < measE.size(); ++i) {	//	loop over energy measurements
				//std::cout << dataContainer.GetMidasData().GetTimeStamp()-dtoff << std::endl;	//Debugging
				//std::cout << i << "/" << measE.size() << ": " << "chan" << measE[i].GetChannel() << " " << measE[i].GetMeasurement() << std::endl;	//Debugging
				Echan[i]->Fill(measE[i].GetMeasurement());	//	Fill raw energy histograms
				if(measE[i].GetChannel()==0)	s1en = measE[i].GetMeasurement();	//	dE branch store
				if(measE[i].GetChannel()==2)	s2en = measE[i].GetMeasurement();	//	 E branch store
				if(measE[i].GetChannel()==4)	geen = measE[i].GetMeasurement();	//	gE branch store
			}
		}

		if(dataT){	//	Does the TDC data exist?
			measT = dataT->GetMeasurements();	//	Fetch array from TDC
			for(unsigned int i = 0; i < measT.size(); i++){	//	loop over time measurements
				//std::cout << dataContainer.GetMidasData().GetTimeStamp() << ":  " << measT[i].GetChannel() << ", " << measT[i].GetMeasurement() << std::endl;	//Debugging
				Tchan[i]->Fill(measT[i].GetMeasurement());	//	Fill time histograms
				if(i==0)	//	Trigger0 time
				{
					trti = measT[i].GetMeasurement();
				}
				if(i==1)	//	Silicon1 time
				{
					s1ti = measT[i].GetMeasurement();
/*					Ts1vGe->Fill(measurementst[1].GetMeasurement()-measurementst[0].GetMeasurement());
					if (measurementst[1].GetMeasurement()-measurementst[0].GetMeasurement() < 760 && measurementst[1].GetMeasurement()-measurementst[0].GetMeasurement() > 720 && data)
					{
						EgvEs1->Fill(measurements[4].GetMeasurement(), measurements[0].GetMeasurement());
					}*/
				}
				if(i==2)	//	Silicon2 time
				{
					s2ti = measT[i].GetMeasurement();
				}
				if(i==3)	//	Germani3 time
				{
					geti = measT[i].GetMeasurement();
/*					if (measurementst[2].GetMeasurement()-measurementst[1].GetMeasurement() < 760 && measurementst[2].GetMeasurement()-measurementst[1].GetMeasurement() > 720 && data)
					{
					EdE->Fill(measurements[0].GetMeasurement(), measurements[0].GetMeasurement() - measurements[2].GetMeasurement());
					Es1vEs2->Fill(measurements[0].GetMeasurement(), measurements[2].GetMeasurement());
					}
					if (measurementst[2].GetMeasurement()-measurementst[0].GetMeasurement() < 760 && measurementst[2].GetMeasurement()-measurementst[0].GetMeasurement() > 720 && data)
					{
					EgvEs2->Fill(measurements[4].GetMeasurement(), measurements[2].GetMeasurement());
					}
					Ts1vs2->Fill(measurementst[2].GetMeasurement()-measurementst[1].GetMeasurement());
					Ts2vGe->Fill(measurementst[2].GetMeasurement()-measurementst[0].GetMeasurement());*/
				}
			}
		}

//		std::cout << n_evts << "time" << dataContainer.GetMidasData().GetTimeStamp()-dtoff << ":sD" << s1en <<  ":sE" << s2en <<  ":gE" << geen <<  ":trt" << trti <<  ":sDt" << s1ti <<  ":sEt" << s2ti <<  ":gEt" << geti <<  std::endl;	//	Check branch info
		evts->Fill();	//	Fill tree
		return true;	//	Holy shit you made it here and it actually worked?!

	}


}; 


int main(int argc, char *argv[])	//	Main loop for initialising analyser
{

	Analyzer::CreateSingleton<Analyzer>();
	return Analyzer::Get().ExecuteLoop(argc, argv);

}
