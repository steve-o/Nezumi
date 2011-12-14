
#include "lock_impl.hh"

nezumi::LockImpl::LockImpl() {
/* The second parameter is the spin count, for short-held locks it avoid the
 * contending thread from going to sleep which helps performance greatly.
 */
	::InitializeCriticalSectionAndSpinCount (&os_lock_, 2000);
}

nezumi::LockImpl::~LockImpl() {
	::DeleteCriticalSection (&os_lock_);
}

bool
nezumi::LockImpl::Try()
{
	if (::TryEnterCriticalSection (&os_lock_) != FALSE) {
		return true;
	}
	return false;
}

void
nezumi::LockImpl::Lock()
{
	::EnterCriticalSection (&os_lock_);
}

void
nezumi::LockImpl::Unlock()
{
	::LeaveCriticalSection (&os_lock_);
}

/* eof */