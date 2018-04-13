#include "TV792Data.hxx"


/// Constructor
TV792Data::TV792Data(int bklen, int bktype, const char* name, void *pdata):
  TGenericData(bklen, bktype, name, pdata)
{
  
  fAdc_header_word = 0;
  fAdc_trailer_word = 0;

  for(int i = 0; i < GetSize(); i++){
    
    uint32_t word = GetData32()[i];
    if((word & 0x07000000) == 0x02000000){ // header
      fAdc_header_word = word;
    }
    

    if((word & 0x07000000) == 0x00000000){ // measurement
      fMeasurements.push_back(VADCMeasurement(fAdc_header_word,word));
    }

    if((word & 0x07000000) == 0x04000000) // trailer
      fAdc_trailer_word = word;
    
    
  }    

}

void TV792Data::Print(){

  std::cout << "V792 decoder for bank " << GetName().c_str() << std::endl;
  
  std::cout << "Geo-Add = " << GetGeoAddress() << ", crate number =  " << GetCrate() 
	    << ", Number of channels= " << GetNumberChannels() << ", event counter = "
	    << GetEventCounter() << std::endl;
  for(unsigned i = 0; i < fMeasurements.size(); i++){
    std::cout << "chan " << fMeasurements[i].GetChannel() 
	      << " meas = " << fMeasurements[i].GetMeasurement() << " " ;
    std::cout << " [UN/OV=" << fMeasurements[i].IsUnderThreshold() << "/" << fMeasurements[i].IsOverFlow() << "],  ";

    if((i-2)%3 == 0) std::cout << std::endl;

  }
  std::cout << std::endl;



}
