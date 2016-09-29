## How platform support is organized

There exist a number of common operations for a program that are (by necessity) platform-dependent for some reason. In Manuvr's notion of platform, the following platform-specific things are abstracted into provincial structures or classes because they either (imply hardware-access) or (have differential implementations that do the same thing).
  1. Random numbers
  2. Trans-runtime data persistence (File/EEPROM/Flash I/O)
  3. Platform state control (Reset, reboot, shutdown, bootloader entry)
  4. Process state control (Sleeping/delay, threading, interrupt suspension)
  5. Date and time
  6. Cryptographic implementation (which falls-back on software)
  7. GPI/O pins
  8. Watchdog, COP, deadman switch (failsafe timers)

In addition, there are points in the platform life-cycle where features (ALU width and endianness) and environment are discovered at runtime, and made available to the rest of the firmware. This eliminates the need to replicate this functionality (#include <net/htons.h>), or bake-in these parameters at runtime (which doesn't always work, because some CPUs exhibit dual-endianness.

-----

## Cryptographic wrappers

As always, your security situation will dictate your choices.

All linkage to cryptographic wrappers is "C-style".

Cryptography.h provides the interface to platform-abstracted cryptographic implementations. It is also responsible for normalizing cryptographic preprocessor definitions across back-ends.

The base functions are implemented as weak references to allow specific hardware support to clobber the software implementations at link-time, should that be desirable.

### Classes of cryptographic support

Cryptographic providers and supported ciphers/digests are itemizable at run-time to facilitate software choices regarding algorithms.

These functions can be used to determine how a given algorithm is implemented:

    // Is the algorithm implemented in hardware?
    bool digest_hardware_backed(Hashes);
    bool cipher_hardware_backed(Cipher);

    // Is the algorithm provided by the default implementation?
    bool digest_deferred_handling(Hashes);
    bool cipher_deferred_handling(Cipher);


A somewhat softer approach would see user code that deals with its own wrappers, and completely ignores Cryptography.h, while leaving it intact for the framework's other (presumably less-critical) purposes.

Another approach might prefer to override software implementations at a more-granular level. For instance, most AES hardware only handles a restricted set of parameters (only AES-128-CBC, for example). In these cases, user code can provide an override at runtime while retaining the software support as a fall-back (if it was built at all). This carries a slightly-higher run-time overhead, but will allow arbitrary-levels of cryptographic support opportunistically intermixed with hardware, when/where available.

The softest condition is no cryptography at all.

### Supported back-ends

The cryptographic back-end is selected at compile-time by the preprocessor. It is not presently possible to mix software back-ends.

Initial support was written against mbedTLS.

Hardware back-ends are universally more-specific, and override is done at runtime.


-----

## Boot sequence
Platform has no constructor. It is statically-allocated on the stack prior to main() invocation, and is accessible anywhere that Platform/Platform.h is included.

The platform object is completely inert until platformPreInit() is called, at which point, basic initialization happens.

Programs should formulate any runtime arguments (if any) and call platformPreInit(). This will setup any basic platform features that can be reasonably initialized without I/O. The preInit function also retains any arguments passed at invocation.

Calling platform.bootstrap() will cause the BOOT_COMPLETED message to be put into the event queue, and the event queue processed until no more events are processed. The allows modules to be instanced and brought into operations state ahead of persisted configuration load (if that option is present). The presence of configuration data will at this point cause a SYS_CONF_LOAD message to be inserted into the event queue (but not processed).

Next, the platformPostInit() function is called for platforms that need to have additional steps taken after technical boot completion, but before control is returned to the main loop.

If at this point, there is no assigned identity, a platform flag will be set to indicate that the platform is in a "first-boot" state. A basic UUID identity will be generated and self-assigned.

In order to avoid the "first-boot" flag being set, a valid identity must be present in the configuration prior to the completion of platformPostInit().

Control is then returned to the main thread.


#### This is a minimum main-loop

    int main(void) {
      platform.platformPreInit();
      platform.bootstrap();
      platform.forsakeMain();
    }

Alternatively, one that accepts command-line arguments (on linux, generally).

    int main(int argc, const char *argv[]) {
      Argument* opts = parseFromArgCV(argc, argv);
      platform.platformPreInit(opts);
      platform.bootstrap();
      platform.forsakeMain();
    }

#### What happens behind the scenes
Blue cells represent the calls from the main() function above.

  ```mermaid
  graph TD
    a[pre-main initializers]
    b[argument parsing]
    c(platformPreInit)
    d(bootstrap)
    e[Load conf from storage]
    f[Raise SYS_CONF_LOAD with configuration]
    g[platformPostInit]
    h[Generate identity]
    i[Set first-boot flag]
    j[Run event queue to exhaustion]

    r(Raise SYS_BOOT_COMPLETE)

    w{Storage-capable platform}
    x{Configuration exists}
    y{Self-identity defined}
    z(Service loop)

    subgraph Boot
    a --> b
    b --> c
    c --> d
    d --> r
    r --> j
    j --> w
    w -->|No| x
    w -->|Yes| e

    e --> x
    x -->|No| g
    x -->|Yes| f
    f --> g
    y -->|No| i
    y -->|Yes| z
    i --> h
    g --> y
    h --> z

    end

    style d fill:#8Cf,stroke:#333,stroke-width:2px;
    style c fill:#8Cf,stroke:#333,stroke-width:2px;
    style z fill:#8Cf,stroke:#333,stroke-width:2px;

  ```
-----

## Configuration storage

Configuration storage is the responsibility of the platform implementation. Default strategy is CBOR encoded maps. Any other strategy must support some means of serializing KVP maps, where "K" is reliably a string.

Configuration structure (loaded by platform) should result in a map that has this rough structure:

    <root>
      |
      |--"_self": Any identity imparted onto the firmware.
      |
      |--"_other": Any identity for counter-parties that must be persisted.
      |
      |--"_conf": The root of runtime configuration data.
      |
      |--"_pipe": Any runtime-defined pipe strategies.
