#ifndef INCLUDED_AUDIOIO_EWF_H
#define INCLUDED_AUDIOIO_EWF_H

#include <string>
#include "audioio.h"
#include "audioio-wave.h"
#include "samplebuffer.h"
#include "eca-audio-time.h"
#include "resource-file.h"

/**
 * Ecasound Wave File - a simple wrapper class for handling 
 * other audio objects. When writing .ewf files, it's possible to 
 * seek beyond end position. When first write_buffer() call is issued, 
 * current sample offset is stored into the .ewf file and corresponding 
 * child object is opened for writing. Read_buffer() calls return silent 
 * buffers until sample_offset is reached. After that, audio object is 
 * processed normally. Similarly .ewf supports audio relocation, looping, etc...
 *
 * Related design patterns:
 *     - Proxy (GoF207
 *
 * @author Kai Vehmanen
 */
class EWFFILE : public AUDIO_IO {

 public:

  EWFFILE (const std::string& name = "") { set_label(name); child = 0; }
  virtual ~EWFFILE(void);

  virtual EWFFILE* clone(void) const;
  virtual EWFFILE* new_expr(void) const { return new EWFFILE(); }

  virtual std::string name(void) const { return("Ecasound wave file"); }
  virtual std::string description(void) const { return("Special format acts as a wrapper for other file formats. It can used for looping, audio data relocation and other special tasks."); }

  virtual bool locked_audio_format(void) const { return(true); }

  /**
   * Set start offset for child object
   */
  void child_offset(const ECA_AUDIO_TIME& v);

  /**
   * Set start position inside child object.
   */
  void child_start_position(const ECA_AUDIO_TIME& v);

  /**
   * Set child length. If not set, defaults to the total length. 
   */
  void child_length(const ECA_AUDIO_TIME& v);

  /**
   * Toggle whether child object data is looped.
   */
  void toggle_looping(bool v) { child_looping_rep = v; }
    
  virtual bool finished(void) const;

  virtual void set_buffersize(long int samples);
  virtual long int buffersize(void) const;

  virtual void read_buffer(SAMPLE_BUFFER* sbuf);
  virtual void write_buffer(SAMPLE_BUFFER* sbuf);

  /** @name Reimplemented functions from ECA_AUDIO_POSITION */
  /*@{*/

  virtual SAMPLE_SPECS::sample_pos_t length_in_samples(void) const;
  virtual void seek_position(void);

  /* -- not reimplemented 
   * virtual SAMPLE_SPECS::sample_pos_t position_in_samples(void) const;
   * virtual void set_length_in_samples(SAMPLE_SPECS::sample_pos_t pos);
   * virtual void set_position_in_samples(SAMPLE_SPECS::sample_pos_t pos);
   */
 
  /*@}*/


  virtual void open(void) throw(AUDIO_IO::SETUP_ERROR&);
  virtual void close(void);
 
private:

  AUDIO_IO* child;
  SAMPLE_BUFFER tmp_buffer;

  bool child_looping_rep;
  ECA_AUDIO_TIME child_offset_rep,
                 child_start_pos_rep,
                 child_length_rep;
  std::string child_name_rep;
  long int buffersize_rep;
  bool child_active;

  RESOURCE_FILE ewf_rc;

  void dump_child_debug(void);    
  void read_ewf_data(void) throw(ECA_ERROR&);
  void write_ewf_data(void);
  void init_default_child(void) throw(ECA_ERROR&);
  
  EWFFILE& operator=(const EWFFILE& x) { return *this; }
  EWFFILE (const EWFFILE& x) { }

};

#endif
