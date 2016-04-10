CPP_STANDARD       = gnu++11

# Includes
INCLUDES = -I../ -iquote.

# Libraries
LIBS   =

# Driver-specific flags.
LOCAL_FLAGS  =  -std=$(CPP_STANDARD) -nostdlib

# Sources
SRCS   = Platform.cpp


all:
	@echo ''
	@echo ''
	@echo '======================================================'
	@echo 'Platform compile'
	@echo $(SRCS)
	@echo $(LIBS)
	@echo '======================================================'
	$(CXX) -r $(INCLUDES) $(LOCAL_FLAGS) $(CPP_FLAGS) $(LIBS) $(SRCS) -o Platform.o


clean:
	rm -f Platform.o
