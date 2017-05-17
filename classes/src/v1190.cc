/*! 
  \file v1190.cc
  \brief CAEN V1190/V1290 source code
  \author Markus Fleck <markus.fleck@oeaw.ac.at>  
  \date 16.05.2017
  \version 0.1
*/ 


#include"v1190.hh"
#include<iostream>
#include <time.h>

bool v1190::init(){
  if(!checkModuleResponse()) return false;

  bool retVal = true;
  retVal &= setupV1190();
  retVal &= setupV1190ODB();
  
  return retVal;
}

bool v1190::setupV1190(){

  bool retVal = true;

  WORD code, value;
  int cmode, status = -1;
 
  mvme_get_dmode(vme, &cmode);
  mvme_set_dmode(vme, MVME_DMODE_D16);

  // perform board reset
  v1190_SoftReset(vme, vmeBaseAdress);

  // standard setup
  
  code = 0x0000;  // Trigger matching Flag
  if ((status = v1190_MicroWrite(vme, vmeBaseAdress, code)) < 0)
     return status;
  code = 0x1000;  // Width
  value = v1190_MicroWrite(vme, vmeBaseAdress, code);
  value = v1190_MicroWrite(vme, vmeBaseAdress, 0x20);   // Width : 800ns
  code = 0x1100;  // Offset
  value = v1190_MicroWrite(vme, vmeBaseAdress, code);
  value = v1190_MicroWrite(vme, vmeBaseAdress, 0xfe8);  // offset: -400ns
  code = 0x1500;  // Subtraction flag
  value = v1190_MicroWrite(vme, vmeBaseAdress, code);
  code = 0x2100;  // Leading Edge Detection
  value = v1190_MicroWrite(vme, vmeBaseAdress, code);

  mvme_set_dmode(mvme, cmode);

  return retVal;
}

bool v1190::setupV1190ODB(){
  char str[1024];
  HNDLE hkey;

  tdcNum = (vmeBaseAdress>>16) & 0xF;

  FTDC_SETTINGS_STR(ftdc_settings_str);
  
  sprintf(str, "/Equipment/V1190/Module-%i-0x%X", tdcNum, vmeBaseAdress);
  db_create_record(ODB, 0, str, strcomb(ftdc_settings_str));
  db_find_key(ODB, 0, str, &hkey);

  if (db_open_record(ODB, hkey, &settings, sizeof(settings), MODE_READ,
		     &v1190::update, (void *)(&mess)) != DB_SUCCESS) {
    cm_msg(MERROR, "frontend_init",
	   "Cannot open settings in ODB");
    
    return false;}

  else{
    update(ODB, hkey, (void *)(&mess));
    return true;
  }
}

bool v1190::checkModuleResponse(){
  unsigned int testval = 0x1010;
  v1190_Write16(vme, vmeBaseAdress, V1190_DUMMY16_RW,    testval);
  if(v1190_Read16(vme, vmeBaseAdress, V1190_DUMMY16_RW) != testval)
    return false;
  else
    return true;
}

bool v1190::read(void *pevent){
  struct timespec sleeptime;
  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 1000; // unit is nano seconds
  
  for(int counter = 0; counter < 200; counter ++){
    if(checkEventStatus()) break;
    else nanosleep(&sleeptime, NULL); 
  }
  if(!checkEventStatus()){
    std::cerr << "SOMETHING STRANGE HAPPENED!!!!!" << std::endl;
    return false;

  }
  DWORD  data[1000000]; // just a very big buffer...
  char   bname[5];
  DWORD *pdata;

  sprintf(bname,"TDC%X",tdcNum);
    
  // create bank for TDC data
  bk_create(pevent, bname, TID_DWORD, &pdata);
  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 200000; // unit is nano seconds
  nanosleep(&sleeptime, NULL); 
  uint32_t entries = getEventSize();
  if(entries>0){
    v1190_DataRead(vme, vmeBaseAdress, data, (uint32_t)entries);
    for(unsigned int i=0; i<entries; i++){
      //std::cout << data[i] << std::endl;
      *pdata++ = data[i];      
    }
  } else {
    return false;
  }

  bk_close(pevent,pdata);

  v1742_RegisterWrite(vme, vmeBaseAdress, V1742_SW_CLEAR, 0x1);
  return true;
}

void v1190::update(INT hDB, INT hkey, void *dump){
  if(dump != NULL){

    messenger *mess = (messenger *)dump;

    v1190_WidthSet_ns(mess->vme, mess->vmeBaseAdress, mess->settings->width);
    v1190_OffsetSet_ns(mess->vme, mess->vmeBaseAdress, mess->settings->offset);
    v1190_SetEdgeDetection(mess->vme, mess->vmeBaseAdress, mess->settings->edge_leading, mess->settings->edge_trailing);

  }
}
