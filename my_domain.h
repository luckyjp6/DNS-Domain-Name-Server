#ifndef MY_DOMAIN_H
#define MY_DOMAIN_H

#include <iomanip>
#include "my_query.h"

#define CONTENT_LENGTH 100000
#define PATH_LENGTH    2000

// details of each domain name
struct details {
    char        NAME[NAME_LENGTH];
    RR_const    r;
    char        RDATA[CONTENT_LENGTH];
    uint32_t    RDATA_int_4;
    in6_addr    RDATA_int_6;

    details () {}
    details (char *msg, char* domain_name) {
        // process the details 
        clear(); 
        r.TYPE1 = 0; r.CLASS1 = 0; r.RDLENGTH = 0;
        char *tmp_name = strtok_r(msg, ",", &msg);
        if (strcmp(tmp_name, "@") == 0) {
            strcpy(NAME, domain_name);
        }else {
            NAME[0] = strlen(tmp_name);
            sprintf(NAME+1, "%s%s", tmp_name, domain_name);
        }        
        r.TTL = atoi(strtok_r(msg, ",", &msg)); 
        r.CLASS = 1;  strtok_r(msg, ",", &msg); 
        char *TYPE_char = strtok_r(msg, ",", &msg);
        if (strcmp(TYPE_char, "A") == 0) r.TYPE = 1;
        else if (strcmp(TYPE_char, "NS") == 0) r.TYPE = 2;
        else if (strcmp(TYPE_char, "CNAME") == 0) r.TYPE = 5;
        else if (strcmp(TYPE_char, "SOA") == 0) r.TYPE = 6;
        else if (strcmp(TYPE_char, "MX") == 0) r.TYPE = 15;
        else if (strcmp(TYPE_char, "TXT") == 0) r.TYPE = 16;
        else if (strcmp(TYPE_char, "AAAA") == 0) r.TYPE = 28;
     
        memset(RDATA, 0, CONTENT_LENGTH);
        if (r.TYPE == 1 || r.TYPE == 28) { /* A and AAAA */
            char *data = strtok_r(msg, "\r\n", &msg);
            strcpy(RDATA, data);
            if (r.TYPE == 1) {
                inet_pton(AF_INET, RDATA, &RDATA_int_4);
                r.RDLENGTH = sizeof(RDATA_int_4);
            }
            else if (r.TYPE == 28) {
                inet_pton(AF_INET6, RDATA, &RDATA_int_6);
                r.RDLENGTH = sizeof(RDATA_int_6);
            }    
        }else if (r.TYPE == 6) { /* SOA */
            for (int i = 0; i < 2; i++) {
                char *data = strtok_r(msg, " ", &msg);
                char *dot;
                while ((dot = strtok_r(data, ".", &data))) {
                    int dot_len = strlen(dot);
                    RDATA[r.RDLENGTH++] = dot_len;
                    memcpy(RDATA + r.RDLENGTH, dot, dot_len);
                    r.RDLENGTH += dot_len;
                    memset(dot, 0, strlen(dot));
                }
                r.RDLENGTH++;
            }
            char *now;
            while (now = strtok_r(msg, " \0", &msg)) {
                uint32_t to_int = atoi(now);
                to_int = htonl(to_int);
                memcpy(RDATA + r.RDLENGTH, &to_int, sizeof(uint32_t));
                r.RDLENGTH += sizeof(uint32_t);
                memset(now, 0, strlen(now));
            }
        }else if (r.TYPE == 15) { /* MX */
            char *now = strtok_r(msg, " ", &msg);
            uint16_t to_int = atoi(now);
            to_int = htons(to_int);
            memcpy(RDATA, &to_int, sizeof(uint16_t));
            r.RDLENGTH += sizeof(uint16_t);

            char *dot;
            while ((dot = strtok_r(msg, ".", &msg))) {
                int dot_len = strlen(dot);
                RDATA[r.RDLENGTH++] = dot_len;
                memcpy(RDATA + r.RDLENGTH, dot, dot_len);
                r.RDLENGTH += dot_len;
                memset(dot, 0, strlen(dot));
            }
            r.RDLENGTH++;
        }
        else if (r.TYPE == 16) { /* TXT */
            RDATA[0] = strlen(msg)-2;
            memcpy(RDATA+1, msg+1, strlen(msg)-2);
            r.RDLENGTH = strlen(msg)-1;
        }
        else { /* others */
            char *data = strtok_r(msg, "\r\n", &msg);
            char *dot;
            while ((dot = strtok_r(data, ".", &data))) {
                int dot_len = strlen(dot);
                RDATA[r.RDLENGTH++] = dot_len;
                memcpy(RDATA + r.RDLENGTH, dot, dot_len);
                r.RDLENGTH += dot_len;
                memset(dot, 0, strlen(dot));
            }
            r.RDLENGTH++;
        }
    }
    void clear() {
        memset(NAME, 0, NAME_LENGTH);
        // memset(&r, 0, sizeof(RR_const));
        memset(RDATA, 0, CONTENT_LENGTH);
        memset(&RDATA_int_4, 0, sizeof(uint32_t));
        memset(&RDATA_int_6, 0, sizeof(in6_addr));
    }
    void to_network_endian() {
        r.TTL = htonl(r.TTL);
        r.RDLENGTH = htons(r.RDLENGTH);
    }
    void print_() {
        std::cout << "subdomain name: " << std::setw(30) << NAME << "\t";
        std::cout << "TTL:" << std::setw(6) << ntohl(r.TTL) << "\t";
        std::cout << "CLASS: " << std::setw(3) << r.CLASS << "\t";
        std::cout << "TYPE: " << std::setw(3) << r.TYPE << "\t";
        std::cout << "RDLENGTH: " << std::setw(5) << ntohs(r.RDLENGTH) << "\t";
        if (r.TYPE != 1 && r.TYPE != 28) std::cout << "RDATA: " << RDATA;
    }
};

// the zone
struct domain {
    char    domain_name[NAME_LENGTH];
    char    path[PATH_LENGTH];
    std::vector<details> det;

    domain () {}
    domain (char *msg, char *dir) {
        char *pos = strtok_r(msg, ",", &msg);
        clear();
        sprintf(path, "%s/%s", dir, msg);
    }
    void read_domain () {
        int     file_fd;
        char    msg[CONTENT_LENGTH];
        char    *m, *pos, *for_dot, *dot;
        int     now_at = 0;

        if ((file_fd = open(path, O_RDONLY)) < 0) {
            std::cout << "can't open file: " << path << std::endl;
            exit(-1);
        }

        read(file_fd, msg, CONTENT_LENGTH);
        m = msg;
        for_dot = strtok_r(m, "\r\n", &m);
        while((dot = strtok_r(for_dot, ".", &for_dot)) != NULL) {
            int dot_len = strlen(dot);
            domain_name[now_at++] = dot_len;
            memcpy(domain_name + now_at, dot, dot_len);
            now_at += dot_len;
            memset(dot, 0, strlen(dot));
        }
        while(pos = strtok_r(m, "\r\n", &m)) {
            details d(pos, domain_name);
            d.to_network_endian();
            det.push_back(d);
            memset(pos, 0, strlen(pos));
        }
        close(file_fd);
    }
    void clear() {
        memset(domain_name, 0, NAME_LENGTH);
        memset(path, 0, PATH_LENGTH);
        det.clear();
    }
    void get_detail (Question q, std::vector<int> &ans_content) {
        for (int i = 0; i < det.size(); i++) {
            if (strcmp(det[i].NAME, q.QNAME) == 0
                && q.qc.QTYPE == det[i].r.TYPE) ans_content.push_back(i);
        }
    }
    void get_NS (std::vector<int> &ns) {
        for (int i = 0; i < det.size(); i++) {
            if (det[i].r.TYPE == 2) ns.push_back(i);
        }
    }
    void get_SOA (std::vector<int> &soa) {
        for (int i = 0; i < det.size(); i++) {
            if (det[i].r.TYPE == 6) soa.push_back(i);
        }
    }
    void get_addi (std::vector<int> ans_constent, std::vector<int> &addi_content) {
        for (auto ans:ans_constent) {
            char *wanted = det[ans].RDATA;
            if (det[ans].r.TYPE == 15) wanted += sizeof(uint16_t);
            for (int now = 0; now < det.size(); now++) {
                if (det[now].r.TYPE != 1 && det[now].r.TYPE != 28 && det[now].r.TYPE != 5) continue;
                if (det[ans].r.RDLENGTH != strlen(det[now].NAME))
                    if (memcmp(det[now].NAME, wanted, det[ans].r.RDLENGTH) == 0) {
                        addi_content.push_back(now);
                    }
            }
        }
        sort(addi_content.begin(), addi_content.end());
        int last_one = -1;
        for (int i = 0; i < addi_content.size(); ) {
            if (addi_content[i] == last_one) addi_content.erase(addi_content.begin()+i);
            else {
                last_one = addi_content[i];
                i++;
            }
        }
    }
    void print_ () {
        std::cout << "domain name: " << domain_name << std::endl;
        std::cout << "path: " << path << std::endl;
        for (auto d:det) {
            d.print_();
            std::cout << std::endl;
        }
    }
};

// the config file
struct config {
    char forwardIP[100];
    std::vector<domain> dom;

    void read_config(char *filename) {
        int  conf;
        char path[1000];
        char msg[CONTENT_LENGTH];
        char dir[] = "./config";
        clear();

        memset(path, 0, 1000);
        memset(msg, 0, CONTENT_LENGTH);

        sprintf(path, "%s/%s", dir, filename);
        conf = open(path, O_RDONLY);
        read(conf, msg, CONTENT_LENGTH);
        char *m = msg;
        char *pos = strtok_r(m, "\r\n", &m);
        strcpy(forwardIP, pos);
        while (strlen(m) > 0) { 
            if ((pos = strtok_r(m, "\r\n", &m)) == NULL) pos = m;
            domain d(pos, dir);
            d.read_domain();
            dom.push_back(d);
            memset(pos, 0, strlen(pos));
        }
        close(conf);
    }
    void clear() {
        memset(forwardIP, 0, 100);
        dom.clear();
    }
    int get_domain(char *wanted) {
        std::string s(wanted);
        for (int i = 0; i < dom.size(); i++) {
            if (s.find(dom[i].domain_name) != std::string::npos) return i;
        }
        return -1;
    }
    void print_() {
        std::cout << "forwardIP: " << forwardIP << std::endl;
        for (auto d: dom) {
            std::cout << "--------------------------------------" << std::endl;
            d.print_();
        }
    }
};

#endif