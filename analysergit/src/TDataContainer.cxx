#include "TDataContainer.hxx"

#include <exception>


TDataContainer::TDataContainer(const TDataContainer &dataContainer){

  fMidasEventPointer = new TMidasEvent(dataContainer.GetMidasData());
  fOwnMidasEventMemory = true;

}

TDataContainer::~TDataContainer(){

  if(fOwnMidasEventMemory)
    delete fMidasEventPointer;

  for(unsigned int i = 0; i < fEventDataList.size(); i++){
    delete fEventDataList[i];
  }

}
 

// Define exception for reset the pointer to MidasEvent when we still own the pointer for
// the previous one.
class TDataContainerBadPointerReset : public std::exception
{
  virtual const char* what() const throw(){
    return "TDataContainer Exception: trying to reset MidasEventPointer without deleting old memory";
  }  
};

// Define exception for trying to access MidasEvent when it doesn't exist.
class TDataContainerNoValidMidasEvent : public std::exception
{
  virtual const char* what() const throw(){
    return "TDataContainer Exception: physics event does not have a valid midas event pointer";
  }  
};


void TDataContainer::CleanupEvent(){

  for(unsigned int i = 0; i < fEventDataList.size(); i++){
    delete fEventDataList[i];
  }
  fEventDataList.clear();
}


void TDataContainer::SetMidasEventPointer(TMidasEvent& event){

  // If the MidasPointer is still valid and we own the memory associated with it,
  // then we shouldn't be reseting the fMidasEventPointer.
  if(fOwnMidasEventMemory && fMidasEventPointer){
    throw TDataContainerBadPointerReset();
  }

  fMidasEventPointer = &event; // ugly!
  fOwnMidasEventMemory = false;

}


TMidasEvent& TDataContainer::GetMidasData() const{

  // Make sure that point is valid.
  if(fMidasEventPointer==0){
    throw TDataContainerNoValidMidasEvent();
  }

  return *fMidasEventPointer;

}

