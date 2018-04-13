// midasio.h

#ifndef MIDASIO_H
#define MIDASIO_H

#include <string>
#include <vector>

#if 1
#include <stdint.h>
typedef uint16_t u16;
typedef uint32_t u32;
#endif

#ifndef TID_LAST
/**
Data types Definition                         min      max    */
#define TID_BYTE      1       /**< unsigned byte         0       255    */
#define TID_SBYTE     2       /**< signed byte         -128      127    */
#define TID_CHAR      3       /**< single character      0       255    */
#define TID_WORD      4       /**< two bytes             0      65535   */
#define TID_SHORT     5       /**< signed word        -32768    32767   */
#define TID_DWORD     6       /**< four bytes            0      2^32-1  */
#define TID_INT       7       /**< signed dword        -2^31    2^31-1  */
#define TID_BOOL      8       /**< four bytes bool       0        1     */
#define TID_FLOAT     9       /**< 4 Byte float format                  */
#define TID_DOUBLE   10       /**< 8 Byte float format                  */
#define TID_BITFIELD 11       /**< 32 Bits Bitfield      0  111... (32) */
#define TID_STRING   12       /**< zero terminated string               */
#define TID_ARRAY    13       /**< array with unknown contents          */
#define TID_STRUCT   14       /**< structure with fixed length          */
#define TID_KEY      15       /**< key in online database               */
#define TID_LINK     16       /**< link in online database              */
#define TID_LAST     17       /**< end of TID list indicator            */
#endif

class TMBank
{
 public:
   std::string name;        ///< bank name, 4 characters max
   u32         type;        ///< type of data, enum of TID_xxx
   u32         data_size;   ///< total data size in bytes
   u32         data_offset; ///< offset of data for this bank in the event data[] container
};

class TMEvent
{
 public:
   bool error; ///< event has an error - incomplete, truncated, inconsistent or corrupted
   bool found_all_banks; ///< all the banks in the event data have been discovered
   
   u16 event_id;         ///< MIDAS event ID
   u16 trigger_mask;     ///< MIDAS trigger mask
   u32 serial_number;    ///< MIDAS event serial number
   u32 time_stamp;       ///< MIDAS event time stamp (unix time in sec)
   u32 data_size;        ///< MIDAS event data size
   u32 data_offset;      ///< MIDAS event data start in the data[] container

   u32 bank_header_flags; ///< flags from the MIDAS event bank header

   std::vector<TMBank> banks; ///< list of MIDAS banks, fill using FindAllBanks()
   std::vector<char> data;      ///< MIDAS event bytes

   u32 bank_scan_position;    ///< location where scan for MIDAS banks was last stopped

public:
   std::string HeaderToString() const;            ///< print the MIDAS event header
   std::string BankListToString() const;          ///< print the list of MIDAS banks
   std::string BankToString(const TMBank*) const; ///< print definition of one MIDAS bank

   TMEvent(); // ctor
   TMEvent(const void* data, int data_size); // ctor
   void FindAllBanks();                      ///< scan the MIDAS event, find all data banks
   TMBank* FindBank(const char* bank_name);  ///< scan the MIDAS event
   char* GetEventData();                     ///< get pointer to MIDAS event data
   char* GetBankData(const TMBank*);         ///< get pointer to MIDAS data bank
   void DeleteBank(const TMBank*);           ///< delete MIDAS bank
   void AddBank(const char* bank_name, int tid, int num_items, const char* data, int size); ///< add new MIDAS bank
};

class TMReaderInterface
{
 public:
   TMReaderInterface(); // ctor
   virtual int Read(void* buf, int count) = 0;
   virtual int Close() = 0;
   virtual ~TMReaderInterface() {};
 public:
   bool fError;
   std::string fErrorString;
   static bool fgTrace;
};

class TMWriterInterface
{
 public:
   virtual int Write(const void* buf, int count) = 0;
   virtual int Close() = 0;
   virtual ~TMWriterInterface() {};
 public:
   static bool fgTrace;
};

TMReaderInterface* TMNewReader(const char* source);
TMWriterInterface* TMNewWriter(const char* destination);

TMEvent* TMReadEvent(TMReaderInterface* reader);
void TMWriteEvent(TMWriterInterface* writer, const TMEvent* event);

extern bool TMTraceCtorDtor;

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

