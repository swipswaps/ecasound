// ------------------------------------------------------------------------
// two-stage-linear-envelope.cpp: Two-stage linear envelope
// Copyright (C) 2000 Kai Vehmanen (kaiv@wakkanet.fi)
//
// This program is fre software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
// ------------------------------------------------------------------------

#include <kvutils/kvutils.h>

#include "two-stage-linear-envelope.h"
#include "eca-debug.h"

DYNAMIC_PARAMETERS::parameter_type TWO_STAGE_LINEAR_ENVELOPE::value(void) {
  curpos += step_length();
  if (curpos <= length_in_seconds()) {
    if (curpos < first_stage_length_rep)
      curval = (curpos / first_stage_length_rep) * midpoint_value_rep;
    else
      curval = ((curpos - first_stage_length_rep) /
		second_stage_length_rep) + midpoint_value_rep;
  }
  return(curval);
}

TWO_STAGE_LINEAR_ENVELOPE::TWO_STAGE_LINEAR_ENVELOPE(void) {
  set_parameter(1, get_parameter(1));
  set_parameter(2, get_parameter(2));
  set_parameter(3, get_parameter(3));
} 

void TWO_STAGE_LINEAR_ENVELOPE::init(DYNAMIC_PARAMETERS::parameter_type step) {
  step_length(step);

  MESSAGE_ITEM otemp;
  otemp << "(two-stage-linear-envelope) Envelope initialized: ";
  otemp.setprecision(3);
  otemp << "1." << get_parameter(1);
  otemp << "- 2." << get_parameter(2);
  otemp << "- 3." << get_parameter(3);
  ecadebug->msg(1, otemp.to_string());
}

void TWO_STAGE_LINEAR_ENVELOPE::set_parameter(int param, DYNAMIC_PARAMETERS::parameter_type value) {
  switch (param) {
  case 1:
    {
      first_stage_length_rep = value;
      length_in_seconds(first_stage_length_rep + second_stage_length_rep);
      curpos = 0.0;
      curval = 0.0;
      break;
    }
  case 2:
    {
      second_stage_length_rep = value;
      length_in_seconds(first_stage_length_rep + second_stage_length_rep);
      curpos = 0.0;
      curval = 0.0;
      break;
    }
  case 3:
    {
      midpoint_value_rep = value / 100.0;
      break;
    }
  }
}

DYNAMIC_PARAMETERS::parameter_type TWO_STAGE_LINEAR_ENVELOPE::get_parameter(int param) const {
  switch (param) {
  case 1:
    return(first_stage_length_rep);

  case 2:
    return(second_stage_length_rep);

  case 3:
    return(midpoint_value_rep * 100.0);
  }
  return(0.0);
}
