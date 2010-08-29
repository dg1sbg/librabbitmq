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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>

#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_private.h"

static const char *client_error_strings[ERROR_MAX + 1] = {
  "No error."                                 /* OK                              */
  "Could not allocate memory.",               /* ERROR_NO_MEMORY                 */
  "Received bad AMQP data.",                  /* ERROR_BAD_AQMP_DATA             */
  "Unknown AMQP class id.",                   /* ERROR_UNKOWN_CLASS              */
  "Unknown AMQP method id.",                  /* ERROR_UNKOWN_METHOD             */
  "Unknown host.",                            /* ERROR_GETHOSTBYNAME_FAILED      */
  "Incompatible AMQP version.",               /* ERROR_INCOMPATIBLE_AMQP_VERSION */
  "Connection closed unexpectedly."           /* ERROR_CONNECTION_CLOSED         */
};

static char        *gpcLibName  = NULL;
static int          gbLibOpened = 0;
static pfnLogFn_t   gpfnLogFn   = NULL;

extern int g_errno;

RABBITMQ_EXPORT char *amqp_libname( void )
{
  return gpcLibName;
}

RABBITMQ_EXPORT unsigned int amqp_libopened( void )
{
  return gbLibOpened;
}

pfnLogFn_t amqp_logfn( void )
{
  return gpfnLogFn;
}

RABBITMQ_EXPORT void amqp_lib_open( pfnLogFn_t pLogFn, int nLogLevel, int nFacility, char *pcName )
{
  if( gbLibOpened == 0 )
  {
	if( pcName != NULL )
	  gpcLibName = strdup( pcName );
	else
	  gpcLibName = strdup( DEFAULT_LIB_NAME );

    amqp_openlog( pLogFn, nLogLevel, nFacility, pcName );
    amqp_log( __FILE__, __LINE__, LOG_NOTICE, "Library %s opened and initialized.", amqp_libname() );
    gbLibOpened = 1;
  }
}

RABBITMQ_EXPORT void amqp_lib_close( void )
{
  if( gbLibOpened == 0 )
  {
    amqp_log( __FILE__, __LINE__, LOG_NOTICE, "Library %s closed.", amqp_libname() );
    amqp_closelog();
    if( gpcLibName != NULL )
    {
      free( gpcLibName);
      gpcLibName = NULL;
    }
    gbLibOpened = 0;
  }
}

RABBITMQ_EXPORT void amgp_log( char *pcFile, int nLine, int nPrio, char *pcFormat, ...)
{
  static char acBuffer[ MAX_BUFFER_SIZE ];

  va_list vArgs;
  va_start(vArgs, pcFormat);

  memset( acBuffer, 0, MAX_BUFFER_SIZE );

  vsnprintf( acBuffer, MAX_BUFFER_SIZE - 1, pcFormat, vArgs);

  amqp_log_helper(pcFile, nLine, nPrio, acBuffer);

  va_end(vArgs);
}

RABBITMQ_EXPORT int amqp_get_error( void )
{
  return g_errno;
}

RABBITMQ_EXPORT void amqp_clear_error( void )
{
  amqp_set_error( OK );
}

RABBITMQ_EXPORT char *amqp_error_string(int err)
{
  const char *str;
  int category = (err & ERROR_CATEGORY_MASK);
  err = (err & ~ERROR_CATEGORY_MASK);

  switch (category) {
  case ERROR_CATEGORY_CLIENT:
    if (err < 0 || err > ERROR_MAX)
      str = "(undefined librabbitmq error)";
    else
      str = client_error_strings[err];
    break;

  case ERROR_CATEGORY_OS:
    return amqp_os_error_string(err);
    
  default:
    str = "(undefined error category)";
  }

  return strdup(str);
}

#define RPC_REPLY(replytype)						\
  (state->most_recent_api_result.reply_type == AMQP_RESPONSE_NORMAL	\
   ? (replytype *) state->most_recent_api_result.reply.decoded		\
   : NULL)

RABBITMQ_EXPORT amqp_channel_open_ok_t *amqp_channel_open(amqp_connection_state_t state,
				  	                                      amqp_channel_t channel)
{
  amqp_bytes_t          bytes;
  amqp_channel_open_t   _simple_rpc_request__;
  amqp_method_number_t  _replies__[2]          = { AMQP_EXPAND_METHOD(CHANNEL,OPEN_OK), 0};
	  
  bytes.bytes = NULL;
  bytes.len   = 0;

  amqp_clear_error();

  _simple_rpc_request__.out_of_band = bytes;

  state->most_recent_api_result =
    amqp_simple_rpc( state, channel,
	                 AMQP_EXPAND_METHOD(CHANNEL,OPEN),
					 (amqp_method_number_t *) &_replies__,
					 &_simple_rpc_request__ );

  return RPC_REPLY(amqp_channel_open_ok_t);
}

RABBITMQ_EXPORT int amqp_basic_publish(amqp_connection_state_t state,
		                               amqp_channel_t channel,
		                               amqp_bytes_t exchange,
		                               amqp_bytes_t routing_key,
		                               amqp_boolean_t mandatory,
		                               amqp_boolean_t immediate,
		                               amqp_basic_properties_t const *properties,
		                               amqp_bytes_t body)
{
  int                     result              = OK;
  amqp_frame_t            f;
  size_t                  body_offset;
  amqp_basic_properties_t default_properties;
  size_t                  usable_body_payload_size = state->frame_max - (HEADER_SIZE + FOOTER_SIZE);
  amqp_basic_publish_t    m;

  amqp_clear_error();

  m.exchange    = exchange;
  m.routing_key = routing_key;
  m.immediate   = immediate;
  m.mandatory   = mandatory;

  result = amqp_send_method(state, channel, AMQP_BASIC_PUBLISH_METHOD, &m);
  if( result < 0 )
	return result;

  if (properties == NULL) {
    memset(&default_properties, 0, sizeof(default_properties));
    properties = &default_properties;
  }

  f.frame_type = AMQP_FRAME_HEADER;
  f.channel = channel;
  f.payload.properties.class_id = AMQP_BASIC_CLASS;
  f.payload.properties.body_size = body.len;
  f.payload.properties.decoded = (void *) properties;
  result = amqp_send_frame(state, &f);
  if( result < 0 )
	return result;

  body_offset = 0;
  while (1) {
    int remaining = body.len - body_offset;
    assert(remaining >= 0);

    if (remaining == 0)
      break;

    f.frame_type = AMQP_FRAME_BODY;
    f.channel = channel;
    f.payload.body_fragment.bytes = buf_at(body, body_offset);
    if (remaining >= usable_body_payload_size) {
      f.payload.body_fragment.len = usable_body_payload_size;
    } else {
      f.payload.body_fragment.len = remaining;
    }

    body_offset += f.payload.body_fragment.len;
    result = amqp_send_frame(state, &f);
    if( result < 0 )
      return result;
  }

  return 0;
}

RABBITMQ_EXPORT amqp_rpc_reply_t amqp_channel_close(amqp_connection_state_t state,
				                                    amqp_channel_t          channel,
				                                    int                     code)
{
  amqp_rpc_reply_t      result;
  char                  codestr[13];
  amqp_bytes_t          bytes;
  amqp_channel_close_t  _simple_rpc_request__;
  amqp_method_number_t  _replies__[2]          = { AMQP_EXPAND_METHOD(CHANNEL,CLOSE_OK), 0};
	  
  bytes.bytes = NULL;
  bytes.len   = 0;
  memset( codestr, 0, 13 );

  amqp_clear_error();

  snprintf(codestr, sizeof(codestr), "%d", code);

  _simple_rpc_request__.class_id   = 0;
  _simple_rpc_request__.method_id  = 0; 
  _simple_rpc_request__.reply_code = code;
  _simple_rpc_request__.reply_text = amqp_cstring_bytes(codestr);

  result = amqp_simple_rpc( state, channel,
	                        AMQP_EXPAND_METHOD(CHANNEL,CLOSE),
					        (amqp_method_number_t *) &_replies__,
					        &_simple_rpc_request__ );

  return result;
}

RABBITMQ_EXPORT amqp_rpc_reply_t amqp_connection_close(amqp_connection_state_t state,
				                                       int code)
{
  amqp_rpc_reply_t         result;
  char                     codestr[13];
  amqp_bytes_t             bytes;
  amqp_connection_close_t  _simple_rpc_request__;
  amqp_method_number_t     _replies__[2]          = { AMQP_EXPAND_METHOD(CONNECTION,CLOSE_OK), 0};
	  
  bytes.bytes = NULL;
  bytes.len   = 0;
  memset( codestr, 0, 13 );

  amqp_clear_error();

  snprintf(codestr, sizeof(codestr), "%d", code);

  _simple_rpc_request__.class_id   = 0;
  _simple_rpc_request__.method_id  = 0; 
  _simple_rpc_request__.reply_code = code;
  _simple_rpc_request__.reply_text = amqp_cstring_bytes(codestr);
  
  result = amqp_simple_rpc( state, 0,
	                        AMQP_EXPAND_METHOD(CONNECTION,CLOSE),
					        (amqp_method_number_t *) &_replies__,
					        &_simple_rpc_request__ );

  return result;
}

RABBITMQ_EXPORT amqp_exchange_declare_ok_t *amqp_exchange_declare(amqp_connection_state_t state,
						                                          amqp_channel_t channel,
						                                          amqp_bytes_t exchange,
						                                          amqp_bytes_t type,
						                                          amqp_boolean_t passive,
						                                          amqp_boolean_t durable,
						                                          amqp_boolean_t auto_delete,
						                                          amqp_table_t arguments)
{
  amqp_exchange_declare_t  _simple_rpc_request__;
  amqp_method_number_t     _replies__[2]          = { AMQP_EXPAND_METHOD(EXCHANGE,DECLARE_OK), 0};
	  
  amqp_clear_error();

  _simple_rpc_request__.exchange    = exchange;
  _simple_rpc_request__.type        = type; 
  _simple_rpc_request__.passive     = passive;
  _simple_rpc_request__.durable     = durable;
  _simple_rpc_request__.auto_delete = auto_delete;
  _simple_rpc_request__.internal    = 0;
  _simple_rpc_request__.nowait      = 0;
  _simple_rpc_request__.arguments   = arguments;
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(EXCHANGE,DECLARE),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_exchange_declare_ok_t);
}

RABBITMQ_EXPORT amqp_queue_declare_ok_t *amqp_queue_declare(amqp_connection_state_t state,
					                                        amqp_channel_t channel,
					                                        amqp_bytes_t queue,
					                                        amqp_boolean_t passive,
					                                        amqp_boolean_t durable,
					                                        amqp_boolean_t exclusive,
					                                        amqp_boolean_t auto_delete,
					                                        amqp_table_t arguments)
{
  amqp_queue_declare_t  _simple_rpc_request__;
  amqp_method_number_t  _replies__[2]          = { AMQP_EXPAND_METHOD(QUEUE,DECLARE_OK), 0};
	  
  amqp_clear_error();

  _simple_rpc_request__.ticket      = 0;
  _simple_rpc_request__.queue       = queue;
  _simple_rpc_request__.passive     = passive;
  _simple_rpc_request__.durable     = durable;
  _simple_rpc_request__.auto_delete = auto_delete;
  _simple_rpc_request__.nowait      = 0;
  _simple_rpc_request__.arguments   = arguments;
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(QUEUE,DECLARE),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_queue_declare_ok_t);
}

RABBITMQ_EXPORT amqp_queue_delete_ok_t *amqp_queue_delete(amqp_connection_state_t state,
					                                      amqp_channel_t channel,
					                                      amqp_bytes_t queue,
					                                      amqp_boolean_t if_unused,
					                                      amqp_boolean_t if_empty)
{
  amqp_queue_delete_t  _simple_rpc_request__;
  amqp_method_number_t _replies__[2]          = { AMQP_EXPAND_METHOD(QUEUE,DELETE_OK), 0};
	  
  amqp_clear_error();

  _simple_rpc_request__.ticket      = 0;
  _simple_rpc_request__.queue       = queue;
  _simple_rpc_request__.if_empty    = if_empty;
  _simple_rpc_request__.if_unused   = if_unused;
  _simple_rpc_request__.nowait      = 0;
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(QUEUE,DECLARE),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_queue_delete_ok_t);
}

RABBITMQ_EXPORT amqp_queue_bind_ok_t *amqp_queue_bind(amqp_connection_state_t state,
				                                      amqp_channel_t channel,
				                                      amqp_bytes_t queue,
				                                      amqp_bytes_t exchange,
				                                      amqp_bytes_t routing_key,
				                                      amqp_table_t arguments)
{
  amqp_queue_bind_t    _simple_rpc_request__;
  amqp_method_number_t _replies__[2]          = { AMQP_EXPAND_METHOD(QUEUE,BIND_OK), 0};
	  
  amqp_clear_error();

  _simple_rpc_request__.ticket      = 0;
  _simple_rpc_request__.queue       = queue;
  _simple_rpc_request__.exchange    = exchange;
  _simple_rpc_request__.routing_key = routing_key;
  _simple_rpc_request__.nowait      = 0;
  _simple_rpc_request__.arguments   = arguments;
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(QUEUE,BIND),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_queue_bind_ok_t);
}

RABBITMQ_EXPORT amqp_queue_unbind_ok_t *amqp_queue_unbind(amqp_connection_state_t state,
					                                      amqp_channel_t channel,
					                                      amqp_bytes_t queue,
					                                      amqp_bytes_t exchange,
					                                      amqp_bytes_t binding_key,
					                                      amqp_table_t arguments)
{
  amqp_queue_unbind_t   _simple_rpc_request__;
  amqp_method_number_t  _replies__[2]          = { AMQP_EXPAND_METHOD(QUEUE,UNBIND_OK), 0};
	  
  amqp_clear_error();

  _simple_rpc_request__.ticket      = 0;
  _simple_rpc_request__.queue       = queue;
  _simple_rpc_request__.exchange    = exchange;
  _simple_rpc_request__.routing_key = binding_key;
  _simple_rpc_request__.arguments   = arguments;
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(QUEUE,UNBIND),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_queue_unbind_ok_t);
}

RABBITMQ_EXPORT amqp_basic_consume_ok_t *amqp_basic_consume(amqp_connection_state_t state,
					                                        amqp_channel_t channel,
					                                        amqp_bytes_t queue,
					                                        amqp_bytes_t consumer_tag,
					                                        amqp_boolean_t no_local,
					                                        amqp_boolean_t no_ack,
					                                        amqp_boolean_t exclusive)
{
  amqp_basic_consume_t _simple_rpc_request__;
  amqp_method_number_t _replies__[2]          = { AMQP_EXPAND_METHOD(BASIC,CONSUME_OK), 0};
	  
  amqp_clear_error();

  _simple_rpc_request__.ticket        = 0;
  _simple_rpc_request__.queue         = queue;
  _simple_rpc_request__.consumer_tag  = consumer_tag;
  _simple_rpc_request__.no_local      = no_local;
  _simple_rpc_request__.no_ack        = no_ack;
  _simple_rpc_request__.nowait        = 0;
  _simple_rpc_request__.exclusive     = exclusive;
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(BASIC,CONSUME),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_basic_consume_ok_t);
}

RABBITMQ_EXPORT int amqp_basic_ack(amqp_connection_state_t state,
		                           amqp_channel_t channel,
		                           uint64_t delivery_tag,
		                           amqp_boolean_t multiple)
{
  int              result = OK;
  amqp_basic_ack_t m;

  amqp_clear_error();

  m.delivery_tag = delivery_tag;
  m.multiple     = multiple;

  result = amqp_send_method(state, channel, AMQP_BASIC_ACK_METHOD, &m);
  return result;
}

RABBITMQ_EXPORT amqp_queue_purge_ok_t *amqp_queue_purge(amqp_connection_state_t state,
					                                    amqp_channel_t channel,
					                                    amqp_bytes_t queue,
					                                    amqp_boolean_t no_wait)
{
  amqp_queue_purge_t   _simple_rpc_request__;
  amqp_method_number_t _replies__[2]          = { AMQP_EXPAND_METHOD(QUEUE,PURGE_OK), 0};
	  
  amqp_clear_error();

  _simple_rpc_request__.ticket = 0;
  _simple_rpc_request__.queue  = queue;
  _simple_rpc_request__.nowait = no_wait;
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(QUEUE,PURGE),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_queue_purge_ok_t);
}

RABBITMQ_EXPORT amqp_rpc_reply_t amqp_basic_get(amqp_connection_state_t state,
				                                amqp_channel_t channel,
				                                amqp_bytes_t queue,
				                                amqp_boolean_t no_ack)
{
  amqp_basic_get_t     _simple_rpc_request__;
  amqp_method_number_t replies[]              = { AMQP_BASIC_GET_OK_METHOD,
				                                  AMQP_BASIC_GET_EMPTY_METHOD,
				                                  0 };
  amqp_clear_error();

  _simple_rpc_request__.ticket = 0;
  _simple_rpc_request__.queue  = queue;
  _simple_rpc_request__.no_ack = no_ack;
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(BASIC,GET),
					                               (amqp_method_number_t *) &replies,
					                               &_simple_rpc_request__ );
  return state->most_recent_api_result;
}

RABBITMQ_EXPORT amqp_tx_select_ok_t *amqp_tx_select(amqp_connection_state_t state,
				                                    amqp_channel_t channel)
{
  amqp_tx_select_t     _simple_rpc_request__;
  amqp_method_number_t _replies__[2]          = { AMQP_EXPAND_METHOD(TX,SELECT_OK), 0};
	  
  amqp_clear_error();

  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(TX,SELECT),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_tx_select_ok_t);
}

RABBITMQ_EXPORT amqp_tx_commit_ok_t *amqp_tx_commit(amqp_connection_state_t state,
				                                    amqp_channel_t channel)
{
  amqp_tx_commit_t     _simple_rpc_request__;
  amqp_method_number_t _replies__[2]          = { AMQP_EXPAND_METHOD(TX,COMMIT_OK), 0};
	  
  amqp_clear_error();

  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(TX,COMMIT),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_tx_commit_ok_t);
}

RABBITMQ_EXPORT amqp_tx_rollback_ok_t *amqp_tx_rollback(amqp_connection_state_t state,
					                                    amqp_channel_t channel)
{
  amqp_tx_rollback_t   _simple_rpc_request__;
  amqp_method_number_t _replies__[2]          = { AMQP_EXPAND_METHOD(TX,ROLLBACK_OK), 0};
	  
  amqp_clear_error();
  
  state->most_recent_api_result = amqp_simple_rpc( state, channel,
	                                               AMQP_EXPAND_METHOD(TX,ROLLBACK),
					                               (amqp_method_number_t *) &_replies__,
					                               &_simple_rpc_request__ );
  return RPC_REPLY(amqp_tx_rollback_ok_t);
}

RABBITMQ_EXPORT amqp_rpc_reply_t amqp_get_rpc_reply(amqp_connection_state_t state)
{
  return state->most_recent_api_result;
}
