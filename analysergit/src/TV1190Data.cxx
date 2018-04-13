#include "TV1190Data.hxx"

#include <iostream>

void TDCMeasurement::SetTrailer(uint32_t trailer){  
  tdc_trailer_word = trailer;
}


/// Get the TDC number
uint32_t TDCMeasurement::GetTDCNumber() const {
  if(HasTDCHeader())
    return ((tdc_header_word & 0x3000000) >> 24 );
  if(HasTDCTrailer())
    return ((tdc_trailer_word & 0x3000000) >> 24 );
  return 0xdeadbeef;  
}

/// Get Event ID
uint32_t TDCMeasurement::GetEventID() const {
  if(HasTDCHeader())
    return ((tdc_header_word & 0xfff000) >> 12 );
  if(HasTDCTrailer())
    return ((tdc_trailer_word & 0xfff000) >> 12 );
  return 0xdeadbeef;
}

/// Get Bunch ID
uint32_t TDCMeasurement::GetBunchID() const {
  if(HasTDCHeader())
    return ((tdc_header_word & 0xfff));
  return 0xdeadbeef;  
}

  /// Get Errors
uint32_t TDCMeasurement::GetErrors() const{
  if(HasTDCErrorWord())
    return ((tdc_error_error & 0x7fff));
  return 0xdeadbeef;  
  

}

TV1190Data::TV1190Data(int bklen, int bktype, const char* name, void *pdata):
    TGenericData(bklen, bktype, name, pdata)
{
  
  // Do decoding.  Decoding is complicated by the fact that there can be 
  // multiple events in the same bank.  So need to find and save multiple
  // event headers, trailers.


  // Do some sanity checking.  
  // Make sure first word has right identifier
//  if( (GetData32()[0] & 0xf8000000) != 0x40000000) 
  if( (GetData32()[0] & 0xf8000000) != 0xf8000000) // This seems to be the correct identifier - GO 
    std::cerr << "First word has wrong identifier; first word = 0x" 
	      << std::hex << GetData32()[0] << std::dec << std::endl;

  // Scan through the data, saving information as we go.
  int found_trailer_word = -1;
  //  fExtendedTriggerTimeTag = -1;

  uint32_t current_tdc_header =0;
  int index_for_trailer = 0;
  int index_for_error = 0;
  int numberEventsInBank = 0;
  fWordCountTotal =0;
  for(int i = 0; i < GetSize(); i++){
    uint32_t word = GetData32()[i];
    //std::cout << "0x"<<std::hex << word << std::dec << std::endl;

  // LP
  if( (GetData32()[i]>>27) == 0x1f){
    continue;
  }
  
    // Header
    if( (word & 0xf8000000) == 0x40000000){
      fGlobalHeader.push_back(word);
      numberEventsInBank++;
    }

    // TDC header
    if( (word & 0xf8000000) == 0x08000000){
      current_tdc_header = word;
    }
    // TDC measurement
    if( (word & 0xf8000000) == 0x00000000){
      fMeasurements.push_back(TDCMeasurement(current_tdc_header,word,numberEventsInBank-1));
    }
    
    // TDC trailer
    if( (word & 0xf8000000) == 0x18000000){
      // Set the TDC trailer word for all the measurements since the last
      // TDC trailer.
      for(unsigned int i = index_for_trailer; i < fMeasurements.size(); i++){
	fMeasurements[i].SetTrailer(word);
      }
      index_for_trailer = fMeasurements.size();      
    }

    // TDC error
    if( (word & 0xf8000000) == 0x20000000){
      // Set the TDC error word for all the measurements since the last
      // TDC error.
      std::cout << "Error word: " << std::hex << "0x"<<word<< std::dec << std::endl;
      for(unsigned int i = index_for_trailer; i < fMeasurements.size(); i++){
	fMeasurements[i].SetErrors(word);
      }
      index_for_error = fMeasurements.size();    
    }


    // Extended Trigger Time Tag
    if( (word & 0xf8000000) == 0x88000000) fExtendedTriggerTimeTag.push_back((int)(word & 0x07ffffff));

    // Found global trailer
    if( (word & 0xf8000000) == 0x80000000){ // Found trailer
      found_trailer_word = i;
      fWordCountTotal += (word & 0x0001fffe0) >> 5;      
      fStatus.push_back( (word & 0x07000000) >> 24);
    }
 
  }

  if(found_trailer_word == -1){
    std::cerr << "Error in decoding V1190 data; didn't find trailer." << std::endl;
    std::cerr << "Bank dump: " << std::endl;
    for(int i = 0; i < GetSize(); i++){
      uint32_t word = GetData32()[i];
      std::cout << "0x"<<std::hex << word << std::dec << std::endl;
    }
  }
  if(found_trailer_word != GetSize()-1)
    std::cerr << "Error; did not find trailer on last word of bank. trailer word = " << found_trailer_word
	      << " last word = " << GetSize()-1 << std::endl;

//  if(fWordCountTotal != GetSize()){
  if(fWordCountTotal != GetSize()-1){ // idk what the deal is with the bank size here but it fixes it - GO
    std::cerr << "Error in decoding V1190 data; word count in all trailers ("<<fWordCountTotal 
	      << ") doesn't match bank size ("<< GetSize()-1 << ")." << std::endl;
  }


}

void TV1190Data::Print(){

  std::cout << "V1190 decoder for bank " << GetName().c_str() << std::endl;
  std::cout << "event counter = " << GetEventCounter() << ", geographic address: " <<  GetGeoAddress() << std::endl;
  std::cout << "Number of events in this bank: " << GetEventsInBank() << std::endl;
  for(unsigned int i = 0; i < fMeasurements.size(); i++){
    std::cout << "Measurement: " <<    fMeasurements[i].GetMeasurement() << " for tdc/chan "  <<
      fMeasurements[i].GetTDCNumber() << "/"<< fMeasurements[i].GetChannel();
    if(fMeasurements[i].IsLeading())
      std::cout << " (leading edge meas)";
    if(fMeasurements[i].IsTrailing())
      std::cout << " (trailing edge meas)";

    std::cout << "[event_id = " << fMeasurements[i].GetEventID() << ",bunch_id=" 
	      << fMeasurements[i].GetBunchID()<< "]" << "Event index=" << fMeasurements[i].GetEventIndex();
    if(fMeasurements[i].HasTDCErrorWord())
      std::cout << "[errors=0x"<< std::hex << fMeasurements[i].GetErrors() << std::dec << "]";
    std::cout << std::endl;
  }


}

