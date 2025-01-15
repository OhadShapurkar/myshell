CC = gcc
CFLAGS = -Wall -g

TARGET1 = mypipeline
OBJECT1 = mypipeline.o

TARGET2 = myshell
OBJECT2 = myshell.o LineParser.o

TARGET3 = looper
OBJECT3 = looper.o

.PHONY: all clean

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(OBJECT1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJECT1)

$(OBJECT1): mypipeline.c
	$(CC) $(CFLAGS) -c mypipeline.c

$(TARGET2): $(OBJECT2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJECT2)

$(TARGET3): $(OBJECT3)
	$(CC) $(CFLAGS) -o $(TARGET3) $(OBJECT3)

looper.o: looper.c
	$(CC) $(CFLAGS) -c looper.c

myshell.o: myshell.c LineParser.h
	$(CC) $(CFLAGS) -c myshell.c

LineParser.o: LineParser.c LineParser.h
	$(CC) $(CFLAGS) -c LineParser.c

clean:
	rm -f $(OBJECT1) $(OBJECT2) $(TARGET1) $(TARGET2) $(OBJECT3) $(TARGET3)
