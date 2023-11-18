CC = gcc
CFLAGS = -g -Wall
LIB = -lpthread
echoclient: echoclient.c csapp.c csapp.h
    $(CC) $(CFLAGS) -o echoclient echoclient.c csapp.c csapp.h $(LIB)

echoserveri: echoserveri.c echo.c csapp.c csapp.h
    $(CC) $(CFLAGS) -o echoserveri echoserveri.c echo.c csapp.c csapp.h $(LIB)
clean:
    rm -f *.o
