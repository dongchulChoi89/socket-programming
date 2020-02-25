all:
	g++ -o client client.cpp
	g++ -o aws aws.cpp
	g++ -o serverA serverA.cpp
	g++ -o serverB serverB.cpp

.PHONY: aws serverA serverB clean

aws:
	./aws

serverA:
	./serverA

serverB:
	./serverB

clean:
	rm client aws serverA serverB