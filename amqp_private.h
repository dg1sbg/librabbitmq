#ifndef librabbitmq_amqp_private_h
#define librabbitmq_amqp_private_h

/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * The Original Code is librabbitmq.
 *
 * The Initial Developers of the Original Code are LShift Ltd, Cohesive
 * Financial Technologies LLC, and Rabbit Technologies Ltd.  Portions
 * created before 22-Nov-2008 00:00:00 GMT by LShift Ltd, Cohesive
 * Financial Technologies LLC, or Rabbit Technologies Ltd are Copyright
 * (C) 2007-2008 LShift Ltd, Cohesive Financial Technologies LLC, and
 * Rabbit Technologies Ltd.
 *
 * Portions created by LShift Ltd are Copyright (C) 2007-2009 LShift
 * Ltd. Portions created by Cohesive Financial Technologies LLC are
 * Copyright (C) 2007-2009 Cohesive Financial Technologies
 * LLC. Portions created by Rabbit Technologies Ltd are Copyright (C)
 * 2007-2009 Rabbit Technologies Ltd.
 *
 * Portions created by Tony Garnock-Jones are Copyright (C) 2009-2010
 * LShift Ltd and Tony Garnock-Jones.
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2 or later (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GPL, and not to allow others to use your
 * version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the
 * notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of
 * this file under the terms of any one of the MPL or the GPL.
 *
 * ***** END LICENSE BLOCK *****
 */

/* $Header$ */

#ifdef __cplusplus
extern "C" {
#endif

/* A couple of defines to make Visual Studio / MSC happy ;-) */
/* frgo, 2010-08-28 */

#if defined( WIN32 )

#define snprintf sprintf_s
#define strdup _strdup

#else

#define RABBITMQ_EXPORT EXPORT

#endif

#define MAX_BUFFER_SIZE  1024
#define DEFAULT_LIB_NAME "LIBRABBITMQ $Version$"

/* Error numbering: Because of differences in error numbering on
 * different platforms, we want to keep error numbers opaque for
 * client code.  Internally, we encode the category of an error
 * (i.e. where its number comes from) in the top bits of the number
 * (assuming that an int has at least 32 bits).
 */

#define ERROR_CATEGORY_MASK (1 << 29)

#define ERROR_CATEGORY_CLIENT (0 << 29) /* librabbitmq error codes */
#define ERROR_CATEGORY_OS (1 << 29) /* OS-specific error codes */

/* librabbitmq error codes */

#define OK                                 0
#define ERROR_NO_MEMORY                    1
#define ERROR_BAD_AMQP_DATA                2
#define ERROR_UNKNOWN_CLASS                3
#define ERROR_UNKNOWN_METHOD               4
#define ERROR_GETHOSTBYNAME_FAILED         5
#define ERROR_INCOMPATIBLE_AMQP_VERSION    6
#define ERROR_CONNECTION_CLOSED            7
#define ERROR_LIMIT_OUT_OF_BOUNDS          8

#define ERROR_MAX                          8

extern void  amqp_set_error(int error);
extern char *amqp_os_error_string(int err);

/*
 * Connection states:
 *
 * - CONNECTION_STATE_IDLE: initial state, and entered again after
 *   each frame is completed. Means that no bytes of the next frame
 *   have been seen yet. Connections may only be reconfigured, and the
 *   connection's pools recycled, when in this state. Whenever we're
 *   in this state, the inbound_buffer's bytes pointer must be NULL;
 *   any other state, and it must point to a block of memory allocated
 *   from the frame_pool.
 *
 * - CONNECTION_STATE_WAITING_FOR_HEADER: Some bytes of an incoming
 *   frame have been seen, but not a complete frame header's worth.
 *
 * - CONNECTION_STATE_WAITING_FOR_BODY: A complete frame header has
 *   been seen, but the frame is not yet complete. When it is
 *   completed, it will be returned, and the connection will return to
 *   IDLE state.
 *
 * - CONNECTION_STATE_WAITING_FOR_PROTOCOL_HEADER: The beginning of a
 *   protocol version header has been seen, but the full eight bytes
 *   hasn't yet been received. When it is completed, it will be
 *   returned, and the connection will return to IDLE state.
 *
 */
typedef enum amqp_connection_state_enum_ {
  CONNECTION_STATE_IDLE = 0,
  CONNECTION_STATE_WAITING_FOR_HEADER,
  CONNECTION_STATE_WAITING_FOR_BODY,
  CONNECTION_STATE_WAITING_FOR_PROTOCOL_HEADER
} amqp_connection_state_enum;

/* 7 bytes up front, then payload, then 1 byte footer */
#define HEADER_SIZE 7
#define FOOTER_SIZE 1

typedef struct amqp_link_t_ {
  struct amqp_link_t_ *next;
  void *data;
} amqp_link_t;

struct amqp_connection_state_t_ {
  amqp_pool_t frame_pool;
  amqp_pool_t decoding_pool;

  amqp_connection_state_enum state;

  int channel_max;
  int frame_max;
  int heartbeat;
  amqp_bytes_t inbound_buffer;

  size_t inbound_offset;
  size_t target_size;

  amqp_bytes_t outbound_buffer;

  int sockfd;
  amqp_bytes_t sock_inbound_buffer;
  size_t sock_inbound_offset;
  size_t sock_inbound_limit;

  amqp_link_t *first_queued_frame;
  amqp_link_t *last_queued_frame;

  amqp_rpc_reply_t most_recent_api_result;
};

extern void    *buf_at(amqp_bytes_t bytes,
		               int          offset);

extern int      check_limit(amqp_bytes_t bytes,
		                    int          offset,
		                    int          length);

extern uint8_t  d_8_helper(amqp_bytes_t bytes,
		                   uint16_t     offset);
extern uint16_t d_16_helper(amqp_bytes_t bytes,
		                    uint16_t     offset);
extern uint32_t d_32_helper(amqp_bytes_t bytes,
		                    uint16_t     offset);
extern uint64_t d_64_helper(amqp_bytes_t bytes,
		                    uint16_t     offset);

extern uint8_t  amqp_d8(amqp_bytes_t bytes,
		                uint16_t     offset);
extern uint16_t amqp_d16(amqp_bytes_t bytes,
		                 uint16_t     offset);
extern uint32_t amqp_d32(amqp_bytes_t bytes,
		                 uint16_t     offset);
extern uint64_t amqp_d64(amqp_bytes_t bytes,
		                 uint16_t     offset);

extern void     e_8_helper(amqp_bytes_t bytes, uint16_t offset, uint8_t value);
extern void     e_16_helper(amqp_bytes_t bytes, uint16_t offset, uint16_t value);
extern void     e_32_helper(amqp_bytes_t bytes, uint16_t offset, uint32_t value);

extern int64_t  amqp_e8(amqp_bytes_t bytes, uint16_t offset, uint8_t value);
extern int64_t  amqp_e16(amqp_bytes_t bytes, uint16_t offset, uint16_t value);
extern int64_t  amqp_e32(amqp_bytes_t bytes, uint16_t offset, uint32_t value);
extern int64_t  amqp_e64(amqp_bytes_t bytes, uint16_t offset, uint64_t value);

extern uint8_t *amqp_dbytes(amqp_bytes_t bytes,
		                    uint16_t     offset,
		                    uint16_t     len);

extern int64_t  amqp_ebytes(amqp_bytes_t  bytes,
		                    uint16_t      offset,
		                    uint16_t      len,
		                    uint64_t     *pvalue);


/***  END OF REWRITE SECTION *** - frgo, 2010-08-24 */

extern int amqp_decode_table(amqp_bytes_t   encoded,
			                 amqp_pool_t   *pool,
			                 amqp_table_t  *output,
			                 int           *offsetptr);

extern int amqp_encode_table(amqp_bytes_t encoded,
			     amqp_table_t *input,
			     int *offsetptr);

void amqp_assert( int nCondition, char *pcFormat, ...);

#ifndef NDEBUG
extern void amqp_dump(void const *buffer, size_t len);
#else
#define amqp_dump(buffer, len) ((void) 0)
#endif

/* Logging functions */

void amqp_openlog( pfnLogFn_t pLogFn, int nLogLevel, int nFacility, char *pcName );
void amqp_closelog(void);
pfnLogFn_t amqp_logfn(void);
void amqp_log_helper(char *pcFile, int nLine, int nPrio, char *pcMsg);
void amqp_error_log_helper(char *pcFile, int nLine, int nPrio, int nError, char *pcMsg);

#ifdef __cplusplus
}
#endif

#endif
