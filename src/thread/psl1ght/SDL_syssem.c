/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* Semaphores in the BeOS environment */

//#include <be/kernel/OS.h>
#include <sys/sem.h>
#include <sys/errno.h> 

#include "SDL_thread.h"


struct SDL_semaphore
{
    sys_sem_t id;
};

/* Create a counting semaphore */
SDL_sem *
SDL_CreateSemaphore(Uint32 initial_value)
{
	SDL_sem *sem;
	sys_sem_attr_t attr;

	SDL_zero( attr);
	attr.attr_protocol = SYS_SEM_ATTR_PROTOCOL;
	attr.attr_pshared = SYS_SEM_ATTR_PSHARED;

	sem = (SDL_sem *) SDL_malloc(sizeof(*sem));
	if (sem) {
		sysSemCreate( &sem->id, &attr, initial_value, 32 * 1024);
	} else {
		SDL_OutOfMemory();
	}
	return (sem);
}

/* Free the semaphore */
void
SDL_DestroySemaphore(SDL_sem * sem)
{
    if (sem) {
		sysSemDestroy( sem->id);
        SDL_free(sem);
    }
}

int
SDL_SemWaitTimeout(SDL_sem * sem, Uint32 timeout)
{
	int32_t val;
	int retval;

	if (!sem) {
		SDL_SetError("Passed a NULL semaphore");
		return -1;
	}

	int finished = 0;
	while( finished == 0)
	{
		// Do not wait
		if( timeout == 0) {
			val = sysSemTryWait( sem->id);
		// Wait Forever
		} else if (timeout == SDL_MUTEX_MAXWAIT) {
			val = sysSemWait(sem->id, 0);
		// Wait until timeout
		} else {
			timeout *= 1000;     /* PS 3uses a timeout in microseconds */
			val = sysSemWait(sem->id, timeout);
		}
		switch (val) {
			case EINTR:
				break;
			case 0:
				retval = 0;
				finished = 1;
				break;
			case ETIMEDOUT:
				retval = SDL_MUTEX_TIMEDOUT;
				finished = 1;
				break;
			default:
				SDL_SetError("sysSem[Try]Wait() failed");
				retval = -1;
				finished = 1;
				break;
		}
	}
	return retval;
}

int
SDL_SemTryWait(SDL_sem * sem)
{
    return SDL_SemWaitTimeout(sem, 0);
}

int
SDL_SemWait(SDL_sem * sem)
{
    return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
}

/* Returns the current count of the semaphore */
Uint32
SDL_SemValue(SDL_sem * sem)
{
    int32_t count;
    Uint32 value;

    value = 0;
    if (sem) {
		sysSemGetValue (sem->id, &count);
        if (count > 0) {
            value = (Uint32) count;
        }
    }
    return value;
}

/* Atomically increases the semaphore's count (not blocking) */
int
SDL_SemPost(SDL_sem * sem)
{
    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    if (sysSemPost(sem->id, 1) != 0) {
        SDL_SetError("release_sem() failed");
        return -1;
    }
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
