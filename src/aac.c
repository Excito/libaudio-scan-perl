/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "aac.h"

static int
get_aacinfo(PerlIO *infile, char *file, HV *info, HV *tags)
{
  off_t file_size;
  Buffer buf;
  unsigned char *bptr;
  int err = 0;
  unsigned int id3_size = 0;
  unsigned int audio_offset = 0;
  
  buffer_init(&buf, AAC_BLOCK_SIZE);
  
  PerlIO_seek(infile, 0, SEEK_END);
  file_size = PerlIO_tell(infile);
  PerlIO_seek(infile, 0, SEEK_SET);
  
  my_hv_store( info, "file_size", newSVuv(file_size) );
  
  if ( !_check_buf(infile, &buf, 10, AAC_BLOCK_SIZE) ) {
    err = -1;
    goto out;
  }
  
  bptr = buffer_ptr(&buf);
  
  // Check for ID3 tag
  if (
    (bptr[0] == 'I' && bptr[1] == 'D' && bptr[2] == '3') &&
    bptr[3] < 0xff && bptr[4] < 0xff &&
    bptr[6] < 0x80 && bptr[7] < 0x80 && bptr[8] < 0x80 && bptr[9] < 0x80
  ) {
    /* found an ID3 header... */
    id3_size = 10 + (bptr[6]<<21) + (bptr[7]<<14) + (bptr[8]<<7) + bptr[9];

    if (bptr[5] & 0x10) {
      // footer present
      id3_size += 10;
    }
    
    audio_offset += id3_size;
    
    my_hv_store( info, "id3_version", newSVpvf( "ID3v2.%d.%d", bptr[3], bptr[4] ) );
    
    DEBUG_TRACE("Found ID3 tag of size %d\n", id3_size);
    
    // Seek past ID3 and clear buffer
    buffer_clear(&buf);
    PerlIO_seek(infile, id3_size, SEEK_SET);
      
    // Read start of AAC data
    if ( !_check_buf(infile, &buf, 10, AAC_BLOCK_SIZE) ) {
      err = -1;
      goto out;
    }
  }
  
  // Find 0xFF sync
  while ( buffer_len(&buf) >= 6 ) {
    bptr = buffer_ptr(&buf);
    
    if ( (bptr[0] == 0xFF) && ((bptr[1] & 0xF6) == 0xF0) ) {
      aac_parse_adts(infile, file, file_size - audio_offset, &buf, info);
      break;
    }
    else {    
      buffer_consume(&buf, 1);
      audio_offset++;
    }
  }
  
/*
 XXX: need an ADIF test file
  else if ( memcmp(bptr, "ADIF", 4) == 0 ) {
    aac_parse_adif(infile, file, &buf, info);
  }
*/
  
  my_hv_store( info, "audio_offset", newSVuv(audio_offset) );
  
  // Parse ID3 at end, since we somehow can't use the filehandle anymore
  // after libid3tag uses it
  if (id3_size) {
    parse_id3(infile, file, info, tags, 0);
  }
  
out:
  buffer_free(&buf);

  if (err) return err;

  return 0;
}

// ADTS parser adapted from faad

void
aac_parse_adts(PerlIO *infile, char *file, off_t audio_size, Buffer *buf, HV *info)
{
  int frames, frame_length;
  int t_framelength = 0;
  int samplerate = 0;
  int bitrate;
  uint8_t profile = 0;
  uint8_t channels = 0;
  float frames_per_sec, bytes_per_frame, length;
  
  unsigned char *bptr;
  
  /* Read all frames to ensure correct time and bitrate */
  for (frames = 0; /* */; frames++) {
    if ( !_check_buf(infile, buf, audio_size > AAC_BLOCK_SIZE ? AAC_BLOCK_SIZE : audio_size, AAC_BLOCK_SIZE) ) {
      break;
    }
    
    bptr = buffer_ptr(buf);
    
    /* check syncword */
    if (!((bptr[0] == 0xFF)&&((bptr[1] & 0xF6) == 0xF0)))
      break;

    if (frames == 0) {
      profile = (bptr[2] & 0xc0) >> 6;
      samplerate = adts_sample_rates[(bptr[2]&0x3c)>>2];
      channels = ((bptr[2] & 0x1) << 2) | ((bptr[3] & 0xc0) >> 6);
    }

    frame_length = ((((unsigned int)bptr[3] & 0x3)) << 11)
      | (((unsigned int)bptr[4]) << 3) | (bptr[5] >> 5);

    t_framelength += frame_length;
    
    if (frame_length > buffer_len(buf))
      break;

    buffer_consume(buf, frame_length);
    audio_size -= frame_length;
  }
  
  frames_per_sec = (float)samplerate/1024.0f;
  if (frames != 0)
    bytes_per_frame = (float)t_framelength/(float)(frames*1000);
  else
    bytes_per_frame = 0;
    
  bitrate = (int)(8. * bytes_per_frame * frames_per_sec + 0.5);
  
  if (frames_per_sec != 0)
    length = (float)frames/frames_per_sec;
  else
    length = 1;
  
  my_hv_store( info, "bitrate", newSVuv(bitrate * 1000) );
  my_hv_store( info, "song_length_ms", newSVuv(length * 1000) );
  my_hv_store( info, "samplerate", newSVuv(samplerate) );
  my_hv_store( info, "profile", newSVpv( aac_profiles[profile], 0 ) );
  my_hv_store( info, "channels", newSVuv(channels) );
}
