###########################################################################
# Makefile for tests
# Author: J. Ian Lindsay
###########################################################################
C_STANDARD   = gnu99
CPP_STANDARD = gnu++11

SOURCES_C      =
SOURCES_CPP    =

LOCAL_C_FLAGS    = $(CFLAGS)
LOCAL_CXX_FLAGS  = $(CPP_FLAGS)

LIBS  = -L$(OUTPUT_PATH) -L$(WHERE_I_AM)/lib -lstdc++ -lm
LIBS += -lmanuvr -lextras -lpthread

###########################################################################
# Option conditionals
###########################################################################
ifeq ($(SECURE),1)
endif



###########################################################################
# Parameter unification and make targets
###########################################################################

all:
	$(CXX) -static -o dstest TestDataStructures.cpp $(CPP_FLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE
	$(CXX) -static -o identtest IdentityTest.cpp $(CPP_FLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE
	$(CXX) -static -o schedtest SchedulerTest.cpp $(CPP_FLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE

clean:
	rm -f dstest identtest schedtest
