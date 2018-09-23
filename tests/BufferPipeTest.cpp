#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include <Platform/Platform.h>

#include "DataStructures/BufferPipe.h"
#include "Transports/BufferPipes/XportBridge/XportBridge.h"


class DummyTransport : public BufferPipe {
  public:
    DummyTransport(const char*);
    DummyTransport(const char*, BufferPipe*);

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    void printDebug(StringBuilder*);
    void runTest(StringBuilder*);


  private:
    const char* _name;
    StringBuilder  _accumulator;
};


DummyTransport::DummyTransport(const char* nom) : BufferPipe() {
  _name = nom;
}

/**
* Constructor. We are passed one BufferPipe. Presumably the instance that gave
*   rise to us.
* Without both sides of the bridge, all transfers in either direction will
*   fail with MEM_MGMT_RESPONSIBLE_CALLER.
*/
DummyTransport::DummyTransport(const char* nom, BufferPipe* x) : BufferPipe() {
  _name = nom;
  setNear(x);
}


/*******************************************************************************
* Functions specific to this class                                             *
*******************************************************************************/
/*
* This should just accumulate.
*/
int8_t DummyTransport::toCounterparty(uint8_t* buf, unsigned int len, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      _accumulator.concat(buf, len);
      return MEM_MGMT_RESPONSIBLE_CREATOR;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      _accumulator.concat(buf, len);
      free(buf);
      return MEM_MGMT_RESPONSIBLE_BEARER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}

/*
* The other side of the bridge.
*/
int8_t DummyTransport::fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm) {
  switch (mm) {
    case MEM_MGMT_RESPONSIBLE_CALLER:
      // NOTE: No break. This might be construed as a way of saying CREATOR.
    case MEM_MGMT_RESPONSIBLE_CREATOR:
      /* The system that allocated this buffer either...
          a) Did so with the intention that it never be free'd, or...
          b) Has a means of discovering when it is safe to free.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, len, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    case MEM_MGMT_RESPONSIBLE_BEARER:
      /* We are now the bearer. That means that by returning non-failure, the
          caller will expect _us_ to manage this memory.  */
      if (haveFar()) {
        /* We are not the transport driver, and we do no transformation. */
        return _far->fromCounterparty(buf, len, mm);
      }
      return MEM_MGMT_RESPONSIBLE_CALLER;   // Reject the buffer.

    default:
      /* This is more ambiguity than we are willing to bear... */
      return MEM_MGMT_RESPONSIBLE_ERROR;
  }
}


void DummyTransport::printDebug(StringBuilder* output) {
  output->concatf("\t-- %s ----------------------------------\n", _name);
  if (haveNear()) {
    output->concatf("\t _near         \t[0x%08x]\n", (unsigned long)_near);
  }
  if (haveFar()) {
    output->concatf("\t _far          \t[0x%08x]\n", (unsigned long)_far);
  }
  if (_accumulator.length() > 0) {
    output->concatf("\t _accumulator (%d bytes):  ", _accumulator.length());
    _accumulator.printDebug(output);
  }
}

void DummyTransport::runTest(StringBuilder* output) {
  printf("runTest() ---> fromCounterparty() ---> %s\n",
    BufferPipe::memMgmtString(
      this->toCounterparty(output->string(), output->length(), MEM_MGMT_RESPONSIBLE_CREATOR)
    )
  );
}



/*
* Allocates two dummy objects and tries to move data between them.
*/
int test_BufferPipe_0(void) {
  printf("Beginning test_BufferPipe_0()....\n");
  StringBuilder log;

  StringBuilder* tmp_str = new StringBuilder("data to move...\n");

  DummyTransport _xport0("Xport0");
  DummyTransport _xport1("Xport1", &_xport0);

  _xport0.setNear(&_xport1);

  _xport1.runTest(tmp_str);
  log.concat("\n_xport0\n");
  _xport0.printDebug(&log);
  log.concat("\n_xport1\n");
  _xport1.printDebug(&log);

  log.concat("\n");

  _xport0.runTest(tmp_str);
  log.concat("\n_xport0\n");
  _xport0.printDebug(&log);
  log.concat("\n_xport1\n");
  _xport1.printDebug(&log);

  printf("%s\n", log.string());
  return 0;
}


/*
* Allocates two dummy objects and tries to move data between them via an XportBridge.
*/
int test_BufferPipe_1(void) {
  printf("Beginning test_BufferPipe_1()....\n");
  StringBuilder log;

  StringBuilder* tmp_str = new StringBuilder("data to move...\n");

  DummyTransport _xport2("Xport2");
  DummyTransport _xport3("Xport3");

  XportBridge _bridge(&_xport2, &_xport3);

  log.concatf("_xport2->toCounterparty() ---> %s\n",
    BufferPipe::memMgmtString(
      _xport2.toCounterparty(tmp_str->string(), tmp_str->length(), MEM_MGMT_RESPONSIBLE_CREATOR)
    )
  );
  _xport2.printDebug(&log);
  _xport3.printDebug(&log);

  log.concat("\n");

  tmp_str->clear();
  tmp_str->concat("This is a differnt bits.\n");
  log.concatf("_xport3->toCounterparty() ---> %s\n",
    BufferPipe::memMgmtString(
      _xport3.toCounterparty(tmp_str->string(), tmp_str->length(), MEM_MGMT_RESPONSIBLE_CREATOR)
    )
  );
  _xport2.printDebug(&log);
  _xport3.printDebug(&log);

  printf("%s\n", log.string());
  return 0;
}




/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int main(int argc, char *argv[]) {
  platform.platformPreInit();   // Our test fixture needs random numbers.
  platform.bootstrap();

  // TODO: This is presently needed to prevent the program from hanging. WHY??
  StringBuilder out;
  platform.kernel()->printScheduler(&out);

  printf("\n");
  test_BufferPipe_0();
  printf("\n\n");
  test_BufferPipe_1();
  printf("\n\n");
  exit(0);
}
