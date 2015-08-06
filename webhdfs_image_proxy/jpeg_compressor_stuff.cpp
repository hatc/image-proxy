#include "jpeg_compressor_stuff.h"

#include <cpcl/trace.h>

/*
 * Error exit handler: must not return to caller.
 *
 * Applications may override this if they want to get control back after
 * an error.  Typically one would longjmp somewhere instead of exiting.
 * The setjmp buffer can be made a private field within an expanded error
 * handler object.  Note that the info needed to generate an error message
 * is stored in the error object, so you can generate the message now or
 * later, at your convenience.
 * You should make sure that the JPEG object is cleaned up (with jpeg_abort
 * or jpeg_destroy) at some point.
 */
METHODDEF(void)
jpeg_error_exit(j_common_ptr cinfo) {
  // always display the message
  (*cinfo->err->output_message)(cinfo);

  // allow JPEG with a premature end of file
  if ((cinfo)->err->msg_parm.i[0] != 13) {
    // let the memory manager delete any temp files before we die
    jpeg_destroy(cinfo);

    JpegErrorManager* err = (JpegErrorManager*)cinfo->err;
    longjmp(err->jexit, 1);
  }
}

/*
 * Actual output of an error or trace message.
 * Applications may override this method to send JPEG messages somewhere
 * other than stderr.
 *
 * On Windows, printing to stderr is generally completely useless,
 * so we provide optional code to produce an error-dialog popup.
 * Most Windows applications will still prefer to override this routine,
 * but if they don't, it'll do something at least marginally useful.
 *
 * NOTE: to use the library in an environment that doesn't support the
 * C stdio library, you may have to delete the call to fprintf() entirely,
 * not just not use this routine.
 */
METHODDEF(void)
jpeg_output_message(j_common_ptr cinfo) {
  char buffer[JMSG_LENGTH_MAX];

  // create the message
  (*cinfo->err->format_message)(cinfo, buffer);
  // send it to user's message proc
  cpcl::Trace(CPCL_TRACE_LEVEL_ERROR, "jpeglib:: %s", buffer);
}

JpegStuff::JpegStuff() {
  // Step 1: allocate and initialize JPEG decompression object
  cinfo.err = jpeg_std_error(&jerr);
  jerr.error_exit     = jpeg_error_exit;
  jerr.output_message = jpeg_output_message;
  
  jpeg_create_compress(&cinfo);
}
JpegStuff::~JpegStuff() {
  /* free only data allocated with memory manager, for example with cinfo->mem->alloc_small */
  jpeg_destroy_compress(&cinfo);
}

// ----------------------------------------------------------
//   Destination manager
// ----------------------------------------------------------

/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written. It must initialize
 * next_output_byte and free_in_buffer. free_in_buffer must be
 * initialized to a positive value.
 */
METHODDEF(void)
method_init_destination(j_compress_ptr cinfo) {
  JpegOutputManager *dest = (JpegOutputManager*)cinfo->dest;
  
  dest->next_output_byte = dest->buffer;
  dest->free_in_buffer = arraysize(dest->buffer);
}

/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 * free_in_buffer must be set to a positive value when TRUE is returned.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).
 * The application should resume compression after it has made more room in the
 * output buffer. Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */
METHODDEF(boolean)
method_empty_output_buffer(j_compress_ptr cinfo) {
  JpegOutputManager *dest = (JpegOutputManager*)cinfo->dest;

  dest->out->Write(dest->buffer, arraysize(dest->buffer));
  dest->next_output_byte = dest->buffer;
  dest->free_in_buffer = arraysize(dest->buffer);
  return TRUE;

  /* bool r = dest->SendBuffer(dest->stream_handler, dest->buffer, arraysize(dest->buffer));
  dest->next_output_byte = dest->buffer;
  dest->free_in_buffer = arraysize(dest->buffer);
  return (r) ? TRUE : FALSE; */
}

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.
 * In most applications, this must flush any data remaining in the buffer.
 * Use either next_output_byte or free_in_buffer to determine how much
 * data is in the buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
METHODDEF(void)
method_term_destination(j_compress_ptr cinfo) {
  JpegOutputManager *dest = (JpegOutputManager*)cinfo->dest;
  
  size_t size = arraysize(dest->buffer) - dest->free_in_buffer;
  // write any data remaining in the buffer
  if (size > 0)
    dest->out->Write(dest->buffer, size);
}

JpegOutputManager::JpegOutputManager(boost::shared_ptr<cpcl::IOStream> out)
  : out(out) {
  init_destination = method_init_destination;
  empty_output_buffer = method_empty_output_buffer;
  term_destination = method_term_destination;
}
