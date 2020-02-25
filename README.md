


# Socket Programming

## Summary
* Build the network between Client, AWS, Server A, and Server B using socket programming. 
* Between Client and AWS, they communicate with each other by TCP, they send and receive the result of the shortest path and delay from the map and the source which client input on the terminal. 
* Between Servers and AWS, they communicate with each other by UDP, they send and receive the client input, and the result of the shortest path from server A using Dijkstra's algorithm, and the calculation of the delay from server B. 
* AWS has a role to get the client input and send the proper information to each server, and then get the results from each servers, and send the results back to the client.

## Skill Set
* C++, Unix Socket programming(TCP/UDP), Unix/Linux(Ubuntu 16.04)

## Details
* client.cpp:

		Client send the ID of map, the source vertex, and the file size, and get the results from the AWS

* aws.cpp:

		AWS get the information from the client, and send the proper information to corresponding servers, and collect the results from servers, and send the results to the client.
		

* serverA.cpp:

		ServerA find the shortest path using Dijkstra's algorithm and send the result which corresponds to the client input from the AWS to the AWS.

* serverB.cpp:

		ServerB calculate the delay and send the result which corresponds to the client input from the AWS to the AWS.

* The format of all the messages exchanged:

		The format is a single line which includes the proper information. The single line includes the division like '/' or ' ' to separate it to proper data structures, and types.


