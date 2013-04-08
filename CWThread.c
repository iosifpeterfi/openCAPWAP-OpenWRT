/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	       *
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


#include "CWCommon.h"

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h> 

#define CW_USE_THREAD_TIMERS

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

CW_THREAD_RETURN_TYPE CWThreadManageTimers(void *arg);

// Creates a thread that will execute a given function with a given parameter
CWBool CWCreateThread(CWThread *newThread, CW_THREAD_FUNCTION threadFunc, void *arg) {
	if(newThread == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWDebugLog("Create Thread\n");
		
	if(pthread_create(newThread, NULL, threadFunc, arg) != 0) {
		return CWErrorRaise(CW_ERROR_NEED_RESOURCE, "Can't create thread (maybe there are too many other threads)");
	}

	return CW_TRUE;
}

// Creates a thread condition (wrapper for pthread_cond_init)
CWBool CWCreateThreadCondition(CWThreadCondition *theCondition) {
	if(theCondition == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	switch(pthread_cond_init(theCondition, NULL)) {
		case 0: // success
			break;
		case ENOMEM:
			return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
		default:
			return CWErrorRaise(CW_ERROR_GENERAL, "Can't create thread condition");
	}
	return CW_TRUE;
}

// Frees a thread condition (wrapper for pthread_cond_destroy)
void CWDestroyThreadCondition(CWThreadCondition *theCondition) {
	if(theCondition == NULL) return;
	pthread_cond_destroy(theCondition);
}

// Wait for a thread condition (wrapper for pthread_cond_wait)
CWBool CWWaitThreadCondition(CWThreadCondition *theCondition, CWThreadMutex *theMutex) {
	
	if(theCondition == NULL || theMutex == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	switch(pthread_cond_wait(theCondition, theMutex)) {
		case 0: // success
			break;
		case  ETIMEDOUT:
			return CWErrorRaise(CW_ERROR_TIME_EXPIRED, NULL);
		default:
			return CWErrorRaise(CW_ERROR_GENERAL, "Error waiting on thread condition");	
	}

	return CW_TRUE;
}

// Wait for a thread condition (wrapper for pthread_cond_wait)
CWBool CWWaitThreadConditionTimeout(CWThreadCondition *theCondition, CWThreadMutex *theMutex, struct timespec* pTimeout) {
	if(theCondition == NULL || theMutex == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	switch(pthread_cond_timedwait(theCondition, theMutex, pTimeout)) {
		case 0: // success
			break;

		case ETIMEDOUT:
			return CWErrorRaise(CW_ERROR_TIME_EXPIRED, NULL);

		default:
			return CWErrorRaise(CW_ERROR_GENERAL, "Error waiting on thread condition");	
	}
	
	return CW_TRUE;
}

// Signal a thread condition (wrapper for pthread_cond_signal)
void CWSignalThreadCondition(CWThreadCondition *theCondition) {
	if(theCondition == NULL) return;
	
	pthread_cond_signal(theCondition);
}

// Creates a thread mutex (wrapper for pthread_mutex_init)
CWBool CWCreateThreadMutex(CWThreadMutex *theMutex) {
	if(theMutex == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	switch(pthread_mutex_init(theMutex, NULL)) {
		case 0: // success
			break;
		case ENOMEM:
			return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
		default:
			return CWErrorRaise(CW_ERROR_GENERAL, "Can't create thread mutex");
	}
	return CW_TRUE;
}


// Free a thread mutex (wrapper for pthread_mutex_destroy)
void CWDestroyThreadMutex(CWThreadMutex *theMutex)  {
	if(theMutex == NULL) return;
	pthread_mutex_destroy( theMutex );
}

// locks a mutex among threads at the specified address (blocking)
CWBool CWThreadMutexLock(CWThreadMutex *theMutex) {
	if(theMutex == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if(pthread_mutex_lock( theMutex ) != 0) {
		return CWErrorRaise(CW_ERROR_GENERAL, "Can't lock thread mutex");
	}
/*
	fprintf(stdout, "Mutex %p locked by %p.\n", theMutex, pthread_self());
	fflush(stdout);
*/
	return CW_TRUE;
}

// locks a mutex among threads at the specified address (non-blocking).
// CW_TRUE if lock was acquired, CW_FALSE otherwise
CWBool CWThreadMutexTryLock(CWThreadMutex *theMutex) {
	if(theMutex == NULL) {
		return CW_FALSE;
	}
	if(pthread_mutex_trylock( theMutex ) == EBUSY) return CW_FALSE;
	else return CW_TRUE;
}

// unlocks a mutex among threads at the specified address
void CWThreadMutexUnlock(CWThreadMutex *theMutex) {
	if(theMutex == NULL) return;
	pthread_mutex_unlock( theMutex );
/*
	fprintf(stdout, "Mutex %p UNlocked by %p.\n", theMutex, pthread_self());
	fflush(stdout);
*/
}

// creates a semaphore
CWBool CWThreadCreateSem(CWThreadSem *semPtr, int value) {
	if(semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	// we use named semaphore on platforms that support only them (e.g. Mac OS X)
	#ifdef CW_USE_NAMED_SEMAPHORES
	{
		static int semCount = 0;
		char name[32];

		snprintf(name, 32, "/CWSem-%d-%4.4d", getpid(), semCount++);
		if ( (semPtr->semPtr = sem_open(name, O_CREAT, 0600, value)) == (sem_t *)SEM_FAILED ) {
			CWErrorRaiseSystemError(CW_ERROR_GENERAL);
		} else {
			sem_unlink(name);
		}
	}
	#else
		if ( sem_init(semPtr, 0, value) < 0 ) {
			CWErrorRaiseSystemError(CW_ERROR_GENERAL);
		}
	#endif
	
	return CW_TRUE;
}

// destroy a semaphore
void CWThreadDestroySem(CWThreadSem *semPtr) {
#ifdef CW_USE_NAMED_SEMAPHORES
	if(semPtr == NULL || semPtr->semPtr == NULL) return;
#else
	if(semPtr == NULL) return;
#endif
	
	#ifdef CW_USE_NAMED_SEMAPHORES
		sem_close(semPtr->semPtr);
	#else
		sem_destroy(semPtr);
	#endif
}

// perform wait on a semaphore
CWBool CWThreadSemWait(CWThreadSem *semPtr) {
#ifdef CW_USE_NAMED_SEMAPHORES
	if(semPtr == NULL || semPtr->semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
#else
	if(semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
#endif

	//CWDebugLog("Sem Wait");
	
#ifdef CW_USE_NAMED_SEMAPHORES
	while(sem_wait(semPtr->semPtr) < 0 ) {
#else
	while(sem_wait(semPtr) < 0 ) {
#endif
		if(errno == EINTR) continue;
		else {
			CWErrorRaiseSystemError(CW_ERROR_GENERAL);
		}
	}
	
	return CW_TRUE;
}

// perform post on a semaphore
CWBool CWThreadSemPost(CWThreadSem *semPtr) {
#ifdef CW_USE_NAMED_SEMAPHORES
	if(semPtr == NULL || semPtr->semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
#else
	if(semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
#endif

#ifdef CW_USE_NAMED_SEMAPHORES
	if(sem_post(semPtr->semPtr) < 0 ) {
#else
	if(sem_post(semPtr) < 0 ) {
#endif
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	return CW_TRUE;
}

// get the value of a semaphore
CWBool CWThreadSemGetValue(CWThreadSem *semPtr, int *valuePtr) {
#ifdef CW_USE_NAMED_SEMAPHORES
	if(valuePtr == NULL || semPtr == NULL || semPtr->semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
#else
	if(valuePtr == NULL || semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
#endif

#ifdef CW_USE_NAMED_SEMAPHORES
	if(sem_getvalue(semPtr->semPtr, valuePtr) < 0) { // note: broken on Mac OS X? Btw we don't need it
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
#else
	if(sem_getvalue(semPtr, valuePtr) < 0) {
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
#endif
	if(*valuePtr < 0 ) {
		*valuePtr = 0;
	}
	
	return CW_TRUE;
}


__inline__ sem_t *CWThreadGetSemT(CWThreadSem *semPtr) {
	#ifdef CW_USE_NAMED_SEMAPHORES
		return (semPtr->semPtr);		
	#else		
		return semPtr;	
	#endif
}


// creates a semaphore that can be used with CWThreadTimedSemWait(). This type of semaphore
// is different from CWThreadSemaphore to support platforms that don't have sem_timedwait() (e.g. Mac OS X)
CWBool CWThreadCreateTimedSem(CWThreadTimedSem *semPtr, int value) {
#ifdef HAVE_SEM_TIMEDWAIT
	return CWThreadCreateSem(semPtr, value);
#else
	// if we don't have sem_timedwait(), the timed semaphore is a pair of unix domain sockets.
	// We write a dummy packet on a socket (client) when we want to post, and select() with timer on the other socket
	// when we want to wait. 
	struct sockaddr_un serverAddr, clientAddr;
	int i;
	
	if(semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if((((*semPtr)[0] = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0) ||
		(((*semPtr)[1] = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)) { // create a pair of datagram unix domain socket
		close((*semPtr)[0]);
		CWErrorRaiseSystemError(CW_ERROR_CREATING);
	}
	
	CW_ZERO_MEMORY(&serverAddr, sizeof(serverAddr));
	serverAddr.sun_family = AF_LOCAL;
	if(tmpnam((char*) &(serverAddr.sun_path)) == NULL) {
		CWErrorRaiseSystemError(CW_ERROR_CREATING);
	}
	
	CW_ZERO_MEMORY(&clientAddr, sizeof(clientAddr));
	clientAddr.sun_family = AF_LOCAL;
	if(tmpnam((char*) &(clientAddr.sun_path)) == NULL) {
		CWErrorRaiseSystemError(CW_ERROR_CREATING);
	}
	
	if(	(bind((*semPtr)[0], (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) ||
		(bind((*semPtr)[1], (struct sockaddr*) &clientAddr, sizeof(clientAddr)) < 0) ||
		(connect((*semPtr)[1], (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) || // connect each socket to the other
		(connect((*semPtr)[0], (struct sockaddr*) &clientAddr, sizeof(clientAddr)) < 0)
	) {
		close((*semPtr)[0]);
		close((*semPtr)[1]);
		CWErrorRaiseSystemError(CW_ERROR_CREATING);
	}
	
	for(i = 0; i < value; i++) {
		if(!CWThreadTimedSemPost(semPtr)) return CW_FALSE;
	}
	
	return CW_TRUE;
#endif
}

// CW_TRUE if the semaphore has zero value, CW_FALSE otherwise
CWBool CWThreadTimedSemIsZero(CWThreadTimedSem *semPtr) {
#ifdef HAVE_SEM_TIMEDWAIT
	int value;
	if(!CWThreadSemGetValue(semPtr, &value)) return CW_FALSE;
	
	return (value==0) ? CW_TRUE : CW_FALSE;
#else
	fd_set fset;
	int r;
	struct timeval timeout;
	
	if(semPtr == NULL) return CW_FALSE;

	FD_ZERO(&fset);
	FD_SET((*semPtr)[0], &fset);
	FD_SET((*semPtr)[1], &fset);
	
	timeout.tv_sec = 0; // poll
	timeout.tv_usec = 0;
	
	while((r=select(max((*semPtr)[1], (*semPtr)[0])+1, &fset, NULL, NULL, &timeout)) < 0) {
		if(errno == EINTR) {
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
			continue;
		}
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	return (r==0) ? CW_TRUE : CW_FALSE;
#endif
}

CWBool CWThreadTimedSemSetValue(CWThreadTimedSem *semPtr, int value) {
#ifdef HAVE_SEM_TIMEDWAIT
	// note: we can implement this, but our implemntation does't really need it in case
	// of a system semaphore. This is useful for our Unix Domain Socket Hack
	return CW_TRUE;
	//return CWErrorRaise(CW_ERROR_NEED_RESOURCE, "Operation Not Supported");
#else
	fd_set fset;
	int r, i;
	struct timeval timeout;
	
	if(semPtr == NULL) return CW_FALSE;

	FD_ZERO(&fset);
	FD_SET((*semPtr)[0], &fset);
	FD_SET((*semPtr)[1], &fset);
	
	timeout.tv_sec = 0; // poll
	timeout.tv_usec = 0;
	
	// first, remove all the pending packets
	CW_REPEAT_FOREVER {
		char dummy;
		while((r=select(max((*semPtr)[1], (*semPtr)[0])+1, &fset, NULL, NULL, &timeout)) < 0) {
			if(errno == EINTR) {
				timeout.tv_sec = 0;
				timeout.tv_usec = 0;
				continue;
			}
			CWErrorRaiseSystemError(CW_ERROR_GENERAL);
		}
		
		if(r == 0) break;
		
		if(FD_ISSET((*semPtr)[0], &fset)) {
			while(read((*semPtr)[0], &dummy, 1) < 0) {
				if(errno == EINTR) continue;
				CWErrorRaiseSystemError(CW_ERROR_GENERAL);
			}
		}
		
		if(FD_ISSET((*semPtr)[1], &fset)) {
			while(read((*semPtr)[1], &dummy, 1) < 0) {
				if(errno == EINTR) continue;
				CWErrorRaiseSystemError(CW_ERROR_GENERAL);
			}
		}
	}
	
	// second, send n packets, where n is the value we want to set for the semaphore
	for(i = 0; i < value; i++) {
		if(!CWThreadTimedSemPost(semPtr)) return CW_FALSE;
	}
	
	return CW_TRUE;
#endif
}

void CWThreadDestroyTimedSem(CWThreadTimedSem *semPtr) {
#ifdef HAVE_SEM_TIMEDWAIT
	CWThreadDestroySem(semPtr);
#else
	if(semPtr == NULL) return;
	close((*semPtr)[0]);
	close((*semPtr)[1]);
#endif
}

CWBool CWThreadTimedSemWait(CWThreadTimedSem *semPtr, time_t sec, time_t nsec) {
#ifdef HAVE_SEM_TIMEDWAIT
	struct timespec timeout;
	time_t t;
	
	#ifdef CW_USE_NAMED_SEMAPHORES
		if(semPtr == NULL || semPtr->semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	#else
		if(semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	#endif
	
	CWDebugLog("Sem Timed Wait");
	
	time(&t);

	timeout.tv_sec = t + sec;
	timeout.tv_nsec = nsec;
	
	#ifdef CW_USE_NAMED_SEMAPHORES
		while(sem_timedwait(semPtr->semPtr, &timeout) < 0 ) {
	#else
		while(sem_timedwait(semPtr, &timeout) < 0 ) {
	#endif
			if(errno == EINTR) { continue;}
			else if(errno == ETIMEDOUT) {
				CWDebugLog("sem_timedwait expired");
				return CWErrorRaise(CW_ERROR_TIME_EXPIRED, NULL);
			} else {
				CWErrorRaiseSystemError(CW_ERROR_GENERAL);
			}
		}
	
#else
	fd_set fset;
	int r;
	struct timeval timeout;
	char dummy;
	
	if(semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWDebugLog("Timed Sem Wait");
	
	FD_ZERO(&fset);
	FD_SET((*semPtr)[0], &fset);
	
	timeout.tv_sec = sec;
	timeout.tv_usec = nsec / 1000;
	
	CWDebugLog("Timed Sem Wait Before Select");
	while((r = select(((*semPtr)[0])+1, &fset, NULL, NULL, &timeout)) <= 0) {
		CWDebugLog("Timed Sem Wait Select error");
		if(r == 0) {
			CWDebugLog("Timed Sem Wait Timeout");
			return CWErrorRaise(CW_ERROR_TIME_EXPIRED, NULL);
		} else if(errno == EINTR) {
			timeout.tv_sec = sec;
			timeout.tv_usec = nsec / 1000;
			continue;
		}
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	CWDebugLog("Timed Sem Wait After Select");
	
	// ready to read
	
	while(read((*semPtr)[0], &dummy, 1) < 0) {
		if(errno == EINTR) continue;
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	// send ack (three-way handshake)
	
	while(send((*semPtr)[0], &dummy, 1, 0) < 0) {
		if(errno == EINTR) continue;
		CWErrorRaiseSystemError(CW_ERROR_SENDING);
	}
	
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	
	CWDebugLog("Timed Sem Wait Before Select 2");
	while((r = select(((*semPtr)[0])+1, &fset, NULL, NULL, &timeout)) <= 0) {
		CWDebugLog("Timed Sem Wait Select error 2");
		if(r == 0) {
			CWDebugLog("Timed Sem Wait Timeout 2");
			return CWErrorRaise(CW_ERROR_TIME_EXPIRED, NULL);
		} else if(errno == EINTR) {
			timeout.tv_sec = 2;
			timeout.tv_usec = 0;
			continue;
		}
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	CWDebugLog("Timed Sem Wait After Select 2");
	
	// read ack
	
	while(read((*semPtr)[0], &dummy, 1) < 0) {
		if(errno == EINTR) continue;
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
#endif
	
	CWDebugLog("End of Timed Sem Wait");
	
	return CW_TRUE;
}

CWBool CWThreadTimedSemPost(CWThreadTimedSem *semPtr) {
#ifdef HAVE_SEM_TIMEDWAIT
	return CWThreadSemPost(semPtr);
#else
	char dummy = 'D';
	fd_set fset;
	int r;
	struct timeval timeout;
	
	if(semPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWDebugLog("Timed Sem Post");
	
	while(send((*semPtr)[1], &dummy, 1, 0) < 0) {
		if(errno == EINTR) continue;
		CWErrorRaiseSystemError(CW_ERROR_SENDING);
	}
	
	// read ack (three-way handshake)
	
	FD_ZERO(&fset);
	FD_SET((*semPtr)[1], &fset);
	
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	
	CWDebugLog("Timed Sem Post Before Select");
	while((r = select(((*semPtr)[1])+1, &fset, NULL, NULL, &timeout)) <= 0) {
		CWDebugLog("Timed Sem Post Select Error");
		if(r == 0) { // timeout, server is not responding
			// note: this is not an error in a traditional semaphore, btw it's an error
			// according to our logic
			CWDebugLog("Timed Sem Post Timeout");
			return CWErrorRaise(CW_ERROR_GENERAL, "Nobody is Waiting on this Sem");
		} else if(errno == EINTR) {
			timeout.tv_sec = 2;
			timeout.tv_usec = 0;
			continue;
		}
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	CWDebugLog("Timed Sem Post After Select");
	
	while(read((*semPtr)[1], &dummy, 1) < 0) {
		if(errno == EINTR) continue;
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	// send ack
	while(send((*semPtr)[1], &dummy, 1, 0) < 0) {
		if(errno == EINTR) continue;
		CWErrorRaiseSystemError(CW_ERROR_SENDING);
	}
	
	CWDebugLog("End of Sem Post");
	
	return CW_TRUE;
#endif
}


// wrappers for pthread_key_*()
CWBool CWThreadCreateSpecific(CWThreadSpecific *specPtr, void (*destructor)(void *)) {
	if(specPtr == NULL) return CW_FALSE;  // NULL destructor is allowed

	if(pthread_key_create(specPtr, destructor) != 0) {
		CWDebugLog("Error pthread key create");
		return CW_FALSE;
	}
	
	return CW_TRUE;
}

void CWThreadDestroySpecific(CWThreadSpecific *specPtr) {
	if(specPtr == NULL) return;
	pthread_key_delete(*specPtr);
}

void *CWThreadGetSpecific(CWThreadSpecific *specPtr) {
	if(specPtr == NULL) return NULL;
	return pthread_getspecific(*specPtr);
}

CWBool CWThreadSetSpecific(CWThreadSpecific *specPtr, void *valPtr) {
	if(specPtr == NULL || valPtr == NULL) return CW_FALSE;

	switch(pthread_setspecific(*specPtr, valPtr)) {
		case 0: // success
			break;
		case ENOMEM:
			return CW_FALSE;
		default:
			return CW_FALSE;
	}
	
	return CW_TRUE;
}

// terminate the calling thread
void CWExitThread() {
	printf("\n*** Exit Thread ***\n");

	pthread_exit((void *) 0);
}


void CWThreadSetSignals(int how, int num, ...) {
	sigset_t mask;
	va_list args;
	
	sigemptyset(&mask);
	
	va_start(args, num);
	
	for(; num > 0; num--) {
		sigaddset(&mask, va_arg(args, int));
	}
	
	CWThreadSigMask(how, &mask, NULL);
	
	va_end(args);
}


// timers
typedef struct {
 	CWThread *requestedThreadPtr;
 	int signalToRaise;
} CWThreadTimerArg;

struct {
	CWThreadSem requestServiceSem;
	CWThreadMutex requestServiceMutex;
	CWThreadSem serviceProvidedSem;

	int requestedSec;
	enum {
		CW_TIMER_REQUEST,
		CW_TIMER_CANCEL,
		CW_TIMER_NONE
	} requestedOp;
	CWThread *requestedThreadPtr;
	int signalToRaise;
	
	CWBool error;
	
	CWTimerID timerID;
} gTimersData;

void CWHandleTimer(CWTimerArg arg) {
	
	CWThreadTimerArg *a = (CWThreadTimerArg*)arg;
 	CWThread requestedThreadPtr = *(a->requestedThreadPtr);
 	int signalToRaise = a->signalToRaise;

	CWThreadSendSignal(requestedThreadPtr, signalToRaise);
 	CWDebugLog("Timer Expired, Sent Signal(%d) to Thread: %d", signalToRaise, requestedThreadPtr);

	CW_FREE_OBJECT(a->requestedThreadPtr);
	CW_FREE_OBJECT(a);

	return;
}

CWBool CWTimerRequest(int sec, CWThread *threadPtr, CWTimerID *idPtr, int signalToRaise) {

	CWThreadTimerArg *arg;

	CWDebugLog("Timer Request");
	if(sec < 0 || threadPtr == NULL || idPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_OBJECT_ERR(arg, CWThreadTimerArg, return CW_FALSE;);
	CW_CREATE_OBJECT_ERR(arg->requestedThreadPtr, CWThread, CW_FREE_OBJECT(arg); return CW_FALSE;);
 	CW_COPY_MEMORY(arg->requestedThreadPtr, threadPtr, sizeof(CWThread));
 	arg->signalToRaise = signalToRaise;
 			
	CWDebugLog("Timer Request: thread(%d), signal(%d)", *(arg->requestedThreadPtr), arg->signalToRaise);
	
	if ((*idPtr = timer_add(sec, 0, &CWHandleTimer, arg)) == -1) {

		return CW_FALSE;
	}

	return CW_TRUE;
}

void CWTimerFreeArg(CWTimerArg arg) {

	CWThreadTimerArg *a = (CWThreadTimerArg*)arg;

	/* LE-03-02-2010.01 */ 
	if (a == NULL) return;
	
	CW_FREE_OBJECT(a->requestedThreadPtr);
	CW_FREE_OBJECT(a);

	return;
}

CWBool CWTimerCancel(CWTimerID *idPtr) {

	timer_rem(*idPtr, CWTimerFreeArg);
	return CW_TRUE;
}


