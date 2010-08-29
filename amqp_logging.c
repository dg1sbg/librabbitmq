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

/* ========================================================================
 * GLOBAL VARS
 * ========================================================================
 */

static pfnLogFn_t  gpLogFn    = NULL;
static int         gnLogLevel = LOG_NOTICE;
static int         gnFacility = LOG_LOCAL7;

/* ========================================================================
 * FUNCTIONS
 * ========================================================================
 */

void amqp_openlog( pfnLogFn_t pLogFn, int nLogLevel, int nFacility, char *pcName )
{
  gpLogFn    = pLogFn;
  gnLogLevel = nLogLevel;
  gnFacility = nFacility;

#if defined( RABBITMQ_C_HAS_SYSLOG )
  openlog(pcName, LOG_NDELAY | LOG_PID, nFacility);
  setlogmask(LOG_UPTO(nLogLevel));
#endif

}

void amqp_closelog(void)
{

#if defined( RABBITMQ_C_HAS_SYSLOG )
  closelog();
#endif

  gpLogFn    = NULL;
  gnLogLevel = LOG_NOTICE;
  gnFacility = LOG_LOCAL7;
}

inline void amqp_log_helper(char *pcFile, int nLine, int nPrio, char *pcMsg)
{
  static char acBuffer[ MAX_BUFFER_SIZE ];
  int         bLogged  = 0;
  pfnLogFn_t  pfnLogFn = NULL;

  memset( acBuffer, 0, MAX_BUFFER_SIZE );

  snprintf(acBuffer, MAX_BUFFER_SIZE, "%s (%s,%d): %s", amqp_libname(), pcFile, nLine, pcMsg );

#if defined( RABBITMQ_C_HAS_SYSLOG )
  syslog( nPrio, acBuffer);
  bLogged = 1;
#endif

  if((pfnLogFn = amqp_logfn()) != NULL )
  {
	(*pfnLogFn)(pcFile, nLine, nPrio, pcMsg);
	bLogged = 1;
  }

  /* Make sure that log message is at least put somewhere */
  if( bLogged == 0 )
	fprintf(stderr, "%s\n", acBuffer);
}

void amqp_log( char *pcFile, int nLine, int nPrio, char *pcFormat, ...)
{
  static char acBuffer[ MAX_BUFFER_SIZE ];

  va_list vArgs;
  va_start(vArgs, pcFormat);

  memset( acBuffer, 0, MAX_BUFFER_SIZE );

  vsnprintf( acBuffer, MAX_BUFFER_SIZE - 1, pcFormat, vArgs);

  amqp_log_helper(pcFile, nLine, nPrio, acBuffer);

  va_end(vArgs);
}

inline void amqp_error_log_helper(char *pcFile, int nLine, int nPrio, int nError, char *pcMsg)
{
  static char acBuffer[ MAX_BUFFER_SIZE ];
  int         bLogged  = 0;
  pfnLogFn_t  pfnLogFn = NULL;

  memset( acBuffer, 0, MAX_BUFFER_SIZE );

  snprintf(acBuffer, MAX_BUFFER_SIZE, "%s (%s,%d): ERROR %d => %s", amqp_libname(), pcFile, nLine, nError, pcMsg );

#if defined( RABBITMQ_C_HAS_SYSLOG )
  syslog( nPrio, acBuffer);
  bLogged = 1;
#endif

  if((pfnLogFn = amqp_logfn()) != NULL )
  {
	(*pfnLogFn)(pcFile, nLine, nPrio, pcMsg);
	bLogged = 1;
  }

  /* Make sure that log message is at least put somewhere */
  if( bLogged == 0 )
	fprintf(stderr, "%s\n", acBuffer);
}

void amqp_log_error( char *pcFile, int nLine, int nPrio, int nError, char *pcFormat, ...)
{
  static char acBuffer[ MAX_BUFFER_SIZE ];

  va_list vArgs;
  va_start(vArgs, pcFormat);

  memset( acBuffer, 0, MAX_BUFFER_SIZE );

  vsnprintf( acBuffer, MAX_BUFFER_SIZE - 1, pcFormat, vArgs);

  amqp_error_log_helper(pcFile, nLine, nPrio, nError, acBuffer);

  va_end(vArgs);

  amqp_set_error( nError );
}
