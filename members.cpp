#include "members.h"
#include <string.h>
Member::Member(const char* rfid, const char* name){
  strncpy(this->_name, name, sizeof(this->_name));
  strncpy(this->_rfid, rfid, sizeof(this->_rfid));
}
void Member::operator=( Member &obj){
  strncpy(this->_name, obj.getName(), sizeof(this->_name));
  strncpy(this->_rfid, obj.getRFID(), sizeof(this->_rfid));
}
 const char* Member::getName()  {
  return (const char *) this->_name;
}
 const char* Member::getRFID() {
  return (const char *) this->_rfid;
}