CC=g++
PROG=MP3enc_cpp
OBJS = \
	   thread.o \
	   debug.o \
	   utils.o \
	   main.o 
OBJ_FLAGS = -lpthread -static

ALL = MP3enc_cpp
all: $(ALL)

%.o: %.cpp
	@$(CC) -c -o $@ $<
	@echo " CC " $<

$(PROG): $(OBJS)
	@$(CC) $(OBJS) -o $@ $(OBJ_FLAGS)
	@echo " LD " $@

clean:
	@echo "clean up"
	@rm -rf *.o
	@rm $(PROG)
