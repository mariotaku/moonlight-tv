#pragma once
// From https://github.com/xiph/opusfile/blob/master/include/opusfile.h
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE libopusfile SOFTWARE CODEC SOURCE CODE. *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE libopusfile SOURCE CODE IS (C) COPYRIGHT 1994-2012           *
 * by the Xiph.Org Foundation and contributors https://xiph.org/    *
 *                                                                  *
 ********************************************************************/
#include <stdint.h>

/**The maximum number of channels in an Ogg Opus stream.*/
#define OPUS_CHANNEL_COUNT_MAX (255)

/**Ogg Opus bitstream information.
   This contains the basic playback parameters for a stream, and corresponds to
    the initial ID header packet of an Ogg Opus stream.*/
struct OpusHead
{
    /**The Ogg Opus format version, in the range 0...255.
     The top 4 bits represent a "major" version, and the bottom four bits
      represent backwards-compatible "minor" revisions.
     The current specification describes version 1.
     This library will recognize versions up through 15 as backwards compatible
      with the current specification.
     An earlier draft of the specification described a version 0, but the only
      difference between version 1 and version 0 is that version 0 did
      not specify the semantics for handling the version field.*/
    int version;
    /**The number of channels, in the range 1...255.*/
    int channel_count;
    /**The number of samples that should be discarded from the beginning of the
      stream.*/
    unsigned pre_skip;
    /**The sampling rate of the original input.
     All Opus audio is coded at 48 kHz, and should also be decoded at 48 kHz
      for playback (unless the target hardware does not support this sampling
      rate).
     However, this field may be used to resample the audio back to the original
      sampling rate, for example, when saving the output to a file.*/
    uint32_t input_sample_rate;
    /**The gain to apply to the decoded output, in dB, as a Q8 value in the range
      -32768...32767.
     The <tt>libopusfile</tt> API will automatically apply this gain to the
      decoded output before returning it, scaling it by
      <code>pow(10,output_gain/(20.0*256))</code>.
     You can adjust this behavior with op_set_gain_offset().*/
    int output_gain;
    /**The channel mapping family, in the range 0...255.
     Channel mapping family 0 covers mono or stereo in a single stream.
     Channel mapping family 1 covers 1 to 8 channels in one or more streams,
      using the Vorbis speaker assignments.
     Channel mapping family 255 covers 1 to 255 channels in one or more
      streams, but without any defined speaker assignment.*/
    int mapping_family;
    /**The number of Opus streams in each Ogg packet, in the range 1...255.*/
    int stream_count;
    /**The number of coupled Opus streams in each Ogg packet, in the range
      0...127.
     This must satisfy <code>0 <= coupled_count <= stream_count</code> and
      <code>coupled_count + stream_count <= 255</code>.
     The coupled streams appear first, before all uncoupled streams, in an Ogg
      Opus packet.*/
    int coupled_count;
    /**The mapping from coded stream channels to output channels.
     Let <code>index=mapping[k]</code> be the value for channel <code>k</code>.
     If <code>index<2*coupled_count</code>, then it refers to the left channel
      from stream <code>(index/2)</code> if even, and the right channel from
      stream <code>(index/2)</code> if odd.
     Otherwise, it refers to the output of the uncoupled stream
      <code>(index-coupled_count)</code>.*/
    unsigned char mapping[OPUS_CHANNEL_COUNT_MAX];
};