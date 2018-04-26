README

Program: A simple HTTP Web Proxy
Dev: Rueben Tiow, Hunter Garrett

Strengths: Code implements all basic features of HTTP proxy in less than 500 source code.

Weaknesses: This is working on TCP connection so there is some latency due to dedicating one round
trip time into setting up TCP connection. It is not secure because it is only on HTTP and not HTTPS.