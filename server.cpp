//UDPServer.c
 
/*  login = works
    logout = works
    
 *  gcc -o server UDPServer.c
 *  ./server
 */
 //#include <endian.h>
       #include <stdint.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <map>
#include <set> 
#include <utility>
#include <vector>
#include "duckchat.h"
//defined
#define BUFLEN 1024
 #define STDIN 0
 #define MAX_MSG_LEN 65536
using namespace std;
//globals
volatile int got_interrupt = 0;//used for signal handeling
struct sockaddr recAddr;
socklen_t fromlen = sizeof(recAddr);
int sockfd;
struct addrinfo *addrAr;
fd_set  fdWait;
map<string,vector<string> > usrTlkChan;
multimap<string,struct sockaddr_in> userToAddrStrct;
map<string,vector<pair<string,struct sockaddr_in> > > chanTlkUser;
map<string,vector<struct sockaddr_in> > chanTlkServer;
vector<string> channels;
vector<struct sockaddr_in> neighborServers;
vector<long long> msgIds;
string homeServerStringAddr;
int homeServerPort;
//methods
void sig_alarm(int signo){got_interrupt = 1;}//used to catch SIGALRM for soft state
string stringAddr(struct sockaddr_in);
string getUserOfCurrAddr();
int checkAddrEq(struct sockaddr_in, struct sockaddr_in);
struct sockaddr_in getAddrStruct();
int checkValidAddr();
int sayReq(struct request_say*);
int loginReq(struct request_login*);
int logoutReq(struct request_logout*);
int errorMsg(struct sockaddr_in,string);
int joinReq(struct request_join*);
int leaveReq(struct request_leave*);
int listReq(struct request_list*);
int whoReq(struct request_who*);
int readRequestType(struct request*, int);
int connectToHomeSocket(char*, char*);
int addNeighborServers(char*, char*);
int req_s2sJoin(struct request_s2s_join*);
int s2sLeave(struct request_s2s_leave*);
int s2sSay(struct request_s2s_say*);
int sendS2SJoin(string);
int sendS2SJoin_Except(string,sockaddr_in); 
int sendS2SSay(struct request_say*,string,string);
//int checkAddrEq_Server(struct sockaddr_in,struct sockaddr_in);

//program
int main(int argc, char **argv)
{
    signal(SIGALRM, sig_alarm);//needed for soft state
    addrAr = NULL;
    sockfd = 0;
    if((argc-1) < 2) { 
        cout << "need at least 2 argument: server IP and port.\n";
        return -1;
    }
    if((argc-1) % 2 != 0) {
        cout << "invalid arguents. must provide ip and port of each server.\n";
        return -1;
    }
    connectToHomeSocket(argv[1], argv[2]);
    homeServerStringAddr = (string)argv[1];
    homeServerPort = atoi(argv[2]);
    if((argc-1) > 2) {
        for(int i=3; i<argc-1; i=i+2) {
            addNeighborServers(argv[i], argv[i+1]);
        }
    }
    int req_tester = 0;
    while(1)
    {
        alarm(60);//testing alarm interval
                if(got_interrupt)
                {
                    cout << "we just got an interupt!" << endl;
                    got_interrupt = 0;//reset interupt
                    for(vector<string>::iterator it = channels.begin(); it != channels.end(); ++it)
                        sendS2SJoin(*it);
                }
        int bal = 0;
        int selCheck = 0;
        FD_ZERO(&fdWait);
        FD_SET(STDIN, &fdWait);
        FD_SET(sockfd, &fdWait);
        selCheck = select(sockfd+1, &fdWait, NULL, NULL, NULL);
        if(selCheck < 0) {
            cout << "should be error !!! \n";
        }
        if(FD_ISSET(sockfd,&fdWait)) {
            void *buf;
            size_t uniqLen;
            struct request *requests;
            char recv_Text[MAX_MSG_LEN];
            buf = &recv_Text;
            uniqLen = sizeof recv_Text;
            fromlen = sizeof(recAddr);
             bal = recvfrom(sockfd, buf, uniqLen, 0, (struct sockaddr*)&recAddr, &fromlen);

            if(bal > 0) {
                requests = (request*)buf;
                req_tester = readRequestType(requests, bal);
                if(req_tester != 0 ) {
                    //cout << "whoooaaaa jackoo\n";
                }
            } 
            //delete requests;
            //delete[] buf; 
        }
       
        // cout << "Printing Neightbor Servers.\n";
        // for(int n=0; n<neighborServers.size(); n++) {
        //     //cout << itoa(neighborServers[n]) << " : \n";
        //     cout << n << " : \n";

        // }
        // cout << "Printing chanTlkUser.\n";
        // map<string,vector<pair<string,struct sockaddr_in> > >::iterator him;
        // for(him = chanTlkUser.begin(); him != chanTlkUser.end(); him++) {
        //     cout << him->first << " : channel has users..\n";
        //     for(int j=0; j<him->second.size(); j++) {
        //         cout << him->second[j].first << " : is a user.\n";
        //     }
        // }
        // cout << "Printing channels global array\n";
        // for(int i=0; i<channels.size(); i++) {
        //     cout << "channel is : " << channels[i] << "\n";
        // }
        // cout << "Printing chanTlkServer.\n";
        // cout << chanTlkServer.size() << " : server size\n";
        // map<string,vector<struct sockaddr_in> >::iterator kin;
        // for(kin = chanTlkServer.begin(); kin != chanTlkServer.end(); kin++) {
        //     cout << kin->first << " : channel has servers #.. " << kin->second.size() << "\n";
        //     for(int j=0; j<kin->second.size(); j++) {
        //         cout << stringAddr(kin->second[j]) << " : is a server.\n";
        //     }
        // }

        // cout << "END OF ROUND____\n";         
    }
     
    return 0;
}

//send ser
int sendS2SJoin_Except(string channel, sockaddr_in sender) 
{
    struct request_s2s_join server_join;
    strncpy(server_join.req_s2s_channel, channel.c_str(), CHANNEL_MAX);
    server_join.req_type = htonl(S2S_JOIN);
    int check = 0;
    //vector<struct sockaddr_in>::iterator i;
    vector<struct sockaddr_in> temporaServs;
    for(int i=0; i <neighborServers.size(); i++) {
        if((check = checkAddrEq(neighborServers[i], sender)) != 0) {
            
            //cout << "Sending S2S Join from server : " << stringAddr(neighborServers[i]) << ". On port : " << (int)ntohs(neighborServers[i].sin_port) << ".\n";
            cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(neighborServers[i]) <<  ":" << (int)ntohs(neighborServers[i].sin_port) << " send S2S Join " << channel << "\n";
            int res = sendto(sockfd, &server_join, sizeof(server_join), 0, (struct sockaddr*)&(neighborServers[i]), sizeof(neighborServers[i]));
            if(res == -1) {
                cout << "error sending join to neighborServers\n";
                return -1;
            }
        }
        temporaServs.push_back(neighborServers[i]);
    }
    chanTlkServer[channel] = temporaServs;
    return 0;
}
//handle request server join
int req_s2sJoin(struct request_s2s_join *r) 
{
    string chan = (string)r->req_s2s_channel;
    struct sockaddr_in reqAddr = getAddrStruct();
    map<string,vector<struct sockaddr_in> >::iterator it;
    for(int i=0; i<channels.size(); i++) {
        if(channels[i] == chan) {
            //old channel
            //map<string,vector<struct sockaddr_in> >::iterator tmpIt = chanTlkServer.find(chan);
            //vector<struct sockaddr_in> tmpV = tmpIt->second;
            vector<struct sockaddr_in> tmpV = chanTlkServer[chan];
            bool alreadySubbed = false;
            cout << "We are in req_s2sJoin" << endl;
            for(vector<struct sockaddr_in>::iterator it = tmpV.begin(); it != tmpV.end(); ++it)
            {
                if(!checkAddrEq(*it,reqAddr))
                {
                    cout << "here we are" << endl;
                    alreadySubbed = true;
                    break;
                 }
             }
            if(!alreadySubbed)
                tmpV.push_back(reqAddr);
            //chanTlkServer.erase(chan);
            //chanTlkServer.insert(pair<string,vector<struct sockaddr_in> >(chan,tmpV));
            chanTlkServer[chan] = tmpV;

            return 0;
        }
    }
    //new channel
    vector<struct sockaddr_in> tmpServers;
    tmpServers.push_back(reqAddr);
    chanTlkServer.insert(pair<string,vector<struct sockaddr_in> >(chan,tmpServers));
    //chanTlkServer[chan] = tmpServers;
    channels.push_back(chan);
    vector<pair<string,struct sockaddr_in> > tmpUsers;
    chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(chan,tmpUsers));
    // vector<struct sockaddr_in> tmpServers;
    // tmpServers.insert(tmpServers.begin(), )
    // chanTlkUser.insert(pair<string,vector<struct sockaddr_in> >(chan,))
    int check = sendS2SJoin_Except(chan,reqAddr);
    if(check == -1) {
        cout << "Horrible s2s send_except\n";
        return -1;
    }
    return 0;
}
//server reveive leave
int s2sLeave(struct request_s2s_leave *r) 
{
    string channel = r->req_s2s_channel;
    struct sockaddr_in senderAddr = getAddrStruct();
    map<string,vector<struct sockaddr_in> >::iterator i = chanTlkServer.find(channel);
    if(i == chanTlkServer.end()) {
        cout << "error can't find channel to leave when server s2sleave is receveived\n";
        return -1;
    }
    vector<struct sockaddr_in> tmpServs;
    int check = 0;
    for(int j=0; j<i->second.size(); j++) {
        if((check = checkAddrEq(senderAddr,i->second[j])) == -1) {
            tmpServs.push_back(i->second[j]);
        }
    }
    chanTlkServer.erase(channel);
    chanTlkServer.insert(pair<string,vector<struct sockaddr_in> >(channel,tmpServs));
    return 0;
}
//server send leave msg
int sendS2sLeave(struct sockaddr_in sender, string channel) 
{
    struct request_s2s_leave leaveMsg;
    leaveMsg.req_type = htonl(S2S_LEAVE);
    strcpy(leaveMsg.req_s2s_channel, channel.c_str());
     //cout << "Sending S2S Leave from server : " << stringAddr(sender) << ". On port : " << (int)ntohs(sender.sin_port) << ".\n";
    cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(sender) <<  ":" << (int)ntohs(sender.sin_port) << " send S2S leave " << channel << "\n";
    int res = sendto(sockfd, &leaveMsg, sizeof(leaveMsg), 0,(struct sockaddr*)&sender, sizeof(sender));
    if(res == -1) {
        cout << "error sending leave\n";
        return -1;
    }
    return 0;

}
//receavi server say
int s2sSay(struct request_s2s_say *r) 
{
    struct sockaddr_in fromAdr = getAddrStruct();
    long long msgId =  htobe64(r->server_id);
    for(int i=0; i<msgIds.size(); i++) {
        if(msgIds[i] == msgId){
            cout << "dup message :\n";
            return 0;
        }
    }
    string chan = r->req_s2s_channel;
    string user = r->req_s2s_username;
    string mess =  r->req_s2s_msg;
    map<string,vector<struct sockaddr_in> >::iterator j = chanTlkServer.find(chan);
    if(j == chanTlkServer.end()) {
        cout << "error finding channel to say \n";
        return -1;
    }
    vector<struct sockaddr_in> tmpServs = j->second;
    int check = 0;
    int res = 0;
    for(int n=0; n<tmpServs.size(); n++) {
        if((check = checkAddrEq(tmpServs[n], fromAdr)) == -1) {
             //cout << "Sending Say from user : " << user << ". To server : " << stringAddr(tmpServs[n]) << ".\n";
        	cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(fromAdr) <<  ":" << (int)ntohs(fromAdr.sin_port) << " send S2S Say " << chan << "\n";
            res = sendto(sockfd, r, sizeof(*r), 0, (struct sockaddr*)&(tmpServs[n]), sizeof tmpServs[n]);
            if(res == -1) {
                cout << "send to servers say msg fail ahh.\n";
            }   
        }
    }
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator hit = chanTlkUser.find(chan);
    if(hit == chanTlkUser.end()) {
        cout << "error with finding channel n user map channel \n";
        return -1; 
    }
    vector<pair<string,struct sockaddr_in> > tmpU = hit->second;
    for(int i=0; i<tmpU.size(); i++) {
        struct sockaddr_in address;
        void *goData;
        address = tmpU[i].second;
        struct text_say msg;
        msg.txt_type= TXT_SAY;
        strncpy(msg.txt_username, user.c_str(), sizeof(user));
        strcpy(msg.txt_text, mess.c_str());
        strncpy(msg.txt_channel, chan.c_str(), sizeof(chan));
        int size = sizeof(struct sockaddr*);
        goData = &msg;
        //cout << "Sending Say from user : " << user << ". To User : " << stringAddr(address) << ".\n";
        cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(fromAdr) <<  ":" << (int)ntohs(fromAdr.sin_port) << " send S2S Say " << chan << "\n";
        int res= sendto(sockfd, goData, sizeof(msg), 0, (struct sockaddr*)&address, sizeof(address));
        if (res == -1) {
            return -1;
        }
    }
    if(tmpServs.size() ==0 && tmpU.size() == 0) {
        for(int s=0; s<neighborServers.size(); s++) {
            if(checkAddrEq(neighborServers[s], fromAdr) != 0) {
                int hector = sendS2sLeave(neighborServers[s], chan);
                if(hector == -1) {
                    cout << "error sending leave back b/c dup msg \n";
                    return -1;
                }
            }
            
        }
        
    }

    return 0;

}
//returns string of sockaddr_in
string stringAddr(struct sockaddr_in a)
{
    char *addrA = (char*)malloc(sizeof(char)*BUFLEN);
    inet_ntop(AF_INET, &(a.sin_addr), addrA, BUFLEN);
    string retString = addrA;
    return retString;
}
//returns string of username of current request address
string getUserOfCurrAddr()
{ 
    struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;
    string aTmp = "";     
    multimap<string,struct sockaddr_in>::iterator i;
    for(i=userToAddrStrct.begin(); i != userToAddrStrct.end(); i++) {
        if(checkAddrEq(i->second,*address) == 0) {
            aTmp = i->first;
        }
    }
    return aTmp;
}
int checkAddrEq(struct sockaddr_in a, struct sockaddr_in b)
{
    char *addrA = (char*)malloc(sizeof(char)*BUFLEN);
    char *addrB = (char*)malloc(sizeof(char)*BUFLEN);
    inet_ntop(AF_INET, &(a.sin_addr), addrA, BUFLEN);
    inet_ntop(AF_INET, &(b.sin_addr), addrB, BUFLEN);
    string stringA = addrA;
    string stringB = addrB;
    if(stringA == stringB) {
        int portA = a.sin_port;
        int portB = b.sin_port;
        if(portA == portB) {
            return 0;
        }
    }
    return -1;
}
struct sockaddr_in getAddrStruct() 
{
    struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;
    return *address;
}
//check if current request address is valid or exist in map
int checkValidAddr() 
{
    struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;
    multimap<string,struct sockaddr_in>::iterator i;
    for(i=userToAddrStrct.begin(); i != userToAddrStrct.end(); i++) {
        if(checkAddrEq(i->second,*address)==0) {
            return 0;
        }
    }
    for(int j=0; j<neighborServers.size(); j++) {
        if(checkAddrEq(neighborServers[j],*address)==0) {
            return 0;
        }
    }
    return -1;
}
int sendS2SSay(struct request_say *r, string username, string channel, string message)
{
    //string message = r->req_text;

    long long id = rand();
    id = id << 32 | rand();
    
    msgIds.push_back(id);
    map<string,vector<struct sockaddr_in> >::iterator i = chanTlkServer.find(channel);
    if(i == chanTlkServer.end()) {
        cout << "error finding channel to say to in channel to server map.\n";
        return -1;
    }  
    vector<struct sockaddr_in> tmpServs = i->second;
    //cout << "size of chantlk server when inside sending s2s say at homeserver when user sends say size is : " << tmpServs.size() << "\n";
    int res = 0;
    
    for(int j=0; j<tmpServs.size(); j++) {
        //cout << "Sending S2SSay to server : " <<  stringAddr(tmpServs[j]) << ".\n";
        struct sockaddr_in tmpAddress;
        tmpAddress = tmpServs[j];
        struct request_s2s_say sayToServMsg;
        sayToServMsg.req_type = htonl(S2S_SAY);
        sayToServMsg.server_id = be64toh(id);
        strncpy(sayToServMsg.req_s2s_channel, channel.c_str(), sizeof(channel));
        strncpy(sayToServMsg.req_s2s_username, username.c_str(), sizeof(username));
        strcpy(sayToServMsg.req_s2s_msg, message.c_str());
        void *goData;
        goData = &sayToServMsg;
        //cout << "Sending S2SSay to server : " <<  stringAddr(tmpServs[j]) << ".\n";
        cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(tmpAddress) <<  ":" << (int)ntohs(tmpAddress.sin_port) << " send S2S Say " << channel << "\n";
        res = sendto(sockfd, goData, sizeof(sayToServMsg), 0, (struct sockaddr*)&tmpAddress, sizeof(tmpAddress));
        if(res == -1) {
            cout << "send to servers say msg fail.\n";
        }     
    }

}
//handle say requests
int sayReq(struct request_say *rs)
{
    
    string channel = rs->req_channel;
    string message = rs->req_text;
    string username = getUserOfCurrAddr();
    sendS2SSay(rs, username, channel, message);
    struct sockaddr_in fromAddr = getAddrStruct();
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator hit = chanTlkUser.find(channel);
    if(hit == chanTlkUser.end()) {
        return -1; 
    }
    vector<pair<string,struct sockaddr_in> > tmpU = hit->second;
    for(int i=0; i<tmpU.size(); i++) {
        struct sockaddr_in address;
        void *goData;
        address = tmpU[i].second;
        struct text_say msg;
        msg.txt_type= TXT_SAY;
        strncpy(msg.txt_username, username.c_str(), sizeof(username));
        strcpy(msg.txt_text, message.c_str());
        strncpy(msg.txt_channel, channel.c_str(), sizeof(channel));
        int size = sizeof(struct sockaddr*);
        goData = &msg;
        //cout << "Sending TXT_Say to user address: " <<  stringAddr(address) << ". On Port : " << (int)ntohs(address.sin_port) << ".\n";
         cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(address) <<  ":" << (int)address.sin_port << " send Say " << channel << "\n";
        int res= sendto(sockfd, goData, sizeof(msg), 0, (struct sockaddr*)&address, sizeof(address));
        if (res == -1) {
            return -1;
        }
    }
    tmpU.clear();
    return 0;
}
//handle login requests
int loginReq(struct request_login *rl)
{
    string username = rl->req_username;
    struct sockaddr_in strctAddr = getAddrStruct();
    userToAddrStrct.insert(userToAddrStrct.end(), pair<string, struct sockaddr_in>(username, strctAddr));
    return 0;
}
//handle login requests
int logoutReq(struct request_logout *rl)
{
/*
    cout << "got a logout req!!!!!\n";
    string username = getUserOfCurrAddr();
    struct sockaddr_in tmpAddr = getAddrStruct();
    multimap<string, struct sockaddr_in>::iterator sockIt = userToAddrStrct.find(username);
    userToAddrStrct.erase(sockIt);
    map<string,vector<string> >::iterator git;
    git = usrTlkChan.find(username);
    if(git != usrTlkChan.end()) {
        usrTlkChan.erase(username);
    }
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator ctu_It;
    for(ctu_It = chanTlkUser.begin(); ctu_It != chanTlkUser.end(); ctu_It++) {
        string tmpChannel = ctu_It->first;
        vector<pair<string,struct sockaddr_in> > tmp_usr_vec;
        for(int i=0; i<ctu_It->second.size(); i++) {
            if(username != ctu_It->second[i].first) {
                if(checkAddrEq(tmpAddr,ctu_It->second[i].second) != 0) {
                    tmp_usr_vec.push_back(ctu_It->second[i]);
                }
            }
        }
        if(tmp_usr_vec.size() > 0) {
            chanTlkUser[tmpChannel] = tmp_usr_vec;
        } else if(tmp_usr_vec.size() == 0) {
            chanTlkUser.erase(tmpChannel);
            // vector<string> temporaChans;
            // for(int h=0; h<channels.size(); h++) {
            //     if(channels[h] != tmpChannel) {
            //         temporaChans.push_back(channels[h]);
            //     }
            // }
            // channels = temporaChans;
        }
    }



    
    //erase user on channels in chanTlkUser
    for(int ick=0; ick<channels.size(); ick++) {
        map<string,vector<pair<string,struct sockaddr_in> > >::iterator itck = chanTlkUser.find(channels[ick]);
        vector<pair<string,struct sockaddr_in> > usersC = itck->second;
        for(int j=0; j<usersC.size(); j++) {
            if(usersC[j].first == username) {
                if(checkAddrEq(usersC[j].second, tmpAddr) == 0) {
                    usersC.erase(usersC.begin()+j);
                }
            }
        }
        chanTlkUser.erase(itck);
        if(usersC.size() > 0) {
            chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(channels[ick],usersC));
            
        } else if(usersC.size() == 0) {
            channels.erase(channels.begin()+ick);
        }
        
    }
    */

	string username = getUserOfCurrAddr();
	multimap<string, struct sockaddr_in>::iterator sockIt = userToAddrStrct.find(username);
	userToAddrStrct.erase(sockIt);
	map<string,vector<string> >::iterator git;
	git = usrTlkChan.find(username);
	if(git != usrTlkChan.end()) {
		usrTlkChan.erase(username);
	}
	//erase user on channels in chanTlkUser
	for(int ick=0; ick<channels.size(); ick++) {
		map<string,vector<pair<string,struct sockaddr_in> > >::iterator itck = chanTlkUser.find(channels[ick]);
		vector<pair<string,struct sockaddr_in> > usersC = itck->second;
		for(int j=0; j<usersC.size(); j++) {
			if(usersC[j].first == username) {
				usersC.erase(usersC.begin()+j);
			}
		}
		chanTlkUser.erase(itck);
		chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(channels[ick],usersC));
	}
	return 0;

}
//adopted from example code of program 1
//error message
int errorMsg(struct sockaddr_in addr, string msg)
{
    ssize_t bytes;
    void *send_data;
    size_t len;
    struct text_error send_msg;
    send_msg.txt_type = TXT_ERROR;
    const char* str = msg.c_str();
    strcpy(send_msg.txt_error, str);
    send_data = &send_msg;
    len = sizeof send_msg;
    struct sockaddr_in send_sock = addr;
    bytes = sendto(sockfd, send_data, len, 0, (struct sockaddr*)&send_sock, sizeof send_sock);
    if(bytes < 0) {
        return -1;
    }
    else {
       return 0;
    }
}
//sends s2s when userjoins server
int sendS2SJoin(string channel) 
{

    struct request_s2s_join server_join;
    strncpy(server_join.req_s2s_channel, channel.c_str(), CHANNEL_MAX);
    server_join.req_type = htonl(S2S_JOIN);
    vector<struct sockaddr_in>::iterator i;
    vector<struct sockaddr_in> tmpServers;
    // for(i=neighborServers.begin(); i != neighborServers.end(); i++) {
    //     tmpServers.push_back(*i);
    //     int res = sendto(sockfd, &server_join, sizeof(server_join), 0, (struct sockaddr*)&(*i), sizeof(*i));
    //     if(res == -1) {
    //         cout << "error sending join to neighborServers\n";
    //         return -1;
    //     }
    // }
    for(int i=0; i<neighborServers.size(); i++) {
        tmpServers.push_back(neighborServers[i]);
        cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(neighborServers[i]) <<  ":" << (int)ntohs(neighborServers[i].sin_port) << " send S2S Join " << channel << "\n";
        int res = sendto(sockfd, &server_join, sizeof(server_join), 0, (struct sockaddr*)&(neighborServers[i]), sizeof neighborServers[i]);
            if(res == -1) {
            cout << "error sending join to neighborServers\n";
            return -1;
        }
    }
    chanTlkServer.insert(pair<string,vector<struct sockaddr_in> >(channel,tmpServers));
    return 0;
}
//handle login requests
int joinReq(struct request_join *rj)
{
    //create tmp vars for username and channel of request
    string chan = (string)rj->req_channel;
    string user = getUserOfCurrAddr();
    struct sockaddr_in reqAddr = getAddrStruct();
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator it = chanTlkUser.find(chan);
    vector<pair<string,struct sockaddr_in> > usersC;
    if(it == chanTlkUser.end()) {
        //new channel
        usersC.insert(usersC.begin(), pair<string,struct sockaddr_in>(user,reqAddr));
        chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(chan, usersC));
        channels.push_back(chan);
        int check = sendS2SJoin(chan);
        if(check == -1) {
            return -1;
        }
    } else {
        //old channel
        it = chanTlkUser.find(chan);
        usersC = it->second;
        for(int i=0; i<usersC.size(); i++) {
            if(usersC[i].first == user) {
                return -1;
            }
        }
        usersC.insert(usersC.begin(), pair<string,struct sockaddr_in>(user, reqAddr));
        chanTlkUser[chan] = usersC;
    }
    vector<string> chanTlk = usrTlkChan[user];
    chanTlk.insert(chanTlk.begin(), chan);
    usrTlkChan[user] = chanTlk;
    return 0;
}
//handle login requests
//NEED TO ADD SERVER LOGIC  to send s2s leave

int leaveReq(struct request_leave *rl)
{
    //tmp vars
    string username = getUserOfCurrAddr();
    //sendS2sLeave
    struct sockaddr_in reqAddr = getAddrStruct();
    string chaNel = (string)(rl->req_channel);
    //multimap<string, struct sockaddr_in>::iterator ui = userToAddrStrct.find(username);
    //find user address
    //struct sockaddr_in address = ui->second;
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator vi;
    //check if channel is there
    if((vi = chanTlkUser.find(chaNel)) == chanTlkUser.end()) {
        return -1;
    }
    //delete user from vector of userrs that belongs specific channel
    vector<pair<string,struct sockaddr_in> > v;
    for(int vecI=0; vecI<vi->second.size(); vecI++) {
        if(vi->second[vecI].first != username) {
            if(checkAddrEq(vi->second[vecI].second, reqAddr) != 0) {
                v.push_back(vi->second[vecI]);
            }
        }
    }
    //erase old entry of channel with list of users
    chanTlkUser.erase(chaNel);
    //don't insert channel if they are the last users on channel
    if(v.size() != 0) {
        chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(chaNel,v));
    //return 0;
    } else if(v.size() == 0) {
        for(int i=0; i<channels.size(); i++) {
            if(channels[i] == chaNel) {
                channels.erase(channels.begin()+i);
            }
        }
    }
    // map<string,vector<struct sockaddr_in> >::iterator servIt;
    // if((servIt = chanTlkServer.find(chaNel)) == chanTlkServer.end()) {
    // return -1;
    // }
    // vector<struct sockaddr_in> servTmps = servIt->second;
    // for(int i=0; i<servTmps.size(); i++) {
    // //if()
    // }
    map<string,vector<string> >::iterator hii = usrTlkChan.find(username);
    vector<string> chanTlk;
    for(int vv=0; vv<hii->second.size(); vv++) {
        if(hii->second[vv] != chaNel) {
    //chanTlk.erase(chanTlk.begin()+vv);
        chanTlk.push_back(hii->second[vv]);
        }
    }
    usrTlkChan.erase(username);
    usrTlkChan.insert(pair<string,vector<string> >(username,chanTlk));
    //usrTlkChan[username] = chanTlk;
    return 0;
}
//handle login requests
int listReq(struct request_list *rl)
{
    string username = getUserOfCurrAddr();
    struct sockaddr_in address; 
    multimap<string, struct sockaddr_in>::iterator ui = userToAddrStrct.find(username);
    int numCHAN = channels.size();
    address = ui->second;
    struct text_list *msg = (struct text_list*)malloc((sizeof(struct text_list)+(numCHAN *sizeof(struct channel_info))));
    msg->txt_type= TXT_LIST;
    msg->txt_nchannels = numCHAN;
    for (int i = 0; i<channels.size(); i++) {
        const char* tstr = channels[i].c_str();
        strcpy(((msg->txt_channels)+i)->ch_channel, tstr);
    }
    int size = sizeof(struct sockaddr);
    cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(address) <<  ":" << (int)ntohs(address.sin_port) << " send List\n";
    int res= sendto(sockfd, msg,  (sizeof(struct text_list)+(numCHAN *sizeof(struct channel_info))), 0, (struct sockaddr*)&address, size);
    if (res == -1) {
        return -1;
    }
    free(msg);
    return 0;
}
//handle login requests
int whoReq(struct request_who *rw)
{
    string username = getUserOfCurrAddr();
    struct sockaddr_in address; 
    string chaNel = (string)(rw->req_channel);
    multimap<string, struct sockaddr_in>::iterator ui = userToAddrStrct.find(username);
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator vi;
    if((vi = chanTlkUser.find(chaNel)) == chanTlkUser.end()) {
        return -1;
    }
    int numCHAN = (vi->second).size();
    vector<pair<string,struct sockaddr_in> > v = vi->second;
    address = ui->second;
    struct text_who *msg = (struct text_who*)malloc(sizeof(struct text_who)+(numCHAN* sizeof(struct user_info)));
    msg->txt_type= TXT_WHO;
    msg->txt_nusernames = numCHAN;
    const char* str = chaNel.c_str();
    strcpy(msg->txt_channel, str);
    for (int i = 0; i<v.size(); i++) {
        const char* tstr = v[i].first.c_str();
        strcpy(((msg->txt_users)+i)->us_username, tstr);
    }
    int size = sizeof(struct sockaddr);
    cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(address) <<  ":" << (int)ntohs(address.sin_port) << " send Who " << chaNel << "\n";
    int res= sendto(sockfd, msg,  (sizeof(struct text_who)+(numCHAN* sizeof(struct user_info))), 0, (struct sockaddr*)&address, size);
    if (res == -1) {
        return -1;
    }
    free(msg);
    return 0;
}

int readRequestType(struct request *r, int b) 
{
    int fin = 0;
    int netHost = 0;
    netHost = ntohl(r->req_type);
   
    //check if addres is a crazy number or normal
    if(netHost > 10 || netHost < 0) {
       netHost = r->req_type;
    }
    //check if request address is valid
    if(netHost != 0) {
        if(checkValidAddr() == -1) {
            cout << "is this it???\n";
            return -1;
        } 
    }
    switch(netHost) {
        case REQ_LOGIN:
            if(sizeof(struct request_login) == b) {
                cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)getAddrStruct().sin_port << " recv Request login " << getUserOfCurrAddr() << "\n";
                fin = loginReq((struct request_login*) r);
                break;
            } else {
                break;
            } 
        case REQ_LOGOUT:
            if(sizeof(struct request_logout) == b) {
                 cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)ntohs(getAddrStruct().sin_port) << " recv Request logout " << getUserOfCurrAddr() << "\n";
                fin = logoutReq((struct request_logout*) r);
                break;
            } else {
                break;
            }   
        case REQ_JOIN:
            if(sizeof(struct request_join) == b) {
                 cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)getAddrStruct().sin_port << " recv Request join " << getUserOfCurrAddr() << " " << ((struct request_join*)r)->req_channel << "\n";
                fin = joinReq((struct request_join*) r);
                break;
            } else {
                break;
            }      
        case REQ_LEAVE:
            if(sizeof(struct request_leave) == b) {
               cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)ntohs(getAddrStruct().sin_port) << " recv Request leave " << getUserOfCurrAddr() << " " << ((struct request_leave*)r)->req_channel << "\n";
                
                fin = leaveReq((struct request_leave*) r);
                break;
            } else {
                break;
            }
        case REQ_SAY:
            if(sizeof(struct request_say) == b) {
                cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)getAddrStruct().sin_port << " recv Request say " << getUserOfCurrAddr() << " " << ((struct request_say*)r)->req_channel << "\n";
                fin = sayReq((struct request_say*) r);
                break;
           } else {
               break;
           }
        case REQ_LIST:
            if(sizeof(struct request_list) == b) {
                 cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)ntohs(getAddrStruct().sin_port) << " recv Request list " << getUserOfCurrAddr() << "\n";
                fin = listReq((struct request_list*) r);
                break;
            } else {
                break;
            }
        case REQ_WHO:
            if(sizeof(struct request_who) == b) {
                 cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)ntohs(getAddrStruct().sin_port) << " recv Request who " << getUserOfCurrAddr() << " " << ((struct request_who*)r)->req_channel << "\n";
                fin = whoReq((struct request_who*) r);
                break;
            } else {
                break;
            }
        case S2S_JOIN:
            if(sizeof(struct request_s2s_join) == b) {
                cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)ntohs(getAddrStruct().sin_port) << " recv S2S Join " << ((struct request_s2s_join*)r)->req_s2s_channel << "\n";
               fin = req_s2sJoin((struct request_s2s_join*) r);
                break;
            } else {
                break;
            }
        case S2S_LEAVE:
            if(sizeof(struct request_s2s_leave) == b) {
                cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)ntohs(getAddrStruct().sin_port) << " recv S2S Leave " << ((struct request_s2s_leave*)r)->req_s2s_channel << "\n";
               fin = s2sLeave((struct request_s2s_leave*) r);
                break;
            } else {
                break;
            }
        case S2S_SAY:
            if(sizeof(struct request_s2s_say) == b) {
                cout << homeServerStringAddr << ":" << homeServerPort << " " << stringAddr(getAddrStruct()) <<  ":" << (int)ntohs(getAddrStruct().sin_port) << " recv S2S Say " << ((struct request_s2s_say*)r)->req_s2s_channel << "\n";
               fin = s2sSay((struct request_s2s_say*) r);
                break;
            } else {
                break;
            }
        default:

            break;
    }
    return fin;
}
//add neighbor server's struct sockaddr_in to vector.  manage host in sockaddr_in struct
int addNeighborServers(char* ip, char* port)
{
    char host[HOST_MAX];
    strcpy(host, ip);
    int tmpPort = atoi(port);
    struct hostent *tmpHost;
    if((tmpHost = gethostbyname(host))== 0) {
        cout << "error getting host name : \n";
        free(tmpHost);
        return false;
    }
    struct sockaddr_in tmpAddr;
    tmpAddr.sin_family = AF_INET;
    tmpAddr.sin_port = htons(tmpPort);
    memcpy(&tmpAddr.sin_addr, tmpHost->h_addr_list[0], tmpHost->h_length);
    neighborServers.push_back(tmpAddr);
    return true;

}
//home server connection
int connectToHomeSocket(char* ip, char* port)
{
    struct addrinfo addressTmp;
    memset(&addressTmp, 0, sizeof addressTmp);
    addressTmp.ai_family = AF_INET;
    addressTmp.ai_socktype = SOCK_DGRAM;
    addressTmp.ai_flags = AI_PASSIVE;
    int check = 0;
    if((check = getaddrinfo(ip, port, &addressTmp, &addrAr))!= 0)
    {
        return false;
    }
    if((sockfd = socket(addrAr->ai_family, addrAr->ai_socktype, addrAr->ai_protocol)) == -1)
    {
        return false;
    }
    if(bind(sockfd, addrAr->ai_addr, addrAr->ai_addrlen) == -1)
    {
        return false;
    }
    return true;
}
