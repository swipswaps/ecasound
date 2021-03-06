#ifndef INCLUDE_TWO_STAGE_LINEAR_ENVELOPE_H
#define INCLUDE_TWO_STAGE_LINEAR_ENVELOPE_H

#include <string>

#include "finite-envelope.h"

/**
 * Linear envelope
 */
class TWO_STAGE_LINEAR_ENVELOPE : public FINITE_ENVELOPE {

 public:

  std::string name(void) const { return("Two-stage linear envelope"); }
  parameter_t value(void);

  void init(parameter_t step);

  std::string parameter_names(void) const { return("1st_stage_sec,2nd_stage_sec,mid_level_%"); }
  void set_parameter(int param, parameter_t value);
  parameter_t get_parameter(int param) const;

  TWO_STAGE_LINEAR_ENVELOPE_ENVELOPE(parameter_t time_in_seconds = 0.0); 
  TWO_STAGE_LINEAR_ENVELOPE_ENVELOPE* clone(void) const { return new TWO_STAGE_LINEAR_ENVELOPE_ENVELOPE(*this); }

  private:

  parameter_t curpos, curval;
};

#endif
