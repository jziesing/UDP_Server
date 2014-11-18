//client.cpp

/*
 * g++ -o client client.cpp or make
 * ./client <server-ip> port# username
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <fcntl.h>
#include <iostream>
#include <set>
#include <string>
#include "duckchat.h"
#define BUFLEN 1024
#define STDIN 0

int connectToSocket(char*, char*);//socket setup
int logUserIn(char*);//send login request and join Common
int sendReqs(char buf[], struct addrinfo* addrAr);//send requests to server
int readMessageType(struct text *r, int b);//reads what type of message recieved from server
int sayMess(struct text_say* rs);
int listMess(struct text_list* rl);
int whoMess(struct text_who* rw);
int errorMess(struct text_error* re);
void err(char*);//error function*/

struct addrinfo *addrAr;
int sockfd;
char* channelName;
char tmpChannelName[CHANNEL_MAX];
char t2[CHANNEL_MAX];

std::set<std::string> myset;
std::set<std::string>::iterator it;

using namespace std;
int main(int argc, char* argv[])
{
    //initialize vars
    char buf[BUFLEN];
    addrAr = NULL;
    sockfd = 0;
    channelName = "Common";
    it = myset.begin();
    myset.insert("Common");
    //error checking correct run input
    if(argc != 4)
    {
      printf("Specify ip port username\n");
      exit(0);
    }
    //call to connect socket and call to send login request to server
    connectToSocket(argv[1], argv[2]);
    logUserIn(argv[3]);
    //printf("argv 1: %s argv 2: %s argv 3: %s\n", argv[1], argv[2], argv[3]);
    //FD_SET(sockfd, &readfds);//add our socket to our set of files to read from

    struct text* response;//for testing

    while(1)
    {
        //We want to get array of request structs
        //then evaluate request and have different method handle each request

        fd_set readfds;
        struct timeval tv;
        tv.tv_sec = .2;
        tv.tv_usec = 0;
        FD_ZERO(&readfds);//clean set
        FD_SET(STDIN, &readfds);//add stdin

        //call select to read from stdin if there is input available
        if(select(STDIN+1, &readfds, NULL, NULL, &tv) == -1){
            perror("select\n");
            exit(4);
        }//end select

        //printf("\nEnter data to send(Type exit and press enter to exit) :");
        if(FD_ISSET(STDIN, &readfds))
        {
            FD_CLR(STDIN, &readfds);
            scanf("%[^\n]",buf);
            getchar();
            //if(strcmp(buf,"exit") == 0)
              //exit(0);

            //if(sendReqs(buf, addrAr))//user is trying to say somthing to the server
            sendReqs(buf, addrAr);
                //cout << "successfull send" << endl;

        }

        int bal = 0;
        int tempsockfd = sockfd;
        bal = recvfrom(sockfd, buf, BUFLEN, 0,(struct sockaddr*)addrAr->ai_addr, &addrAr->ai_addrlen);
        if(bal > 0)//if we recieve somthing, print it.
        {
            //printf("recv()'d %d bytes of data in buf\n", bal);
            response = (text*) buf;
            readMessageType(response, bal);

        }
        else
            sockfd = tempsockfd;
    }
}//end main

void err(char *s)
{
    perror(s);
    exit(1);
}

int logUserIn(char* user)
{
    //send login request to server and then send join common request
    //printf("logging in...\n");
    //setting up structures
    struct request_login login;
    login.req_type = REQ_LOGIN;
    strcpy(login.req_username, user);
    struct request_join join;
    join.req_type = REQ_JOIN;
    strcpy(join.req_channel, "Common");
    //printf("attempting to send login_req\n");
    //sending structs
    if (sendto(sockfd, &login, sizeof(login), 0, (struct
        sockaddr*)addrAr->ai_addr, addrAr->ai_addrlen)==-1)
            err("sendto()");

    //printf("attempting to send join_req\n");
    if (sendto(sockfd, &join, sizeof(join), 0, (struct
        sockaddr*)addrAr->ai_addr, addrAr->ai_addrlen)==-1)
            err("sendto()");

    return true;
}

int connectToSocket(char* ip, char* port)
{
    struct addrinfo addressTmp;
    struct addrinfo *tmpAdAr;
    memset(&addressTmp, 0, sizeof addressTmp);
    addressTmp.ai_family = AF_INET;
    addressTmp.ai_socktype = SOCK_DGRAM;
    int check = 0;
    if((check = getaddrinfo(ip, port, &addressTmp, &addrAr))!= 0)
    {
        err("Client : getaddrinfo() NOT successful\n");
        return false;
    }
    for(tmpAdAr = addrAr; tmpAdAr != NULL; tmpAdAr = tmpAdAr->ai_next)
    {
        sockfd = socket(tmpAdAr->ai_family, tmpAdAr->ai_socktype, tmpAdAr->ai_protocol);
    //sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(sockfd == -1)
        {
            err("Client : socket() NOT successful\n");
            continue;
        }
        break;
    }
    addrAr = tmpAdAr;
    /*int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);*/
    fcntl(sockfd, F_SETFL, O_NONBLOCK);//set socket to be non-blocking
    //printf("socket and bind successful! \n");
    return true;
}

int sayMess(text_say *rs)
{
    cout << "[" << rs->txt_channel << "]" << "[" << rs->txt_username << "]:" << rs->txt_text << endl;
    return 0;
}

int listMess(text_list *rl)
{
    cout << "Existing channels:" << endl;
    for(int i = 0; i < rl->txt_nchannels; i++)
        cout << "  " << rl->txt_channels[i].ch_channel << endl;
    return 0;
}

int whoMess(text_who *rw)
{
    cout << "Users on channel "<< rw->txt_channel << ":" << endl;
    for(int i = 0; i < rw->txt_nusernames; i++)
        cout << "  " << rw->txt_users[i].us_username << endl;
    return 0;
}

int errorMess(text_error *re)
{
    cout << "error message from server: " << re->txt_error << endl;
    return 0;
}

int readMessageType(struct text *r, int b) //what type of message have we recieved?
{
    int fin = 0;
    int netHost = 0;
    netHost = ntohl(r->txt_type);
    //check if addres is a crazy number or normal
    if(netHost > 3 || netHost < 0) {
       netHost = r->txt_type;
    }
    //check if request address is valid
    /*if(netHost != 0) {
        if(checkValidAddr(r) == -1) {
            //bad address, return
            cout << "invalid address\n";
            return -1;
        }
    }*/
    switch(netHost) {
    //printf("the value isss: %s \n", ntohl(r->req_type));
        case TXT_SAY:
            if(sizeof(struct text_say) == b) {
                //cout << "say message\n";
                fin = sayMess((struct text_say*) r);
                break;
            } else {
                break;
            }
        case TXT_LIST:
            if(sizeof(struct text_list) + (((struct text_list*)r)->txt_nchannels)*sizeof(struct channel_info) == b) {
                //cout << "list message\n";
                fin = listMess((struct text_list*) r);                
                break;
            } else {
                break;
            }
        case TXT_WHO:
            //printf("join case made \n");

            if(sizeof(struct text_who) + (((struct text_who*)r)->txt_nusernames)*sizeof(struct user_info) == b) {
                //cout << "who message\n";
                fin = whoMess((struct text_who*) r);
                break;
            } else {
                break;
            }
        case TXT_ERROR:
            if(sizeof(struct text_error) == b) {
                //cout << "error message\n";
                fin = errorMess((struct text_error*) r);
                break;
            } else {
                break;
            }
        default:
            cout << "default case hit! Error occured!\n";
    }
    return fin;
}

int sendReqs(char buf[], struct addrinfo* addrAr)//parse user input and send appropriate message to server
{
    if(buf[0] == '/')
    {
        //cout << "special request" << endl;
        // /exit
        if(buf[1] == 'e' && buf[2] == 'x' && buf[3] == 'i' && buf[4] == 't' && buf[5] == '\0')
        {
            //cout << "exit detected" << endl;
            struct request_logout logout;
            logout.req_type = REQ_LOGOUT;
            int size = sizeof(struct sockaddr);
            if (sendto(sockfd, &logout, sizeof(logout), 0, (struct sockaddr*)addrAr->ai_addr, size)==-1)//addrAr->ai_addrlen
                    err("sendto()");
            exit(1);//quit program
        }
        // /join
        else if(buf[1] == 'j' && buf[2] == 'o' && buf[3] == 'i' && buf[4] == 'n' && buf[5] == ' ')
        {
            //cout << "join detected" << endl;
            struct request_join join;
            join.req_type = REQ_JOIN;
            int i;
            for(i = 0; i < CHANNEL_MAX; i++)
            {
                if(buf[i+6] == '\0')
                {
                    tmpChannelName[i] = '\0';
                    break;
                }
                tmpChannelName[i] = buf[i + 6];
            }
            //it=mylist.begin();

            //cout << "Inserting " << tmpChannelName << " in set" << endl;
            std::string str((char*)tmpChannelName);
            myset.insert(str);

            channelName = tmpChannelName;
            strcpy(join.req_channel, channelName);
            int size = sizeof(struct sockaddr);
            if (sendto(sockfd, &join, sizeof(join), 0, (struct sockaddr*)addrAr->ai_addr, size)==-1)//addrAr->ai_addrlen
                    err("sendto()");
        }
        // /leave
        else if(buf[1] == 'l' && buf[2] == 'e' && buf[3] == 'a' && buf[4] == 'v' && buf[5] == 'e' && buf[6] == ' ')
        {
            //cout << "leave decected" << endl;
            if(myset.size() == 0)//empty set, nothing to leave
                return(0);
            struct request_leave leave;
            leave.req_type = REQ_LEAVE;
            int i;
            for(i = 0; i < CHANNEL_MAX; i++)
            {
                if(buf[i+7] == '\0')
                {
                    tmpChannelName[i] = '\0';
                    break;
                }
                tmpChannelName[i] = buf[i + 7];
            }
            std::string strn1((char*)tmpChannelName);
            it=myset.find(strn1);
            if(it == myset.end())
            {
                cout << "Error: No channel " << tmpChannelName << " exists" << endl;
                it = myset.begin();
                std::string tempString = *it;
                channelName = (char*)tempString.c_str();
                return 0;
            }
            else
            {
                myset.erase(strn1);
                if(myset.size() != 0)
                {
                    it = myset.begin();
                    std::string tempString2 = *it;
                    channelName = (char*)tempString2.c_str();
                }
                else
                    channelName = "";

            }
            strcpy(leave.req_channel, tmpChannelName);
            int size = sizeof(struct sockaddr);
            if (sendto(sockfd, &leave, sizeof(leave), 0, (struct sockaddr*)addrAr->ai_addr, size)==-1)//addrAr->ai_addrlen
                    err("sendto()");
        }
        // /list
        else if(buf[1] == 'l' && buf[2] == 'i' && buf[3] == 's' && buf[4] == 't' && buf[5] == '\0')
        {
            //cout << "list detected" << endl;
            struct request_list list;
            list.req_type = REQ_LIST;
            int size = sizeof(struct sockaddr);
            if (sendto(sockfd, &list, sizeof(list), 0, (struct sockaddr*)addrAr->ai_addr, size)==-1)//addrAr->ai_addrlen
                    err("sendto()");
        }
        // /who
        else if(buf[1] == 'w' && buf[2] == 'h' && buf[3] == 'o' && buf[4] == ' ')
        {
            //cout << "who dectected" << endl;
            struct request_who who;
            who.req_type = REQ_WHO;
            int i;
            for(i = 0; i < CHANNEL_MAX; i++)
            {
                if(buf[i+5] == '\0')
                {
                    tmpChannelName[i] = '\0';
                    //channelName[i] = '\0';
                    break;
                }
                tmpChannelName[i] = buf[i + 5];
                //cout << "tmp[i] = " << tmp[i] << endl;
            }
            if(myset.size() != 0)//if we are on a channel
            {
                channelName = tmpChannelName;
                strcpy(who.req_channel, channelName);
            }
            else
                strcpy(who.req_channel, tmpChannelName);
            int size = sizeof(struct sockaddr);
            if (sendto(sockfd, &who, sizeof(who), 0, (struct sockaddr*)addrAr->ai_addr, size)==-1)//addrAr->ai_addrlen
                    err("sendto()");
        }
        // /switch
        else if(buf[1] == 's' && buf[2] == 'w' && buf[3] == 'i' &&
                buf[4] == 't' && buf[5] == 'c' && buf[6] == 'h' && buf[7] == ' ')
        {
            int i;
            //bool found = false;//valid channel check
            //char t2[CHANNEL_MAX];
            strcpy(t2, channelName);
            std::string str;
            for(i = 0; i < CHANNEL_MAX; i++)
            {
                if(buf[i+8] == '\0')
                {
                    tmpChannelName[i] = '\0';
                    break;
                }
                tmpChannelName[i] = buf[i + 8];
            }
            //check if user has specified valid channel
            //str((char*)tmpChannelName);
            //mylist.push_back(str);
            /*for(it=mylist.begin(); it!=mylist.end(); ++it)
            {
                if(strcmp(*it.second.cstr(), (char*)tmpChannelName) == 0)
                {
                    found = true;
                    cout << "found channel name "<< *it << " in vector" << endl;
                     channelName = tmpChannelName;
                    break;
                }
            }*/
            std::string strn((char*)tmpChannelName);
            it=myset.find(strn);
            if(it == myset.end())
            {
                cout << "You have not subscribed to channel " << tmpChannelName << endl;
                channelName = t2;
            }
            else
                channelName = tmpChannelName;
        }

    }
    // must be a say request
    else
    {
        struct request_say* say = new struct request_say;
        say->req_type = REQ_SAY;
        if(strcmp("", channelName) != 0)
        {
            strcpy(say->req_channel, channelName);//copy channel name into usable format
            strcpy(say->req_text, buf);
            int size = sizeof(struct sockaddr);
            if (sendto(sockfd, say, sizeof(*say), 0, (struct sockaddr*)addrAr->ai_addr, size)==-1)//addrAr->ai_addrlen
                    err("sendto()");
        }
        delete say;
    }
    return 1;
}
