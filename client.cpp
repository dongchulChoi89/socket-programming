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
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
using namespace std;

#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define LOCALHOST "127.0.0.1"
#define PORT_AWS_TCP "24422" // the port client will be connecting to

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    // Print the monitor
    cout << "The client is up and running." << endl;
    
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    
    if (argc != 4) {
        fprintf(stderr,"client: args; <Map ID> <Source Vertex Index> <File Size>\n");
        exit(1);
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(LOCALHOST, PORT_AWS_TCP, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
            
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    freeaddrinfo(servinfo); // all done with this structure
        
    // Send to AWS
    // Store arguments into the buffer
    char mapId[MAXDATASIZE];
    char srcIdx[MAXDATASIZE];
    char fileSize[MAXDATASIZE];
    
    strcpy(mapId,argv[1]);
    strcpy(srcIdx,argv[2]);
    strcpy(fileSize,argv[3]);
    
    strcpy(buf,mapId);
    strcat(buf,"/");
    strcat(buf,srcIdx);
    strcat(buf,"/");
    strcat(buf,fileSize);
    strcat(buf,"/");
    
    if ((numbytes = send(sockfd, buf, strlen(buf), 0)) == -1) {
        perror("client: send");
        exit(1);
    }
    // Print the monitor
    printf("The client has sent query to AWS using TCP: start vertex %s; map %s; file size %s.\n", srcIdx, mapId, fileSize);
    
    // Receive from AWS
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    
    // Get single string and seprate it by '/'
    // Save each segment into the vector of 'info'
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
    // Convert each element of the vector into a temporary array
    // Then, using the index of the temporary array, assing elements into the proper data type and structures
    double arrTmp[info.size()];
    for (int i=0; i<info.size(); i++) {
        arrTmp[i] = atof(info[i].c_str());
    }
    int arrTmpSize = sizeof(arrTmp)/sizeof(arrTmp[0]);
    
    // Each array has same length of distIdx array because the index of each properties(i.e. Tt, Tp, delay ..) are corresponds to the distance array
    // Index Information:
    // idx [0:arrTmpSize/5-1]: distIdx, idx [arrTmpSize/5:arrTmpSize/5*2-1]: distVal, idx [arrTmpSize/5*2:arrTmpSize/5*3-1]: dTrans
    // idx [arrTmpSize/5*3:arrTmpSize/5*4-1]: dProp, idx [arrTmpSize/5*4:arrTmpSize]: delay
    int distIdx[arrTmpSize/5];
    for (int i=0; i<arrTmpSize/5; i++) {
        distIdx[i] = arrTmp[i];
    }
    int distVal[arrTmpSize/5];
    for (int i=arrTmpSize/5; i<arrTmpSize/5*2; i++) {
        int idx = i-arrTmpSize/5;
        distVal[idx] = arrTmp[i];
    }
    double dTrans[arrTmpSize/5];
    for (int i=arrTmpSize/5*2; i<arrTmpSize/5*3; i++) {
        int idx = i-arrTmpSize/5*2;
        dTrans[idx] = arrTmp[i];
    }
    double dProp[arrTmpSize/5];
    for (int i=arrTmpSize/3; i<arrTmpSize/5*4; i++) {
        int idx = i-arrTmpSize/5*3;
        dProp[idx] = arrTmp[i];
    }
    double delay[arrTmpSize/5];
    for (int i=arrTmpSize/5*4; i<arrTmpSize; i++) {
        int idx = i-arrTmpSize/5*4;
        delay[idx] = arrTmp[i];
    }
    // Print result
    cout << "The client has received results from AWS:" << endl;
    cout << "-------------------------------------------------------------------------" << endl;
    cout << "Destination\tMin Length\tTt\t\tTp\t\tDelay" << endl;
    cout << "-------------------------------------------------------------------------" << endl;
    for (int i=0; i<sizeof(distIdx)/sizeof(distIdx[0]); i++) {
        cout << fixed << setprecision(2) << distIdx[i] <<"\t\t"<< distVal[i] <<"\t\t" << dTrans[i] << "\t\t" << dProp[i] << "\t\t" << delay[i] << endl;
    }
    cout << "-------------------------------------------------------------------------" << endl;
    
    close(sockfd);
    
    return 0;
}
