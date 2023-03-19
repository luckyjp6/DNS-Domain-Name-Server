#include <poll.h>
#include "my_domain.h"
#include "my_dns.h"

using namespace std;

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: ./dns <port-number> <config_file_name>\n");
        return -1;
    }
    int			srv, nready;
    char        config_file[PATH_LENGTH];
    config      my_conf;
    pollfd      p_fd[2];
	sockaddr_in	cliaddr, servaddr;
	socklen_t	clilen = sizeof(cliaddr);
    
    srv = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));
	if (bind(srv, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		cout << "failed to bind" << endl;
		return -1;
	}

    // get config
    memset(config_file, 0, PATH_LENGTH);
    strcpy(config_file, argv[2]);
    my_conf.read_config(config_file);
    
    // set poll events
    p_fd[0].fd = 0; // stdin
    p_fd[0].events = POLLIN;

    p_fd[1].fd = srv;
    p_fd[1].events = POLLIN;

    cout << "start service" << endl;
    cout << "% "; fflush(stdout);
    while (1) {
        nready = poll(p_fd, 2, -1);

        // get administrator input
        if (p_fd[0].revents & POLLIN) {
            char command[1000];
            memset(command, 0, 1000);
            read(0, command, 1000);
            if (memcmp(command, "reset", strlen("reset")) == 0) {
                cout << "reset configuration" << endl;
                my_conf.read_config(config_file);
            }
            else if (memcmp(command, "list", strlen("list")) == 0) {
                cout << "now using configuration in " << config_file << endl;
                my_conf.print_();
            }
            else if (memcmp(command, "set config", strlen("set config")) == 0) {
                sscanf(command, "set config %s", config_file);
                cout << "now using configuration in " << config_file << endl;
                my_conf.read_config(config_file);
            }
            else if (strlen(command) > 1) {
                cout << "wrong command" << endl;
                // Usage
            }
            fflush(stdin);
            cout << "% "; fflush(stdout);
        }
        if (p_fd[1].revents & POLLIN) {
            int         D_len, domain_index;
            DNS_request request;
            DNS_reply   reply;
            // get request
            if ((D_len = recvfrom(srv, &request, sizeof(DNS_request), 0, (sockaddr *)&cliaddr, &clilen)) < 0) {
                cout << "client error" << endl;
                continue;
            } 
            
            request.process_request_payload();
            reply.add_question(request.question);
            domain_index = my_conf.get_domain(request.question.QNAME);
            
        // handle unknown domain
            if (domain_index < 0) {
                cout << "Unknown domain. Forward the request to " << my_conf.forwardIP << endl;
                // send dns request to the "forward ip" indicate in config file
                int         resolve_fd, reply_len;
                sockaddr_in forward_addr;
                forward_addr.sin_family = AF_INET;
                forward_addr.sin_port = htons(53);
                if(inet_pton(AF_INET, my_conf.forwardIP, &forward_addr.sin_addr) != 1) {
                    cout << "** cannot convert IPv4 address for " << my_conf.forwardIP << endl;
                    return -1;
                }

                resolve_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                connect(resolve_fd, (sockaddr*)(&forward_addr), sizeof(forward_addr));
                
                // forward the request
                request.to_network_endian();
                write(resolve_fd, &request, D_len);
                
                // get the response
                reply_len = read(resolve_fd, &reply, sizeof(DNS_reply));
                sendto(srv, &reply, reply_len, 0, (sockaddr*)&cliaddr, clilen);
                cout << "% "; fflush(stdout);
                continue;
            }
            
            int reply_idx = strlen(request.question.QNAME) + sizeof(Question_const) + 1;
            
        // handle nip request
            if (request.check_nip(my_conf.dom[domain_index].domain_name)) {
                reply.set_header(request.header, true);
                reply_idx = reply.add_nip(reply_idx, request);
                reply.header.to_network_endian();
                sendto(srv, &reply, sizeof(Header)+reply_idx, 0, (sockaddr *)&cliaddr, clilen);
                continue;
            }        
            
        // handle normal request
            vector<int> ans_content, addi_content;
            reply.set_header(request.header, false);
            my_conf.dom[domain_index].get_detail(request.question, ans_content);
            reply_idx = reply.add_content(my_conf.dom[domain_index], ans_content, reply_idx, request.question);

            // add authority, except for NS type
            if (request.question.qc.QTYPE != 2) 
                reply_idx = reply.add_author(my_conf.dom[domain_index], reply_idx);
            
            // get additional addi for NS and MX type
            if (request.question.qc.QTYPE == 2 || request.question.qc.QTYPE == 15) 
                my_conf.dom[domain_index].get_addi(ans_content, addi_content);

            // add addi
            reply_idx = reply.add_addi(addi_content, my_conf.dom[domain_index], reply_idx);

            reply.header.to_network_endian();
            
            sendto(srv, &reply, sizeof(Header)+reply_idx, 0, (sockaddr *)&cliaddr, clilen);
        }
        
    }


    return 0;
}