#ifndef _msg_h
#define _msg_h

struct msg{
	int msgid;
	int msgllen;
	int sid;
	int csid;
	int count;
	char buff[1024];
};

#endif //_msg_h