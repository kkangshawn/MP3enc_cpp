CC=g++
PROG=MP3enc_cpp
OBJS = \
	   thread.o \
	   debug.o \
	   utils.o \
	   audio.o \
	   main.o 
CPP_FLAGS = -std=c++11
OBJ_FLAGS = -Llib -lpthread -lmp3lame -static
ALL = MP3enc_cpp
all: $(ALL)

%.o: %.cpp
	@$(CC) $(CPP_FLAGS) -c -o $@ $<
	@echo " CC " $<

$(PROG): $(OBJS)
	@$(CC) $(OBJS) -o $@ $(OBJ_FLAGS)
	@echo " LD " $@

clean:
	@echo "clean up"
	@rm -rf *.o
	@rm $(PROG)
