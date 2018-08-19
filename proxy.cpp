/*
 *          Filename: proxy.cpp
 *  Last Modified on: Apr 27, 2018
 *        Developers: Rueben Tiow, Hunter Garrett
 *       Description: A simple HTTP web proxy
 */
#include "proxy.h"

//Handles CTRL-C signals
void termination_handler(int signum) {
	std::cout << std::endl << "Terminating proxy..." << std::endl;
	fflush(stdout);
	done = 1;
	shutdown(server_s, SHUT_RDWR);
	close(server_s);
	// Fake job to wake up consumers one last time
	for (int i = 0; i < MAX_THREADS; i++) {
		sem_post(job_queue_count);
	}
}

void setupSigHandlers() {
	// Handle CTRL-C signals
	signal(SIGINT, termination_handler);

	// Handle SIGPIPE signals
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGPIPE, &act, NULL);
}

//Receive a message 
void receive(int s, int size, char *ptr, bool headerOnly) {
	int received = 0;
	char* buf = ptr;
	while ((size > 0) && (received = read(s, ptr, size - 1))) {
		if (received < 0 && !done) {
			break;
		}
		ptr += received;
		size -= received;
		if (headerOnly) {
			if (strstr(buf, CRLF CRLF)) {
				break;
			}
		}
	}
}

// Send a message 
void send(int s, int size, char *ptr) {
	int sent = 0;
	while ((size > 0) && (sent = write(s, ptr, size))) {
		if (sent < 0) {
			break;
		}
		ptr += sent;
		size -= sent;
	}
}

void closeSocket(int sock, char* &Buffer) {
	if (sock > -1) {
		shutdown(sock, SHUT_RDWR);
		close(sock);
	}
	delete[] Buffer;
}

void* parse(void* threads) {
	while (!done) {
		int sock = -1;
		
		sem_wait(job_queue_count);
		pthread_mutex_lock(count_mutex);
		if (!socketQ->empty()) {
			sock = socketQ->front();
			socketQ->pop();
		}
		pthread_mutex_unlock(count_mutex);
		
		if (!done && sock >= 0) {
			char* Buffer = new char[MSG_BUF_SIZE];
			memset(Buffer, '\0', MSG_BUF_SIZE);
			receive(sock, MSG_BUF_SIZE, Buffer, true);
			
			//Header info
			char* savePtr;
			char* parsedCMD;
			char* parsedHost;
			char* parsedHttp;
			char* parsedHeaders;

			parsedCMD = strtok_r(Buffer, " ", &savePtr);
			parsedHost = strtok_r(NULL, " ", &savePtr);
			parsedHttp = strtok_r(NULL, CRLF, &savePtr);
			parsedHeaders = strtok_r(NULL, CRLF, &savePtr);

			// Verify headers
			if (!parsedCMD || (strcmp(parsedCMD, "GET") != 0) || !parsedHost
					|| !parsedHttp) {
				send(sock, strlen(errorMessage), (char*) errorMessage);
				closeSocket(sock, Buffer);
				continue;
			}

			//check HTTP version
			if (strcmp(parsedHttp, "HTTP/1.0") != 0) {
				cout << errorMessage << endl;
				closeSocket(sock, Buffer);
				continue;
			}
			
			string::size_type index;
			map<string, string> headerMap;
			while (parsedHeaders != NULL) {
				//split by :
				index = string(parsedHeaders).find(':', 0);
				if (index != string::npos) {
					pair<string, string> tempPair;
					string key = string(parsedHeaders).substr(0, index + 1);
					string value = string(parsedHeaders).substr(index + 1);
					tempPair = make_pair(key, value);
					headerMap.insert(tempPair);
				}
				parsedHeaders = strtok_r(NULL, CRLF, &savePtr);
			}
			map<string, string>::iterator it = headerMap.find(
					"connection:");
			if (it != headerMap.end()) {
				it->second = " close";
			} else {
				pair<string, string> tempPair;
				tempPair = make_pair("connection:", " close");
				headerMap.insert(tempPair);
			}

			// Remove http://
			string parsedHostStr = string(parsedHost);
			string http = "http://";
			index = parsedHostStr.find(http);
			if (index != string::npos) {
				parsedHostStr.erase(index, http.length());
			}

			string relPath;
			index = parsedHostStr.find('/');
			if (index != string::npos) {
				relPath = parsedHostStr.substr(index);
				parsedHostStr.erase(index, relPath.length());
			}
			
			/*
			string beginning = "http://www.";
			if(string(parsedHost).find(beginning) == string::npos){
				cout << errorMessage << endl;
				closeSocket(sock, Buffer);
				continue;
			}*/

			string parsedPort;
			index = parsedHostStr.find(':');
			if (index != string::npos) {
				parsedPort = parsedHostStr.substr(index + 1);
			} else {
				parsedPort = "80";
			}

			stringstream ss;
			ss << parsedCMD << " " << relPath << " " << parsedHttp << CRLF;

			for (it = headerMap.begin(); it != headerMap.end(); ++it) {
				ss << it->first << it->second << CRLF;
			}

			it = headerMap.find("host:");
			if (it == headerMap.end()) {
				ss << "Host: " << parsedHostStr << CRLF;
			}

			ss << CRLF;
			string fullRequest = ss.str();

			ss.str(string());
			ss.clear();
			
			struct sockaddr_in sa;
			bzero((char*) &sa, sizeof(sa));

			struct hostent *host = gethostbyname(parsedHostStr.c_str());
			if (!host) {
				closeSocket(sock, Buffer);
				continue;
			}

			memcpy(&sa.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
			int port = atoi(parsedPort.c_str());
			sa.sin_family = AF_INET;
			sa.sin_port = htons(port);

			int web_s;
			if ((web_s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				closeSocket(sock, Buffer);
				continue;
			}

			if (connect(web_s, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
				closeSocket(sock, Buffer);
				continue;
			}

			char* req = new char[fullRequest.length() + 1];
			memset(req, '\0', fullRequest.length() + 1);
			strcpy(req, fullRequest.c_str());
			send(web_s, strlen(req), req);
			delete[] req;
			char* responseBuf = new char[MSG_BUF_SIZE];
			memset(responseBuf, '\0', MSG_BUF_SIZE);
			receive(web_s, MSG_BUF_SIZE, responseBuf, false);

			shutdown(web_s, SHUT_RDWR);
			close(web_s);
			send(sock, MSG_BUF_SIZE, responseBuf);
			delete[] Buffer;
			delete[] responseBuf;
			shutdown(sock, SHUT_RDWR);
			close(sock);
		}
	}
	pthread_exit(EXIT_SUCCESS);
	return 0;
}

void initSynchronization() {
	count_mutex = new pthread_mutex_t;
	if (pthread_mutex_init(count_mutex, NULL) != 0) {
		error("Mutex init failed");
	}

	job_queue_count = new sem_t;
	sem_init(job_queue_count, 0, 0);
	if (job_queue_count == SEM_FAILED) {
		error("Unable to create semaphore");
	}
}

void initThreadPool() {
	// Thread pool
	pool = new pthread_t[MAX_THREADS];

	// Start consumer threads
	for (int i = 0; i < MAX_THREADS; i++) {
		if (pthread_create(&pool[i], NULL, parse, &pool[i]) < 0) {
			error("Could not create consumer thread");
		}
	}
}

int main(int argc, char *argv[]) {
	//Check number of arguments
	if (argc < 2) {
		error("usage: ./proxy <port>\n");
	}
	// Catch CTRL-C and SIGPIPE
	setupSigHandlers();
	
	// Setup thread synchronization
	initSynchronization();

	// Start the thread pool
	initThreadPool();
	
	//Socket information/properties
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	int port = atoi(argv[1]);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	
	//Create Socket
	if ((server_s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		error("Failed to open socket");
	}
	//Bind Socket
	if (bind(server_s, (struct sockaddr*) &server, sizeof(server)) < 0) {
		error("Failed to bind");
	}
	//Awaiting connections
	cout << "Listening on port " << port << "..." << endl;
	if (listen(server_s, SOMAXCONN) < 0) {
		error("Failed to listen");
	}
	//Queue for client connections
	struct sockaddr_in client;
	socketQ = new std::queue<int>();
	memset(&client, 0, sizeof(client));
	socklen_t csize = sizeof(client);
	while (!done) {
		//Accept Socket
		int client_s = accept(server_s, (struct sockaddr*) &client, &csize);
		if (client_s < 0 && !done) {
			shutdown(client_s, SHUT_RDWR);
			close(client_s);
			continue;
		}
		pthread_mutex_lock(count_mutex);
		socketQ->push(client_s);
		pthread_mutex_unlock(count_mutex);
		sem_post(job_queue_count);
	}
	
	//Deallocate & Cleanup before exit
	for (int i = 0; i < MAX_THREADS; i++) {
		pthread_join(pool[i], NULL);
	}
	pthread_mutex_destroy(count_mutex);
	if (sem_destroy(job_queue_count) < 0) {
		error("Destroy semaphore failed");
	}
	//empty queue then deallocate
	while (!socketQ->empty()) {
		socketQ->pop();
	}
	delete socketQ;
	delete[] pool;
	return 0;
}

