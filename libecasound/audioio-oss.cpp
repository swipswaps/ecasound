// ------------------------------------------------------------------------
// audioio-oss.cpp: OSS (/dev/dsp) input/output.
// Copyright (C) 1999-2000 Kai Vehmanen (kaiv@wakkanet.fi)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef COMPILE_OSS

#include <string>
#include <cstring>
#include <cstdio>

#include <kvutils/message_item.h>
#include <kvutils/kvu_numtostr.h>

#include "audioio-types.h"
#include "audioio-oss_impl.h"
#include "audioio-oss.h"

#include "eca-error.h"
#include "eca-debug.h"

OSSDEVICE::OSSDEVICE(const string& name,
		     bool precise_sample_rates) 
{
  label(name);
  precise_srate_mode = precise_sample_rates;
}

void OSSDEVICE::open(void) throw(SETUP_ERROR&) {
  if (is_open() == true) return;
  if (io_mode() == io_read) {
    if ((audio_fd = ::open(label().c_str(), O_RDONLY, 0)) == -1) {
      throw(SETUP_ERROR(SETUP_ERROR::io_mode, "AUDIOIO-OSS: unable to open OSS-device to O_RDONLY"));
    }
  }
  else if (io_mode() == io_write) {
    if ((audio_fd = ::open(label().c_str(), O_WRONLY, 0)) == -1) {
      // Opening device failed
      perror("(eca-oss)");
      throw(SETUP_ERROR(SETUP_ERROR::io_mode, "AUDIOIO-OSS: unable to open OSS-device to O_RWONLY"));
    }
  }
  else {
      throw(SETUP_ERROR(SETUP_ERROR::io_mode, "AUDIOIO-OSS: Simultanious intput/output not supported."));
  }
  toggle_open_state(true);

  // -------------------------------------------------------------------
  // Check capabilities

  if (ioctl(audio_fd, SNDCTL_DSP_GETCAPS, &oss_caps) == -1)
    throw(SETUP_ERROR(SETUP_ERROR::unexpected, "AUDIOIO-OSS: OSS-device doesn't support SNDCTL_DSP_GETCAPS"));

  // -------------------------------------------------------------------
  // Set triggering 

#ifndef DISABLE_OSS_TRIGGER
  if ((oss_caps & DSP_CAP_TRIGGER) == DSP_CAP_TRIGGER) {
    if (io_mode() == io_read) {
      int enable_bits = ~PCM_ENABLE_INPUT; // This disables recording
      if (::ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &enable_bits) == -1)
	throw(SETUP_ERROR(SETUP_ERROR::unexpected, "AUDIOIO-OSS: OSS-device doesn't support SNDCTL_DSP_SETTRIGGER"));
    }      
    else if (io_mode() == io_write) {
      int enable_bits = ~PCM_ENABLE_OUTPUT; // This disables playback
      if (::ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &enable_bits) == -1)
	throw(SETUP_ERROR(SETUP_ERROR::unexpected, "AUDIOIO-OSS: OSS-device doesn't support SNDCTL_DSP_SETTRIGGER"));
    }
  }
  else {
    ecadebug->msg("(audioio-oss) Warning: OSS-device doesn't support SNDCTL_DSP_SETTRIGGER!");
  }
#endif

  // -------------------------------------------------------------------
  // Set fragment size.

  if (buffersize() == 0) 
    throw(SETUP_ERROR(SETUP_ERROR::buffersize, "AUDIOIO-OSS: Buffersize() is 0!"));
    
  int fragsize, fragtotal = 16;
  unsigned short int fr_size, fr_count = 0x7fff; // 0x7fff = not limited
    
  MESSAGE_ITEM m;
  m << "Setting OSS fragment size according to buffersize() " << buffersize() << ".\n";
  m << "Setting OSS fragment size to " << buffersize() * frame_size() << ".";
  ecadebug->msg(ECA_DEBUG::user_objects, m.to_string());

  // fr_size == 4  -> the minimum fragment size: 2^4 = 16 bytes
  for(fr_size = 4; fragtotal < buffersize() * frame_size(); fr_size++)
    fragtotal = fragtotal * 2;

  fragsize = ((fr_count << 16) | fr_size);
    
  if (::ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &fragsize)==-1)
    throw(SETUP_ERROR(SETUP_ERROR::buffersize, "AUDIOIO-OSS: general OSS-error SNDCTL_DSP_SETFRAGMENT"));

  ecadebug->msg(ECA_DEBUG::user_objects, "set OSS fragment size to (2^x) " +
		   kvu_numtostr(fr_size) + ".");
    
  // -------------------------------------------------------------------
  // Select audio format

  int format;
  switch(sample_format()) 
    {
    case ECA_AUDIO_FORMAT::sfmt_u8:     { format = AFMT_U8; break; }
    case ECA_AUDIO_FORMAT::sfmt_s8:     { format = AFMT_S8; break; }
    case ECA_AUDIO_FORMAT::sfmt_s16_le: { format = AFMT_S16_LE; break; }
    case ECA_AUDIO_FORMAT::sfmt_s16_be: { format = AFMT_S16_BE; break; }
    case ECA_AUDIO_FORMAT::sfmt_s32_le: { format = AFMT_S32_LE; break; }
    case ECA_AUDIO_FORMAT::sfmt_s32_be: { format = AFMT_S32_BE; break; }
    default:
      {
	throw(SETUP_ERROR(SETUP_ERROR::sample_format, "AUDIOIO-OSS: audio format not supported (1)"));
      }
    }

  int f = format;
  if (::ioctl(audio_fd, SNDCTL_DSP_SETFMT, &f)==-1)
    throw(SETUP_ERROR(SETUP_ERROR::sample_format, "AUDIOIO-OSS: audio format not supported (2)"));
  if (f != format)
    throw(SETUP_ERROR(SETUP_ERROR::sample_format, "AUDIOIO-OSS: audio format not supported (2)"));

  // -------------------------------------------------------------------
  // Select number of channels

  int stereo; /* 0=mono, 1=stereo */
  if (channels() > 1) 
    stereo = 1;
  else
    stereo = 0;

  int t = stereo;
  if (::ioctl(audio_fd, SNDCTL_DSP_STEREO, &t)==-1)
    ecadebug->msg("(audioio-oss) Warning! Error when setting sample rate."); 

  if (stereo != t)
    throw(SETUP_ERROR(SETUP_ERROR::channels, "AUDIOIO-OSS: audio format not supported SNDCTL_DSP_STEREO"));

  // -------------------------------------------------------------------
  // Select sample rate
  // ---
  int speed = samples_per_second();
  if (::ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed) == -1)
    throw(SETUP_ERROR(SETUP_ERROR::sample_rate, "AUDIOIO-OSS: audio format not supported SNDCTL_DSP_SPEED"));
  
  if (speed != samples_per_second()) {
    if (precise_srate_mode) {
      throw(SETUP_ERROR(SETUP_ERROR::unexpected, "AUDIOIO-OSS: Requested sample rate is not supported. Audio device suggests sample rate of " + kvu_numtostr(speed) + ". Disable precise-sample-rate mode to ignore the difference."));
    }
    else {
      ecadebug->msg("(audioio-oss) Warning! Requested sample rate is not supported. Ignoring the the difference between requested (" + kvu_numtostr(samples_per_second()) + ") and suggested (" + kvu_numtostr(speed) + ") sample rates."); 
    }
  }

  // -------------------------------------------------------------------
  // Get fragment size.

  if (::ioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &fragment_size) == -1)
    throw(SETUP_ERROR(SETUP_ERROR::buffersize, "AUDIOIO-OSS: general OSS error SNDCTL_DSP_GETBLKSIZE"));

  ecadebug->msg(ECA_DEBUG::user_objects, "OSS set to use fragment size of " + 
		   kvu_numtostr(fragment_size) + ".");
}

void OSSDEVICE::stop(void) {
  AUDIO_IO_DEVICE::stop();
  ::ioctl(audio_fd, SNDCTL_DSP_POST, 0);
  ecadebug->msg(ECA_DEBUG::user_objects,"(audioio-oss) Audio device \"" + label() + "\" disabled.");
}

void OSSDEVICE::close(void) {
  if (is_open()) { 
    toggle_open_state(false);
    ::close(audio_fd);
  }
}

void OSSDEVICE::start(void) {
  AUDIO_IO_DEVICE::start();
  ecadebug->msg(ECA_DEBUG::user_objects,"(audioio-oss) Audio device \"" + label() + "\" started.");
  if ((oss_caps & DSP_CAP_TRIGGER) == DSP_CAP_TRIGGER) {
    int enable_bits;
    if (io_mode() == io_read) enable_bits = PCM_ENABLE_INPUT;
    else if (io_mode() == io_write) enable_bits = PCM_ENABLE_OUTPUT;
      ::ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &enable_bits);
  }   
  gettimeofday(&start_time, NULL);
}

long OSSDEVICE::position_in_samples(void) const { 
  if (is_running() != true) return(0);
  if ((oss_caps & DSP_CAP_REALTIME) == DSP_CAP_REALTIME) {
    count_info info;
    info.bytes = 0;
    if (io_mode() == io_read) {
      ::ioctl(audio_fd, SNDCTL_DSP_GETIPTR, &info);
    }
    else {
      ::ioctl(audio_fd, SNDCTL_DSP_GETOPTR, &info);
    }
    return(info.bytes / frame_size());
  }
  struct timeval now;
  gettimeofday(&now, NULL);
  double time = now.tv_sec * 1000000.0 + now.tv_usec -
                start_time.tv_sec * 1000000.0 - start_time.tv_usec;
  return(static_cast<long>(time * samples_per_second() / 1000000.0));
}

long int OSSDEVICE::read_samples(void* target_buffer, 
				 long int samples) {
  return(::read(audio_fd,target_buffer, frame_size() * samples) / frame_size());
}

void OSSDEVICE::write_samples(void* target_buffer, long int samples) {
  ::write(audio_fd, target_buffer, frame_size() * samples);
}

OSSDEVICE::~OSSDEVICE(void) { close(); }

#endif // COMPILE_OSS
