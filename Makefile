CPPC=g++
PROG=MP3enc_cpp
OBJS = \
	thread.o \
	debug.o \
	utils.o \
	audio.o \
	main.o
CPP_FLAGS = -std=c++11 -Wall
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CPP_FLAGS += -O0 -g
endif
LD_FLAGS = -Llib -lpthread -lmp3lame -static

ALL = $(PROG)
all: $(ALL)

%.o: %.cpp
	@$(CPPC) $(CPP_FLAGS) -c -o $@ $<
	@echo " CC " $<

$(PROG): $(OBJS)
	@$(CPPC) $(OBJS) -o $@ $(LD_FLAGS)
	@echo " LD " $@

clean:
	@echo "clean up"
	@rm -rf *.o
ifneq (,$(wildcard $(PROG)))
	@rm $(PROG) 2>/dev/null
endif
