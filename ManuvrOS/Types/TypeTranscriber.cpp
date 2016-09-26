/*
File:   TypeTranscriber.cpp
Author: J. Ian Lindsay
Date:   2016.08.13

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


This is where we collect adapters and converters between type-representations.
*/

#include "TypeTranscriber.h"
#include <DataStructures/Argument.h>
#include <stdlib.h>  // TODO: Cut free() and malloc().

#if defined(MANUVR_CBOR)
CBORArgListener::CBORArgListener(Argument** target) {    built = target;    }

CBORArgListener::~CBORArgListener() {
	// JIC...
	if (nullptr != _wait) {
		free(_wait);
		_wait = nullptr;
	}
}

void CBORArgListener::_caaa(Argument* nu) {
	if (nullptr != _wait) {
		nu->setKey(_wait);
		_wait = nullptr;
		nu->reapKey(true);
	}

  if ((nullptr != built) && (nullptr != *built)) {
    (*built)->link(nu);
  }
  else {
    *built = nu;
  }

	if (0 < _wait_map)   _wait_map--;
	if (0 < _wait_array) _wait_array--;
}


void CBORArgListener::on_string(char* val) {
	if (nullptr == _wait) {
		// We need to copy the string. It will be the key for the Argument
		//   who's value is forthcoming.
		int len = strlen(val);
		_wait = (char*) malloc(len+1);
		if (nullptr != _wait) {
			memcpy(_wait, val, len+1);
		}
	}
	else {
		// There is a key assignment waiting. This must be the value.
		_caaa(new Argument(val));
	}
};

void CBORArgListener::on_bytes(uint8_t* data, int size) { _caaa(new Argument(data, size));      };


void CBORArgListener::on_integer(unsigned int val) {
	// TODO: Deal with this in the CBOR library.
	if (val < 256) {
		_caaa(new Argument((uint8_t) val));
	}
	else if (val < 65536) {
		_caaa(new Argument((uint16_t) val));
	}
	else {
		_caaa(new Argument((uint32_t) val));
	}
};

void CBORArgListener::on_integer(int val) {
	// TODO: Deal with this in the CBOR library.
	int magnitude = (val >= 0) ? val : (val * -1);
	if (magnitude < 256) {
		_caaa(new Argument((int8_t) val));
	}
	else if (magnitude < 65536) {
		_caaa(new Argument((int16_t) val));
	}
	else {
		_caaa(new Argument((int32_t) val));
	}
};

void CBORArgListener::on_float32(float f) {            _caaa(new Argument(f));               };
void CBORArgListener::on_tag(unsigned int tag) {       _caaa(new Argument((uint32_t) tag));  };
void CBORArgListener::on_special(unsigned int code) {  _caaa(new Argument((uint32_t) code)); };

void CBORArgListener::on_array(int size) {
	_wait_array = (int32_t) size;
};

void CBORArgListener::on_map(int size) {
	_wait_map   = (int32_t) size;

	if (nullptr != _wait) {
		// Flush so we can discover problems.
		free(_wait);
		_wait = nullptr;
	}
};

void CBORArgListener::on_error(const char* error) {    _caaa(new Argument(error));   };
void CBORArgListener::on_extra_integer(unsigned long long value, int sign) {}
void CBORArgListener::on_extra_tag(unsigned long long tag) {}
void CBORArgListener::on_extra_special(unsigned long long tag) {}

#endif  // MANUVR_CBOR
