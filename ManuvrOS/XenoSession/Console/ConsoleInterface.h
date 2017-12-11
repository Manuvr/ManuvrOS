/*
File:   ConsoleInterface.h
Author: J. Ian Lindsay
Date:   2017.11.19

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This is the interface a class should extend to support console interaction with
  a human.

If you want this feature, you must define MANUVR_CONSOLE_SUPPORT in the
  firmware defs file, or pass it into the build. Logging support will remain
  independently.
*/

#ifndef __MANUVR_CONSOLE_CMD_DEF_H__
#define __MANUVR_CONSOLE_CMD_DEF_H__

class StringBuilder;
#include <EnumeratedTypeCodes.h>

/* A unifying type for different threading models. */
typedef void* (*ConsoleFxn)(StringBuilder*);

TCode const tcode_array_none[] = {TCode::NONE};

// TODO: This name is awful and misleading. But I'm sick of putting this task off.
class ConsoleCommand {
  public:
    const char* const shortcut;
    const char* const help_text;
    const TCode* const args;

    /** Copy constructor. */
    ConsoleCommand(const ConsoleCommand* o) :
      shortcut(o->shortcut),
      help_text(o->help_text),
      args(o->args) {};

    /**
    * Constructor.
    *
    * @param _sc
    * @param _lam
    */
    ConsoleCommand(
      const char* const _sc,
      const TCode* const _a
    ) :
      shortcut(_sc),
      help_text("No help"),
      args(_a) {};

    /**
    * Constructor.
    *
    * @param _sc
    * @param _lam
    */
    ConsoleCommand(
      const char* const _sc,
      const char* const _help
    ) :
      shortcut(_sc),
      help_text(_help),
      args(&tcode_array_none[0]) {};

    /**
    * Constructor.
    *
    * @param _sc
    * @param _help
    * @param _lam
    */
    ConsoleCommand(
      const char* const _sc,
      const char* const _help,
      const TCode* const _a
    ) :
      shortcut(_sc),
      help_text(_help),
      args(_a)  {};

  private:
};


/**
* This is the class to extend to support a console in a given class.
*/
class ConsoleInterface {
  public:
    static void consoleSchemaAdd(ConsoleInterface* obj);
    static void consoleSchemaDrop(ConsoleInterface* obj);

    /* Must be ovverridden to implement. */
    virtual unsigned int consoleGetCmds(ConsoleCommand**) =0;
    virtual const char* consoleName() =0;
    virtual void consoleCmdProc(StringBuilder*) =0;


  protected:
    ConsoleInterface() {
      // Register the console commands we've defined above.
      ConsoleInterface::consoleSchemaAdd(this);
    };

    virtual ~ConsoleInterface() {
      ConsoleInterface::consoleSchemaDrop(this);
    };
};

#endif  // __MANUVR_CONSOLE_CMD_DEF_H__
