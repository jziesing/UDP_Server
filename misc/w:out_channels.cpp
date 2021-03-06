//UDPServer.c
 
/*  login = works
    logout = works
    
 *  gcc -o server UDPServer.c
 *  ./server
 */
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
using namespace std;
//globals
struct sockaddr recAddr;
socklen_t fromlen = sizeof(recAddr);
int sockfd;
struct addrinfo *addrAr;
map<string,vector<string> > usrTlkChan;
multimap<string,struct sockaddr_in> userToAddrStrct;
map<string,vector<pair<string,struct sockaddr_in> > > chanTlkUser;
map<string,vector<struct sockaddr_in> > chanTlkServer;
//vector<string> channels;
vector<struct sockaddr_in> neighborServers;
//methods
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
int checkAddrEq_Server(struct sockaddr_in,struct sockaddr_in);

//program
int main(int argc, char **argv)
{
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
    if((argc-1) > 2) {
        for(int i=3; i<argc-1; i=i+2) {
            addNeighborServers(argv[i], argv[i+1]);
        }
    }
    while(1)
    {
        cout << "\n"; 
        char *buf = new char[BUFLEN];
        struct request *requests = (struct request*)malloc(sizeof(struct request*) + BUFLEN);  
        int bal = 0;
        bal = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&recAddr, &fromlen);
        if(bal > 0) {
            requests = (request*) buf;
            readRequestType(requests, bal);
        } 
       delete[] buf;   
    }
    return 0;
}
//check if servers addr is epual
int checkAddrEq_Server(struct sockaddr_in a, struct sockaddr_in b)
{

}
//send ser
int sendS2SJoin_Except(string channel, sockaddr_in sender) 
{
    struct request_s2s_join server_join;
    strncpy(server_join.req_s2s_channel, channel.c_str(), CHANNEL_MAX);
    server_join.req_type = htonl(S2S_JOIN);
    int check = 0;
    vector<struct sockaddr_in>::iterator i;
    for(i=neighborServers.begin(); i != neighborServers.end(); i++) {
        if((check = checkAddrEq_Server(i, sender)) == -1) {
            int res = sendto(sockfd, &server_join, sizeof(server_join), 0, (struct sockaddr*)&(*i), sizeof(*i));
            if(res == -1) {
                cout << "error sending join to neighborServers\n";
                return -1;
            }
        } else {
            
        }
    }
    return 0;
}
//handle request server join
int req_s2sJoin(struct request_s2s_join *r) 
{
    string chan = (string)r->req_s2s_channel;
    struct sockaddr_in reqAddr = getAddrStruct();
    map<string,vector<struct sockaddr_in> >::iterator it = chanTlkServer.find(chan);
    if(it == chanTlkServer.end()) {
        //new channel
        //usersC.insert(usersC.begin(), pair<string,struct sockaddr_in>(user,reqAddr));
        //chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(chan, usersC));
        chanTlkServer.insert(pair<string,struct sockaddr_in>(chan,reqAddr));
        //channels.push_back(chan);
        int check = sendS2SJoin_Except(chan,reqAddr);
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
//server leave
int s2sLeave(struct request_s2s_leave *r) 
{


return 0;
}
//server say
int s2sSay(struct request_s2s_say *r) 
{

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
    return -1;
}
//handle say requests
int sayReq(struct request_say *rs)
{
    string channel = rs->req_channel;
    string message = rs->req_text;
    string username = getUserOfCurrAddr();
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
        strncpy(msg.txt_text, message.c_str(), sizeof(message));
        strncpy(msg.txt_channel, channel.c_str(), sizeof(channel));
        int size = sizeof(struct sockaddr*);
        goData = &msg;
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
    string username = getUserOfCurrAddr();
    multimap<string, struct sockaddr_in>::iterator sockIt = userToAddrStrct.find(username);
    userToAddrStrct.erase(sockIt);
    map<string,vector<string> >::iterator git;
    git = usrTlkChan.find(username);
    if(git != usrTlkChan.end()) {
        usrTlkChan.erase(username);
    }
    //erase user on channels in chanTlkUser
    //for(int ick=0; ick<channels.size(); ick++) {
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator sat;
    for(sat=chanTlkUser.begin(); sat != chanTlkUser.end(); sat++) {
        map<string,vector<pair<string,struct sockaddr_in> > >::iterator itck = chanTlkUser.find(sat->first);
        vector<pair<string,struct sockaddr_in> > usersC = itck->second;
        for(int j=0; j<usersC.size(); j++) {
            if(usersC[j].first == username) {
                usersC.erase(usersC.begin()+j);
            }
        }
        chanTlkUser.erase(itck);
        chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(sat->first,usersC));
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
int sendS2SJoin(string channel) 
{

    struct request_s2s_join server_join;
    strncpy(server_join.req_s2s_channel, channel.c_str(), CHANNEL_MAX);
    server_join.req_type = htonl(S2S_JOIN);
    vector<struct sockaddr_in>::iterator i;
    for(i=neighborServers.begin(); i != neighborServers.end(); i++) {
        int res = sendto(sockfd, &server_join, sizeof(server_join), 0, (struct sockaddr*)&(*i), sizeof(*i));
        if(res == -1) {
            cout << "error sending join to neighborServers\n";
            return -1;
        }

    }
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
        //channels.push_back(chan);
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
int leaveReq(struct request_leave *rl)
{
    string username = getUserOfCurrAddr();
    struct sockaddr_in reqAddr = getAddrStruct();
    string chaNel = (string)(rl->req_channel);
    multimap<string, struct sockaddr_in>::iterator ui = userToAddrStrct.find(username);
    struct sockaddr_in address = ui->second;
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator vi;
    if((vi = chanTlkUser.find(chaNel)) == chanTlkUser.end()) {
        return -1;
    }
    vector<pair<string,struct sockaddr_in> > v = vi->second;
    for(int vecI=0; vecI<v.size(); vecI++) {
        if(v[vecI].first == username) {
            if(checkAddrEq(v[vecI].second, reqAddr) == 0) {
                v.erase (v.begin()+vecI);
            } 
        }
    }
    chanTlkUser.erase(vi);
    if(v.size() != 0) {
        chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(chaNel,v));
        return 0;
    } else {
        // for(int i=0; i<channels.size(); i++) {
        //     if(channels[i] == chaNel) {
        //         channels.erase(channels.begin()+i);
        //     }
        // }
    }
    vector<string> chanTlk = usrTlkChan[username];
    for(int vv=0; vv<chanTlk.size(); vv++) {
        if(chanTlk[vv] == chaNel) {
            chanTlk.erase(chanTlk.begin()+vv);
        }
    }
    usrTlkChan[username] = chanTlk;
    return 0;
}
//handle login requests
int listReq(struct request_list *rl)
{
    string username = getUserOfCurrAddr();
    struct sockaddr_in address; 
    multimap<string, struct sockaddr_in>::iterator ui = userToAddrStrct.find(username);
    int numCHAN = chanTlkUser.size();
    address = ui->second;
    struct text_list *msg = (struct text_list*)malloc((sizeof(struct text_list)+(numCHAN *sizeof(struct channel_info))));
    msg->txt_type= TXT_LIST;
    msg->txt_nchannels = numCHAN;
    
    for (int i = 0; i<chanTlkUser.size(); i++) {
        const char* tstr = channels[i].c_str();
        strcpy(((msg->txt_channels)+i)->ch_channel, tstr);
    }
    int size = sizeof(struct sockaddr);
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
            return -1;
        } 
    }
    switch(netHost) {
        case REQ_LOGIN:
            if(sizeof(struct request_login) == b) {
                fin = loginReq((struct request_login*) r);
                break;
            } else {
                break;
            } 
        case REQ_LOGOUT:
            if(sizeof(struct request_logout) == b) {
                fin = logoutReq((struct request_logout*) r);
                break;
            } else {
                break;
            }   
        case REQ_JOIN:
            if(sizeof(struct request_join) == b) {
                fin = joinReq((struct request_join*) r);
                break;
            } else {
                break;
            }      
        case REQ_LEAVE:
            if(sizeof(struct request_leave) == b) {
                fin = leaveReq((struct request_leave*) r);
                break;
            } else {
                break;
            }
        case REQ_SAY:
            if(sizeof(struct request_say) == b) {
                fin = sayReq((struct request_say*) r);
                break;
            } else {
                break;
            }
        case REQ_LIST:
            if(sizeof(struct request_list) == b) {
                fin = listReq((struct request_list*) r);
                break;
            } else {
                break;
            }
        case REQ_WHO:
            if(sizeof(struct request_who) == b) {
                fin = whoReq((struct request_who*) r);
                break;
            } else {
                break;
            }
        case S2S_JOIN:
            if(sizeof(struct request_s2s_join) == b) {
                fin = req_s2sJoin((struct request_s2s_join*) r);
                break;
            } else {
                break;
            }
        case S2S_LEAVE:
            if(sizeof(struct request_s2s_leave) == b) {
                fin = s2sLeave((struct request_s2s_leave*) r);
                break;
            } else {
                break;
            }
        case S2S_SAY:
            if(sizeof(struct request_s2s_say) == b) {
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