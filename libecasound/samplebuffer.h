#ifndef INCLUDED_SAMPLEBUFFER_H
#define INCLUDED_SAMPLEBUFFER_H

#include <vector>

#include "eca-audio-format.h"
#include "sample-specs.h"

class SAMPLE_BUFFER_FUNCTIONS;
class SAMPLE_BUFFER_impl;

/**
 * A dynamic container for storing blocks of 
 * audio data.
 * 
 * Static attributes are:
 *  - samples of type 'sample_t' (usually 32bit float)
 *
 * Dynamic attributes are:
 *  - number of channels
 *  - data length
 *  - sampling rate
 *  - temporal length (data length + samplerate)
 *
 * Provided services:
 *  - copying from/to other samplebuffer objects
 *  - basic audio operations
 *  - importing and exporting data from/to\n
 *    raw buffers of audio data
 *  - changing channel count, length and \n
 *    sample-rate
 *  - reserving space before-hand
 *  - realtime-safety and pointer locking
 */
class SAMPLE_BUFFER {

  friend class SAMPLE_BUFFER_FUNCTIONS;
  friend class SAMPLE_ITERATOR;
  friend class SAMPLE_ITERATOR_CHANNEL;
  friend class SAMPLE_ITERATOR_CHANNELS;
  friend class SAMPLE_ITERATOR_INTERLEAVED;

 public:

  /** @name Public type definitions */
  /*@{*/

  typedef SAMPLE_SPECS::channel_t channel_size_t;
  typedef long int buf_size_t;
  typedef SAMPLE_SPECS::sample_rate_t srate_size_t;
  typedef SAMPLE_SPECS::sample_t sample_t;

  /*@}*/

 public:

  /** @name Constructors/destructors */
  /*@{*/

  SAMPLE_BUFFER (buf_size_t buffersize = 0, channel_size_t channels = 0, srate_size_t sample_rate = 0);
  ~SAMPLE_BUFFER(void);

  /*@}*/

 public:
    
  /** @name Copying from/to other samplebuffer objects */
  /*@{*/

  void add(const SAMPLE_BUFFER& x);
  void add_with_weight(const SAMPLE_BUFFER& x, int weight);
  void copy(const SAMPLE_BUFFER& x);
  void copy_range(const SAMPLE_BUFFER& x, buf_size_t start_pos, buf_size_t end_pos, buf_size_t to_pos);

  /*@}*/

  /** @name Basic audio operations */
  /*@{*/ 

  void divide_by(sample_t dvalue);
  void limit_values(void);
  void make_silent(void);
  void make_silent_range(buf_size_t start_pos, buf_size_t end_pos);
  void resample_from(srate_size_t from_srate) { resample_with_memory(from_srate, sample_rate_rep); }
  void resample_to(srate_size_t to_srate) { resample_with_memory(sample_rate_rep, to_srate); }

  /*@}*/

  /** 
   * @name Importing and exporting data from/to raw buffers of audio data */
  /*@{*/

  void import_interleaved(unsigned char* source, buf_size_t samples, ECA_AUDIO_FORMAT::Sample_format fmt, channel_size_t ch, buf_size_t srate);
  void import_noninterleaved(unsigned char* source, buf_size_t samples, ECA_AUDIO_FORMAT::Sample_format fmt, channel_size_t ch, buf_size_t srate);
  void export_interleaved(unsigned char* target, ECA_AUDIO_FORMAT::Sample_format fmt, channel_size_t ch, buf_size_t srate);
  void export_noninterleaved(unsigned char* target, ECA_AUDIO_FORMAT::Sample_format fmt, channel_size_t ch, buf_size_t srate);
  
  /*@}*/
        
 public:

  /** @name Changing channel count, length and sample-rate. */
  /*@{*/

  void number_of_channels(channel_size_t num);
  inline channel_size_t number_of_channels(void) const { return(channel_count_rep); }

  void sample_rate(srate_size_t srate);
  inline srate_size_t sample_rate(void) const { return(sample_rate_rep); }

  void length_in_samples(buf_size_t len);
  inline buf_size_t length_in_samples(void) const { return(buffersize_rep); }
  inline double length_in_seconds(void) const { return(static_cast<double>(buffersize_rep) / sample_rate_rep); }

  /*@}*/

  /** @name Reserving space before-hand */
  /*@{*/

  void resample_init_memory(srate_size_t from_srate, srate_size_t to_srate);
  void reserve_channels(channel_size_t num);
  void reserve_length_in_samples(buf_size_t len);

  /*@}*/

  /** @name Realtime-safety and pointer locking */
  /*@{*/

  void set_rt_lock(bool state);
  void get_pointer_reflock(void);
  void release_pointer_reflock(void);

  /*@}*/

 private:

  void resample_extfilter(srate_size_t from_srate, srate_size_t to_srate);
  void resample_simplefilter(srate_size_t from_srate, srate_size_t to_srate);
  void resample_nofilter(srate_size_t from_srate, srate_size_t to_srate);
  void resample_with_memory(srate_size_t from_srate, srate_size_t to_srate);

 public:

  /** @name Data representation */

  /**
   * WARNING! Although 'buffer' is a public member, you should only 
   * use it directly for a very, very good reason. All normal 
   * input/output should be done via the SAMPLEBUFFER_ITERATORS 
   * class. Representation of 'buffer' may change at any time, 
   * and this will break all code using direct-access.
   *
   * If you do use direct access, then you must also 
   * use the get_pointer_reflock() and release_pointer_reflock()
   * calls so that reference counting is possible.
   */
  std::vector<sample_t*> buffer;

  /*@}*/

 private:

  /** @name Private data */
  /*@{*/

  channel_size_t channel_count_rep;
  buf_size_t buffersize_rep;
  srate_size_t sample_rate_rep;
  buf_size_t reserved_samples_rep;

  /*@}*/

  SAMPLE_BUFFER_impl* impl_repp;

private:

  SAMPLE_BUFFER& operator= (const SAMPLE_BUFFER& t);
  SAMPLE_BUFFER (const SAMPLE_BUFFER& x);

};

#endif
