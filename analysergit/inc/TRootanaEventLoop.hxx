#ifndef TRootanaEventLoop_hxx_seen
#define TRootanaEventLoop_hxx_seen

// C++ includes
#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <typeinfo>

// ROOTANA includes
//#include "TMidasFile.h"
//#include "TMidasOnline.h"
//#include "TMidasEvent.h"
#include "VirtualOdb.h"
#include "TDataContainer.hxx"

// ROOT includes
#include "TApplication.h"
#include "TDirectory.h"
#include <TTimer.h>
#include <TFile.h>

/// This is a base class for event loops that are derived from rootana.
/// 
/// The user should create a class that derives from this TRootanaEventLoop class 
/// and then fill in the methods that they want to implement.
///
/// The user must implement the method ProcessMidasEvent(), which will get executed 
/// on each event.
/// 
/// The user can also implement methods like Initialize, BeginRun, EndRun, Finalize
/// if there are actions they want to execute at certain points.
///
/// The event loop will work in both offline and online mode (online only if the user has MIDAS installed).
/// 
/// In example of this type of event loop is shown in examples/analyzer_example.cxx
///
class TRootanaEventLoop {
 
public:
  virtual ~TRootanaEventLoop ();


  static TRootanaEventLoop& Get(void);
  
  /// Method to get the data container that event loop owns.
  TDataContainer* GetDataContainer(){return fDataContainer;};


  /// The main method, called for each event.  Users must implement this
  /// function!
  virtual bool ProcessMidasEvent(TDataContainer& dataContainer) = 0;
  //virtual bool ProcessEvent(TMidasEvent& event) = 0;


  /// Called after the arguments are processes but before reading the first
  /// event is read
  virtual void Initialize(void);
  
  /// Called before the first event of a file is read, but you should prefer
  /// Initialize() for general initialization.  This method will be called
  /// once for each input file.  
  virtual void BeginRun(int transition,int run,int time);
  
  /// Called after the last event of a file is read, but you should prefer
  /// Finalize() for general finalization.  This method will be called once
  /// for each input file.
  virtual void EndRun(int transition,int run,int time);
  
  /// Special version of Init method, to be used only by TRootanaDisplay
  virtual void InitializeRAD(void){};
  /// Special version of BOR method, to be used only by TRootanaDisplay
  /// This is just so that users still have ability to set their own
  /// BOR methods, in addition to what TRootanaDisplay needs to do at BOR
  virtual void BeginRunRAD(int transition,int run,int time){};
  /// Also special version of EOR method, to be used only by TRootanaDisplay
  virtual void EndRunRAD(int transition,int run,int time){};

  /// Called after the last event has been processed, but before any open
  /// output files are closed.
  virtual void Finalize();

  /// Called when there is a usage error.  This code should print a usage
  /// message and then return. 
  virtual void Usage(void);
  
  /// Check an option and return true if it is valid.  
  /// The return value is used to flag errors during option handling.  If
  /// the options are valid, then CheckOption should return true to indicate
  /// success.  If there is a problem processing the options, then CheckOption
  /// should return false.  If this returns false, then the event loop will
  /// print the Usage message and exit with a non zero value (i.e. indicate
  /// failure).
  virtual bool CheckOption(std::string option);

	/// The PreFilter method allows user to specify whether to ignore a particular event.
	/// Specifically, if PreFilter returns
	///
	/// true -> then ProcessMidasEvent will be called
	/// or
	/// false -> then ProcessMidasEvent will not be called
	/// 
	/// This is particularly useful for the RootanaDisplay, where you might 
	/// want to only process and plot certain events.
	virtual bool PreFilter(TDataContainer& dataContainer){return true;}

  /// Are we processing online data?
  bool IsOnline() const {return !fIsOffline;};
  
  /// Are we processing offline data?
  bool IsOffline() const {return fIsOffline;};
  
  /// Current Run Number
  int GetCurrentRunNumber() const {return fCurrentRunNumber;};

  /// Current Run Number
  void SetCurrentRunNumber(int run) {fCurrentRunNumber = run;};

  /// Method to actually process the Midas information, either as file or online.
  int ExecuteLoop(int argc, char *argv[]);

  int ProcessMidasFile(TApplication*app,const char*fname);

#ifdef HAVE_MIDAS
  int ProcessMidasOnline(TApplication*app, const char* hostname, const char* exptname);
#endif


  /// This static templated function will make it a little easier
  /// for users to create the singleton instance.
  template<typename T>
  static void CreateSingleton()
  {
    if(fTRootanaEventLoop)
      std::cout << "Singleton has already been created" << std::endl;
    else
      fTRootanaEventLoop = new T();
  } 
  

  /// Disable automatic creation of MainWindow
  void DisableAutoMainWindow(){  fCreateMainWindow = false;}

  /// Use a batch mode, where we don't check ROOT status
  void UseBatchMode(){  fUseBatchMode = true;}

  /// Get pointer to ODB variables
  VirtualOdb* GetODB(){return fODB;}


  /// Open output ROOT file
  void OpenRootFile(int run, std::string midasFilename = std::string(""));

  /// Cloe output ROOT file
  void CloseRootFile();
  
  /// Check if output ROOT file is valid and open
  bool IsRootFileValid(){    
    if(fOutputFile) return true;
    return false;
  }
  
  
  void DisableRootOutput(bool disable=true){fDisableRootOutput = disable;};

  int IsRootOutputEnabled(){return !fDisableRootOutput;};

  /// Set the output filename.
  /// File name will be $(fOutputFilename)XXX.root, where XXX is run number  
  void SetOutputFilename(std::string name){fOutputFilename = name;};

	/// This is an alternative, more complicated way of setting the output ROOT filename.
	/// In this case the user is given the run number and the midas file name and,
	/// from that information, constructs the output ROOT filename themselves.
	virtual std::string SetFullOutputFileName(int run, std::string midasFilename){
		return std::string("");
	}

  void SetOnlineName(std::string name){fOnlineName = name;};

  /// Provide a way to force program to only process certain event IDs.
  /// This method can be called repeatedly to specify several different event IDs to accept.
  /// If the method is not called then all eventIDs are accepted.
  void ProcessThisEventID(int eventID){
    fProcessEventIDs.push_back(eventID);
  };

  /// Little helper method to check if EventID matchs requested EventID list.
  bool CheckEventID(int eventId);

  /// Suppress the warning methods regarding old timestamp events for online 
  /// ie warnings about analyzer falling behind data taking.
  void SuppressTimestampWarnings(){ fSuppressTimestampWarnings = true;};

  /// Suppress timestamp warnings?  true = suppress warnings
  bool GetSuppressTimestampWarnings(){ return fSuppressTimestampWarnings;};

  /// Method to set whether analyzer should operate in GET_RECENT mode, 
  /// where we only process data that is less than 1 second old (this is not default).
  /// Setting true will use this option.
  void UseOnlyRecent(bool setting = true);//{ fUseOnlyRecent = setting;};

  // Set ReadWrite mode fot THttpServer (to allow operation on histograms through web; like histogram reset).
  void SetTHttpServerReadWrite(bool readwrite = true);

protected:

  bool CreateOutputFile(std::string name, std::string options = "RECREATE"){
    
    fOutputFile = new TFile(name.c_str(),options.c_str());
    
    return true;
  }


  TRootanaEventLoop ();

  /// The static pointer to the singleton instance.
  static TRootanaEventLoop* fTRootanaEventLoop;

  /// TDirectory for online histograms.
  TDirectory* fOnlineHistDir;

  /// This is a special version of CheckOption that is only used by TRootanaDisplay.
  /// This is just so that users still have the ability to set options for  
  /// executables derived from TRootanaDisplay.
  virtual bool CheckOptionRAD(std::string option);

  /// Also a special version of usage for TRootanaDisplay.  See CheckOptionRAD
  virtual void UsageRAD(void);
 
private:
  
  /// Help Message
  void PrintHelp();

  /// Output ROOT file
  TFile *fOutputFile;

  /// Base part of the output filename
  /// File name will be $(fOutputFilename)XXX.root, where XXX is run number
  std::string fOutputFilename;

  /// Variable for disabling/enabling Root output
  bool fDisableRootOutput;

  /// Pointer to the ODB access instance
  VirtualOdb* fODB;
 
  /// Are we processing offline or online data?
  bool fIsOffline;

  /// Current run number
  int fCurrentRunNumber;


  /// Pointer to the physics event; the physics event is what we pass to user.
  /// The midas event is accessible through physics event.
  /// We make a single instance of the physics event for whole execution,
  /// because sometimes the decoded information needs to persist 
  /// across multiple midas events.
  TDataContainer *fDataContainer;

  /// This is the set of eventIDs to process
  std::vector<int> fProcessEventIDs;

  // ________________________________________________
  // Variables for online analysis

  /// Buffer to connect to
  std::string fBufferName;

  /// Name of program, as seen by MIDAS.
  std::string fOnlineName;
    
  /// Bool for suppressing the warnings about old timestamps.
  bool fSuppressTimestampWarnings;

  // ________________________________________________
  // Variables for offline analysis
  int fMaxEvents;

  // The TApplication...
  TApplication *fApp;

  // Should we automatically create a Main Window?
  bool fCreateMainWindow;

  // Use a batch mode.
  bool fUseBatchMode;

  


};
#endif
