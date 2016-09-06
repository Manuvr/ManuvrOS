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

What a monster weekend....
_---J. Ian Lindsay_
