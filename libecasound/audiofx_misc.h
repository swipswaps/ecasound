#ifndef INCLUDED_AUDIOFX_MISC_H
#define INCLUDED_AUDIOFX_MISC_H

#include <vector>
#include "sample-specs.h"
#include "samplebuffer_iterators.h"
#include "audio-stamp.h"
#include "audiofx.h"

/**
 * Adjusts DC-offset.
 * @author Kai Vehmanen
 */
class EFFECT_DCFIX : public EFFECT_BASE {

private:

  SAMPLE_SPECS::sample_t deltafix_rep[2];
  SAMPLE_ITERATOR_CHANNEL i_rep;

public:

  virtual std::string name(void) const { return("DC-Fix"); }
  virtual std::string description(void) const { return("Adjusts DC-offset."); }
  virtual std::string parameter_names(void) const { return("delta-value-left,delta-value-right"); }
  virtual void parameter_description(int param, struct PARAM_DESCRIPTION *pd);

  virtual void set_parameter(int param, parameter_t value);
  virtual parameter_t get_parameter(int param) const;

  virtual void init(SAMPLE_BUFFER *insample);
  virtual void process(void);

  EFFECT_DCFIX* clone(void) const { return new EFFECT_DCFIX(*this); }
  EFFECT_DCFIX* new_expr(void) const { return new EFFECT_DCFIX(); }
  EFFECT_DCFIX (const EFFECT_DCFIX& x);
  EFFECT_DCFIX (parameter_t delta_left = 0.0, parameter_t delta_right = 0.0);
};

/**
 * Modify audio pitch by altering its length
 * @author Kai Vehmanen
 */
class EFFECT_PITCH_SHIFT : public EFFECT_BASE {

private:

  parameter_t pmod_rep;
  long int target_rate_rep;
  SAMPLE_BUFFER* sbuf_repp;

public:

  static const int resample_low_limit;

  virtual std::string name(void) const { return("Pitch shifter"); }
  virtual std::string description(void) const { return("Modify audio pitch by altering its length."); }
  virtual std::string parameter_names(void) const { return("change-%"); }
  virtual void parameter_description(int param, struct PARAM_DESCRIPTION *pd);

  virtual void set_parameter(int param, parameter_t value);
  virtual parameter_t get_parameter(int param) const;

  virtual void init(SAMPLE_BUFFER *insample);
  virtual void process(void);

  virtual long int output_samples(long int i_samples);

  EFFECT_PITCH_SHIFT(void) : pmod_rep(100.0), target_rate_rep(0), sbuf_repp(0) { }
  EFFECT_PITCH_SHIFT (const EFFECT_PITCH_SHIFT& x);
  EFFECT_PITCH_SHIFT* clone(void) const { return new EFFECT_PITCH_SHIFT(*this); }
  EFFECT_PITCH_SHIFT* new_expr(void) const { return new EFFECT_PITCH_SHIFT(); }
};

/**
 * Store an audio stamp object. Otherwise just let's the audio go through.
 * @author Kai Vehmanen
 */
class EFFECT_AUDIO_STAMP : public EFFECT_BASE,
			   public AUDIO_STAMP {

  SAMPLE_BUFFER* sbuf_repp;

  public:

  virtual std::string name(void) const { return("Audio stamp"); }
  virtual std::string description(void) const { return("Takes a snapshot of passing audio buffers."); }

  virtual std::string parameter_names(void) const { return("stamp-id"); }
  virtual void parameter_description(int param, struct PARAM_DESCRIPTION *pd);

  virtual void set_parameter(int param, parameter_t value);
  virtual parameter_t get_parameter(int param) const;

  virtual void init(SAMPLE_BUFFER *insample);
  virtual void process(void);

  EFFECT_AUDIO_STAMP(void);
  EFFECT_AUDIO_STAMP(const EFFECT_AUDIO_STAMP& arg);

  EFFECT_AUDIO_STAMP* clone(void) const { return new EFFECT_AUDIO_STAMP(*this); }
  EFFECT_AUDIO_STAMP* new_expr(void) const { return new EFFECT_AUDIO_STAMP(); }
};

#endif
