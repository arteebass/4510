README

Program: A simple HTTP Web Proxy
Dev: Rueben Tiow, Hunter Garrett

Strengths: Code implements all basic features of HTTP proxy in less than 500 source code. This proxy
is able to handle concurrent connections and requests. A pool of 30 threads is created to handle a queue
of requests. 

Weaknesses: This is working on TCP connection so there is some latency due to dedicating one round
trip time into setting up TCP connection. It is not secure because it is only on HTTP and not HTTPS. 
The implementation requires strict usage to avoid error messages, such as but not limited to: the 
HTTP request message, any typos, only GET request, and only works for websites on HTTP version 1.0.

Usage:
1. type "make" in command line to compile > press enter.
2. type "./MyProxy <port#>" in command line to start the proxy server > press enter.
3. type "telnet localhost <port#>" in command line to start telnet > press enter.
4. type "GET http://www.aDNShere.com/ HTTP/1.0" > press enter > press enter.
5. restart step 3 to run concurrent requests.