#ifndef __ID3TAG_H
#define __ID3TAG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char id3_byte_t;
typedef unsigned long id3_length_t;

/* tag flags */

enum {
  ID3_TAG_FLAG_UNSYNCHRONISATION     = 0x80,
  ID3_TAG_FLAG_EXTENDEDHEADER        = 0x40,
  ID3_TAG_FLAG_EXPERIMENTALINDICATOR = 0x20,
  ID3_TAG_FLAG_FOOTERPRESENT         = 0x10,

  ID3_TAG_FLAG_KNOWNFLAGS            = 0xf0
};

signed long id3_tag_query(id3_byte_t const *, id3_length_t);

#ifdef __cplusplus
}
#endif

#endif