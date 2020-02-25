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
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <iomanip>
using namespace std;

#define MAXBUFLEN 1024
#define LOCALHOST "127.0.0.1"
#define PORT_SERVER_A "21422" // the port users will be connecting to
#define PORT_AWS_UDP "23422"

// for calculating min distance
int minDistance(int dist[], bool sptSet[], int numVertex);

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
    cout << "The Server A is up and running using UDP on port " << PORT_SERVER_A <<"." << endl;
    
    // Phase 2. Map Construction
    // Read the file
    ifstream file ("map.txt");
    
    if(!file.is_open()){
        perror("server A: file");
        exit(1);
    }
    // Read by word
    // Make single line to separate inserting symbols for later separation
    // Division for bigger parts(i.e. unit of mapId) is '/'
    //  e.x. A ....... /B ....... /C ........
    // Division for smaller parts(i.e. each component of the map) is ' '
    //  e.x. MAPID Tp Tt Vertex Vertext Weight Vertex Vertext Weight ......
    string seg = "";
    string single = "";
    int i = 0;
    while(file >> seg){
        if((seg[0]>='A' && seg[0]<='Z') || (seg[0]>='a' && seg[0]<='z')){
            if(i==0){
                single += seg;
                i=1;
            }
            else {
                single += "/" + seg;
            }
        }
        else {
            single += " " + seg;
        }
    }
    file.close();
    
    // Separate singe line from the file
    // Seprate the bigger parts by '/'
    // Save each bigger parts into the vector of 'info'
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
    
    // Separate small parts of the each bigger segment by ' '
    // each components of each map goes into vTmp vector
    vector<vector<double> > vTmp;
    for(int i=0; i<info.size(); i++){
        vector<double> vvTmp;
        size_t pos2 = info[i].find(" ");
        string tmp2 = info[i];
        
        while(pos2 != string::npos){
            info[i].erase(pos2);
            tmp2.erase(0, pos2+1);
            pos2 = tmp2.find(" ");

            string tmpAskii = info[i].c_str();
            if((tmpAskii[0]>='A' && tmpAskii[0]<='Z') || (tmpAskii[0]>='a' && tmpAskii[0]<='z')){
                double askii = (double)((int)tmpAskii[0]);
                vvTmp.push_back(askii);
            }
            else{
                vvTmp.push_back(atof(info[i].c_str()));
            }
            info[i] = tmp2;
        }
        vvTmp.push_back(atof(info[i].c_str()));
        vTmp.push_back(vvTmp);
    }
    
    // Separate each component of each map
    // Save each compnent into each proper data type and data structures
    // Basically, all of the following structures have same corresponding index which is the index of MapID in mapId vector
    // i.e. if the index of Map A which is in the mapId vector is 0, then all other components of Map A is corresponding to index 0 of other data structures
    // mapIdx is the map data structures which is used for path finding later, and it saves the pair of (MapID, the Index of MapID in mapId vector)
    // The element of the vector v is another vector which type is int.
    // This includes each pair of (vertex, vertex, weight), and These are saved sequentially, and repeated by the unit of 3 index
    // e.g.
    // 0        1       2       3       4       5       6    7       8
    // vertext vertex weight vertext vertex weight vertext vertex weight
    vector<char> mapId;
    vector<double> prop;
    vector<double> trans;
    vector<vector<int> > v;
    map<char, int> mapIdx;
    
    for(int i=0; i<vTmp.size(); i++){
        vector<int> vEach;
        
        for(int j=0; j<vTmp[i].size(); j++){
            
            if(j==0){
                char mapIdTmp = (char)((int)(vTmp[i].at(j)));
                mapId.push_back(mapIdTmp);
                mapIdx.insert(pair<char,int>(mapIdTmp,i));
            }
            else if(j==1){
                prop.push_back(vTmp[i].at(j));
            }
            else if(j==2){
                trans.push_back(vTmp[i].at(j));
            }
            else {
                vEach.push_back((int)(vTmp[i].at(j)));
            }
        }
        v.push_back(vEach);
    }
    
    // Calculate number of edges and vertecies of each map
    // also the index corresponds to the index of MapID
    // To calculate the number of vertecies, use the set
    vector<int> numEdge;
    vector<int> numVertex;
    
    for(int i=0; i<v.size(); i++){
        set<int> s;
        int numEdgeTmp = 0;
        int numVertexTmp = 0;
        
        for(int j=0; j<v[i].size(); j++){
            if((j+1)%3==0){
                numEdgeTmp++;
            } else {
                s.insert(v[i].at(j));
            }
        }
        numVertexTmp = s.size();
        
        numEdge.push_back(numEdgeTmp);
        numVertex.push_back(numVertexTmp);
    }
    // Print the monitor
    cout << "The Server A has constructed a list of "<< mapId.size() <<" maps: " << endl;
    cout << "-------------------------------------------" << endl;
    cout << "Map ID\t\tNum Vertices\tNum Edges" << endl;
    cout << "-------------------------------------------" << endl;
    for(int i=0; i<mapId.size();i++){
        cout << mapId[i] << "\t\t" << numVertex[i] << "\t\t" << numEdge[i] << endl;
    }
    cout << "-------------------------------------------" << endl;
    
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    
    while (1) {
        // recvfrom
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP
        if ((rv = getaddrinfo(NULL, PORT_SERVER_A, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }
        // loop through all the results and bind to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                perror("server A: socket");
                continue;
            }
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("server A: bind");
                continue;
            }
            break;
        }
        if (p == NULL) {
            fprintf(stderr, "server A: failed to bind socket\n");
            return 2;
        }
        freeaddrinfo(servinfo);
        addr_len = sizeof their_addr;
        
        // Receive from AWS
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        // Get single string and seprate it by '/'
        // Save each segment into the vector of 'info'
        string singleTmp(buf);
        vector<string> infoTmp;
        size_t posTmp = singleTmp.find("/");
        string tmpTmp = singleTmp;
        
        while(posTmp != string::npos){
            singleTmp.erase(posTmp);
            tmpTmp.erase(0, posTmp+1);
            posTmp = tmpTmp.find("/");
            infoTmp.push_back(singleTmp);
            singleTmp = tmpTmp;
        }
        infoTmp.push_back(singleTmp);
        // Index Information:
        // idx 0: id, idx 1: source Idx
        string mapIdUser = infoTmp[0];
        string srcIdxUser = infoTmp[1];
        
        close(sockfd);
        
        char userInputId = mapIdUser[0];
        // Print the monitor
        cout << "The Server A has received input for finding shortest paths: starting vertex " << srcIdxUser << " of map "<< userInputId << "." << endl;
        
        // Phase 2. Path Finding
        // Find the map of user input in mapIdx map
        int inputIdx = mapIdx.find(userInputId)->second;
        // Make map vertexIdx, a pair (index used in making the graph, real value of node), to build the graph
        // Make map vertexIdxReverse, a pair (real value of node, index used in making the graph), for printing the result of the Dijkstra's
        map<int, int> vertexIdx;
        map<int, int> vertexIdxReverse;
        int k=0;
        for (int i = 0; i <v[inputIdx].size(); i+=3){
            if (vertexIdx.find(v[inputIdx].at(i)) == vertexIdx.end()) {
                vertexIdx.insert(pair<int,int>(v[inputIdx].at(i), k));
                vertexIdxReverse.insert(pair<int,int>(k++, v[inputIdx].at(i)));
            }
            if (vertexIdx.find(v[inputIdx].at(i+1)) == vertexIdx.end()) {
                vertexIdx.insert(pair<int,int>(v[inputIdx].at(i+1), k));
                vertexIdxReverse.insert(pair<int,int>(k++, v[inputIdx].at(i+1)));
            }
        }
        // Build the graph of the map of user input
        // initialize all elements as 0 in the graph 2d array
        int graph[numVertex[inputIdx]][numVertex[inputIdx]]={};
        for (int i = 0; i <v[inputIdx].size(); i+=3){
            graph[vertexIdx.find(v[inputIdx].at(i))->second][vertexIdx.find(v[inputIdx].at(i+1))->second] = v[inputIdx].at(i+2);
            graph[vertexIdx.find(v[inputIdx].at(i+1))->second][vertexIdx.find(v[inputIdx].at(i))->second] = v[inputIdx].at(i+2);
        }
        
        // Dijkstra's algorithm
        // Refer to https://www.geeksforgeeks.org/map-associative-containers-the-c-standard-template-library-stl/
        // find the shortest path from source which user input
        int src = vertexIdx.find(atoi(srcIdxUser.c_str()))->second;
        
        int dist[numVertex[inputIdx]]; // The output array.  dist[i] will hold the shortest
        // distance from src to i
        
        bool sptSet[numVertex[inputIdx]]; // sptSet[i] will be true if vertex i is included in shortest
        // path tree or shortest distance from src to i is finalized
        
        // Initialize all distances as INFINITE and stpSet[] as false
        for (int i = 0; i < numVertex[inputIdx]; i++)
            dist[i] = INT_MAX, sptSet[i] = false;
        
        // Distance of source vertex from itself is always 0
        dist[src] = 0;
        
        // Find shortest path for all vertices
        for (int count = 0; count < numVertex[inputIdx] - 1; count++) {
            // Pick the minimum distance vertex from the set of vertices not
            // yet processed. u is always equal to src in the first iteration.
            int u = minDistance(dist, sptSet, numVertex[inputIdx]);
            
            // Mark the picked vertex as processed
            sptSet[u] = true;
            
            // Update dist value of the adjacent vertices of the picked vertex.
            for (int v = 0; v < numVertex[inputIdx]; v++)
                
                // Update dist[v] only if is not in sptSet, there is an edge from
                // u to v, and total weight of path from src to  v through u is
                // smaller than current value of dist[v]
                if (!sptSet[v] && graph[u][v] && dist[u] != INT_MAX
                    && dist[u] + graph[u][v] < dist[v])
                    dist[v] = dist[u] + graph[u][v];
        }
        
        // Make distResult Map for printing
        map<int, int> distResult;
        
        for (int i = 0; i <sizeof(dist)/sizeof(dist[0]); i++){
            if (dist[i] != 0) {
                distResult.insert(pair<int,int>(vertexIdxReverse.find(i)->second, dist[i]));
            }
        }
        // Print the monitor
        map<int, int>::iterator itDistRes;
        cout << "The Server A has identified the following shortest paths:" << endl;
        cout << "-----------------------------" << endl;
        cout << "Destination\tMin Length" << endl;
        cout << "-----------------------------" << endl;
        for (itDistRes = distResult.begin(); itDistRes != distResult.end(); ++itDistRes  )
        {
            cout << itDistRes->first << "\t\t" << itDistRes->second << endl;
        }
        cout << "-----------------------------" << endl;
        
        // sendto
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        if ((rv = getaddrinfo(LOCALHOST, PORT_AWS_UDP, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            
            return 1;
        }
        // loop through all the results and make a socket
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                perror("server A: socket");
                continue;
            }
            break;
        }
        if (p == NULL) {
            fprintf(stderr, "server A: failed to create socket\n");
            return 2;
        }
        
        // Send to AWS
        // Make single string to send to AWS
        ostringstream sstream;
        sstream << setprecision(15) <<prop[inputIdx];
        string stringProp(sstream.str());
        sstream.str("");
        sstream << setprecision(15) << trans[inputIdx];
        string stringTrans(sstream.str());
        sstream.str("");
        
        string tokenToAws = stringProp+"/"+stringTrans+"/";
        
        for (itDistRes = distResult.begin(); itDistRes != distResult.end(); ++itDistRes  )
        {
            sstream << itDistRes->first;
            string distIdxTmp(sstream.str());
            sstream.str("");
            tokenToAws += distIdxTmp + "/";
        }
        
        for (itDistRes = distResult.begin(); itDistRes != distResult.end(); ++itDistRes  )
        {
            sstream << itDistRes->second;
            string distTmp(sstream.str());
            sstream.str("");
            tokenToAws += distTmp + "/";
        }
        strcpy(buf, tokenToAws.c_str());
        
        if ((numbytes = sendto(sockfd, buf, strlen(buf), 0,
                               p->ai_addr, p->ai_addrlen)) == -1) {
            perror("server A: sendto");
            exit(1);
        }
        
        freeaddrinfo(servinfo);
        
        // Print the monitor
        cout << "The Server A has sent shortest paths to AWS." << endl;
        
        close(sockfd);
    }
    return 0;
}

// for calculating min distance
int minDistance(int dist[], bool sptSet[], int numVertex)
{
    // Initialize min value
    int min = INT_MAX, min_index;
    
    for (int v = 0; v < numVertex; v++)
        if (sptSet[v] == false && dist[v] <= min)
            min = dist[v], min_index = v;
    
    return min_index;
}
