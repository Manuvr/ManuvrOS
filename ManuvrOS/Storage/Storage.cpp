/*
File:   Storage.cpp
Author: J. Ian Lindsay
Date:   2016.08.28

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


This is the basal implementation of the storage interface.
*/

#if defined(MANUVR_STORAGE)
#include "Storage.h"

/**
* Constructor
*/
Storage::Storage() {
}


void Storage::printDebug(StringBuilder* output) {
  output->concatf("-- Storage              [%sencrypted, %sremovable]\n",
    _pl_flag(MANUVR_PL_ENCRYPTED) ? "" : "un",
    _pl_flag(MANUVR_PL_REMOVABLE) ? "" : "non-"
  );
  if (isMounted()) {
    output->concatf("--\t Medium mounted %s %s  (%lu bytes free)\t%s\n",
      _pl_flag(MANUVR_PL_MEDIUM_READABLE) ? "+r" : "",
      _pl_flag(MANUVR_PL_MEDIUM_WRITABLE) ? "+w" : "",
      freeSpace(),
      isBusy() ? "[BUSY]" : ""
    );
  }
  if (_pl_flag(MANUVR_PL_USES_FILESYSTEM)) {
    output->concat("--\t On top of FS\n");
  }
  if (_pl_flag(MANUVR_PL_BLOCK_ACCESS)) {
    output->concat("--\t Block access\n");
  }
  if (_pl_flag(MANUVR_PL_BATTERY_DEPENDENT)) {
    output->concat("--\t Battery-backed\n");
  }
}

#endif   // MANUVR_STORAGE
