/*! 
  \file v1190.hh
  \brief interface class for easy readout and control of CAEN V1190(B) TDCs
  \author Markus Fleck <markus.fleck@oeaw.ac.at>
  \date 15.05.2017
  \version 0.1
*/ 

#ifndef __V1190_CC__
#define __V1190_CC__

#include "midas.h"

#include "mvmestd.h"
extern "C" {
#include "v1190B.h"
}

class v1190 {
public:
  v1190(unsigned int vmeBaseAdr,MVME_INTERFACE *vmeInt, HNDLE hODB) 
    : vmeBaseAdress(vmeBaseAdr), vme(vmeInt), ODB(hODB),
      mess(vme,&settings,vmeBaseAdr, &haveTables) {}

  // initialise everything
  bool init();
  
  // setup device
  bool setupV1190();
  
  // setup midas ODB
  bool setupV1190ODB();
  
  //! check if the module is still responding
  bool checkModuleResponse();

  //! read data into midas banks
  bool read(void *pevent);
  
  //! start acquistion
  inline void start()  const { v1190_SoftClear(vme, vmeBaseAdress);              }

private:
  v1190(){}

  inline int getEventSize() const {return v1190_EvtStored(vme, vmeBaseAdress);}
  inline bool checkEventStatus() const {return (v1190_DataReady(vme, vmeBaseAdress);}

  static void update(INT hDB, INT hkey, void *dump);
    
  // VME base adress A32
  unsigned int vmeBaseAdress;
  
  // VME handle
  MVME_INTERFACE *vme;
  
  // MIDAS ODB handle
  HNDLE ODB;

  // number of the TDCs (range 0-15)
  int tdcNum;

  // MIDAS ODB settings
  typedef struct {
    INT       offset;
    INT       width;
    INT       reject;
    INT       extsearch;
//    INT       trigger_time_sub;
    INT       edge_leading;
    INT       edge_trailing;
  } FTDC_SETTINGS;
  FTDC_SETTINGS settings;
  class messenger {
  public:
    messenger(MVME_INTERFACE *mvme, FTDC_SETTINGS *set, unsigned int vmeBaseAdr) 
      : vme(mvme), settings(set), vmeBaseAdress(vmeBaseAdr) {}
    ~messenger(){vme = NULL; settings = NULL;}
    MVME_INTERFACE *vme;
    FTDC_SETTINGS *settings;
    unsigned int vmeBaseAdress;
    messenger(){}    
  } mess;
  
};

#define FTDC_SETTINGS_STR(_name) const char *_name[] = {	\
    "[.]",							        \
    "offset = INT : 400",					\
    "width = INT : 800",					\
    "reject = INT : 0",					    \
    "extsearch = INT : 0",					\
    "edge_leading = INT : 1",				\
    "edge_trailing = INT : 0",				\
    "",								        \
    NULL }

#endif
