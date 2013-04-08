/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	   *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.                *
 *                                                                                         *
 * You should have received a copy of the GNU General Public License along with this       *
 * program; if not, write to the:                                                          *
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,                    *
 * MA  02111-1307, USA.                                                                    *
 *                                                                                         *
 * --------------------------------------------------------------------------------------- *
 * Project:  Capwap                                                                        *
 *                                                                                         *
 * Author :  Ludovico Rossi (ludo@bluepixysw.com)                                          *
 *           Del Moro Andrea (andrea_delmoro@libero.it)                                    *
 *           Giovannini Federica (giovannini.federica@gmail.com)                           *
 *           Massimo Vellucci (m.vellucci@unicampus.it)                                    *
 *           Mauro Bisson (mauro.bis@gmail.com)                                            *
 *******************************************************************************************/


#ifndef __CAPWAP_CWThread_HEADER__
#define __CAPWAP_CWThread_HEADER__

#include "CWErrorHandling.h"
#include <pthread.h>
#include <limits.h>
#include <semaphore.h>
#include <time.h>
#include "timerlib.h"

#ifdef MACOSX
	/* Mac OS X */
	#define CW_USE_NAMED_SEMAPHORES
	#include <unistd.h>
	typedef struct {
		sem_t *semPtr;
	} CWThreadSem;
	#include <sys/types.h>
	#include <sys/un.h>

	#define max(a,b) ( (a) > (b) ? (a) : (b) )

#else
	typedef sem_t CWThreadSem;
#endif


#ifdef HAVE_SEM_TIMEDWAIT
	typedef CWThreadSem CWThreadTimedSem;
#else	
	typedef int CWThreadTimedSem[2]; /* pair of Unix Domain Socket */
#endif


typedef pthread_t CWThread;
typedef pthread_mutex_t CWThreadMutex;
typedef pthread_cond_t CWThreadCondition;
typedef pthread_key_t CWThreadSpecific;
typedef pthread_once_t CWThreadOnce;

typedef void* (*CW_THREAD_FUNCTION)(void*);
typedef int CWThreadId;

typedef int 	CWTimerID;
typedef void 	*CWTimerArg;

#define	CW_THREAD_RETURN_TYPE			void*
#define	CWThreadSigMask(how, set, old_set)	pthread_sigmask(how, set, old_set)
#define	CWThreadIsEqual(t1, t2)			pthread_equal(t1,t2)
#define	CWThreadSelf()				pthread_self()
#define	CWThreadKill(t1, signal)		pthread_kill(t1,signal)
#define	CWThreadSendSignal			CWThreadKill
#define	CW_THREAD_ONCE_INIT			PTHREAD_ONCE_INIT
#define	CWThreadCallOnce			pthread_once

__inline__ sem_t *CWThreadGetSemT(CWThreadSem *semPtr);

CWBool CWThreadInitLib(void);

CWBool CWCreateThread(CWThread *newThread, 
		      CW_THREAD_FUNCTION threadFunc,
		      void *arg);

CWBool CWCreateThreadCondition(CWThreadCondition *theCondition);
void CWDestroyThreadCondition(CWThreadCondition *theCondition);

CWBool CWWaitThreadCondition(CWThreadCondition *theCondition,
			     CWThreadMutex *theMutex);

CWBool CWWaitThreadConditionTimeout(CWThreadCondition *theCondition,
				    CWThreadMutex *theMutex,
				    struct timespec *pTimeout);

void CWSignalThreadCondition(CWThreadCondition *theCondition);
CWBool CWCreateThreadMutex(CWThreadMutex *theMutex);
void CWDestroyThreadMutex(CWThreadMutex *theMutex);
CWBool CWThreadMutexLock(CWThreadMutex *theMutex);
CWBool CWThreadMutexTryLock(CWThreadMutex *theMutex);
void CWThreadMutexUnlock(CWThreadMutex *theMutex);

CWBool CWThreadCreateSem(CWThreadSem *semPtr, int value);
void CWThreadDestroySem(CWThreadSem *semPtr);
CWBool CWThreadSemWait(CWThreadSem *semPtr);
CWBool CWThreadSemPost(CWThreadSem *semPtr);
CWBool CWThreadSemGetValue(CWThreadSem *semPtr, int *valuePtr);

CWBool CWThreadCreateSpecific(CWThreadSpecific *specPtr,
			      void (*destructor)(void *));

void CWThreadDestroySpecific(CWThreadSpecific *specPtr);
void *CWThreadGetSpecific(CWThreadSpecific *specPtr);
CWBool CWThreadSetSpecific(CWThreadSpecific *specPtr, void *valPtr);

void CWExitThread(void);

//void *CWThreadManageTimers(void *arg);
CWBool CWTimerCancel(CWTimerID *idPtr);
CWBool CWTimerRequest(int sec,
		      CWThread *threadPtr,
		      CWTimerID *idPtr,
		      int signalToRaise);

void CWThreadSetSignals(int how, int num, ...);

CWBool CWThreadCreateTimedSem(CWThreadTimedSem *semPtr, int value);
CWBool CWThreadTimedSemIsZero(CWThreadTimedSem *semPtr);
CWBool CWThreadTimedSemSetValue(CWThreadTimedSem *semPtr, int value);
void CWThreadDestroyTimedSem(CWThreadTimedSem *semPtr);
CWBool CWThreadTimedSemWait(CWThreadTimedSem *semPtr, time_t sec, time_t nsec);
CWBool CWThreadTimedSemPost(CWThreadTimedSem *semPtr);

#endif
