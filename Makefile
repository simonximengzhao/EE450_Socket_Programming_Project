
all: client.c servermain.c serverA.c serverB.c
	gcc -o serverA serverA.c
	gcc -o serverB serverB.c
	gcc -o servermain servermain.c
	gcc -o client client.c

serverA:serverA.o
	./serverA
serverB:serverB.o
	./serverB
client:serverC.o
	./client
servermain:servermain.o
	./servermain

clean:
	$(RM) serverA
	$(RM) serverB
	$(RM) servermain
	$(RM) client
	$(RM) serverA.o
	$(RM) serverB.o
	$(RM) servermain.o
	$(RM) client.o
