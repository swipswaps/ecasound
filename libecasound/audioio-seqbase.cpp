// ------------------------------------------------------------------------
// audioio-seqbase.cpp: Base class for audio sequencer objects
// Copyright (C) 1999,2002,2005,2008 Kai Vehmanen
//
// Attributes:
//     eca-style-version: 3 (see Ecasound Programmer's Guide)
//
// This program is free software; you can redistribute it and/or modify
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

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#include <kvu_message_item.h>
#include <kvu_numtostr.h>
#include <kvu_dbc.h>

#include "eca-object-factory.h"
#include "samplebuffer.h"
#include "audioio-seqbase.h"

#include "eca-error.h"
#include "eca-logger.h"

using std::cout;
using std::endl;
using SAMPLE_SPECS::sample_pos_t;

/**
 * FIXME notes  (last update 2008-03-04)
 *
 *  - Add check for supports_sample_accurate_seek(). This could
 *    be used to warn users if EWF source file cannot provide
 *    accurate enough seeking (which will lead to unexpected
 *    results).
 *  - Remove write-support altogether (changes supported io_modes(),
 *    modify write_buffer(), and remove child_write_started.
 *  - Should extend the object length when looping is enabled.
 */

/**
 * Returns the child position in samples for the given 
 * public position.
 */
sample_pos_t AUDIO_SEQUENCER_BASE::priv_public_to_child_pos(sample_pos_t pubpos) const
{
  if (pubpos <= child_offset_rep.samples()) 
    return child_start_pos_rep.samples();

  if (child_looping_rep == true) {
    sample_pos_t one_loop = 
      child_length_rep.samples() -
      child_start_pos_rep.samples();
    sample_pos_t res =
      pubpos - child_offset_rep.samples();
    DBC_CHECK(res > 0);
    if (one_loop > 0)
      res = res % one_loop;
    return res + child_start_pos_rep.samples();
  }
  else {
    /* not looping */
    return pubpos - child_offset_rep.samples() + child_start_pos_rep.samples();
  }
}

AUDIO_SEQUENCER_BASE::AUDIO_SEQUENCER_BASE (void)
{
}

AUDIO_SEQUENCER_BASE::~AUDIO_SEQUENCER_BASE(void)
{
}

AUDIO_SEQUENCER_BASE* AUDIO_SEQUENCER_BASE::clone(void) const
{
  AUDIO_SEQUENCER_BASE* target = new AUDIO_SEQUENCER_BASE();
  for(int n = 0; n < number_of_params(); n++) {
    target->set_parameter(n + 1, get_parameter(n + 1));
  }
  return target;
}

void AUDIO_SEQUENCER_BASE::open(void) throw(AUDIO_IO::SETUP_ERROR &)
{
  if (init_rep != true) {
    AUDIO_IO* tmp = ECA_OBJECT_FACTORY::create_audio_object(child_name_rep);
    if (tmp == 0) 
      throw(SETUP_ERROR(SETUP_ERROR::unexpected, "AUDIOIO-SEQBASE: Couldn't open child object."));

    ECA_LOG_MSG(ECA_LOGGER::user_objects, "Opening audio sequencer file:" + tmp->label() + ".");

    set_child(tmp);
    init_rep = true;
  }

  pre_child_open();
  child()->open();

  /* step: set srate for audio time variable */
  child_offset_rep.set_samples_per_second_keeptime(child()->samples_per_second());
  child_start_pos_rep.set_samples_per_second_keeptime(child()->samples_per_second());
  child_length_rep.set_samples_per_second_keeptime(child()->samples_per_second());

  post_child_open();

  /* step: set length of the used audio segment 
   *       (note, result may still be zero) */
  if (child_length().samples() == 0)
    set_child_length(ECA_AUDIO_TIME(child()->length_in_samples(),
				    child()->samples_per_second()));

  /* step: set public length of the EWF object */
  if (child_looping_rep != true)
    set_length_in_samples(child_offset_rep.samples() + child_length_rep.samples());

  //dump_child_debug();

  tmp_buffer.number_of_channels(child()->channels());
  tmp_buffer.length_in_samples(child()->buffersize());

  AUDIO_IO_PROXY::open();
}

void AUDIO_SEQUENCER_BASE::close(void)
{
  if (child()->is_open() == true)
    child()->close();

  AUDIO_IO_PROXY::close();
}


void AUDIO_SEQUENCER_BASE::read_buffer(SAMPLE_BUFFER* sbuf)
{
  /**
   * implementation notes:
   * 
   * position:             the current global position (note, this
   *                       can exceed child_offset+child_length when
   *                       looping is used.
   * child_offset:         global position when child is activated
   * child_start_position: position inside the child-object where
   *                       input is started (data between child
   *                       beginning and child_start_pos is not used)
   * child_length:         amount of child data between start and end
   *                       positions (end can in middle of stream or
   *                       at end-of-file position)
   * child_looping:        when child end is reached, whether to jump 
   *                       back to start position?
   * 
   * note! all cases (if-else blocks) end to setting a new 
   *       position_in_samples value
   */

  //dump_child_debug();

  if (position_in_samples() + buffersize() <= child_offset_rep.samples()) {
    // ---
    // case 1: child not active yet
    // ---
    sbuf->make_silent();
    /* ensure that channel count matches that of child */
    sbuf->number_of_channels(child()->channels());
    change_position_in_samples(buffersize());
  }
  else if (position_in_samples() < child_offset_rep.samples()) {
    // ---
    // case 2: next read will bring in the first child samples
    // ---
    
    DBC_CHECK(position_in_samples() + buffersize() > child_offset_rep.samples());

    /* make sure child position is correct at start */
    sample_pos_t chipos = 
      priv_public_to_child_pos(position_in_samples());
    if (chipos != child()->position_in_samples())
      child()->seek_position_in_samples(chipos);
    
    sample_pos_t segment = 
      position_in_samples()
      + buffersize() 
      - child_offset_rep.samples();

    if (segment > buffersize())
      segment = buffersize();

    ECA_LOG_MSG(ECA_LOGGER::user_objects, 
		"child-object '" + child()->label() + "' activated.");

    /* step: read segment of audio and copy it to the correct 
     *       location */
    long int save_bsize = child()->buffersize();
    DBC_CHECK(save_bsize == buffersize());
    child()->set_buffersize(segment);
    child()->read_buffer(&tmp_buffer);
    sbuf->copy_range(tmp_buffer, 
		     0,
		     tmp_buffer.length_in_samples(), 
		     buffersize() - segment);
    /* ensure that channel count matches that of child */
    sbuf->number_of_channels(child()->channels());
    child()->set_buffersize(save_bsize);
    change_position_in_samples(buffersize());
  }
  else {
    // ---
    // case 3: child is active
    // ---

    sample_pos_t chipos1 = 
      priv_public_to_child_pos(position_in_samples());
    sample_pos_t chipos2 = 
      priv_public_to_child_pos(position_in_samples() + buffersize());

    if (chipos2 >= 
	(child_length_rep.samples() + child_start_pos_rep.samples()) &&
	child_looping_rep != true) {
	// ---
	// case 3a: not looping, reaching child file end during the next read
	// ---

      child()->read_buffer(sbuf);

      sample_pos_t over_child_eof = 
	chipos2 - (child_length_rep.samples() + child_start_pos_rep.samples());
      
      DBC_CHECK(over_child_eof >= 0);
      DBC_CHECK((child()->finished() == true &&
		 buffersize() > sbuf->length_in_samples()) ||
		child()->finished() != true);
      
      /* resize the sbuf if needed: either EOF was encountered
       * and sbuf is shorter than buffersize() or otherwise we
       * need to drop some of the samples read so at to not
       * go beyond set length */
      sample_pos_t data_left = buffersize() - over_child_eof;
      sbuf->length_in_samples(data_left);
      change_position_in_samples(sbuf->length_in_samples());
    }
    else if (chipos2 < chipos1 &&
	     child_looping_rep == true) {
      // ---
      // case 3b: looping, we will run out of data during read
      // ---

      child()->read_buffer(sbuf);
      
      sample_pos_t over_child_eof = chipos2 - child_start_pos_rep.samples();
      
      /* step: copy segment 1 from loop end, and segment 2 from
       *       loop start point */
      sample_pos_t chistartpos = 
	priv_public_to_child_pos(position_in_samples() + 
				 buffersize() - over_child_eof);

      DBC_CHECK(chistartpos == child_start_pos_rep.samples());
      child()->seek_position_in_samples(chistartpos);

      if (over_child_eof > 0) {
	long int save_bsize = child()->buffersize();
	DBC_CHECK(save_bsize == buffersize());
	child()->set_buffersize(over_child_eof);
	tmp_buffer.number_of_channels(channels());
	child()->read_buffer(&tmp_buffer);
	DBC_CHECK(tmp_buffer.length_in_samples() == over_child_eof);
	child()->set_buffersize(save_bsize);
	DBC_CHECK((buffersize() - over_child_eof) < buffersize());
	sbuf->length_in_samples(buffersize());
	sbuf->copy_range(tmp_buffer, 
			 0,
			 tmp_buffer.length_in_samples(), 
			 buffersize() - over_child_eof);
      }
      change_position_in_samples(buffersize());
    }
    else {
      // ---
      // case 3c: normal case, read samples from child
      // ---

      child()->set_buffersize(buffersize());      
      child()->read_buffer(sbuf);
      /* note: if the 'length' parameter value is longer than 
       *       actual child object length, less than buffersize()
       *       samples will be read */

      /* before commit: delete */
#if 1
      if (sbuf->length_in_samples() < buffersize())
	cout << "CASE 3c-2\n";
#endif

      change_position_in_samples(sbuf->length_in_samples());
    }
  }
}

void AUDIO_SEQUENCER_BASE::dump_child_debug(void)
{
  cout << "global position (in samples): " << position_in_samples() << endl;
  cout << "child-pos: " << child()->position_in_samples() << endl;
  cout << "child-offset: " << child_offset_rep.samples() << endl;
  cout << "child-startpos: " << child_start_pos_rep.samples() << endl;
  cout << "child-length: " << child_length_rep.samples() << endl;
}

void AUDIO_SEQUENCER_BASE::write_buffer(SAMPLE_BUFFER* sbuf)
{
  if (child_write_started != true) {
    child_write_started = true;
    child_offset_rep.set_samples(position_in_samples());
    MESSAGE_ITEM m;
    m << "found child_offset_rep " << child_offset_rep.seconds() << ".";
    ECA_LOG_MSG(ECA_LOGGER::user_objects, m.to_string());
  }
  
  child()->write_buffer(sbuf);
  change_position_in_samples(sbuf->length_in_samples());
  extend_position();
}

void AUDIO_SEQUENCER_BASE::seek_position(void)
{
  /* in write mode, seek can be only performed once
   * the initial write has been performed to the child */
  if (is_open() == true &&
      (io_mode() == AUDIO_IO::io_read ||
       (io_mode() != AUDIO_IO::io_read &&
	child_write_started == true))) {
    sample_pos_t chipos = 
      priv_public_to_child_pos(position_in_samples());

    child()->seek_position_in_samples(chipos);
  }
}

void AUDIO_SEQUENCER_BASE::set_child_object_string(const std::string& v)
{
  child_name_rep = v;
}

void AUDIO_SEQUENCER_BASE::set_child_offset(const ECA_AUDIO_TIME& v)
{
  child_offset_rep = v;
}

void AUDIO_SEQUENCER_BASE::set_child_start_position(const ECA_AUDIO_TIME& v)
{
  child_start_pos_rep = v;
}

void AUDIO_SEQUENCER_BASE::set_child_length(const ECA_AUDIO_TIME& v)
{
  child_length_rep = v;
}

bool AUDIO_SEQUENCER_BASE::finished(void) const
{
  /** 
   * File is finished if...
   *  1) the child object is out of data (implies that looping
   *     is disabled), or
   *  2) file is open in read mode, looping is disabled and the child 
   *     position has gone beyond the requested child_length
   */
  if (child()->finished())
    return true;

  if (io_mode() != AUDIO_IO::io_read) 
    return false;

  if (child_looping_rep != true &&
      priv_public_to_child_pos(position_in_samples()) 
      >= child_length_rep.samples() + child_start_pos_rep.samples())
    return true;

  return false;
}