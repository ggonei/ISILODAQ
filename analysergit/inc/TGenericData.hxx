#ifndef TGenericData_hxx_seen
#define TGenericData_hxx_seen

#include <string>
#include <iostream>
#include <inttypes.h>

/// A generic ABC for storing decoded data banks.
/// Provides methods for accessing unstructured data.
/// INherited classes will provide more user-friendly data access.
class TGenericData{

 public:

  TGenericData(int bklen, int bktype, const char* name, void *pdata):fSize(bklen),
fBankType(bktype), fBankName(name),fData(pdata){
   

  };

  virtual ~TGenericData(){};

  //  GetData16()
  const uint16_t* GetData16() const { return reinterpret_cast<const uint16_t*>(fData); }

  //  GetData32()
  const uint32_t* GetData32() const { return reinterpret_cast<const uint32_t*>(fData); }

  //  GetData64()
  const uint64_t* GetData64() const { return reinterpret_cast<const uint64_t*>(fData); }

  //  GetFloat()
  const float* GetFloat() const { return reinterpret_cast<const float*>(fData); }

  //  GetDouble()
  const double* GetDouble() const { return reinterpret_cast<const double*>(fData); }

  int GetSize() const {return fSize;}

  int GetType() const {return fBankType;}

  std::string GetName() const {return fBankName;}

  /// Dump the bank contents in an unstructured way
  void Dump(){

    std::cout << "Generic decoder for bank named " <<  GetName().c_str() << std::endl;
    for(int i = 0; i < GetSize(); i++){
      std::cout << std::hex << "0x" << GetData32()[i] << std::dec << std::endl;
    }

  }

  /// Print the bank contents in a structured way.
  virtual void Print(){
    Dump();
  }
  
  private:

  /// Size of the bank (in what units?)
  int fSize;

  /// Bank data type (MIDAS TID_xxx).
  int fBankType;

  /// Bank name
  std::string fBankName;

  /// Pointer to the unstructured data.
  /// The data itself is NOT owned by TGenericData.
  void *fData;


};

#endif
