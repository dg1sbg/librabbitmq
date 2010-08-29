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
 * Portions created by Frank Goenninger are Copyright (C) 2010
 * Consequor Consulting AG and Frank Goenninger.
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 * *
 * ***** END LICENSE BLOCK *****
 */

/* ========================================================================
 * INCLUDES
 * ========================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>

#include "amqp.h"
#include "amqp_private.h"

#if defined( WIN32 )
#include <WinSock2.h>
#endif

/* ========================================================================
 * GLOBAL VARS
 * ========================================================================
 */

int g_errno = OK;

/* ========================================================================
 * FUNCTIONS
 * ========================================================================
 */

void amqp_set_error( int error )
{
  g_errno = error;
}

#if !defined( NDEBUG )
void amqp_assert( int nCondition, char *pcFormat, ...)
{
  static char acBuffer[ MAX_BUFFER_SIZE ];

  va_list vArgs;

  if ( nCondition == 0 )
  {
	va_start(vArgs, pcFormat);

    memset( acBuffer, 0, MAX_BUFFER_SIZE );

    vsnprintf( acBuffer, MAX_BUFFER_SIZE - 1, pcFormat, vArgs);

	amqp_log( __FILE__, __LINE__, LOG_EMERG, "ASSERTION FAILED !!! - See next log line(s) for details.");
	amqp_log( __FILE__, __LINE__, LOG_EMERG, acBuffer);

    va_end(vArgs);

	abort();
  }
}
#else
void amqp_assert( int condition, char *pcFormat, ...)
{
  return;
}
#endif

#if !defined( NDEBUG )
inline
#endif
void * buf_at(amqp_bytes_t bytes, int offset)
{
  /* (&(((uint8_t *) (b).bytes)[o])); */
  uint8_t *pBuf = bytes.bytes;

#ifdef NDEBUG
  fprintf(stderr, "buf_at: At entry: Address = 0x%x.\n", pBuf);
#endif

  /* pBuf += o; */

  pBuf = &((uint8_t*)bytes.bytes)[offset];

#ifdef NDEBUG
  fprintf(stderr, "buf_at: At exit:  Address = 0x%x.\n", pBuf);
#endif

  return pBuf;
}

#if !defined( NDEBUG )
inline
#endif
int check_limit( amqp_bytes_t bytes, int offset, int length)
{
  if ((offset + length) > bytes.len)
  {
	amqp_set_error(ERROR_BAD_AMQP_DATA);
	return -ERROR_BAD_AMQP_DATA;
  }
  else
	return 0;
}

#if !defined( NDEBUG )
inline
#endif
uint8_t d_8_helper(amqp_bytes_t bytes, uint16_t offset)
{
  return * (uint8_t *) buf_at(bytes, offset);
}

#if !defined( NDEBUG )
inline
#endif
uint8_t amqp_d8(amqp_bytes_t bytes, uint16_t offset)
{
  return d_8_helper(bytes, offset);
}

#if !defined( NDEBUG )
inline
#endif
uint16_t d_16_helper(amqp_bytes_t bytes, uint16_t offset)
{
  uint16_t value;
  memcpy(&value, buf_at(bytes, offset), 2);
  return ntohl(value);
}

#if !defined( NDEBUG )
inline
#endif
uint16_t amqp_d16(amqp_bytes_t bytes, uint16_t offset)
{
  int check_result = check_limit(bytes, offset, 2);

  if( check_result < 0)
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
			        ERROR_LIMIT_OUT_OF_BOUNDS,
			        "Offset = %d.", offset );
	return 0;
  }

  return d_16_helper(bytes, offset);
}

#if !defined( NDEBUG )
inline
#endif
uint32_t d_32_helper(amqp_bytes_t bytes, uint16_t offset)
{
  uint32_t value  = 0;
  uint32_t result = 0;
  
  memcpy(&value, buf_at(bytes, offset), 4);
  
  result = ntohl(value);
  
#if defined( NDEBUG )
  fprintf(stderr, "d_32_helper: Result = %d.\n", result );
#endif
  
  return result;
}

#if !defined( NDEBUG )
inline
#endif
uint32_t amqp_d32(amqp_bytes_t bytes, uint16_t offset)
{
  int check_result = check_limit(bytes, offset, 4);

  if( check_result < 0)
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
				    ERROR_LIMIT_OUT_OF_BOUNDS,
				    "Offset = %d.", offset );
	return 0;
  }

  return d_32_helper(bytes, offset);
}

#if !defined( NDEBUG )
inline
#endif
uint64_t d_64_helper(amqp_bytes_t bytes, uint16_t offset)
{
  uint64_t hi = d_32_helper(bytes, offset);
  uint64_t lo = d_32_helper(bytes, offset + 4);

  if( amqp_get_error() != 0 )
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
				    ERROR_LIMIT_OUT_OF_BOUNDS,
				    "Offset = %d.", offset );
	return 0;
  }

  if( amqp_get_error() != 0 )
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
				    ERROR_LIMIT_OUT_OF_BOUNDS,
				    "Offset = %d.", offset );
	return 0;
  }

  return hi << 32 | lo;
}

#if !defined( NDEBUG )
inline
#endif
uint64_t amqp_d64(amqp_bytes_t bytes, uint16_t offset)
{
  int check_result = check_limit(bytes, offset, 8);

  if( check_result < 0)
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
				    ERROR_LIMIT_OUT_OF_BOUNDS,
				    "Offset = %d.", offset );
	return 0;
  }

  return d_64_helper(bytes, offset);
}

#if !defined( NDEBUG )
inline
#endif
uint8_t *amqp_dbytes(amqp_bytes_t bytes, uint16_t offset, uint16_t len)
{
  if(( offset + len ) > bytes.len )
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
					ERROR_LIMIT_OUT_OF_BOUNDS,
					"Offset = %d.", offset );
    return NULL;
  }
  else
  {
    return buf_at( bytes, offset );
  }
}

#if !defined( NDEBUG )
inline
#endif
void e_8_helper(amqp_bytes_t bytes, uint16_t offset, uint8_t value)
{
  * (uint8_t *) buf_at(bytes, offset) = value;
}

#if !defined( NDEBUG )
inline
#endif
int64_t amqp_e8(amqp_bytes_t bytes, uint16_t offset, uint8_t value)
{
  int64_t check = check_limit(bytes, offset, 1);
  if( check < 0 )
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
				    ERROR_LIMIT_OUT_OF_BOUNDS,
				    "Offset = %d.", offset );
	return check;
  }

  e_8_helper(bytes, offset, value);

  return 0;
}

#if !defined( NDEBUG )
inline
#endif
void e_16_helper(amqp_bytes_t bytes, uint16_t offset, uint16_t value)
{
  uint16_t vv = htons(value);
  memcpy(buf_at(bytes, offset), &vv, 2);
}

#if !defined( NDEBUG )
inline
#endif
int64_t amqp_e16(amqp_bytes_t bytes, uint16_t offset, uint16_t value)
{
  int check_result = check_limit(bytes, offset, 2);

  if( check_result < 0)
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
				    ERROR_LIMIT_OUT_OF_BOUNDS,
				    "Offset = %d.", offset );
	return check_result;
  }

  e_16_helper(bytes, offset, value);

  return 0;
}

#if !defined( NDEBUG )
inline
#endif
void e_32_helper(amqp_bytes_t bytes, uint16_t offset, uint32_t value)
{
  uint32_t vv = htonl(value);
  memcpy(buf_at(bytes, offset), &vv, 4);
}

#if !defined( NDEBUG )
inline
#endif
int64_t amqp_e32(amqp_bytes_t bytes, uint16_t offset, uint32_t value)
{
  int check_result = check_limit(bytes, offset, 2);

  if( check_result < 0)
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
				    ERROR_LIMIT_OUT_OF_BOUNDS,
				    "Offset = %d.", offset );
	return check_result;
  }

  e_32_helper(bytes, offset, value);

  return 0;
}

#if !defined( NDEBUG )
inline
#endif
int64_t amqp_e64(amqp_bytes_t bytes, uint16_t offset, uint64_t value)
{
  e_32_helper(bytes, offset, (uint32_t) (((uint64_t) value) >> 32));
  e_32_helper(bytes, offset + 4, (uint32_t) (((uint64_t) value) & 0xFFFFFFFF));

  return 0;
}

#if !defined( NDEBUG )
inline
#endif
int64_t amqp_ebytes(amqp_bytes_t bytes, uint16_t offset, uint16_t len, uint64_t *pvalue)
{
  int64_t check = check_limit( bytes, offset, len);
  if( check < 0 )
  {
	amqp_log_error( __FILE__, __LINE__, LOG_CRIT,
				    ERROR_LIMIT_OUT_OF_BOUNDS,
				    "Offset = %d.", offset );
	return check;
  }

  memcpy(buf_at(bytes, offset), pvalue, len);

  return 0;
}

/* ========================================================================
 * *** END OF FILE ***
 * ========================================================================
 */
