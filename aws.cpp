/*
 Socket Programming
 Dongchul Choi(USC ID: 7806678422) EE450 Session 2
 The basic overall structures and codes related socket programming are from 'Beej-Guide-Network-Programming'
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define MAXBUFLEN 1024
#define LOCALHOST "127.0.0.1"
#define PORT_AWS_UDP "23422"
#define PORT_AWS_TCP "24422"
#define PORT_SERVER_A "21422" // the port users will be connecting to
#define PORT_SERVER_B "22422"

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    // Print the monitor
    cout << "The AWS is up and running." << endl;
    
    // TCP between Client
    char buf[MAXDATASIZE];
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int numbytes;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    if ((rv = getaddrinfo(NULL, PORT_AWS_TCP, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("AWS: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("AWS: bind");
            continue;
        }
        break;
    }
    
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL) {
        fprintf(stderr, "AWS: failed to bind\n");
        exit(1);
    }
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    while(1) { // main accept() loop
        
        
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the AWS
            
            // TCP: Receive from client
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1){
                perror("recv");
                exit(1);
            }
            buf[numbytes] = '\0';
            
            // Get single string and seprate it by '/'
            // Save each segment into the vector of 'info'
            // Index Information
            // idx 0: id, idx 1: source Idx, idx 2: fileSize
            string single(buf);
            vector<string> info;
            size_t pos = single.find("/");
            string tmp = single;
            
            while(pos != string::npos){
                single.erase(pos);
                tmp.erase(0, pos+1);
                pos = tmp.find("/");
                info.push_back(single);
                single = tmp;
            }
            info.push_back(single);
            
            // When user inputs the lowercase alphabet
            // Make it uppercase alphabet
            char tmpMapId = info[0].c_str()[0];
            string mapId = info[0];
            string srcIdx = info[1];
            string fileSize = info[2];
            
            // Print the monitor
            cout << "The AWS has received map ID "
            << mapId <<", start vertex "
            << srcIdx << " and file size "
            << fileSize <<" from the client using TCP over port "
            << PORT_AWS_TCP << endl;
            
            //UDP: between server A
            // sendto
            int sockfd_udp_A;
            socklen_t addr_len;
            
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;
            // receiver port
            if ((rv = getaddrinfo(LOCALHOST, PORT_SERVER_A, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                
                return 1;
            }
            // loop through all the results and make a socket
            for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd_udp_A = socket(p->ai_family, p->ai_socktype,
                                           p->ai_protocol)) == -1) {
                    perror("AWS: socket");
                    continue;
                }
                break;
            }
            if (p == NULL) {
                fprintf(stderr, "AWS: failed to create socket\n");
                return 2;
            }
            // Send to Server A
            // send the map ID and starting vertex to server A
            string tmpToServerA = mapId+"/"+srcIdx+"/";
            strcpy(buf, tmpToServerA.c_str());
    
            if ((numbytes = sendto(sockfd_udp_A, buf, strlen(buf), 0,
                                   p->ai_addr, p->ai_addrlen)) == -1) {
                perror("AWS: sendto");
                exit(1);
            }
            freeaddrinfo(servinfo);
            // Print the monitor
            cout << "The AWS has sent map ID and starting vertex to server A using UDP over port "<< PORT_AWS_UDP << endl;
            
            close(sockfd_udp_A);
            
            // recvfrom
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_flags = AI_PASSIVE; // use my IP
            
            if ((rv = getaddrinfo(NULL, PORT_AWS_UDP, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
            }
            // loop through all the results and bind to the first we can
            for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd_udp_A = socket(p->ai_family, p->ai_socktype,
                                           p->ai_protocol)) == -1) {
                    perror("AWS: socket");
                    continue;
                }
                if (bind(sockfd_udp_A, p->ai_addr, p->ai_addrlen) == -1) {
                    close(sockfd_udp_A);
                    perror("AWS: bind");
                    continue;
                }
                break;
            }
            if (p == NULL) {
                fprintf(stderr, "AWS: failed to bind socket\n");
                return 2;
            }
            freeaddrinfo(servinfo);
            addr_len = sizeof their_addr;
            
            // Receive from Server A
            if ((numbytes = recvfrom(sockfd_udp_A, buf, MAXBUFLEN-1 , 0,
                                     (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                perror("recvfrom");
                exit(1);
            }
            buf[numbytes] = '\0';
            // Get single string and seprate it by '/'
            // Save each segment into the vector of 'info'
            // Index Information
            // idx 0: prop, idx 1: trans, idx [2:info2.size()-1]: dist
            string single2(buf);
            vector<string> info2;
            size_t pos2 = single2.find("/");
            string tmp2 = single2;
            
            while(pos2 != string::npos){
                single2.erase(pos2);
                tmp2.erase(0, pos2+1);
                pos2 = tmp2.find("/");
                info2.push_back(single2);
                single2 = tmp2;
            }
            string prop = info2[0];
            string trans = info2[1];
            info2.erase(info2.begin());
            info2.erase(info2.begin());
            
            // Convert each element of the vector into a temporary array
            // Then, using the index of the temporary array, assing elements into the proper data type and structures
            int distTmp[info2.size()];
            for (int i=0; i<info2.size(); i++) {
                distTmp[i] = atoi(info2[i].c_str());
            }
            int distTmpSize = sizeof(distTmp)/sizeof(distTmp[0]);
            
            // Each array has same length of distIdx array because the index of each properties(i.e. Tt, Tp, delay ..) are corresponds to the distance array
            // Index Information:
            // idx [0:distTmpSize/2-1]: distIdx, idx [distTmpSize/5:distTmpSize-1]: distVal
            int distIdx[distTmpSize/2];
            for (int i=0; i<distTmpSize/2; i++) {
                distIdx[i] = distTmp[i];
            }
            int distVal[distTmpSize/2];
            for (int i=distTmpSize/2; i<distTmpSize; i++) {
                int idx = i-distTmpSize/2;
                distVal[idx] = distTmp[i];
            }
            
            // Print the monitor
            cout << "The AWS has received shortest path from server A:" << endl;
            cout << "-----------------------------" << endl;
            cout << "Destination\tMin Length" << endl;
            cout << "-----------------------------" << endl;
            for (int i = 0; i < sizeof(distIdx)/sizeof(distIdx[0]); i++){
                cout << distIdx[i] << "\t\t" << distVal[i] << endl;
            }
            cout << "-----------------------------" << endl;
            
            close(sockfd_udp_A);
            
            
            //UDP between server B
            // sendto
            int sockfd_udp_B;
            socklen_t addr_len_B;
            
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;
            // receiver port
            if ((rv = getaddrinfo(LOCALHOST, PORT_SERVER_B, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                
                return 1;
            }
            // loop through all the results and make a socket
            for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd_udp_B = socket(p->ai_family, p->ai_socktype,
                                           p->ai_protocol)) == -1) {
                    perror("AWS: socket");
                    continue;
                }
                break;
            }
            if (p == NULL) {
                fprintf(stderr, "AWS: failed to create socket\n");
                return 2;
            }
            
            // Send to ServerB
            // Make single string token to send
            string tokenToB = prop+"/"+trans+"/"+fileSize+"/";
            ostringstream sstream;
            for (int i=0; i<sizeof(distIdx)/sizeof(distIdx[0]); i++)
            {
                sstream << distIdx[i];
                string distIdxTmp(sstream.str());
                sstream.str("");
                tokenToB += distIdxTmp + "/";
            }
            for (int i=0; i<sizeof(distVal)/sizeof(distVal[0]); i++)
            {
                sstream << distVal[i];
                string distValTmp(sstream.str());
                sstream.str("");
                tokenToB += distValTmp + "/";
            }
            strcpy(buf, tokenToB.c_str());
            
            if ((numbytes = sendto(sockfd_udp_B, buf, strlen(buf), 0,
                                   p->ai_addr, p->ai_addrlen)) == -1) {
                perror("AWS: sendto");
                exit(1);
            }
            freeaddrinfo(servinfo);
            
            // Print the monitor
            cout << "The AWS has sent path length, propagation speed and transmission speed to server B using UDP over port " << PORT_AWS_UDP << endl;
            
            close(sockfd_udp_B);
            
            // recvfrom
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_flags = AI_PASSIVE; // use my IP
            // myport
            if ((rv = getaddrinfo(NULL, PORT_AWS_UDP, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
            }
            // loop through all the results and bind to the first we can
            for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd_udp_B = socket(p->ai_family, p->ai_socktype,
                                           p->ai_protocol)) == -1) {
                    perror("AWS: socket");
                    continue;
                }
                if (bind(sockfd_udp_B, p->ai_addr, p->ai_addrlen) == -1) {
                    close(sockfd_udp_B);
                    perror("AWS: bind");
                    continue;
                }
                break;
            }
            if (p == NULL) {
                fprintf(stderr, "AWS: failed to bind socket\n");
                return 2;
            }
            freeaddrinfo(servinfo);
            addr_len_B = sizeof their_addr;
            
            // Receive from serverB
            if ((numbytes = recvfrom(sockfd_udp_B, buf, MAXBUFLEN-1 , 0,
                                     (struct sockaddr *)&their_addr, &addr_len_B)) == -1) {
                perror("recvfrom");
                exit(1);
            }
            buf[numbytes] = '\0';
            
            // Get single string and seprate it by '/'
            // Save each segment into the vector of 'info'
            // Index Information
            // idx [0:distArrSize-1]: dTrans, idx [distArrSize:2*distArrSize-1]: dProp, idx [2*distArrSize:3*distArrSize-1]: delay
            int distArrSize = sizeof(distIdx)/sizeof(distIdx[0]);
            string single3(buf);
            vector<string> info3;
            size_t pos3 = single3.find("/");
            string tmp3 = single3;
            
            while(pos3 != string::npos){
                single3.erase(pos3);
                tmp3.erase(0, pos3+1);
                pos3 = tmp3.find("/");
                info3.push_back(single3);
                single3 = tmp3;
            }
            // Convert each element of the vector into a temporary array
            // Then, using the index of the temporary array, assing elements into the proper data type and structures
            double arrTmp[info3.size()];
            for (int i=0; i<info3.size(); i++) {
                arrTmp[i] = atof(info3[i].c_str());
            }
            int arrTmpSize = sizeof(arrTmp)/sizeof(arrTmp[0]);
            double dTrans[arrTmpSize/3];
            for (int i=0; i<arrTmpSize/3; i++) {
                dTrans[i] = arrTmp[i];
            }
            double dProp[arrTmpSize/3];
            for (int i=arrTmpSize/3; i<(arrTmpSize/3*2); i++) {
                int idx = i-arrTmpSize/3;
                dProp[idx] = arrTmp[i];
            }
            double delay[arrTmpSize/3];
            for (int i=(arrTmpSize/3*2); i<arrTmpSize; i++) {
                int idx = i-arrTmpSize/3*2;
                delay[idx] = arrTmp[i];
            }
            
            // Print the monitor
            cout << "The AWS has received delays from server B:" << endl;
            cout << "-----------------------------------------------------------" << endl;
            cout << "Destination\tTt\t\tTp\t\tDelay" << endl;
            cout << "-----------------------------------------------------------" << endl;
            for (int i=0; i<distArrSize; i++) {
                cout << fixed << setprecision(2) << distIdx[i] << "\t\t"<< dTrans[i] << "\t\t" << dProp[i] << "\t\t" << delay[i] << endl;
            }
            cout << "-----------------------------------------------------------" << endl;
            
            close(sockfd_udp_B);
            
            // TCP: Send to client
            // Make single string token to send
            string tokenToClient = "";
            sstream;
            for (int i=0; i<distArrSize; i++)
            {
                sstream << distIdx[i];
                string sstreamTmp(sstream.str());
                sstream.str("");
                tokenToClient += sstreamTmp + "/";
            }
            for (int i=0; i<distArrSize; i++)
            {
                sstream << distVal[i];
                string sstreamTmp(sstream.str());
                sstream.str("");
                tokenToClient += sstreamTmp + "/";
            }
            for (int i=0; i<distArrSize; i++)
            {
                sstream << fixed << setprecision(2) << dTrans[i];
                string sstreamTmp(sstream.str());
                sstream.str("");
                tokenToClient += sstreamTmp + "/";
            }
            for (int i=0; i<distArrSize; i++)
            {
                sstream << fixed << setprecision(2) << dProp[i];
                string sstreamTmp(sstream.str());
                sstream.str("");
                tokenToClient += sstreamTmp + "/";
            }
            for (int i=0; i<distArrSize; i++)
            {
                sstream << fixed << setprecision(2) << delay[i];
                string sstreamTmp(sstream.str());
                sstream.str("");
                tokenToClient += sstreamTmp + "/";
            }
            strcpy(buf, tokenToClient.c_str());
            
            if ((numbytes=send(new_fd, buf, strlen(buf), 0)) == -1){
                perror("send");
                exit(1);
            }
            
            // Print the monitor
            cout << "The AWS has sent calculated delay to client using TCP over port "<< PORT_AWS_TCP << "." << endl;
            
            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }
    return 0;
}
