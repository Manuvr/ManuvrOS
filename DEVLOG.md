# This is not a CHANGELOG
It is a verbose running commentary with no relationship to versioning.
File should be read top-to-bottom for chronological order.
The contents of this file will be periodically archived and purged to keep the log related to the "current events" of the code base.

_---J. Ian Lindsay 2016.08.27_

------

### 2016.08.16

       text    data     bss     dec     hex filename
    1593164   16672   42916 1652752  193810 manuvr
    1593364   16672   42916 1652952  1938d8 manuvr
    1593120   16672   42916 1652708  1937e4 manuvr
    1592128   16672   42916 1651716  193404 manuvr
    1592144   16672   42916 1651732  193414 manuvr
    1592128   16672   42916 1651716  193404 manuvr
    1592096   16672   42916 1651684  1933e4 manuvr


* Pipe-strategy works, even if a bit cumbersome linguistically.
* Memory-management cleanup and simplifications in BufferPipe.
* Tremendous cleanup effort surrounding inherited virtuals and alignments.
* NULL ---> nullptr for static analysis reasons. Sadly this broke [a pejoration of PriorityQueue](http://www.joshianlindsay.com/index.php?id=145) that I was quite fond of.
* Rough Teensy3 USB VCP support under ManuvrSerial.
* Dropping all pretense of supporting C++ standards below C++11.
* Furthering the goal of threading-model unification.
* Teensy3 platform fixes.
* GPS via TCP works.
* Addition of examples in their own directory.
* TLS client improvements.


       text    data     bss     dec     hex filename
    1592528   16672   42852 1652052  193554 manuvr  New baseline.
    1592012   16672   42852 1651536  193350 manuvr

    1591484   16672   42852 1651008  193140 manuvr  Prior to cleaning up LinkedList.
    1591468   16672   42852 1650992  193130 manuvr  Following cleanup.

    1591468   16672   42852 1650992  193130 manuvr  Prior to inline StringBuilder members.
    1591244   16672   42852 1650768  193050 manuvr  After inline StringBuilder members.

    2039985   16928   51940 2108853  202db5 manuvr  Prior to Xport encapsulation.
    2239967   30652  121432 2392051  247ff3 manuvr  64-bit build


* More constructor delegation, inlining, and cruft-removal in data structures.
* 64-bit builds now work, and so does valgrind.
* More platform-specific support.
* Ground broken on uniform crpyto API issue by adding hash support via mbedtls.
* Toying with UUID libraries, Avro, and SensorManager.
* Will be using Jansson for JSON support, despite the higher overhead. Avro depends on it.


       text    data     bss     dec     hex filename
    2223011   30588  121432 2375031  243d77 manuvr  Prior to addressing technical debt in Argument class.
    2223011   30588  121432 2375031  243d77 manuvr  Making args private, and inlining accessor.
    2225739   30588  121432 2377759  24481f manuvr  Making event_code private, and inlining accessor. Owch... 2.7KB.

I believe the space penalty is being caused by the added function call in every place the (now private)
  event_code member is being accessed outside of the class. This means a runtime compute penalty.
  Nevertheless, I am retaining it for now. I will reclaim it later when the abstraction is re-generalized.

    ---< Type sizes (32-bit) >---------------------
    -- Elemental data structures:
            StringBuilder         40
            BufferPipe            20
            LinkedList<void*>     8
            PriorityQueue<void*>  32
            TaskProfilerData      32
    -- Core singletons:
            ManuvrPlatform        8
            Kernel                2132
    -- Messaging components:
            EventReceiver         60
            Argument              8
            ManuvrMsg             20
            ManuvrRunnable        56


Switching to a 32-bit build....

       text    data     bss     dec     hex filename
    2031229   16864   51940 2100033  200b41 manuvr   Debug re-arrangements. New baseline.
    2031213   16864   51940 2100017  200b31 manuvr   Removal of remaining Argument constructors. Maximum delegation.
    2031589   16864   51940 2100393  200ca9 manuvr   Further encapsulation of Argument, and some small optimizations.
    2031669   16864   51940 2100473  200cf9 manuvr   Argument length field encapsulated.
    2031621   16864   51940 2100425  200cc9 manuvr   Simple functions inlined.
    2031297   16864   51940 2100101  200b85 manuvr   Taking gains and cutting needless Argument pass-through accessors.

------

### 2016.08.27:
Major strides. UUID type taken from iotivity-constrained. Might end up taking
  more code from that package if I can't easily link against it as it stands.
The testing effort was advanced back into a state of relevance. The DataStructure
  tests are now bearing the code responsible for reporting type sizes.
  Still need to choose a good framework for automated-testing.

       text    data     bss     dec     hex filename
    2031297   16864   51940 2100101  200b85 manuvr   Baseline
    2024893   16864   51876 2093633  1ff241 manuvr   Platform re-org into C++. Hasty.
    2024813   16864   51876 2093553  1ff1f1 manuvr   More consolidation into ManuvrPlatform.
    2024781   16864   51876 2093521  1ff1d1 manuvr   Cost of a non-abstracted log flush.
    2025149   16928   51876 2093953  1ff381 manuvr   Re-introducing ser-num accessors.
    2025965   16928   51876 2094769  1ff6b1 manuvr   Writing some long-desired platform discovery stuff.
    2026161   16928   51876 2094965  1ff775 manuvr   Made runtime architecture discovery less confusing.
    2028369   16992   51876 2097237  200055 manuvr   Implementing first storage device on linux platform.
    2030637   16992   51876 2099505  200931 manuvr   Added Storage and Identity.
    2031685   16928   51876 2100489  200d09 manuvr   Cleanup, more Platform generalization. Abstraction thrust.
    2032021   16928   51876 2100825  200e59 manuvr   Turning in for the night. Left myself some good breadcrumbs.

  At some point Sunday night during the Identity abstraction, I crossed the binary size that was my former maximum since switching back to 32-bit builds. This is note-worthy because it highlights how much cruft can cost, in terms of features that fit within the same space. There are yet more gains to reap on a later day.

  Today was a mad binge. Turning in for the night.

_---J. Ian Lindsay 2016.08.28 23:17 MST_


    -- Elemental data structures:
            StringBuilder         40
            Vector3<float>        12
            Quaternion            16
            BufferPipe            20
            LinkedList<void*>     8
            PriorityQueue<void*>  32
            Argument              16
            UUID                  16
            TaskProfilerData      32
            SensorWrapper         28

    -- Core singletons:
            ManuvrPlatform        28
              Storage             12
              Identity            20
                IdentityUUID      36
            Kernel                2004

    -- Messaging components:
            EventReceiver         60
            ManuvrMsg             16
              ManuvrRunnable      52

    -- Transports:
            ManuvrXport           160
              StandardIO          160
              ManuvrSerial        236
              ManuvrSocket        192
                ManuvrTCP         200
                ManuvrUDP         216
                  UDPPipe         72

    -- Sessions:
            XenoSession           228
              ManuvrConsole       312
              ManuvrSession       328
              CoAPSession         320
              MQTTSession         348
            XenoMessage           72
              MQTTMessage         104
              CoAPMessage         144
              XenoManuvrMessage   92

------

### 2016.08.27:
Made a big Makefile fix. Wasn't passing optimization params downstream.

       text    data     bss     dec     hex filename
    1966828   16864   52068 2035760  1f1030 Baseline without debug symbols.
    2013860   16868   52068 2082796  1fc7ec Baseline with debug symbols.

Fixed raspi platform to conform to new abstraction (prior to extension).
Numerous build-system fixes and improvements.

------

### 2016.09.05:
Much thankless grind this weekend. Driver modernization, Teensy3 support. Followed
  that up with storage implementation on Teensy3, and moved into CBOR support.

While including CBOR for Viam Sonus, I ran up against the fact that my microcontroller
  builds do not include the C standard library. I still haven't sorted out how to handle it,
  but I found a gem while I was working the problem.

Adding gc-sections to my linker directives caused this:

       text    data     bss     dec     hex filename
    2008463   16860   53988 2079311  1fba4f manuvr
    1743591   12160   50148 1805899  1b8e4b manuvr

I am now using Viam Sonus as my IoT desk lamp, as it is the only source of light
  in the room apart from my monitors.

Update: I am handling the linker issue by hard-forking cbor-cpp, and mod'ing it
  to not depend on srt:string. This represented a needless addition of ~50KB to
  Viam Souns (~50% increase!). As long as I've forked it in this manner, I can
  safely add the parse-in-place stuff that makes such an enormous memory
  difference.

Finally moved the remaining message definitions out of the CPP file that handles their logic.
  They now reside in the kernel, where they will be scrutinized in the near future.
  This resolved a bug I've been having since the conversion of the Kernel to stack
  allocation along with platform. The problem was centered around the order-of-exec
  for static intializers.

CBOR loop closes tighter. Test is correct, and massively-extended. Simple map
  support. It will be enough to write the next phase of the storage API. Commit...

       Linux, 32-bit: DEBUG=1 SECURE=1
       text    data     bss     dec     hex filename
    1743583   12160   50148 1805891  1b8e43 New baseline

81 files changed, 3718 insertions(+), 1398 deletions(-)
What a monster weekend....

_---J. Ian Lindsay_

------

### 2016.09.07:
I finally found the crash on program exit, that was specific to linux. It was
  in the kernel destructor. This has been a problem for a LONG time (at least
  8-months), but never surfaced because the kernel was never torn down. After
  the recent platform abstraction improvements, the linux build exits naturally
  and therefore invokes the kernel destructor as it leaves scope.
For technical reasons, microcontroller programs simply halt, or hard-jump to a
  bootloader, debug, or reset vector.

Much OIC wrapper. Such ugly. But it will make a great toy very soon. If it
  works out, I'll clean up the bindings to iotivity-constrained, and make
  it integrate more naturally. Right now, it's lifted demo code.

    Linux, 32-bit: DEBUG=1 SECURE=1
    1910674   12740   66404 1989818  1e5cba With OIC support via iotivity-constrained.

Lots of redundant crap in that build. Two of everything.
Two DTLS stacks. Two IP adapters. Two CBOR implementations. Two CoAP PDU parsers/packers.
EVERYTHING.

It won't stay this way, and ALL of that redundancy is due to the hasty graft of
  another wide-scope package. Commit. Sleep.

_---J. Ian Lindsay_

------

### 2016.09.09:

Alright... I've been putting this off.

##### Today's situation:
BufferPipes have been thread/mem safe to this point because they are synchronous.
Although state might mutate in an unsafe manner within the objects that extend
BufferPipe, the fate of the memory was not at risk of being in-question.

##### The issue is this:
This causes deeeeep call stacks. Here is a recent callgrind graph. Task was parsing CoAP packets and dumping results to kernel log (StandardIO).
2016.09.09-callgrind.png

This is _fine_ for a linux build. But will start to cause problems in tight-spaces.

So....
I now have to implement some simple next_tick() style functionality to cause an
automatic synchronicity-break for transfers that desire it.

Full platform polymorphism achieved.

The next_tick() idea was implemented, but no tests written yet. So I'll have a fun surprise later. More identity and CBOR extension...

_---J. Ian Lindsay_

------

### 2016.09.25:

**Messages need to do these things...**

  * Pack/parse for transport.
  * Convey asynchronous events internally
  * Carry context-specific arguments
  * Carry knowledge of their GC policy
  * Be passed uniformly
  * Be bridged with outbound transport messages. IE, be locked and held by modules.
  * Be statically-allocated.
  * Be schedulable by the kernel.
  * Carry callback pointers as either a class or fxn*.


    Linux, 32-bit: DEBUG=1 SECURE=1
    1744195   12160   50148 1806503  1b90a7 Tonight's baseline....
    1744179   12160   50148 1806487  1b9097 First step of Msg/Runnable condensation.
    1743551   12160   50020 1805731  1b8da3 Runnable and Msg smash-merged.
    1743487   12160   50020 1805667  1b8d63 Cut class_initializer().
    1743423   12160   50020 1805603  1b8d23 Cut arg form statics.
    1743651   12160   50020 1805831  1b8e07 Msg scope tightening with addition of accessors.
    1743923   12160   50020 1806103  1b8f17 More scope-tightening.
    1743907   12160   49892 1805959  1b8e87 Struct padding fix.
    1743971   12160   49892 1806023  1b8ec7 More encapsulation.
    1743939   12160   49892 1805991  1b8ea7 Re-working scheduler responsibilities.
    1744051   12160   49892 1806103  1b8f17 Legacy fxn ptr is now private for first time.
    1743987   12160   49892 1806039  1b8ed7 Turning in for the night.
    1743971   12160   49892 1806023  1b8ec7 Shaving a few more needless public members.
    1743583   12160   49892 1805635  1b8d43 Finally dropped messages soft-idempotency.
    1743503   12160   49892 1805555  1b8cf3 Shaved a few more redundancies. Doc cleanup.


**Review:**

  * Merged ManuvrRunnable down into ManuvrMsg. The merger is not yet complete. There is presently a namespace-stitch being performed by a typedef.
  * Some much-needed cruft-removal was done. Scheduler has been reworked a bit to enhance safety, readability, consistency. General "best-practices" work.
  * Wrote a set of stubs for testing schedules. Nothing filled in yet. Makefile break-out for tests.
  * Finished the preprocessor case-offs to allow building without the profiler. It's a special class of debugging tool that will never find a place in production builds.
  * Finally lost a bad idea (soft-idempotency).
  * Doxygen conformance audit in ManuvrMsg.

  Looks like my re-work shaved 256-bytes of resting memory load and ~900 bytes of flash. But it also saved a number of heavily-trafficked virtual execution pathways.

_---J. Ian Lindsay_

------

### 2016.10.02:

Massive amounts of work in the cryptographic wrappers this week. About to pull
mbedtls completely out as *the* dependency for cryptography.

    Linux, 32-bit: DEBUG=1 SECURE=1
    1777199   12204   52324 1841727  1c1a3f New baseline.

------

### 2016.10.07:

    Linux, 32-bit: DEBUG=1 SECURE=1
    1544566   12024   43940 1600530  186c12 Following semantic_args excision.
    1544566   12024   43940 1600530  186c12 Removal of redundant thread_id member had no effect.
    1544586   12024   43940 1600550  186c26 Removal of kernel pointer from ER (safety).

Binged on cleaning up technical debt in the build system. The goal of separating threading concerns from platform was advanced. More crypto-wrapper sanity-checking.

    Linux, 32-bit: DEBUG=1 SECURE=1
    1544474   12024   43940 1600438  186bb6 New baseline.
    1544102   12024   43940 1600066  186a42 Excision of xport_id. It is no longer needed.
    1544070   12024   43940 1600034  186a22 Class member re-arrangement.
    1544086   12024   43940 1600050  186a32 Kernel cleanup.
    1544198   12024   43940 1600162  186aa2 Reintroducing wakeThread() for Kernel.


The ManuvrRunnable namespace died at 03:59 PST on 2016.10.09. A brief re-cap of its evolution feels appropriate.

In the beginning, there was ScheduleItem, ManuvrMsg, and Event (which was the child of ManuvrMsg). Each did its own specific job, and for a time, it was good.

Then, much complexity was built around scheduling events. Darkness settled over the code base. Dissatisfied with the uneasy truce between ScheduleItem and Event, the monarch forged the two into one hasty blob and named it ManuvrRunnable (which still extended ManuvrMsg).

This worked for awhile, but was only a thin veneer over the disharmony between the two original houses. True resolution of the split happened gradually over many ages (about twelve months).  The final battle was waged today between ManuvrMsg and ManuvrRunnable on the battlefield of "Namespace". ManuvrMsg was the victor.

    Linux, 32-bit: DEBUG=1 SECURE=1
    1544182   12024   43940 1600146  186a92 New baseline.

_---J. Ian Lindsay_

------

### 2016.10.14:
Beginning the final pass over ManuvrMsg to finally unify all of the different memory management strategies into a single reference-counter. Using Digitabulum for this comparison for other reasons...

Before the adventure....

    ManuvrMsg storage size is 44 bytes.
    Digitabulum DEBUG=1
    281376    2800    9700  293876   47bf4

Expansion of flags member and consequent alignment fixes...

    ManuvrMsg storage size is 40 bytes.
    Digitabulum DEBUG=1
    281376    2800    9660  293836   47bcc
