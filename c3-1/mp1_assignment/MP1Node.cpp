/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"
#include <byteswap.h>
#include <sstream>

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
//	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif
    if ( memberNode->addr == *joinaddr) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;

        IdPort idPort = *(IdPort*)memberNode->addr.addr;
		int id = idPort.getId();
		short port = idPort.getPort();
		memberNode->memberList.push_back(MemberListEntry(id, port));
//		memberNode->memberList.push_back(MemberListEntry(idPort.getId(), idPort.getPort()));

    }
    else {
#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
    	JoinReqPkg joinPkg;
    	joinPkg.hdr.msgType = JOINREQ;
    	joinPkg.adr = memberNode->addr;
    	joinPkg.hrt = memberNode->heartbeat;

        emulNet->ENsend(&memberNode->addr, joinaddr, (char *) &joinPkg, sizeof(JoinReqPkg));

    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
	return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
	MessageHdr *msg;
	msg = (MessageHdr*) data;
	switch (msg->msgType) {
	case JOINREQ: {
		JoinReqPkg* p = (JoinReqPkg*) data;
		Address addr = p->adr;

		log->logNodeAdd(&memberNode->addr, &addr);

		// reply with JOINREP
        vector<MemberListEntry> memberList = memberNode->memberList;
		size_t n = memberList.size();

        size_t msgsize = sizeof(JoinRepPkg) + n * sizeof(MemberInfo);
        JoinRepPkg* joinRep = (JoinRepPkg*) malloc(msgsize);
    	joinRep->hdr.msgType = JOINREP;
    	joinRep->n = n;

    	MemberInfo *members = &joinRep->member;
        for(vector<MemberListEntry>::iterator memberIterator = memberList.begin();
                memberIterator != memberList.end();
                memberIterator++) {
        	MemberListEntry member = *memberIterator;
        	MemberInfo memberInfo;
        	memberInfo.id = member.id;
        	memberInfo.port = member.port;
        	memberInfo.heartbeat = member.heartbeat;
        	*members = memberInfo;
            ++members;
        }

        IdPort idPort = *(IdPort*)&addr;
		int id = idPort.getId();
		short port = idPort.getPort();
		memberNode->memberList.push_back(MemberListEntry(id, port));
//		memberNode->memberList.push_back(MemberListEntry(idPort.getId(), idPort.getPort()));
		memberNode->nnb++;

#ifdef DEBUGLOG
        static char s[1024];
        sprintf(s, "JOINREQ received ... send JOINREP");
        log->LOG(&memberNode->addr, s);
#endif


        // send JOINREP message to member
        emulNet->ENsend(&memberNode->addr, &addr, (char *) joinRep, msgsize);

        free(joinRep);
	} break;
	case JOINREP: {
        memberNode->inGroup = true;
#ifdef DEBUGLOG
		log->LOG(&memberNode->addr, "JOINREP ... node has joined group");
#endif
		JoinRepPkg* p = (JoinRepPkg*) data;
		size_t n = p->n;

		MemberInfo* memberInfo = &((JoinRepPkg*)data)->member;
		for (size_t i=0; i < n; ++i) {
/*			int id = __bswap_32 (memberInfo->id);
			short port = __bswap_16 (memberInfo->port);
			MemberListEntry member = MemberListEntry(MemberListEntry(id ,port));*/
			MemberListEntry member = MemberListEntry(MemberListEntry(memberInfo->id ,memberInfo->port));
			memberNode->memberList.push_back(member);
			ostringstream address;
			address << member.id << ":" << member.port;
			Address addr = Address(address.str());
			log->logNodeAdd(&memberNode->addr, &addr);
			memberNode->nnb++;
			memberInfo++;
		}

	} break;
	case PING: {
		PingPkg* p = (PingPkg*) data;
		Address addr = p->adr;
		int a = p->member.info.id;
		MemberStatus stat = p->member.status;
        static char s[1024];
        sprintf(s, "PING received from node %s, %c%d", addr.getAddress().c_str(), memberStatus(stat), a);
        log->LOG(&memberNode->addr, s);

		PongPkg pongPkg;
		pongPkg.hdr.msgType = PONG;
		pongPkg.adr = memberNode->addr;
		if (memberNode->nnb > 1) {
			size_t inode = rand() % memberNode->nnb;
			MemberListEntry mle = memberNode->memberList[inode];
			pongPkg.n = 1;
			MemberInfo info;
			info.id = mle.id; info.port = mle.port; info.heartbeat = mle.heartbeat;
			pongPkg.member.status = ALIVE;
			pongPkg.member.info = info;
        } else {
			pongPkg.n = 0;
        }
		emulNet->ENsend(&memberNode->addr, &addr, (char *) &pongPkg, sizeof(PingPkg));
			/*
		ostringstream address;
		address << memberNode->memberList[node].id << ":" << memberNode->memberList[node].port;
		Address addr = Address(address.str());
*/
		if (!inMemberList(&addr)) {
			log->logNodeAdd(&memberNode->addr, &addr);
	        IdPort idPort = *(IdPort*)&addr;
			int id = idPort.getId();
			short port = idPort.getPort();
			memberNode->memberList.push_back(MemberListEntry(id, port));
			memberNode->nnb++;

		}
	} break;
	case PONG: {
		PongPkg* p = (PongPkg*) data;
		Address adr = p->adr;
		int a = p->member.info.id;
		MemberStatus stat = p->member.status;
        static char s[1024];
        sprintf(s, "PONG received from node %s, %c%d", adr.getAddress().c_str(), memberStatus(stat), a);
        log->LOG(&memberNode->addr, s);
	} break;
	case DUMMYLASTMSGTYPE: {

	} break;
	case TEST: {
		TestPkg* p = (TestPkg*) data;
		Address a = p->adr;
		const char * adr = a.getAddress().c_str();
		log->LOG(&memberNode->addr, adr);

	} break;
	default: {
		char* s = data;
//		int sz = size;
#ifdef DEBUGLOG
		log->LOG(&memberNode->addr, s);
#endif

	}

	}
	/*
#ifdef DEBUGLOG
    static char s[1024];
    sprintf(s, "recvCallBack: %s", data);
    log->LOG(&memberNode->addr, s);
#endif*/
    return true;
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
#ifdef DEBUGLOG
//        static char s[1024];
//        sprintf(s, "nodeLoopOps() called, timeoutCntr = %d, pingCntr = %d, hbeat = %ld", memberNode->timeOutCounter, memberNode->pingCounter, memberNode->heartbeat);
//        log->LOG(&memberNode->addr, s);
#endif

    // send PING message to random member
	int neighbours = memberNode->nnb;
	if (neighbours > 1) {
		size_t node = rand() % memberNode->nnb;
		size_t inode = rand() % memberNode->nnb;
		while (inode == node) {
			inode = rand() % memberNode->nnb;
		}

		MemberListEntry mle = memberNode->memberList[inode];
		PingPkg pingPkg;
		pingPkg.hdr.msgType = PING;
		pingPkg.adr = memberNode->addr;
		pingPkg.n = 1;
		pingPkg.member.status = ALIVE;
		MemberInfo info;
		info.id = mle.id; info.port = mle.port; info.heartbeat = mle.heartbeat;
		pingPkg.member.info = info;

		if (memberNode->addr.addr[0]==1) {
//			int dummy = 0;
		}
		ostringstream address;
		address << memberNode->memberList[node].id << ":" << memberNode->memberList[node].port;
		Address addr = Address(address.str());
		emulNet->ENsend(&memberNode->addr, &addr, (char *) &pingPkg, sizeof(PingPkg));
	}
	memberNode->timeOutCounter++;



    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

char memberStatus(MemberStatus m) {
	switch (m) {
		case ALIVE: return 'A';
		case SUSPECT: return 'S';
		case DEAD: return 'D';
	}
	return '?';
}

bool MP1Node::inMemberList(Address* addr) {
	IdPort idPort = IdPort(addr);
	for (size_t i = 0; i < memberNode->memberList.size(); ++i) {
		if (memberNode->memberList[i].id == idPort.getId() &&
				memberNode->memberList[i].port == idPort.getPort()) {
			return true;
		}
	}
	return false;
}
