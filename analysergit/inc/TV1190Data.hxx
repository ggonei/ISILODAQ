#ifndef TV1190Data_hxx_seen
#define TV1190Data_hxx_seen

#include <vector>

#include "TGenericData.hxx"

/// Class for each TDC measurement
/// For the definition of obscure variables see the CAEN V1190 manual.
class TDCMeasurement {

  friend class TV1190Data;

public:
  
  /// Is this the leading edge measurement?
  bool IsLeading() const {return ((tdc_measurement_word & 0x4000000) == 0x0000000);}
  /// Is this the trailing edge measurement?
  bool IsTrailing() const {return ((tdc_measurement_word & 0x4000000) == 0x4000000);}
  /// Get the TDC measurement
  //uint32_t GetMeasurement() const {return (tdc_measurement_word & 0x7ffff);}
  uint32_t GetMeasurement() const {return (tdc_measurement_word & 0x1ffff);} // LP

/// Get the TDC number
  uint32_t GetTDCNumber() const;
  /// Get the channel number
  //uint32_t GetChannel() const {return ((tdc_measurement_word & 0x3f80000) >> 19 );}
  uint32_t GetChannel() const {return ((tdc_measurement_word >> 21 ) & 0x1f );}

  /// Get Event ID; this is event number defined by V1190 module
  uint32_t GetEventID() const;

  /// Get Bunch ID
  uint32_t GetBunchID() const;

  /// Get Event Index; this is which event number within the bank.
  uint32_t GetEventIndex() const{return event_index;};

  /// Check if measurement has a TDC header
  bool HasTDCHeader() const {return (tdc_header_word != 0);}
  /// Check if measurement has a TDC trailer
  bool HasTDCTrailer() const {return (tdc_trailer_word != 0);}
  /// Check if measurement has a TDC error word
  bool HasTDCErrorWord() const {return (tdc_error_error != 0);}

  /// Get Errors
  uint32_t GetErrors() const;

private:

  /// Found fields to hold the header, measurement, trailer and error words.
  uint32_t tdc_header_word;
  uint32_t tdc_measurement_word;
  uint32_t tdc_trailer_word;
  uint32_t tdc_error_error;
  int event_index;
  
  /// Constructor; need to pass in header and measurement.
  TDCMeasurement(uint32_t header, uint32_t measurement, int index):
    tdc_header_word(header),
    tdc_measurement_word(measurement),
    tdc_trailer_word(0),tdc_error_error(0),event_index(index){};

  /// Set the trailer word.
  void SetTrailer(uint32_t trailer);

  /// Set the error word.
  void SetErrors(uint32_t error){tdc_error_error = error;}  


  TDCMeasurement();    
};


/// Class to store data from CAEN V1190.
/// We store the information as a vector of TDCMeasurement's
/// Question: do we need a way of retrieving information about TDCs that
/// have no measurements?  Currently this information is not exposed.
/// For the definition of obscure variables see the CAEN V1190 manual.
class TV1190Data: public TGenericData {

public:

  /// Constructor
  TV1190Data(int bklen, int bktype, const char* name, void *pdata);


  /// Get Event Counter
  uint32_t GetEventCounter(int index = 0) const {return (fGlobalHeader[index] & 0x07ffffe0) >> 5;};

  /// Get Geographical Address
  uint32_t GetGeoAddress(int index = 0) const {return (fGlobalHeader[index] & 0x1f) ;};

  /// Get the extended trigger time tag
  int GetExtendedTriggerTimeTag(int index = 0) const {return fExtendedTriggerTimeTag[index];};

  /// Get the word count
  int GetWordCount() const {return fWordCountTotal;};

  // Methods to check the status flags in trailer.
  bool IsTriggerLost(int index = 0) const {
    if(fStatus[index] & 0x4)
      return true;
    else
      return false;
  }  
  bool HasBufferOverflow(int index = 0) const {
    if(fStatus[index] & 0x2)
      return true;
    else
      return false;
  }
  bool HasTDCError(int index = 0) const {
    if(fStatus[index] & 0x1)
      return true;
    else
      return false;
  }

  /// Get the number of events in this bank
  int GetEventsInBank(){return fGlobalHeader.size();};

  void Print();

  /// Get the Vector of TDC Measurements.
  std::vector<TDCMeasurement>& GetMeasurements() {return fMeasurements;}


private:
  
  // We have vectors of the headers/trailers/etc, since there can be 
  // multiple events in a bank.

  /// The overall global header
  std::vector<uint32_t> fGlobalHeader;
  
    
  std::vector<int> fExtendedTriggerTimeTag; // Extended trigger time
  int fWordCountTotal; // Word count

  std::vector<uint32_t> fStatus;


  /// Vector of TDC Measurements.
  std::vector<TDCMeasurement> fMeasurements;

};

#endif

