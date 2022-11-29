#ifndef _MEMBERS
#define _MEMBERS
class Member {
public:
  Member(const char* rfid, const char* name);
  void operator=( Member &obj);
  const char* getName();
  const char* getRFID();
private:
  char _rfid[10];
  char _name[128];
};
#endif