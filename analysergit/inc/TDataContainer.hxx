#ifndef TDataContainer_hxx_seen
#define TDataContainer_hxx_seen

#include <stdio.h>
#include <vector>
#include <exception>

#include "TMidasEvent.h"
#include "TGenericData.hxx"
#include <typeinfo>

///
class failed_midas_bank_cast: public std::exception
{
  virtual const char* what() const throw()
  {
    return "Incorrect bank cast";
  }
};


/// This class is what will get passed back to users for each midas event.
/// It will contain a pointer to the midas event, which the user can access.
/// It will also contain routines that the user can call to access decoded
/// versions of the midas banks.
/// Generally the decoded information will be cleaned out at the end of each 
/// event; but there will be options to allow this information to persist.
class TDataContainer 
{

 
public:

  TDataContainer():
    fMidasEventPointer(0), fOwnMidasEventMemory(false)
  {}

  TDataContainer(const TDataContainer &dataContainer);

  ~TDataContainer();

  /// Get the MIDAS data for this event, in TMidasEvent format
  TMidasEvent& GetMidasData() const;
  TMidasEvent& GetMidasEvent() const{ return GetMidasData(); }


  /// Add a templated function that returns event data in the format that we want.
  template <typename T> T* GetEventData(const char* name){
      
    // Try to find a cached version of this bank
    for(unsigned int ibank = 0; ibank < fEventDataList.size(); ibank++){
      if(fEventDataList[ibank]->GetName().compare(name) == 0){
	// Found bank.  Now check it has correct type.
	T* cast_bank = dynamic_cast<T*>(fEventDataList[ibank]);
	// Throw exception if cached bank is of different type.
	if(!cast_bank){
	  std::cout << "TMidasEvent::GetMidasBank: ERROR: you requested bank with name=" << name << std::endl 
		    << "A cached version of this bank (of type " << typeid(fEventDataList[ibank]).name()
		    << ") already exists in event; cannot create bank class of new type "
		    << typeid(T).name()<<std::endl;
	  throw failed_midas_bank_cast();
	}
	return cast_bank;
      }

    }

    void *ptr;
    int bklen,bktype;
    int status = GetMidasData().FindBank(name, &bklen, &bktype, &ptr);

    /// If we couldn't find bank, return null.
    if(status == 0) return 0;
   
    T *bank = new T(bklen,bktype,name, ptr);

     // Cache a version of this bank...
    fEventDataList.push_back(bank);
    
    return bank;
  
  }


  /// Method to clean up the current events list of event data.
  /// Generally this will delete all the existing information,
  /// but this behaviour may be modified in some cases.
  void CleanupEvent();
  
  /// This is the ugly function where we de-reference to get pointer for a TMidasEvent (ugly!).
  /// In this case TDataContainer does not own the memory referenced by fMidasEventPointer.
  void SetMidasEventPointer(TMidasEvent& event);

private:
    
  /// Pointer to the TMidasEvent;
  /// In some cases we own the memory referenced by pointer; other cases not
  TMidasEvent *fMidasEventPointer;

  /// Do we own the memory pointed to by TMidasEvent pointer?
  bool fOwnMidasEventMemory;
  

  /// For the moment make empty assign operator
  TDataContainer& operator= (const TDataContainer &event);

  /// This is the list of banks associated with this event.
  /// The event owns these banks and needs to take care of 
  /// deleting them.
  std::vector<TGenericData*> fEventDataList;


};


#endif
