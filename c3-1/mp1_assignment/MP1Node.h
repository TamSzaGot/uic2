/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"
#include <byteswap.h>

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,
    JOINREP,
    TEST,
    DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
} MessageHdr;

/**
 * STRUCT NAME: MemberInfo
 *
 * DESCRIPTION: Address and heartbeat
 */
typedef struct IdPort {
	int getId() {return __bswap_32(_id); };
	void setId(int id) {_id = __bswap_32(id); };
	short getPort() { return __bswap_16(_port); };
	void setPort(short port) { _port = __bswap_16(port); };
private:
	int _id;
	short _port;
} IdPort;

typedef struct MemberInfo {
	int id;
	short int port;
	long heartbeat;
} MemberInfo;

typedef struct TestPkg {
	MessageHdr hdr;
	Address adr;
} TestPkg;

typedef struct JoinReqPkg {
	MessageHdr hdr;
	Address adr;
	long hrt;
} JoinReqPkg;

typedef struct JoinRepPkg {
	MessageHdr hdr;
	size_t n;
	MemberInfo members[];
} JoinRepPkg;

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
