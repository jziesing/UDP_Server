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
multimap<string, int> addrToPort;
multimap<pair<string,string>, string> addrToUser;
multimap<string, pair<string,string> > userToAddr;
map<string,vector<string> > usrLisChan;
map<string,vector<string> > usrTlkChan;
map<string,vector<string> > chanTlkUser;
vector<string> channels;
//methods
int connectToSocket(char*, char*);
void err(char*);
int readRequestType(struct request*, int);
int sayReq(struct request_say*);
int checkValidAddr();
string getUserOfCurrAddr();
string getAddr_string();
string getSemiAddr_string();
int getAddr_Port();

//program
int main(int argc, char **argv)
{
    addrAr = NULL;
    sockfd = 0;
    connectToSocket(argv[1], argv[2]);
    while(1)
    {
        //for multiple requests maybe
        // requests = (struct request*) malloc(sizeof (struct request) + BUFLEN); 
        char *buf = new char[BUFLEN];
        struct request *requests = (struct request*)malloc(sizeof(struct request*) + BUFLEN);  
        int bal = 0;
        bal = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&recAddr, &fromlen);
        if(bal > 0) {
            printf("recv()'d %d bytes of data in buf\n", bal);
            requests = (request*) buf;
            cout << "do I get herer??? \n";
            readRequestType(requests, bal);  
            //print stuff
            // map<pair<string,string>,string>::iterator it;
            // if(!addrToUser.empty()) {
            //     cout << "SIZE OF AtoU: " << addrToUser.size() << "\n";
            //     for(it = addrToUser.begin(); it != addrToUser.end(); it++) {
            //                 cout << it->first.first << " is the complex address.\n";
            //                 cout << it->first.second << " is the simple address.\n";
            //                 cout << it->second << " is the user.\n";
            //     }  
            // }
            // map<string,pair<string,string> >::iterator its;
            // if(!userToAddr.empty()) {
            //     cout << "SIZE OF UtoA: " << userToAddr.size() << "\n";
            //     for(its = userToAddr.begin(); its != userToAddr.end(); its++) {
            //         cout << its->first << " is the user.\n";
            //         cout << its->second.first << " is the complex addrr.\n";
            //         cout << its->second.second << " is the simple addrr.\n";
            //     }
            // } 
            // for(int i=0; i<channels.size(); i++) {
            //     vector<string> uOnC = chanTlkUser[channels[i]];
            //     if(!uOnC.empty()) {
            //         for(int j=0; j<uOnC.size(); j++) {
            //             cout << uOnC[j] << " is user on channel: " << channels[i] << " from chanTlkUser.\n";
            //         }
            //     }
            //     chanTlkUser[channels[i]] = uOnC;
            // }   
        } 
       //requests = NULL;
       delete[] buf;   
    }
    return 0;
}
//returns string of username of current request address
string getUserOfCurrAddr()
{ 
    pair<string,string> realAddrString (getAddr_string(),getSemiAddr_string());
    multimap<pair<string,string>, string>::iterator i = addrToUser.find(realAddrString);
    string aTmp = i->second;
    return aTmp;
}
//return semi address
string getSemiAddr_string()
{
    struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;   
    char *addrString = (char*)malloc(sizeof(char)*BUFLEN);
    //make address string
    inet_ntop(AF_INET, &address, addrString, BUFLEN);
    //have tmp var
    string realAddrString = addrString;
    free (addrString);
    return realAddrString;
}
//returns string form of address
string getAddr_string() {
    //new request address info
    struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;   
    char *addrString = (char*)malloc(sizeof(char)*BUFLEN);
    //make address string
    inet_ntop(AF_INET, &(address->sin_addr), addrString, BUFLEN);
    //have tmp var
    string realAddrString = addrString;
    free (addrString);
    return realAddrString;
}
//returns string form of address
int getAddr_Port() {
    // //new request address info
    // struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;   
    // //char *addrString = (char*)malloc(sizeof(char)*BUFLEN);
    // //make address string
    // //inet_ntop(AF_INET, &(address->sin_port), addrString, BUFLEN);
    // //have tmp var
    
    // int realAddrString = address->sin_port;
    // //free (addrString);
    // return realAddrString;
    //new request address info
    struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;   
    //char *addrString = (char*)malloc(sizeof(char)*BUFLEN);
    //make address string
    //inet_ntop(AF_INET, &(address->sin_port), addrString, BUFLEN);
    //have tmp var
    
    int realAddrString = htons(address->sin_port);
    //free (addrString);
    return realAddrString;
}
//check if current request address is valid or exist in map
int checkValidAddr(struct request *r) 
{
    //OLD CODE //new request address info
    // struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;   
    // char *addrString = (char*)malloc(sizeof(char)*BUFLEN);
    // //make address string
    // inet_ntop(AF_INET, &(address->sin_addr), addrString, BUFLEN);
    // //have tmp var
    string realAddrString = getAddr_string();
    string smiAddrString = getSemiAddr_string();
    //free (addrString);
    //look in map for address
    //string aTmp = addrToUser[realAddrString];
    map<pair<string,string>,string>::iterator it = addrToUser.find(pair<string,string>(realAddrString,smiAddrString));
    if(it == addrToUser.end()) {
        cout << "super baddd addressss mann\n";
        cout << realAddrString << " that THING\n";
        return -1;
    } 
    return 0;
}
//for errors
void err(char *str)
{
    perror(str);
    exit(1);
}
//handle say requests
int sayReq(struct request_say *rs)
{
    /*
    get username of request
    get channel of request*/
    string channel = rs->req_channel;
    string message = rs->req_text;
    string username = getUserOfCurrAddr();
    cout << "Username fromuser: " << username << "\n";
    //cout << "this is username in sayReq " << username << "\n";
    //get list of users on channel from usrLisChan
    //vector<string> tmpU = usrLisChan[username];
    map<string,vector<string> >::iterator hit = chanTlkUser.find(channel);
    if(hit == chanTlkUser.end()) {
        cout << "JESUS CHRIST: " << channel <<"\n";  
        return -1; 
    }
    vector<string> tmpU = hit->second;
    //for all users on the channel
        //write the message to those users by there address
    for(int i=0; i<tmpU.size(); i++) {
        cout << "user: " << tmpU[i] << " on channel: " << channel << " in iteration on say loop:  " << i <<"\n";
        //get address of current user
        /*struct sockaddr_in6* address;
        multimap<string, pair<string,string> >::iterator ui = userToAddr.find(tmpU[i]);
        pair<string,string> ad = ui->second;
        cout << "this is address in loop of say " << ad.first << " and " << ad.second << "\n";
        char *s= (char*) malloc(sizeof(char)*BUFLEN);
        //move ad to t (address)
        strncpy(s, ad.second.c_str(), strlen(ad.second.c_str()));
        cout << ad.second << " which that should be 0 and " << ad.first << " is 127\n"; 
        //from s to address and format
        inet_pton(AF_INET, s, &(address->sin6_addr));
        
        multimap<string,int>::iterator addrToPit = addrToPort.find(ad.first);
        cout << "port as is: " << addrToPit->second << " \n";*/
        /*struct sockaddr_in* addrSEND = (struct sockaddr_in*)address;
        
        addrSEND->sin_port = addrToPit->second;
        addrSEND->sin_family = AF_INET;*/
        //cout << "PPPPPBBbb___port as is: " << address->sin_port << " \n";
        
        //address->sin_addr = addrToPit->first;
        //struct sockaddr* realAddr = (sockaddr*)address;
        //char *p= (char*) malloc(sizeof(char)*BUFLEN);
        //strncpy(p, ad.second.c_str(), strlen(ad.second.c_str()));
        //inet_pton(AF_INET, p, &realAddr);
        //setup message to send
        /*struct text_say *msg= (struct text_say*) malloc(sizeof(struct text_say));
        //set message type
        msg->txt_type = htonl(TXT_SAY);
        //add username (from) and message 
        strncpy(msg->txt_username, username.c_str(), USERNAME_MAX);
        strncpy(msg->txt_text, message.c_str(), SAY_MAX);
        strncpy(msg->txt_channel, channel.c_str(), CHANNEL_MAX);
        //send message
        int size = sizeof(struct sockaddr*);
        int res= sendto(sockfd, msg, sizeof(struct text_say), 0, (struct sockaddr*)address, size);*/
        struct sockaddr* address;
        
        //map<string, string>::iterator hit = userToAddr.find(tmpU[i]);
        //string ad = hit->second;
        multimap<string, pair<string,string> >::iterator ui = userToAddr.find(tmpU[i]);
        pair<string,string> tad = ui->second;
        string ad = tad.second;
        //cout << tad.second << " LISTEN MANN\n";
        char *s= (char*) malloc(sizeof(char)*BUFLEN);
        multimap<string,int>::iterator addrToPit = addrToPort.find(tad.first);
        cout << "portPARIS as is: " << addrToPit->second << " \n";
        //move ad to t (address)
        strncpy(s, ad.c_str(), strlen(ad.c_str()));
        //from s to address and format
        inet_pton(AF_INET, s, &address);
        //setup message to send
        struct text_say *msg= (struct text_say*) malloc(sizeof(struct text_say));
        //set message type
        msg->txt_type= htonl(TXT_SAY);
        //add username (from) and message 
        strncpy(msg->txt_username, username.c_str(), USERNAME_MAX);
        strncpy(msg->txt_text, message.c_str(), SAY_MAX);
        strncpy(msg->txt_channel, channel.c_str(), CHANNEL_MAX);
        //send message
        int size = sizeof(struct sockaddr);
        int res= sendto(sockfd, msg, sizeof(struct text_say), 0, address, size);
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
    int poort = getAddr_Port();
    cout << "this should be PORT " << poort << " \n";
    int prt = (int)poort;
    //cout << "this spot 1 \n";
    string realAddrString = getAddr_string();
    //cout << "this spot 2 \n";
    //username
    string username = rl->req_username;
    //cout << "this is the real addr string in login: " << realAddrString << "\n";

    //cout << "username in login req: " << username << "\n";
    //cout << "address in login req: " << realAddrString << "\n";
    //add address and username to map
    string smiAddr = getSemiAddr_string();
    //cout << "this spot 3 \n";
    addrToUser.insert(pair<pair<string,string>,string>(pair<string,string>(realAddrString,smiAddr), username));
    userToAddr.insert(pair<string,pair<string,string> >(username, pair<string,string>(realAddrString,smiAddr)));
    addrToPort.insert(pair<string,int>(realAddrString, prt));
    //printing
    // map<pair<string,string>,string>::iterator mit;
    // if(!addrToUser.empty()) {
    //     cout << "SIZE OF AtoU: " << addrToUser.size() << "\n";
    //     for(mit = addrToUser.begin(); mit != addrToUser.end(); mit++) {
    //                 cout << mit->first.first << " is the complex address.\n";
    //                 cout << mit->first.second << " is the simple address.\n";
    //                 cout << mit->second << " is the user.\n";
    //     }  
    // }
    // map<string,pair<string,string> >::iterator its;
    // if(!userToAddr.empty()) {
    //     cout << "SIZE OF UtoA: " << userToAddr.size() << "\n";
    //     for(its = userToAddr.begin(); its != userToAddr.end(); its++) {
    //         cout << its->first << " is the user.\n";
    //         cout << its->second.first << " is the complex addrr.\n";
    //         cout << its->second.second << " is the simple addrr.\n";
    //     }
    // } 
    //cout << "this spot 4 \n";
    //add user to common
    map<string,vector<string> >::iterator it = chanTlkUser.find("Common");
    vector<string> usersC;
    //cout << "this spot 5 \n";
    if(it == chanTlkUser.end()) {
        //cout << "this spot 6 \n";
        chanTlkUser.insert(pair<string,vector<string> >("Common", usersC));
        channels.push_back("Common");
        //cout << "this spot 7 \n";
    }
    //cout << "this spot 8 \n";
    it = chanTlkUser.find("Common");
    //cout << "this spot 9 \n";
    usersC = it->second;
    //cout << "this spot 10 \n";
    usersC.insert(usersC.begin(), username);
    //cout << "this spot 11 \n";
    chanTlkUser["Common"] = usersC;
    //add to user lisChan
    //cout << "this spot 12 \n";
    vector<string> chans;
    chans.insert(chans.begin(), "Common");
    //cout << "this spot 13 \n";
    usrLisChan.insert(pair<string,vector<string> >(username, chans));
    //cout << "this spot 14 \n";
    usrTlkChan.insert(pair<string,vector<string> >(username, chans));
    //cout << "this spot 15 \n";
    // OLD CODE//new request address inf.find()o
    // struct sockaddr_in* address = (struct sockaddr_in*)&recAddr;
    
    // char *addrString = (char*)malloc(sizeof(char)*BUFLEN);
    // //make address string
    // inet_ntop(AF_INET, &(address->sin_addr), addrString, BUFLEN);
    // //this is our readable address
    // string realAddrString = addrString;
    
    
    //free (addrString);
    //look for address in addrToUser
    //__OLD_CODE__ map<string, string>::iterator hit = addrToUser.find(realAddrString);
    // if(hit != addrToUser.end()) {
    //     addrToUser.erase(realAddrString);
    // }
    // // look for use in userToAddr
    // hit = userToAddr.find(username);
    // if(hit != userToAddr.end()) {
    //     userToAddr.erase(username);
    // }
    // // look for users in channel listen usrLisChan
    // map<string, vector<string> >::iterator git = usrLisChan.find(username);
    // if(git != usrLisChan.end()) {
    //     usrLisChan.erase(username);
    // }
    // //look for users in channel talk usrTlkChan
    // git = usrTlkChan.find(username);
    // if(git != usrTlkChan.end()) {
    //     usrTlkChan.erase(username);
    // }
    //for all channels, if there is a user in the channels talk user list chanTlkUser[i] then erase the user from the chanTlkUser and add it back to channels talk user list chanTlkUser[i]
    // for(int i=0; i<channels.size(); i++) {
    //     vector<string> uOnC = chanTlkUser[channels[i]];
    //     if(!uOnC.empty()) {
    //         for(int j=0; j<uOnC.size(); j++) {
    //             if(username == uOnC[j]) {
    //                 uOnC.erase(uOnC.begin()+j);
    //             }
    //         }
    //     }
    //     chanTlkUser[channels[i]] = uOnC;
    // }
    // addrToUser[realAddrString] = username;
    // userToAddr[username] = realAddrString;
    // map<string,string>::iterator it = addrToUser.find(realAddrString);
    // if(it != addrToUser.end()) {
        
    //     cout << it->first << " that THING is in LoGIN " << it->second << " so is that\n";
    //     //return -1;
    // } else {
    //     cout << "super baddd addressss mann\n";
    // }
    return 0;
}
//handle login requests
int logoutReq(struct request_logout *rl)
{
    //new request address info
    cout << "Logout REquesssttt \n";
    //have tmp var
    string realAddrString = getAddr_string();
    string username = getUserOfCurrAddr();
    string tmpaddr;
    //pair<string,string> realAddrString (getAddr_string(),getSemiAddr_string());
    multimap<pair<string,string>, string>::iterator i;
    for(i=addrToUser.begin(); i!=addrToUser.end(); i++) {
        if(i->second == username) {
            tmpaddr = i->first.first;
            cout << "deleting  1 \n";
            addrToUser.erase(i);
        }
    }
    multimap<string, pair<string,string> >::iterator ii;
    for(ii=userToAddr.begin(); ii!=userToAddr.end(); ii++) {
        if(ii->first == username) {
            cout << "deleting  2 \n";
            userToAddr.erase(ii);
        }
    }
    multimap<string, int>::iterator imm = addrToPort.find(tmpaddr);
    if(imm != addrToPort.end()) {
        cout << "deleting  3 \n";
        addrToPort.erase(imm);
    }

    
    // look for user in channel listen
    map<string, vector<string> >::iterator git = usrLisChan.find(username);
    if(git != usrLisChan.end()) {
        cout << "deleting  4 \n";
        usrLisChan.erase(username);
    }
    //look for user in channel talk
    git = usrTlkChan.find(username);
    if(git != usrTlkChan.end()) {
        cout << "deleting  5 \n";
        usrTlkChan.erase(username);
    }
    //erase user on channels in chanTlkUser
    cout << "before fail  " << channels.size() << " \n";

    for(int ick=0; ick<channels.size(); ick++) {
        cout << "in outerr \n";
        map<string,vector<string> >::iterator itck = chanTlkUser.find(channels[ick]);
        vector<string> usersC = itck->second;
        for(int j=0; j<usersC.size(); j++) {
            if(usersC[j] == username) {
                cout << "deleting user: " << usersC[j] << " \n";
                usersC.erase(usersC.begin()+j);
            }
        }
        chanTlkUser.erase(itck);
        chanTlkUser.insert(pair<string,vector<string> >(channels[ick],usersC));
    }
    // it = usrLisChan.find(user);
    // usrLisChan.erase(it);
    // it = usrTlkChan.find(user);
    // usrTlkChan.erase(it);
    return 0;
}
//handle login requests
int joinReq(struct request_join *rj)
{
    //create tmp vars for username and channel of request
    string chan = (string)rj->req_channel;

    string user = getUserOfCurrAddr();
    int trig = 0;
    map<string,vector<string> >::iterator it = chanTlkUser.find(chan);
    vector<string> usersC;
    if(it == chanTlkUser.end()) {
        //NEW CHANNEL
        //chanTlkUser.insert(pair<string,vector<string> >("Common", usersC));
        usersC.insert(usersC.begin(), user);
        chanTlkUser.insert(pair<string,vector<string> >(chan, usersC));
        channels.push_back(chan);

    } else {
        //old channel
        it = chanTlkUser.find(chan);
        usersC = it->second;
        for(int i=0; i<usersC.size(); i++) {
            if(usersC[i] == user) {
                //cout << "User exists in channel already!!!!!!!!!!__\n";
                return -1;
            }
        }
        usersC.insert(usersC.begin(), user);
        chanTlkUser[chan] = usersC;
    }
    //tmp vector for channel user is listening to
    vector<string> chanList = usrLisChan[user];
    //tmp string for channel user is talking to
    vector<string> chanTlk = usrTlkChan[user];
    //tmp vec for users on channel
    //add new channel to back
    chanList.push_back(chan);
    //add channel to most recent channel to back
    chanTlk.push_back(chan);
    //
    //add vectors back to map, new channel at the back.
    usrLisChan[user] = chanList;
    usrTlkChan[user] = chanTlk;
    chanList.clear();
    chanTlk.clear();
    return 0;
}
//handle login requests
int leaveReq(struct request_leave *rl)
{
    return 0;
}
//handle login requests
int listReq(struct request_list *rl)
{
    return 0;
}
//handle login requests
int whoReq(struct request_who *rw)
{
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
    //cout << netHost << " this is the request type!!!! \n";
    if(netHost != 0) {
        if(checkValidAddr(r) == -1) {
            //bad address, return
            cout << "invalid address\n";
            return -1;
        } 
    }
    switch(netHost) {
    //printf("the value isss: %s \n", ntohl(r->req_type));
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