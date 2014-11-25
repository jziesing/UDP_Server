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
vector<string> channels;
//methods
int connectToSocket(char*, char*);
int readRequestType(struct request*, int);
int sayReq(struct request_say*);
int checkValidAddr();
string getUserOfCurrAddr();
int getAddr_Port();
struct sockaddr_in getAddrStruct();
int checkAddrEq(struct sockaddr_in, struct sockaddr_in);
string stringAddr(struct sockaddr_in);

//program
int main(int argc, char **argv)
{
    addrAr = NULL;
    sockfd = 0;
    connectToSocket(argv[1], argv[2]);
    fd_set sockfds;
    //struct timeval timeV;
    while(1)
    {
        //for multiple requests maybe
        // requests = (struct request*) malloc(sizeof (struct request) + BUFLEN);
        // timeV.tv_sec = 120;
        // timeV.tv_usec = 0;
        // FD_ZERO(&sockfds);
        // FD_SET(sockfd, &sockfds);
        // int ret;
        // ret = select(sockfd+1, &sockfds, NULL, NULL, &timeV);

        cout << "\n"; 
        char *buf = new char[BUFLEN];
        struct request *requests = (struct request*)malloc(sizeof(struct request*) + BUFLEN);  
        int bal = 0;
        bal = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&recAddr, &fromlen);
        if(bal > 0) {
            printf("recv()'d %d bytes of data in buf\n", bal);
            requests = (request*) buf;
            cout << "do I get herer??? \n";
            readRequestType(requests, bal);
        } 
        cout << "printing user talk channel map:\n";
        for(map<string,vector<string> >::iterator imu = usrTlkChan.begin(); imu != usrTlkChan.end(); imu++) {
            cout << "User: " << imu->first << " talking on channel: " << imu->second[0] << "\n"; 
        }
        cout << "printing user to addres map:\n";
        cout << "user to addres map size is: " << userToAddrStrct.size() <<"\n";
        for(multimap<string,struct sockaddr_in>::iterator isu = userToAddrStrct.begin(); isu != userToAddrStrct.end(); isu++) {
            cout << "User: " << isu->first << " with address: " << stringAddr(isu->second) << " with port #: " << isu->second.sin_port <<"\n"; 
        }
        cout << "printing users on each channel: \n";
        for(int ick=0; ick<channels.size(); ick++) {
            cout << channels[ick] << " has users:\n";
            map<string,vector<pair<string,struct sockaddr_in> > >::iterator itck = chanTlkUser.find(channels[ick]);
            vector<pair<string,struct sockaddr_in> > usersC = itck->second;
            for(int j=0; j<usersC.size(); j++) {
                cout << " : "<< usersC[j].first << " : " ;
            }
            cout << "\n";
        }
       delete[] buf;   
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
        cout << "before checkEQ call\n";
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
    cout << "Username fromuser: " << username << "\n";
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator hit = chanTlkUser.find(channel);
    if(hit == chanTlkUser.end()) {
        cout << "JESUS CHRIST: " << channel <<"\n";  
        return -1; 
    }
    vector<pair<string,struct sockaddr_in> > tmpU = hit->second;
    for(int i=0; i<tmpU.size(); i++) {
        struct sockaddr_in address;
        address = tmpU[i].second;
        struct text_say *msg = (struct text_say*) malloc(sizeof(struct text_say) + BUFLEN);
        msg->txt_type= htonl(TXT_SAY);
        char *AAA = (char*)malloc(sizeof(char)*BUFLEN);
        inet_ntop(AF_INET, &(address.sin_addr), AAA, BUFLEN);
        string printString = AAA;
        strncpy(msg->txt_username, username.c_str(), USERNAME_MAX);
        strncpy(msg->txt_text, message.c_str(), SAY_MAX);
        strncpy(msg->txt_channel, channel.c_str(), CHANNEL_MAX);
        int size = sizeof(struct sockaddr*);
        cout << "sockfd is: " << sockfd << " there!\n";
        int res= sendto(sockfd, msg, sizeof(msg), 0, (struct sockaddr*)&address, sizeof(address));
        if (res == -1) {
            cout << "sendto very badd \n";
            //return -1;
        }
        free(msg);
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
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator it = chanTlkUser.find("Common");
    vector<pair<string,struct sockaddr_in> > usersC;
    if(it == chanTlkUser.end()) {
        chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >("Common", usersC));
        channels.push_back("Common");
    }
    it = chanTlkUser.find("Common");
    usersC = it->second;
    usersC.insert(usersC.begin(), pair<string,struct sockaddr_in>(username,strctAddr));
    chanTlkUser["Common"] = usersC;
    vector<string> uTalkVect;
    uTalkVect.insert(uTalkVect.begin(),"Common");
    usrTlkChan.insert(pair<string,vector<string> >(username, uTalkVect));    
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
        cout << "deleting  5 \n";
        usrTlkChan.erase(username);
    }
    //erase user on channels in chanTlkUser
    for(int ick=0; ick<channels.size(); ick++) {
        cout << "in outerr \n";
        map<string,vector<pair<string,struct sockaddr_in> > >::iterator itck = chanTlkUser.find(channels[ick]);
        vector<pair<string,struct sockaddr_in> > usersC = itck->second;
        for(int j=0; j<usersC.size(); j++) {
            if(usersC[j].first == username) {
                //cout << "deleting user: " << usersC[j] << " \n";
                usersC.erase(usersC.begin()+j);
            }
        }
        chanTlkUser.erase(itck);
        chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(channels[ick],usersC));
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
    cout << "USER IN JOIN METHOD: " << user << "\n";
    int trig = 0;
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator it = chanTlkUser.find(chan);
    vector<pair<string,struct sockaddr_in> > usersC;
    if(it == chanTlkUser.end()) {
        //NEW CHANNEL

        usersC.insert(usersC.begin(), pair<string,struct sockaddr_in>(user,reqAddr));
        chanTlkUser.insert(pair<string,vector<pair<string,struct sockaddr_in> > >(chan, usersC));
        channels.push_back(chan);

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
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator vi = chanTlkUser.find(chaNel);
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
        for(int i=0; i<channels.size(); i++) {
            if(channels[i] == chaNel) {
                channels.erase(channels.begin()+i);
            }
        }
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
    int res= sendto(sockfd, msg,  (sizeof(struct text_list)+(numCHAN *sizeof(struct channel_info))), 0, (struct sockaddr*)&address, size);
    if (res == -1) {
        cout << "sendto very badd \n";
        //return -1;
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
    map<string,vector<pair<string,struct sockaddr_in> > >::iterator vi = chanTlkUser.find(chaNel);
    int numCHAN = (vi->second).size();
    vector<pair<string,struct sockaddr_in> > v = vi->second;
    address = ui->second;
    struct text_who *msg = (struct text_who*)malloc(sizeof(struct text_who)+(numCHAN* sizeof(struct user_info)));
    cout <<"right heree\n";
    msg->txt_type= TXT_WHO;
    cout <<" ORRRright heree\n";
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
        cout << "sendto very badd \n";
        //return -1;
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
    if(netHost > 6 || netHost < 0) {
       netHost = r->req_type;
    }
    //check if request address is valid
    if(netHost != 0) {
        if(checkValidAddr() == -1) {
            cout << "invalid address\n";
            return -1;
        } 
    }
    switch(netHost) {
        case REQ_LOGIN:
            if(sizeof(struct request_login) == b) {
                cout << "login request\n";
                fin = loginReq((struct request_login*) r);
                break;
            } else {
                cout << "login request FAILED\n";
                break;
            } 
        case REQ_LOGOUT:
            if(sizeof(struct request_logout) == b) {
                cout << "logout request\n";
                fin = logoutReq((struct request_logout*) r);
                break;
            } else {
                cout << "logout request bad size\n";
                break;
            }   
        case REQ_JOIN:
            //printf("join case made \n");
            if(sizeof(struct request_join) == b) {
                cout << "join request\n";
                fin = joinReq((struct request_join*) r);
                break;
            } else {
                cout << "switch request bad size\n";
                break;
            }      
        case REQ_LEAVE:
            if(sizeof(struct request_leave) == b) {
                cout << "leave request\n";
                fin = leaveReq((struct request_leave*) r);
                break;
            } else {
                cout << "leave request bad size\n";
                break;
            }
        case REQ_SAY:
            if(sizeof(struct request_say) == b) {
                cout << "say request\n";
                fin = sayReq((struct request_say*) r);
                break;
            } else {
                cout << "say request bad size\n";
                break;
            }
        case REQ_LIST:
            if(sizeof(struct request_list) == b) {
                cout << "list request\n";
                fin = listReq((struct request_list*) r);
                break;
            } else {
                cout << "list request bad size\n";
                break;
            }
        case REQ_WHO:
            if(sizeof(struct request_who) == b) {
                cout << "who request\n";
                fin = whoReq((struct request_who*) r);
                break;
            } else {
                cout << "who request bad size\n";
                break;
            }
        default:
            cout << "default case hit!!\n";
    }
    return fin;
}

int connectToSocket(char* ip, char* port)
{
    struct addrinfo addressTmp;
    memset(&addressTmp, 0, sizeof addressTmp);
    addressTmp.ai_family = AF_INET;
    addressTmp.ai_socktype = SOCK_DGRAM;
    addressTmp.ai_flags = AI_PASSIVE;
    int check = 0;
    if((check = getaddrinfo(ip, port, &addressTmp, &addrAr))!= 0)
    {
        cout << "Server : getaddrinfo() NOT successful \n";
        return false;
    }
    if((sockfd = socket(addrAr->ai_family, addrAr->ai_socktype, addrAr->ai_protocol)) == -1)
    {
        cout << "Server : socket() NOT successful \n";
        return false;
    }
    if(bind(sockfd, addrAr->ai_addr, addrAr->ai_addrlen) == -1)
    {
        cout << "Server : bind() NOT successful \n";
        return false;
    }
    cout << "socket and bind successful! \n";
    return true;
}