#define JPEG_INTERNALS
#include <jmorecfg.h>

#if !((RGB_RED == 2) && (RGB_GREEN == 1) && (RGB_BLUE == 0))
#error must use bgr order
#endif
