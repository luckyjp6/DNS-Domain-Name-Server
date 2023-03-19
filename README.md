# Domain Name Server (DNS)
This repo implement the domain name server that support both IPv4 and IPv6, and it also support nip service.
The DNS document：
- https://www.rfc-editor.org/rfc/rfc1035
- https://www.rfc-editor.org/rfc/rfc3596
- https://nip.io/

## Usage
Use ```make``` to generate the execution file "dns"  
Use the following intruction to start the DNS server. 
```
./dns <port-number> <path/to/the/config/file>
```

## Config File
The first line will be a IP address that our server will forward the request to if the request domain is not in the config file.
The following lines are the domain(s) and its corresponding zone file(s).
There are two sets of example files in 'config/', please follow the format to write your own config file(s).

## Implemented Query Types
- A:     IPv4 host address
- AAAA:  IPv6 host address
- CNAME: The canonical name for an alias
- NS:    Authoritative name server
- SOA:   The start of a zone of authority
- MX:    mail exchange
- TXT:   text strings

## Implemented Terminal Command
- ```reset```: re-load the configuration file.
- ```set config <file_name>```: use another config file.
- ```list```: list all the domain of current configuration.