// Rootana includes
#include "TRootanaEventLoop.hxx"
#ifdef HAVE_MIDAS
#include "TMidasOnline.h"
#endif
#ifdef HAVE_ROOT_XML
#include "XmlOdb.h"
#endif
#ifdef OLD_SERVER
#include "midasServer.h"
#endif
#ifdef HAVE_LIBNETDIRECTORY
#include "netDirectoryServer.h"
#endif
#ifdef HAVE_THTTP_SERVER
#include "THttpServer.h"
#endif
#include "TPeriodicClass.hxx"
#include "MainWindow.hxx"

// ROOT includes.
#include <TSystem.h>
#include <TROOT.h>
#include <TH1D.h>

#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#include "TMidasFile.h"

#include "sys/time.h"
/// Little function for printing the number of processed events and processing rate.
struct timeval raLastTime;  
int raTotalEventsProcessed = 0;
int raTotalEventsSkippedForAge = 0;

// Use only recent data (less than 1 second old) when processing online
bool gUseOnlyRecent;

void PrintCurrentStats(){

  if((raTotalEventsProcessed%5000)==0){
    if(raTotalEventsProcessed==0){
      gettimeofday(&raLastTime, NULL);
    }else{

      struct timeval nowTime;  
      gettimeofday(&nowTime, NULL);
      
      double dtime = nowTime.tv_sec - raLastTime.tv_sec + (nowTime.tv_usec - raLastTime.tv_usec)/1000000.0;
      double rate = 0;
      if (dtime !=0)
        rate = 5.0/(dtime);
      printf("Processed %d events.  Analysis rate = %6.3fkHz. \n",raTotalEventsProcessed,rate);
      gettimeofday(&raLastTime, NULL);

      if(gUseOnlyRecent){
        printf("Skipped %i events that were too old (>1sec old) out of %i events\n",
               raTotalEventsSkippedForAge, raTotalEventsSkippedForAge+raTotalEventsProcessed);
      }
    }
  }
  
  raTotalEventsProcessed++;

}
 



TRootanaEventLoop* TRootanaEventLoop::fTRootanaEventLoop = NULL;

TRootanaEventLoop& TRootanaEventLoop::Get(void) {
  
  if(!fTRootanaEventLoop){
    std::cerr << "Singleton Not Instantiated! " 
	      << " Need to call something like SomeClass::CreateSingleton<SomeClass>(); Exiting!"
	      <<std::endl; exit(0);
  }
  return *fTRootanaEventLoop;
}
  
// ROOT THttpServer... leave as global variable, so others don't need to know about this.
#ifdef HAVE_THTTP_SERVER
  THttpServer* gRoot_http_serv = 0;
#endif


TRootanaEventLoop::TRootanaEventLoop (){

  fOutputFile = 0;
  fOutputFilename = std::string("out/rootfiles/root");
  fDisableRootOutput = false;
  fODB = 0;
  fOnlineHistDir = 0;
  fMaxEvents = 0;
  fCurrentRunNumber = 0;
  fIsOffline = true;

  fCreateMainWindow = true;
  fUseBatchMode = false;
  fSuppressTimestampWarnings = false;    

  gUseOnlyRecent = false;

  fBufferName = std::string("SYSTEM");
  fOnlineName = std::string("rootana");

  fDataContainer = new TDataContainer();

  /// Create the TApplication
  char **argv2 = NULL;
  fApp = new TApplication("rootana", 0, argv2);
}

TRootanaEventLoop::~TRootanaEventLoop (){

  if(fODB) delete fODB;
  CloseRootFile();

}


void TRootanaEventLoop::Initialize(void){};
  
void TRootanaEventLoop::BeginRun(int transition,int run,int time){};

void TRootanaEventLoop::EndRun(int transition,int run,int time){};
  
void TRootanaEventLoop::Finalize(){};

void TRootanaEventLoop::Usage(void){};
void TRootanaEventLoop::UsageRAD(void){};
  

bool TRootanaEventLoop::CheckOption(std::string option){return false;}
bool TRootanaEventLoop::CheckOptionRAD(std::string option){return false;}


bool TRootanaEventLoop::CheckEventID(int eventId){

  // If we didn't specify list of accepted IDs, then accept all.
  if(fProcessEventIDs.size()==0) return true;

  // Otherwise check event ID against list
  for(unsigned int i = 0; i < fProcessEventIDs.size(); i++){
    if(fProcessEventIDs[i] == (eventId & 0xFFFF))
      return true;
  }
  
  return false;
}

void TRootanaEventLoop::SetTHttpServerReadWrite(bool readwrite){ 
#ifdef HAVE_THTTP_SERVER
  if(gRoot_http_serv) gRoot_http_serv->SetReadOnly(!readwrite);
#endif
}

void TRootanaEventLoop::PrintHelp(){

  printf("\nUsage:\n");
  printf("\n./analyzer.exe [-h] [-Hhostname] [-Eexptname] [-eMaxEvents] [-P9091] [-p9090] [-m] [file1 file2 ...]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t-T: test mode - start and serve a test histogram\n");
  printf("\t-D: Become a daemon\n");
  printf("\t-Hhostname: connect to MIDAS experiment on given host\n");
  printf("\t-Eexptname: connect to this MIDAS experiment\n");
  printf("\t-bbuffer: connect to this MIDAS buffer\n");
  printf("\t-P: Start the TNetDirectory server on specified tcp port (for use with roody -Plocalhost:9091)\n");
  printf("\t-p: Start the old midas histogram server on specified tcp port (for use with roody -Hlocalhost:9090)\n");
#ifdef HAVE_THTTP_SERVER
  printf("\t-r: Start THttpServer on specified tcp port\n");
#endif
  printf("\t-eXXX: Number of events XXX to read from input data files\n");
  //printf("\t-m: Enable memory leak debugging\n");
  UsageRAD();  // Print description of TRootanaDisplay options.
  Usage();  // Print description of user options.
  printf("\n");
  printf("Example1: analyze online data: ./analyzer.exe -P9091\n");
  printf("Example2: analyze existing data: ./analyzer.exe /data/alpha/current/run00500.mid\n");

  exit(1);
}

// Stolen from MIDAS...
// returns 1 = success, 0 = failure
// copied almost completely from MIDAS system.c
int ss_daemon_init(){

  bool keep_stdout = false;
#ifdef OS_LINUX

   /* only implemented for UNIX */
   int i, fd, pid;

   if ((pid = fork()) < 0)
      return 0;
   else if (pid != 0)
      exit(0);                  /* parent finished */

   /* child continues here */

   /* try and use up stdin, stdout and stderr, so other
      routines writing to stdout etc won't cause havoc. Copied from smbd */
   for (i = 0; i < 3; i++) {
      close(i);
      fd = open("/dev/null", O_RDWR, 0);
      if (fd < 0)
         fd = open("/dev/null", O_WRONLY, 0);
      if (fd < 0) {
	std::cout << "Can't open /dev/null" << std::endl;
         return 0;
      }
      if (fd != i) {
	std::cout << "Did not get file descriptor" << std::endl;
	return 0;
      }
   }

   setsid();                    /* become session leader */

#endif

   return 1;
}


int TRootanaEventLoop::ExecuteLoop(int argc, char *argv[]){
  
  setbuf(stdout,NULL);
  setbuf(stderr,NULL);
  
  signal(SIGILL,  SIG_DFL);
  signal(SIGBUS,  SIG_DFL);
  signal(SIGSEGV, SIG_DFL);

  std::vector<std::string> args;
  for (int i=0; i<argc; i++)
    {
      if (strcmp(argv[i],"-h")==0)
	PrintHelp(); // does not return
      args.push_back(argv[i]);
    }
  
  
  if(fUseBatchMode){ // Disable creating extra window if batch mode requested.
    gROOT->SetBatch();
    fCreateMainWindow = false;
  }
    
  if(gROOT->IsBatch() && !fUseBatchMode) {
    printf("Cannot run in batch mode\n");
    return 1;
  }

  bool testMode = false;
  bool daemonMode = false;
  int  tcpPort = 0;
#ifdef HAVE_THTTP_SERVER
  int  rhttpdPort = 0; // ROOT THttpServer port
#endif
  const char* hostname = NULL;
  const char* exptname = NULL;
  
  for (unsigned int i=1; i<args.size(); i++) // loop over the commandline options
    {
      const char* arg = args[i].c_str();
      //printf("argv[%d] is %s\n",i,arg);
      
      if (strncmp(arg,"-e",2)==0)  // Event cutoff flag (only applicable in offline mode)
	fMaxEvents = atoi(arg+2);
      else if (strncmp(arg,"-m",2)==0) // Enable memory debugging
	;//	 gEnableShowMem = true;
      else if (strncmp(arg,"-P",2)==0) // Set the histogram server port
	tcpPort = atoi(arg+2);
#ifdef HAVE_THTTP_SERVER
      else if (strncmp(arg,"-r",2)==0) // Set the THttpdServer port
	rhttpdPort = atoi(arg+2);
#endif
      else if (strcmp(arg,"-T")==0)
	testMode = true;
      else if (strcmp(arg,"-D")==0)
	daemonMode = true;
      else if (strncmp(arg,"-H",2)==0)
	hostname = strdup(arg+2);
      else if (strncmp(arg,"-E",2)==0)
	exptname = strdup(arg+2);
      else if (strncmp(arg,"-b",2)==0){
	fBufferName = std::string(arg+2);        
      }else if (strcmp(arg,"-h")==0)
	PrintHelp(); // does not return
      else if(arg[0] == '-')// Check if a TRootanaDisplay or user-defined options
        if(!CheckOptionRAD(args[i]))
          if(!CheckOption(args[i]))
            PrintHelp(); // does not return
    }
    
  // Do quick check if we are processing online or offline.
  // Want to know before we initialize.
  fIsOffline = false;  
  for (unsigned int i=1; i<args.size(); i++){
    const char* arg = args[i].c_str();
    if (arg[0] != '-')  
      {  
	fIsOffline = true;
      }
  }

  if (daemonMode) {
    printf("\nBecoming a daemon...\n");
    ss_daemon_init();
  }

  MainWindow *mainWindow=0;
  if(fCreateMainWindow){
    std::cout << "Create main window! " << std::endl;
    mainWindow = new MainWindow(gClient->GetRoot(), 200, 300);
  }

   gROOT->cd();
   fOnlineHistDir = new TDirectory("rootana", "rootana online plots");

#ifdef HAVE_LIBNETDIRECTORY
   if (tcpPort)
     StartNetDirectoryServer(tcpPort, fOnlineHistDir);
#else
   if (tcpPort)
     fprintf(stderr,"ERROR: No support for the TNetDirectory server!\n");
#endif

#ifdef HAVE_THTTP_SERVER
   if(rhttpdPort){
     char address[100];
     sprintf(address,"http:%i",rhttpdPort); 
     gRoot_http_serv = new THttpServer(address);
   }
#endif
  
   // Initialize the event loop with user initialization.
   Initialize();

   // Initialize the event loop with rootana display initialization.
   InitializeRAD();


   for (unsigned int i=1; i<args.size(); i++){
     const char* arg = args[i].c_str();
     if (arg[0] != '-')  
       {  
	   ProcessMidasFile(fApp,arg);
       }
   }

   if (testMode){
     std::cout << "Entering test mode." << std::endl;
     fOnlineHistDir->cd();
     TH1D* hh = new TH1D("test", "test", 100, 0, 100);
     hh->Fill(1);
     hh->Fill(10);
     hh->Fill(50);
     
     fApp->Run(kTRUE);
     if(fCreateMainWindow) delete mainWindow;
     return 0;
   }

   // if we processed some data files,
   // do not go into online mode.
   if (fIsOffline){
     if(fCreateMainWindow) delete mainWindow;
     return 0;
   }
 
#ifdef HAVE_MIDAS
   ProcessMidasOnline(fApp, hostname, exptname);;
#endif
   
   if(fCreateMainWindow) delete mainWindow;
   
   Finalize();
   
   return 0;
  
}



int TRootanaEventLoop::ProcessMidasFile(TApplication*app,const char*fname)
{
  TMidasFile f;
  bool tryOpen = f.Open(fname);

  if (!tryOpen){
    printf("Cannot open input file \"%s\"\n",fname);
    return -1;
  }

  // This parameter is irrelevant for offline processing.
  gUseOnlyRecent = false;

  int i=0;
  while (1)
    {
      TMidasEvent event;
      if (!f.Read(&event))
	break;
      
      /// Treat the begin run and end run events differently.
      int eventId = event.GetEventId();

      

      if ((eventId & 0xFFFF) == 0x8000){// begin run event
	
        event.Print();
	
        // Load ODB contents from the ODB XML file
        if (fODB) delete fODB;
#ifdef HAVE_ROOT_XML
        fODB = new XmlOdb(event.GetData(),event.GetDataSize());
#endif
        fCurrentRunNumber = event.GetSerialNumber();
        OpenRootFile(fCurrentRunNumber,fname);
        BeginRun(0,event.GetSerialNumber(),0);
        BeginRunRAD(0,event.GetSerialNumber(),0);
        raTotalEventsProcessed = 0;
        raTotalEventsSkippedForAge = 0;
	
      } else if ((eventId & 0xFFFF) == 0x8001){// end run event
	  
				event.Print();
        //EndRun(0,fCurrentRunNumber,0);
	

      } else if ((eventId & 0xFFFF) == 0x8002){

        event.Print(); 
        printf("Log message: %s\n", event.GetData()); 

      }else if(CheckEventID(eventId)){ // all other events; check that this event ID should be processed.

        // Set the bank list for midas event.
        event.SetBankList();
        
        // Set the midas event pointer in the physics event.
        fDataContainer->SetMidasEventPointer(event);
        
        //ProcessEvent if prefilter is satisfied...
				if(PreFilter(*fDataContainer))
					 ProcessMidasEvent(*fDataContainer);
        
        // Cleanup the information for this event.
        fDataContainer->CleanupEvent();
        
      }
 
      PrintCurrentStats();

      // Check if we have processed desired number of events.
      i++;
      if ((fMaxEvents!=0)&&(i>=fMaxEvents)){
	printf("Reached event %d, exiting loop.\n",i);
	break;
      }
    }
  
  f.Close(); 

  EndRunRAD(0,fCurrentRunNumber,0);
  EndRun(0,fCurrentRunNumber,0);
  CloseRootFile();  

  // start the ROOT GUI event loop
  //  app->Run(kTRUE);

  return 0;
}

void TRootanaEventLoop::UseOnlyRecent(bool setting){ 

  gUseOnlyRecent = setting;
};


void TRootanaEventLoop::OpenRootFile(int run, std::string midasFilename){

  if(fDisableRootOutput) return;

  if(fOutputFile) {
    fOutputFile->Write();
    fOutputFile->Close();
    fOutputFile=0;
  }  

  char filename[1024];
	// This is the default filename, using fOutputFilename
	sprintf(filename, "%s%08d.root",fOutputFilename.c_str(), run);

	// See if user has implemented a function where they specify 
	// the root file name using the midas file name...
	// Only works offline, because we need midas file name
	if(midasFilename.compare("") != 0){
		std::string fullname = SetFullOutputFileName(run,midasFilename);
		if(fullname.compare("") != 0){
			sprintf(filename, "%s",fullname.c_str());
		}
	}
	
  fOutputFile = new TFile(filename,"RECREATE");
  std::cout << "Opened output file with name : " << filename << std::endl;


#ifdef HAVE_LIBNETDIRECTORY
  NetDirectoryExport(fOutputFile, "outputFile");
#endif
}


void TRootanaEventLoop::CloseRootFile(){

  if(fOutputFile) {
		std::cout << "Closing ROOT file " << std::endl;
    fOutputFile->Write();
    fOutputFile->Close();
    fOutputFile=0;
  } 

}



/// _________________________________________________________________________
/// _________________________________________________________________________
/// _________________________________________________________________________
/// The following code is only applicable for online MIDAS programs

#ifdef HAVE_MIDAS

// This global variable allows us to keep track of whether we are already in the process
// of analyzing a particular event. 
static bool onlineEventLock = false;


// number of events consecutively skipped.
int numberConsSkipped=0;
double nextWarn = 1.0;

// number of events with late (>10sec old) timestamp.
int numberOldTimestamps=0;
double nextWarnTimestamps = 1.0;
bool disableOnlyRecentMode = false;
struct timeval lastTimeProcessed;  


/// We need to use a regular function, so that it can be passed 
/// to the TMidasOnline event handler.  This function calles the 
/// event loop singleton, allowing the user to add their own function code. 
void onlineEventHandler(const void*pheader,const void*pdata,int size)
{

  // If we are already processing a previous event, then just dump this one.
  // !!!!!!!!!!! This is adding a potential dangerous race condition!!!!!
  // !!!!!!!!!!! Need to think hard if this is safe!!!!!!!!!!!!!!!!!!!!!!
  if(onlineEventLock) return;
  onlineEventLock = true;

  // Do a check; if the we are using output files, but the output file is null
  // then dump the event.  This will usually occur if we try to process additional
  // events after the end of the run.  Trying to fill a histogram will result in
  // seg-faults, since the histograms will have been deleted when the last ROOT file
  // was closed.
  if(TRootanaEventLoop::Get().IsRootOutputEnabled() 
     && !TRootanaEventLoop::Get().IsRootFileValid()){

    numberConsSkipped++;
    if(numberConsSkipped >= nextWarn){
      printf("onlineEventHandler Warning:  Output ROOT file is not validly open, so can't fill histograms. Have skipped %i events now.\n",
             numberConsSkipped);
      nextWarn *= 3.16227;
    }
    
    onlineEventLock = false;
    return;
  }
  numberConsSkipped = 0;
  nextWarn = 1.0;

  // If user asked for only recent events, throw out any events
  // that are more than 1 second old.
  if(gUseOnlyRecent && !disableOnlyRecentMode){
    TMidas_EVENT_HEADER *header = (TMidas_EVENT_HEADER*)pheader;

    struct timeval now;  
    gettimeofday(&now, NULL);
    if(header->fTimeStamp < now.tv_sec - 1){      
      
      // We need to add a check here: if you are connecting to MIDAS data remotely 
      // and the network link is being saturated, you will never get the most recent
      // data and so will never have any data.
      // This seems worse than getting old data, so disable these GET_RECENT checks if you haven't gotten
      // any new data for 5 seconds.
      if(lastTimeProcessed.tv_sec < now.tv_sec - 5 && raTotalEventsProcessed > 0){
        printf("You are running in 'Only Recent Data' mode, but you haven't gotten any new data in more than 5 seconds.\n");
        printf("Disabling 'Only Recent Data' mode for this run.\n");
        disableOnlyRecentMode = true;
      }

      raTotalEventsSkippedForAge++;
      onlineEventLock = false;
      return;
      
    }
  }

  // Make a MIDAS event.
  TMidasEvent event;
  memcpy(event.GetEventHeader(), pheader, sizeof(TMidas_EVENT_HEADER));
  event.SetData(size, (char*)pdata);
  event.SetBankList();
  


  // Make sure that this is an event that we actually want to process.
  if(!TRootanaEventLoop::Get().CheckEventID(event.GetEventId())){
    onlineEventLock = false;
    return;
  }

  /// Set the midas event pointer in the physics event.
  TRootanaEventLoop::Get().GetDataContainer()->SetMidasEventPointer(event);

  // Now pass this to the user event function, if pre-filter is satisfied
  if(TRootanaEventLoop::Get().PreFilter(*TRootanaEventLoop::Get().GetDataContainer())){		
    TRootanaEventLoop::Get().ProcessMidasEvent(*TRootanaEventLoop::Get().GetDataContainer());
  }

  gettimeofday(&lastTimeProcessed,NULL);
  PrintCurrentStats();

  // Cleanup the information for this event.
  TRootanaEventLoop::Get().GetDataContainer()->CleanupEvent();


  // Do another check.  If the event timestamp is more than 10 sec older than the current timestamp,
  // then the analyzer is probably falling behind the data taking.  Warn user.
  if(!TRootanaEventLoop::Get().GetSuppressTimestampWarnings()){
    struct timeval now;  
    gettimeofday(&now, NULL);
    if(event.GetTimeStamp() < now.tv_sec - 10){      
      numberOldTimestamps++;
      if(numberOldTimestamps >= nextWarnTimestamps){
        printf("onlineEventHandler Warning: the time for this bank (%i) is more than 10 sec older \nthan current time (%i). Has happenned %i times now.",       
               event.GetTimeStamp(),(int) now.tv_sec,numberOldTimestamps);
        printf("Either the analyzer is falling behind the data taking \n(try modifying the fraction of events plotted) or times on different computers are not synchronized.\n");  
        
        int buffer_level = TMidasOnline::instance()->getBufferLevel();
        int buffer_size = TMidasOnline::instance()->getBufferSize();
        printf("Buffer level = %i bytes out of %i bytes maximum \n\n",buffer_level,buffer_size);        
        nextWarnTimestamps *= 3.16227;
      }      
    }
  }

  onlineEventLock = false;
}


void onlineBeginRunHandler(int transition,int run,int time)
{
  TRootanaEventLoop::Get().OpenRootFile(run);
  TRootanaEventLoop::Get().SetCurrentRunNumber(run);
  TRootanaEventLoop::Get().BeginRun(transition,run,time);
  TRootanaEventLoop::Get().BeginRunRAD(transition,run,time);
  raTotalEventsProcessed = 0;
  raTotalEventsSkippedForAge = 0;
  numberOldTimestamps = 0;
  nextWarnTimestamps = 1.0;
  gettimeofday(&raLastTime, NULL);
  disableOnlyRecentMode = false;
}

void onlineEndRunHandler(int transition,int run,int time)
{
  TRootanaEventLoop::Get().SetCurrentRunNumber(run);
  TRootanaEventLoop::Get().EndRunRAD(transition,run,time);
  TRootanaEventLoop::Get().EndRun(transition,run,time);
  TRootanaEventLoop::Get().CloseRootFile();
}


void MidasPollHandlerLocal()
{

  if (!(TMidasOnline::instance()->poll(0)))
    gSystem->ExitLoop();
}

int TRootanaEventLoop::ProcessMidasOnline(TApplication*app, const char* hostname, const char* exptname)
{
   TMidasOnline *midas = TMidasOnline::instance();

   int err = midas->connect(hostname, exptname, fOnlineName.c_str());
   if (err != 0)
     {
       fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
       return -1;
     }

   fODB = midas;

   /* fill present run parameters */

   fCurrentRunNumber = fODB->odbReadInt("/runinfo/Run number");

   //   if ((fODB->odbReadInt("/runinfo/State") == 3))
   //startRun(0,gRunNumber,0);
   OpenRootFile(fCurrentRunNumber);
   BeginRun(0,fCurrentRunNumber,0);
   BeginRunRAD(0,fCurrentRunNumber,0);

   // Register begin and end run handlers.
   midas->setTransitionHandlers(onlineBeginRunHandler,onlineEndRunHandler,NULL,NULL);
   midas->registerTransitions();

   /* reqister event requests */
   midas->setEventHandler(onlineEventHandler);

   // 2015-02-12: this doesn't seem to work, at least not when looking 
   // at remote mserver.
   // use different options if user requested only recent data.
   //if(fUseOnlyRecent){
   //midas->eventRequest(fBufferName.c_str(),-1,-1,(1<<2));  
   //}else{
   midas->eventRequest(fBufferName.c_str(),-1,-1,(1<<1)); 
   //}
   
   if(gUseOnlyRecent){
     std::cout << "Using 'Only Recent Data' mode; all events more than 1 second old will be discarded." << std::endl;
   }
   
   // printf("Startup: run %d, is running: %d, is pedestals run: %d\n",gRunNumber,gIsRunning,gIsPedestalsRun);
   
   TPeriodicClass tm(100,MidasPollHandlerLocal);

   /*---- start main loop ----*/

   //loop_online();
   app->Run(kTRUE); // kTRUE means return to here after finished with online processing... this ensures that we can disconnect.
   
   // Call user-defined EndRun and close the ROOT file.
   EndRunRAD(0,fCurrentRunNumber,0);
   EndRun(0,fCurrentRunNumber,0);
   CloseRootFile();  

   /* disconnect from experiment */
   midas->disconnect();

   return 0;
}

#endif
