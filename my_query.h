#ifndef MY_QUERY_H
#define MY_QUERY_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define MSG_MAX 100000
#define NAME_LENGTH 10000

/* 
1:  A
2:  NS
5:  CNAME
6:  SOA
15: MX
16: TXT
28: AAAA
*/

// binary part of the header
struct Header_flag {
    uint    QR : 1;     // query(0) or reponse(1)
    uint    Opcode : 4;
    uint    AA : 1;
    uint    TC : 1;
    uint    RD : 1;
    uint    RA : 1;
    uint    Z : 3;
    uint    RCODE : 4;  // error code
}__attribute__((packed));

// DNS header
struct Header {
    uint16_t     ID;
    uint16_t     flg; 
    uint16_t     QDCOUNT : 16; // number of questions
    uint16_t     ANCOUNT : 16; // number of answers
    uint16_t     NSCOUNT : 16; // number of authority(s)
    uint16_t     ARCOUNT : 16; // number of additional records

    void to_host_endian() {
        QDCOUNT = ntohs(QDCOUNT);
        ANCOUNT = ntohs(ANCOUNT);
        NSCOUNT = ntohs(NSCOUNT);
        ARCOUNT = ntohs(ARCOUNT);
        flg     = ntohs(flg);
    }
    void to_network_endian() {
        QDCOUNT = htons(QDCOUNT);
        ANCOUNT = htons(ANCOUNT);
        NSCOUNT = htons(NSCOUNT);
        ARCOUNT = htons(ARCOUNT);
        flg     = htons(flg);
    }
    int get_Opcode() {
        uint16_t tmp = flg;
        tmp = tmp << 1;
        tmp = tmp >> 12;
        return tmp;
    }
    // void print_() {
    //     using namespace std;
    //     cout << "ID: " << ID << endl;
    //     cout << "flg: " << flg << endl;
    //     cout << "QDCOUNT: " << QDCOUNT << endl;
    //     cout << "ANCOUNT: " << ANCOUNT << endl;
    //     cout << "NSCOUNT: " << NSCOUNT << endl;
    //     cout << "ARCOUNT: " << ARCOUNT << endl;
    //     cout << endl;
    // }
}__attribute__((packed));

struct Question_const {
    uint    QTYPE2 : 8 = 0;
    uint    QTYPE : 8;
    uint    QCLASS2 : 8 = 0;
    uint    QCLASS : 8;
};

struct Question {
    char    QNAME[NAME_LENGTH];  
    Question_const qc;
    // void print_() {
    //     using namespace std;
    //     cout << "question: " << QNAME << endl;
    //     cout << "QTYPE: " << qc.QTYPE << endl;
    //     cout << "QCLASS: " << qc.QCLASS << endl;
    // }
};

struct  RR_const {
    uint    TYPE1 : 8;
    uint    TYPE : 8;
    uint    CLASS1 : 8;
    uint    CLASS : 8;
    uint    TTL : 32;
    uint    RDLENGTH : 16;
}__attribute__((packed));
struct RR {
    char    name[NAME_LENGTH];
    RR_const rc;
    char    RDATA[MSG_MAX];
};

#endif