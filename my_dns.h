#ifndef MY_DNS_H
#define MY_DNS_H

#include "my_query.h"

struct DNS_request {
    Header      header;
    char        payload[MSG_MAX];
    Question    question;
    uint32_t    nip;

    void process_request_payload() { 
        // process the payload of the request
        char *r = payload, empty[]="0"; empty[0] = 0;
        char *pos = strtok_r(r, empty, &r);
        strcpy(question.QNAME, payload);
        
        r++;
        question.qc.QTYPE = (int)r[1]; 
        question.qc.QCLASS = (int)r[3];
        
        to_host_endian();
    }
    bool check_nip(char *domain_name) {
        char *now = question.QNAME;
        char tmp_ip[NAME_LENGTH] = {0};
        int tmp_len = 0;
        for (int i = 0; i < 4; i++) {
            int len = *now;
            if (*now > 3) return false;
            now++;
            for (int j = 0; j < len; j++) {
                if (*now < '0' || *now > '9') return false;
                tmp_ip[tmp_len++] = *now;
                now++;
            }
            if (i != 4-1) tmp_ip[tmp_len++] = '.';
        }
        
        inet_pton(AF_INET, tmp_ip, &nip);

        // check domain name
        int len = *now;
        now++;
        now += len;
        if (strcmp(now, domain_name) != 0) return false;
        
        return true;
    }
    void to_host_endian() {
        header.to_host_endian();
        // question.to_host_endian();
    }
    void to_network_endian() {
        header.to_network_endian();
        // question.to_host_endian();
    }
    // void print_() {
    //     header.print_();
    //     question.print_();
    // }
};
struct  DNS_reply {
    Header      header;
    char        content[MSG_MAX];

    DNS_reply () { memset(content, 0, MSG_MAX); }

    void set_header(Header rq, int is_author) {
        header.ID = rq.ID;
        header.flg = rq.flg + (1<<15) + (is_author << 10); // set QR and AA
        header.QDCOUNT = 1;
        header.ANCOUNT = 0;
        header.NSCOUNT = 0;
        header.ARCOUNT = 0;
    }
    void add_question(Question q) {
        memcpy(content, q.QNAME, strlen(q.QNAME));        
        memcpy(content + strlen(q.QNAME)+1, &q.qc, sizeof(Question_const));
    }
    int add_content(domain dom, std::vector<int> ans_content, int from, Question q) {
        for (auto index:ans_content) {
            int name_len = strlen(dom.det[index].NAME);
            memcpy(content + from, dom.det[index].NAME, name_len); from += name_len+1;
            memcpy(content + from, &dom.det[index].r, sizeof(RR_const)); from += sizeof(RR_const);

            if (dom.det[index].r.TYPE == 1) 
                memcpy(content + from, &dom.det[index].RDATA_int_4, ntohs(dom.det[index].r.RDLENGTH)); 
            else if (dom.det[index].r.TYPE == 28)
                memcpy(content + from, &dom.det[index].RDATA_int_6, ntohs(dom.det[index].r.RDLENGTH));
            else 
                memcpy(content + from, dom.det[index].RDATA, ntohs(dom.det[index].r.RDLENGTH)); 

            from += ntohs(dom.det[index].r.RDLENGTH);
            header.ANCOUNT++;
        }
        return from;
    }
    int add_author(domain d, int from) {
        std::vector<int> author_content;
        if (header.ANCOUNT > 0) d.get_NS(author_content); // add NS
        else d.get_SOA(author_content);// add SOA
        for (auto index:author_content) {
            header.NSCOUNT++;
            int name_len = strlen(d.det[index].NAME);
            memcpy(content + from, d.det[index].NAME, name_len); from += name_len+1;
            memcpy(content + from, &d.det[index].r, sizeof(RR_const)); from += sizeof(RR_const);
            
            memcpy(content + from, d.det[index].RDATA, ntohs(d.det[index].r.RDLENGTH)); 
            from += ntohs(d.det[index].r.RDLENGTH);
        }
        
        return from;
    }
    int add_addi(std::vector<int> addi_content, domain dom, int from) {
        for (auto index:addi_content) {
            int name_len = strlen(dom.det[index].NAME);
            memcpy(content + from, dom.det[index].NAME, name_len); from += name_len+1;
            memcpy(content + from, &dom.det[index].r, sizeof(RR_const)); from += sizeof(RR_const);
            if (dom.det[index].r.TYPE == 1) 
                memcpy(content + from, &dom.det[index].RDATA_int_4, ntohs(dom.det[index].r.RDLENGTH)); 
            else if (dom.det[index].r.TYPE == 28)
                memcpy(content + from, &dom.det[index].RDATA_int_6, ntohs(dom.det[index].r.RDLENGTH));
            else if (dom.det[index].r.TYPE == 5)
                memcpy(content + from, dom.det[index].RDATA, ntohs(dom.det[index].r.RDLENGTH));
            from += ntohs(dom.det[index].r.RDLENGTH);
            header.ARCOUNT++;
        }
        return from;
    }
    int add_nip(int from, DNS_request rq) {
        int name_len = strlen(rq.question.QNAME);
        memcpy(content + from, rq.question.QNAME, name_len); from += name_len+1;
        RR_const rc;
        rc.TTL = 1; rc.TTL = ntohl(rc.TTL);
        rc.TYPE = 1; rc.TYPE1 = 0;
        rc.CLASS = 1; rc.CLASS1 = 0;
        rc.RDLENGTH = sizeof(uint32_t); rc.RDLENGTH = ntohs(rc.RDLENGTH);
        memcpy(content + from, &rc, sizeof(RR_const)); from += sizeof(RR_const);
        memcpy(content + from, &rq.nip, sizeof(uint32_t)); from += sizeof(uint32_t);
        header.ANCOUNT++;
        return from;
    }
}__attribute__((packed));

#endif