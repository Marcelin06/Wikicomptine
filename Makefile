CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -pthread

all: wcp_clt wcp_srv
wcp_srv: wcp_srv.c 
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
wcp_clt: wcp_clt.c 
	$(CC) $(CFLAGS) $^ -o $@
#comptine_utils.o: comptine_utils.c comptine_utils.h
#	$(CC) $(CFLAGS) -o $@ -c $<  
#wcp_clt.o: wcp_clt.c comptine_utils.h
#	$(CC) $(CFLAGS) -o $@ -c $<  
#wcp_srv.o: wcp_srv.c comptine_utils.h
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $< 
.PHONY: clean
clean:
	rm -f *.o wcp_srv wcp_clt
