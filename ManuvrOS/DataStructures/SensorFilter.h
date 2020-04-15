/*
File:   SensorFilter.h
Author: J. Ian Lindsay
Date:   2020.01.30

Copyright 2020 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#ifndef __SENSOR_FILTER_H__
#define __SENSOR_FILTER_H__

#include <inttypes.h>
#include <stdint.h>
#if defined (ARDUINO)
  #include <Arduino.h>
#else
  #include <stdlib.h>
#endif
#include "StringBuilder.h"
#include "Vector3.h"

#define FILTER_MAX_ELEMENTS   8000  // Arbitrary.


enum class FilteringStrategy : uint8_t {
  RAW            = 0,  // No filtering
  MOVING_AVG     = 1,  // Moving average with a given window size. Arithmetic mean.
  MOVING_MED     = 2,  // Moving median with a given window size.
  HARMONIC_MEAN  = 3,  // Moving harmonic mean with a given window size.
  GEOMETRIC_MEAN = 4,  // Moving geometric mean.
  QUANTIZER      = 5   // A filter that divides inputs up into bins.
};


/*
* Filters for linear sequences of scalar values.
*/
template <typename T> class SensorFilter {
  public:
    SensorFilter(FilteringStrategy, int param0, int param1);
    virtual ~SensorFilter();

    int8_t feedFilter(T);
    int8_t purge();
    int8_t init();
    T      value();
    T      minValue();
    T      maxValue();

    int8_t setParam0(int);
    int8_t setParam1(int);
    int    getParam0();
    int    getParam1();

    inline FilteringStrategy strategy() {   return _strat;    };
    int8_t setStrategy(FilteringStrategy);
    void printFilter(StringBuilder*);

    inline T*       memPtr() {        return samples;         };
    inline uint16_t lastIndex() {     return sample_idx;      };
    inline uint16_t windowSize() {    return window_size;     };
    inline bool     dirty() {         return filter_dirty;    };
    inline bool     initialized() {   return filter_initd;    };
    inline T        rmsValue() {      return rms;             };
    inline T        stdevValue() {    return stdev;           };


  private:
    T*       samples         = nullptr;
    uint16_t sample_idx      = 0;
    uint16_t window_size     = 0;
    int32_t  _param_0        = 0;  // Purpose depends on filter strategy.
    int32_t  _param_1        = 0;  // Purpose depends on filter strategy.
    T        last_value      = T(0);
    T        min_value       = T(0);
    T        max_value       = T(0);
    T        rms             = T(0);
    T        stdev           = T(0);
    FilteringStrategy _strat = FilteringStrategy::RAW;
    bool     window_full     = false;
    bool     filter_dirty    = false;
    bool     filter_initd    = false;

    int8_t  _reallocate_sample_window(uint16_t);
    int8_t  _zero_samples();

    T  _calculate_rms();
    T  _calculate_stdev();
    T  _calculate_median();
};



/*
* Filters for linear sequences of vector values.
* Pure virtual interface.
*/
template <typename T> class SensorFilter3 {
  public:
    SensorFilter3(FilteringStrategy, int param0, int param1);
    virtual ~SensorFilter3();

    int8_t feedFilter(Vector3<T>*);
    int8_t feedFilter(T x, T y, T z);  // Alternate API for discrete values.
    int8_t purge();
    int8_t init();
    Vector3<T>* value();
    Vector3<T>* minValue();
    Vector3<T>* maxValue();

    int8_t setParam0(int);
    int8_t setParam1(int);
    int    getParam0();
    int    getParam1();

    inline FilteringStrategy strategy() {   return _strat;    };
    int8_t setStrategy(FilteringStrategy);
    void printFilter(StringBuilder*);

    inline Vector3<T>* memPtr() {        return samples;         };
    inline uint16_t    lastIndex() {     return sample_idx;      };
    inline uint16_t    windowSize() {    return window_size;     };
    inline bool        dirty() {         return filter_dirty;    };
    inline bool        initialized() {   return filter_initd;    };
    inline Vector3f64* rmsValue() {      return &rms;            };
    inline Vector3f64* stdevValue() {    return &stdev;          };


  private:
    Vector3<T>* samples      = nullptr;
    uint16_t    sample_idx   = 0;
    uint16_t    window_size  = 0;
    int32_t     _param_0     = 0;  // Purpose depends on filter strategy.
    int32_t     _param_1     = 0;  // Purpose depends on filter strategy.
    Vector3<T>  last_value;
    Vector3<T>  min_value;
    Vector3<T>  max_value;
    Vector3f64  rms;
    Vector3f64  stdev;
    FilteringStrategy _strat = FilteringStrategy::RAW;
    bool     window_full     = false;
    bool     filter_dirty    = false;
    bool     filter_initd    = false;

    int8_t  _reallocate_sample_window(uint16_t);
    int8_t  _zero_samples();
    int8_t  _calculate_median();
    int8_t  _calculate_rms();
    int8_t  _calculate_stdev();
};




/******************************************************************************
* Statics and externs
******************************************************************************/
extern const char* getFilterStr(FilteringStrategy);
static const char* FILTER_HEADER_STRING = "\t%s filter\n\t-----------------------------\n";


/*******************************************************************************
* Linear filter class
*******************************************************************************/

/*
* Base constructor
*/
template <typename T> SensorFilter<T>::SensorFilter(FilteringStrategy s, int param0, int param1) {
  _strat      = s;
  window_size = param0;
}

/*
* Base destructor. Free sample memory.
*/
template <typename T> SensorFilter<T>::~SensorFilter() {
  if (nullptr != samples) {
    free(samples);
    samples = nullptr;
  }
}

/*
* This must be called ahead of usage to allocate the needed memory.
*/
template <typename T> int8_t SensorFilter<T>::init() {
  uint16_t tmp_window_size = window_size;
  window_size = 0;
  int8_t ret = _reallocate_sample_window(tmp_window_size);
  filter_initd = (0 == ret);
  return ret;
}

/*
* Reallocates the sample memory, freeing the prior allocation if necessary.
* Returns 0 of success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter<T>::_reallocate_sample_window(uint16_t win) {
  int8_t ret = -1;
  uint16_t normd_win = (win > FILTER_MAX_ELEMENTS) ? FILTER_MAX_ELEMENTS : win;
  if (normd_win != window_size) {
    window_size = normd_win;
    window_full = false;
    if (nullptr != samples) {
      free(samples);
      samples = nullptr;
    }
    if (window_size > 0) {
      samples = (T*) malloc(window_size * sizeof(T));
      ret = _zero_samples();
      sample_idx = 0;
    }
    else {   // Program asked for no sample depth.
      ret = 0;
    }
  }
  else {
    ret = _zero_samples();
  }
  return ret;
}


/*
* Zero's the content of the sample memory, whatever that means to this type.
* Returns 0 of success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter<T>::_zero_samples() {
  int8_t ret = -1;
  last_value = T(0);
  rms        = T(0);
  stdev      = T(0);
  if (nullptr != samples) {
    if (window_size > 0) {
      ret = 0;
      for (uint32_t i = 0; i < window_size; i++) {
        samples[i] = T(0);
      }
    }
  }
  return ret;
}


template <typename T> int8_t SensorFilter<T>::setStrategy(FilteringStrategy s) {
  int8_t ret = -1;
  if (_strat != s) {
    _strat = s;
    last_value = T(0);
    ret = 0;
  }
  return ret;
}


template <typename T> void SensorFilter<T>::printFilter(StringBuilder* output) {
  output->concatf(FILTER_HEADER_STRING, getFilterStr(strategy()));
  output->concatf("\tInitialized:  %c\n",   initialized() ? 'y':'n');
  output->concatf("\tDirty:        %c\n",   filter_dirty ? 'y':'n');
  output->concatf("\tWindow full:  %c\n",   window_full  ? 'y':'n');
  output->concatf("\tMin             = %.8f\n", min_value);
  output->concatf("\tMax             = %.8f\n", max_value);
  output->concatf("\tSample window   = %u\n",   window_size);
  switch (_strat) {
    case FilteringStrategy::RAW:
      output->concatf("\tValue           = %.8f\n", last_value);
      break;
    case FilteringStrategy::MOVING_AVG:
      output->concatf("\tRunning average = %.8f\n", last_value);
      output->concatf("\tRMS             = %.8f\n", rmsValue());
      output->concatf("\tSTDEV           = %.8f\n", stdevValue());
      break;
    case FilteringStrategy::MOVING_MED:
      output->concatf("\tRunning median  = %.8f\n", last_value);
      break;
    case FilteringStrategy::HARMONIC_MEAN:
      output->concatf("\tHarmonic mean   = %.8f\n", last_value);
      break;
    case FilteringStrategy::GEOMETRIC_MEAN:
      output->concatf("\tGeometric mean  = %.8f\n", last_value);
      break;
    case FilteringStrategy::QUANTIZER:
      output->concatf("\tQuantized value = %.8f\n", last_value);
      break;
  }
}


/**
* Add data to the filter.
*
* @param val The value to be fed to the filter.
* @return -1 if filter not initialized, 0 on value acceptance, or 1 one acceptance with new result.
*/
template <typename T> int8_t SensorFilter<T>::feedFilter(T val) {
  int8_t ret = -1;
  if (initialized()) {
    ret = 0;
    if (window_size > 1) {
      switch (_strat) {
        case FilteringStrategy::HARMONIC_MEAN:   // TODO: This.
        case FilteringStrategy::GEOMETRIC_MEAN:  // TODO: This.
        case FilteringStrategy::QUANTIZER:       // TODO: This.

        case FilteringStrategy::RAW:
          last_value = val;
          samples[sample_idx++] = val;
          if (sample_idx >= window_size) {
            window_full = true;
            sample_idx = 0;
          }
          ret = window_full ? 1 : 0;
          break;
        case FilteringStrategy::MOVING_AVG:   // Calculate the moving average...
          {
            T temp_avg = (last_value * (window_size-1)) + val;
            last_value = temp_avg / window_size;
            samples[sample_idx++] = val;
            if (sample_idx >= window_size) {
              window_full = true;
              sample_idx = 0;
              rms   = _calculate_rms();    // These are expensive, and are calculated once per-window.
              stdev = _calculate_stdev();  // These are expensive, and are calculated once per-window.
            }
            ret = window_full ? 1 : 0;
          }
          break;
        case FilteringStrategy::MOVING_MED:   // Calculate the moving median...
          // Calculate the moving median...
          samples[sample_idx++] = val;
          if (sample_idx >= window_size) {
            window_full = true;
            sample_idx = 0;
          }
          if (window_full) {
            _calculate_median();
            ret = 1;
          }
          break;
      }
    }
    else {   // This is a null filter with extra steps.
      last_value = val;
      ret = 1;
    }
    filter_dirty = (1 == ret);
  }
  return ret;
}


/**
* Returns the most recent result from the filter. Marks the filter 'not dirty'
*   as a side-effect, so don't call this for internal logic.
*
* @return The new result.
*/
template <typename T> T SensorFilter<T>::value() {
  filter_dirty = false;
  return last_value;
};


/**
* Returns the smallest value in the current dataset.
*
* @return The minimum value from the data we currently have.
*/
template <typename T> T SensorFilter<T>::minValue() {
  T ret = last_value;
  for (int i = 0; i < window_size; i++) {
    ret = (samples[i] < ret) ? samples[i] : ret;
  }
  return ret;
};


/**
* Returns the largest value in the current dataset.
*
* @return The maximum value from the data we currently have.
*/
template <typename T> T SensorFilter<T>::maxValue() {
  T ret = last_value;
  for (int i = 0; i < window_size; i++) {
    ret = (samples[i] > ret) ? samples[i] : ret;
  }
  return ret;
};


template <typename T> int8_t SensorFilter<T>::purge() {
  return _zero_samples();
}


template <typename T> int8_t SensorFilter<T>::setParam0(int x) {
  if (x >= 0) {
    return _reallocate_sample_window((uint16_t) x);
  }
  return -1;
}


template <typename T> int SensorFilter<T>::getParam0() {  return window_size;  }


/* Parameter 1 is presently unused. */
template <typename T> int8_t SensorFilter<T>::setParam1(int x) {  return -1;  }
template <typename T> int SensorFilter<T>::getParam1() {          return 0;   }



/*
* Calulates the RMS over the entire sample window.
*/
template <typename T> T SensorFilter<T>::_calculate_rms() {
  T result;
  if ((window_size > 1) && (nullptr != samples)) {
    T squared_samples = T(0);
    for (int i = 0; i < window_size; i++) {
      T s_tmp = samples[i] * samples[i];
      squared_samples += s_tmp;
    }
    result = sqrt(squared_samples / window_size);
  }
  else {
    result = T(0);
  }
  return result;
}


/*
* Calulates the standard deviation of the samples.
*/
template <typename T> T SensorFilter<T>::_calculate_stdev() {
  T result;
  if ((window_size > 1) && (nullptr != samples)) {
    T deviation_sum = T(0);
    for (int i = 0; i < window_size; i++) {
      T tmp_val = samples[i] - last_value;
      deviation_sum += (tmp_val * tmp_val);
    }
    result = sqrt(deviation_sum / window_size);
  }
  else {
    result = T(0);
  }
  return result;
}


/*
* Calulates the median value of the samples.
*/
template <typename T> T SensorFilter<T>::_calculate_median() {
  T sorted[window_size];
  for (uint16_t i = 0; i < window_size; i++) {
    sorted[i] = samples[i];
  }
  // Selection sort.
  uint16_t i  = 0;
  T swap;
  for (uint16_t c = 0; c < (window_size - 1); c++) {
    i = c;
    for (uint16_t d = c + 1; d < window_size; d++) {
      if (sorted[i] > sorted[d]) i = d;
    }
    if (i != c) {
      swap = sorted[c];
      sorted[c] = sorted[i];
      sorted[i] = swap;
    }
  }

  if (window_size & 0x01) {
    // If there are an off number of samples...
    last_value = sorted[(window_size-1) >> 1];
  }
  else {
    // ...otherwise, we take the mean to the two middle values.
    uint16_t lower = (window_size-1) >> 1;
    uint16_t upper = lower + 1;
    last_value = (sorted[upper] + sorted[lower]) / 2;
  }
  return 0;
}


/*******************************************************************************
* Linear Vector3 filter class
*******************************************************************************/

/*
* Base constructor
*/
template <typename T> SensorFilter3<T>::SensorFilter3(FilteringStrategy s, int param0, int param1) {
  _strat      = s;
  window_size = param0;
}

/*
* Base destructor. Free sample memory.
*/
template <typename T> SensorFilter3<T>::~SensorFilter3() {
  if (nullptr != samples) {
    free(samples);
    samples = nullptr;
  }
}

/*
* This must be called ahead of usage to allocate the needed memory.
*/
template <typename T> int8_t SensorFilter3<T>::init() {
  uint16_t tmp_window_size = window_size;
  window_size = 0;
  int8_t ret = _reallocate_sample_window(tmp_window_size);
  filter_initd = (0 == ret);
  return ret;
}

/*
* Reallocates the sample memory, freeing the prior allocation if necessary.
* Returns 0 of success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter3<T>::_reallocate_sample_window(uint16_t win) {
  int8_t ret = -1;
  uint16_t normd_win = (win > FILTER_MAX_ELEMENTS) ? FILTER_MAX_ELEMENTS : win;
  if (normd_win != window_size) {
    window_size = normd_win;
    window_full = false;
    if (nullptr != samples) {
      free(samples);
      samples = nullptr;
    }
    if (window_size > 0) {
      samples = (Vector3<T>*) malloc(window_size * sizeof(Vector3<T>));
      ret = _zero_samples();
      sample_idx = 0;
    }
    else {   // Program asked for no sample depth.
      ret = 0;
    }
  }
  else {
    ret = _zero_samples();
  }
  return ret;
}

/*
* Zero's the content of the sample memory, whatever that means to this type.
* Returns 0 of success, -1 otherwise.
*/
template <typename T> int8_t SensorFilter3<T>::_zero_samples() {
  int8_t ret = -1;
  last_value(T(0), T(0), T(0));
  rms(T(0), T(0), T(0));
  stdev(T(0), T(0), T(0));
  if (nullptr != samples) {
    if (window_size > 0) {
      ret = 0;
      for (uint32_t i = 0; i < window_size; i++) {
        samples[i](T(0), T(0), T(0));
      }
    }
  }
  return ret;
}


template <typename T> int8_t SensorFilter3<T>::setStrategy(FilteringStrategy s) {
  int8_t ret = -1;
  if (_strat != s) {
    _strat = s;
    last_value(T(0), T(0), T(0));
    ret = 0;
  }
  return ret;
}


template <typename T> void SensorFilter3<T>::printFilter(StringBuilder* output) {
  const char* lv_label;
  output->concatf(FILTER_HEADER_STRING, getFilterStr(strategy()));
  output->concatf("\tInitialized:  %c\n",   initialized() ? 'y':'n');
  output->concatf("\tDirty:        %c\n",   filter_dirty ? 'y':'n');
  output->concatf("\tWindow full:  %c\n",   window_full  ? 'y':'n');
  output->concatf("\tMin             = %.8f\n", min_value);
  output->concatf("\tMax             = %.8f\n", max_value);
  output->concatf("\tSample window   = %u\n",   window_size);
  switch (_strat) {
    case FilteringStrategy::MOVING_AVG:
      output->concatf("\tRMS             = (%.4f, %.4f, %.4f)\n", rms.x, rms.y, rms.z);
      output->concatf("\tSTDEV           = (%.4f, %.4f, %.4f)\n", stdev.x, stdev.y, stdev.z);
      lv_label = "Arithmetic mean";
      break;
    case FilteringStrategy::MOVING_MED:
      lv_label = "Median";
      break;
    case FilteringStrategy::HARMONIC_MEAN:
      lv_label = "Harmonic mean";
      break;
    case FilteringStrategy::GEOMETRIC_MEAN:
      lv_label = "Geometric mean";
      break;
    case FilteringStrategy::QUANTIZER:
      lv_label = "Quantized value";
      break;
    default:
      lv_label = "Value";
      break;
  }
  output->concatf("\t%15s = (%.4f, %.4f, %.4f)\n", lv_label, last_value.x, last_value.y, last_value.z);
}


/**
* Add data to the filter.
*
* @param val The value to be fed to the filter.
* @return -1 if filter not initialized, 0 on value acceptance, or 1 one acceptance with new result.
*/
template <typename T> int8_t SensorFilter3<T>::feedFilter(T x, T y, T z) {
  int8_t ret = -1;
  if (initialized()) {
    ret = 0;
    if (window_size > 1) {
      samples[sample_idx++](x, y, z);
      if (sample_idx >= window_size) {
        window_full = true;
        sample_idx = 0;
        ret = 1;
      }
      switch (_strat) {
        case FilteringStrategy::HARMONIC_MEAN:   // TODO: This.
        case FilteringStrategy::GEOMETRIC_MEAN:  // TODO: This.
        case FilteringStrategy::QUANTIZER:       // TODO: This.

        case FilteringStrategy::RAW:
          last_value(x, y, z);
          break;
        case FilteringStrategy::MOVING_AVG:   // Calculate the moving average...
          {
            Vector3f64 input_vect((double) x, (double) y, (double) z);
            Vector3f64 temp_avg((double) last_value.x, (double) last_value.y, (double) last_value.z);
            temp_avg *= (window_size-1);
            temp_avg += input_vect;
            temp_avg /= (double) window_size;
            last_value((T) temp_avg.x, (T) temp_avg.y, (T) temp_avg.z);
            if (window_full) {
              _calculate_rms();    // These are expensive, and are calculated once per-window.
              _calculate_stdev();  // These are expensive, and are calculated once per-window.
            }
          }
          break;
        case FilteringStrategy::MOVING_MED:   // Calculate the moving median...
          // Calculate the moving median...
          if (window_full) {
            _calculate_median();
          }
          break;
      }
    }
    else {   // This is a null filter with extra steps.
      last_value(x, y, z);
      ret = 1;
    }
    filter_dirty = (1 == ret);
  }
  return ret;
}


/**
* Returns the most recent result from the filter. Marks the filter 'not dirty'
*   as a side-effect, so don't call this for internal logic.
*
* @return The new result.
*/
template <typename T> Vector3<T>* SensorFilter3<T>::value() {
  filter_dirty = false;
  return &last_value;
}


/**
* Returns the vector with the smallest magnitude in the current dataset.
*
* @return The minimum value from the data we currently have.
*/
template <typename T> Vector3<T>* SensorFilter3<T>::minValue() {
  T ret = last_value.length();
  uint16_t ret_idx = FILTER_MAX_ELEMENTS;
  for (int i = 0; i < window_size; i++) {
    if (samples[i].length() < ret) {
      ret = samples[i].length();
      ret_idx = i;
    }
  }
  return (FILTER_MAX_ELEMENTS == ret_idx) ? &last_value : &samples[ret_idx];
};


/**
* Returns the vector with the largest magnitude in the current dataset.
*
* @return The maximum value from the data we currently have.
*/
template <typename T> Vector3<T>* SensorFilter3<T>::maxValue() {
  T ret = last_value.length();
  uint16_t ret_idx = FILTER_MAX_ELEMENTS;
  for (int i = 0; i < window_size; i++) {
    if (samples[i].length() > ret) {
      ret = samples[i].length();
      ret_idx = i;
    }
  }
  return (FILTER_MAX_ELEMENTS == ret_idx) ? &last_value : &samples[ret_idx];
};


template <typename T> int8_t SensorFilter3<T>::purge() {
  return _zero_samples();
}


template <typename T> int8_t SensorFilter3<T>::setParam0(int x) {
  if (x >= 0) {
    return _reallocate_sample_window((uint16_t) x);
  }
  return -1;
}

template <typename T> int SensorFilter3<T>::getParam0() {  return window_size; }


/* The mean filter has no parameter 1 */
template <typename T> int8_t SensorFilter3<T>::setParam1(int x) {  return -1;  }
template <typename T> int SensorFilter3<T>::getParam1() {          return 0;   }


/*
* Calulates the RMS over the entire sample window.
*/
template <typename T> int8_t SensorFilter3<T>::_calculate_rms() {
  int8_t ret = -1;
  if ((window_size > 1) && (nullptr != samples)) {
    double squared_samples[3] = {0.0, 0.0, 0.0};
    for (uint16_t i = 0; i < window_size; i++) {
      squared_samples[0] += samples[i].x * samples[i].x;
      squared_samples[1] += samples[i].y * samples[i].y;
      squared_samples[2] += samples[i].z * samples[i].z;
    }
    rms(
      sqrt(squared_samples[0] / window_size),
      sqrt(squared_samples[1] / window_size),
      sqrt(squared_samples[2] / window_size)
    );
  }
  else {
    rms(0.0, 0.0, 0.0);
  }
  return ret;
}


/*
* Calulates the standard deviation of the samples.
*/
template <typename T> int8_t SensorFilter3<T>::_calculate_stdev() {
  int8_t ret = -1;
  if ((window_size > 1) && (nullptr != samples)) {
    double deviation_sum[3] = {0.0, 0.0, 0.0};
    for (uint16_t i = 0; i < window_size; i++) {
      deviation_sum[0] += (samples[i].x * samples[i].x) - last_value.x;
      deviation_sum[1] += (samples[i].y * samples[i].y) - last_value.y;
      deviation_sum[2] += (samples[i].z * samples[i].z) - last_value.z;
    }
    stdev(
      sqrt(deviation_sum[0] / window_size),
      sqrt(deviation_sum[1] / window_size),
      sqrt(deviation_sum[2] / window_size)
    );
  }
  else {
    stdev(0.0, 0.0, 0.0);
  }
  return ret;
}


/*
* Calulates the median of the samples.
*/
template <typename T> int8_t SensorFilter3<T>::_calculate_median() {
  double sorted[3][window_size];
  for (uint16_t i = 0; i < window_size; i++) {
    sorted[0][i] = samples[i].x;
    sorted[1][i] = samples[i].y;
    sorted[2][i] = samples[i].z;
  }
  // Selection sort.
  uint16_t i = 0;
  double swap;
  for (uint8_t n = 0; n < 3; n++) {
    for (uint16_t c = 0; c < (window_size - 1); c++) {
      i = c;
      for (uint16_t d = c + 1; d < window_size; d++) {
        if (sorted[n][i] > sorted[n][d]) i = d;
      }
      if (i != c) {
        swap = sorted[n][c];
        sorted[n][c] = sorted[n][i];
        sorted[n][i] = swap;
      }
    }
  }

  if (window_size & 0x01) {
    // If there are an off number of samples...
    uint16_t offset = (window_size-1) >> 1;
    last_value.set(sorted[0][offset], sorted[1][offset], sorted[2][offset]);
  }
  else {
    // ...otherwise, we take the mean to the two middle values.
    uint16_t lower = (window_size-1) >> 1;
    uint16_t upper = lower + 1;
    last_value.set(
      ((sorted[0][upper] + sorted[0][lower]) / 2),
      ((sorted[1][upper] + sorted[1][lower]) / 2),
      ((sorted[2][upper] + sorted[2][lower]) / 2)
    );
  }
  return 0;
}


#endif  // __SENSOR_FILTER_H__
