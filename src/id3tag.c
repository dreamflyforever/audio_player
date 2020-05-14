#include "id3tag.h"

enum tagtype {
  TAGTYPE_NONE = 0,
  TAGTYPE_ID3V1,
  TAGTYPE_ID3V2,
  TAGTYPE_ID3V2_FOOTER
};

#define assert(x)

static
unsigned long id3_parse_uint(id3_byte_t const **ptr, unsigned int bytes)
{
  unsigned long value = 0;

  assert(bytes >= 1 && bytes <= 4);

  switch (bytes) {
  case 4: value = (value << 8) | *(*ptr)++;
  case 3: value = (value << 8) | *(*ptr)++;
  case 2: value = (value << 8) | *(*ptr)++;
  case 1: value = (value << 8) | *(*ptr)++;
  }

  return value;
}

static
unsigned long id3_parse_syncsafe(id3_byte_t const **ptr, unsigned int bytes)
{
  unsigned long value = 0;

  assert(bytes == 4 || bytes == 5);

  switch (bytes) {
  case 5: value = (value << 4) | (*(*ptr)++ & 0x0f);
  case 4: value = (value << 7) | (*(*ptr)++ & 0x7f);
          value = (value << 7) | (*(*ptr)++ & 0x7f);
	  value = (value << 7) | (*(*ptr)++ & 0x7f);
	  value = (value << 7) | (*(*ptr)++ & 0x7f);
  }

  return value;
}

static
enum tagtype tagtype(id3_byte_t const *data, id3_length_t length)
{
  if (length >= 3 &&
      data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
    return TAGTYPE_ID3V1;

  if (length >= 10 &&
      ((data[0] == 'I' && data[1] == 'D' && data[2] == '3') ||
       (data[0] == '3' && data[1] == 'D' && data[2] == 'I')) &&
      data[3] < 0xff && data[4] < 0xff &&
      data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80 && data[9] < 0x80)
    return data[0] == 'I' ? TAGTYPE_ID3V2 : TAGTYPE_ID3V2_FOOTER;

  return TAGTYPE_NONE;
}

static
void parse_header(id3_byte_t const **ptr,
		  unsigned int *version, int *flags, id3_length_t *size)
{
  *ptr += 3;

  *version = id3_parse_uint(ptr, 2);
  *flags   = id3_parse_uint(ptr, 1);
  *size    = id3_parse_syncsafe(ptr, 4);
}

signed long id3_tag_query(id3_byte_t const *data, id3_length_t length)
{
  unsigned int version;
  int flags;
  id3_length_t size;

  assert(data);

  switch (tagtype(data, length)) {
  case TAGTYPE_ID3V1:
    return 128;

  case TAGTYPE_ID3V2:
    parse_header(&data, &version, &flags, &size);

    if (flags & ID3_TAG_FLAG_FOOTERPRESENT)
      size += 10;

    return 10 + size;

  case TAGTYPE_ID3V2_FOOTER:
    parse_header(&data, &version, &flags, &size);
    return -size - 10;

  case TAGTYPE_NONE:
    break;
  }

  return 0;
}
