// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License

#ifndef __PROCESS_SEMAPHORE_HPP__
#define __PROCESS_SEMAPHORE_HPP__

#ifdef __MACH__
#include <mach/mach.h>
#elif __WINDOWS__
#include <stout/windows.hpp>
#else
#include <semaphore.h>
#endif // __MACH__

#include <stout/check.hpp>

// TODO(benh): Introduce a user-level semaphore that _only_ traps into
// the kernel if the thread would actually need to wait.

// TODO(benh): Add tests for these!

#ifdef __MACH__
class KernelSemaphore
{
public:
  KernelSemaphore()
  {
    CHECK_EQ(
        KERN_SUCCESS,
        semaphore_create(mach_task_self(), &semaphore, SYNC_POLICY_FIFO, 0));
  }

  KernelSemaphore(const KernelSemaphore& other) = delete;

  ~KernelSemaphore()
  {
    CHECK_EQ(KERN_SUCCESS, semaphore_destroy(mach_task_self(), semaphore));
  }

  KernelSemaphore& operator=(const KernelSemaphore& other) = delete;

  void wait()
  {
    CHECK_EQ(KERN_SUCCESS, semaphore_wait(semaphore));
  }

  void signal()
  {
    CHECK_EQ(KERN_SUCCESS, semaphore_signal(semaphore));
  }

private:
  semaphore_t semaphore;
};
#elif __WINDOWS__
class KernelSemaphore
{
public:
  KernelSemaphore()
  {
    semaphore = CHECK_NOTNULL(CreateSemaphore(nullptr, 0, LONG_MAX, nullptr));
  }

  KernelSemaphore(const KernelSemaphore& other) = delete;

  ~KernelSemaphore()
  {
    CHECK(CloseHandle(semaphore));
  }

  KernelSemaphore& operator=(const KernelSemaphore& other) = delete;

  void wait()
  {
    CHECK_EQ(WAIT_OBJECT_0, WaitForSingleObject(semaphore, INFINITE));
  }

  void signal()
  {
    CHECK(ReleaseSemaphore(semaphore, 1, nullptr));
  }

private:
  HANDLE semaphore;
};
#else
class KernelSemaphore
{
public:
  KernelSemaphore()
  {
    PCHECK(sem_init(&semaphore, 0, 0) == 0);
  }

  KernelSemaphore(const KernelSemaphore& other) = delete;

  ~KernelSemaphore()
  {
    PCHECK(sem_destroy(&semaphore) == 0);
  }

  KernelSemaphore& operator=(const KernelSemaphore& other) = delete;

  void wait()
  {
    int result = sem_wait(&semaphore);

    while (result != 0 && errno == EINTR) {
      result = sem_wait(&semaphore);
    }

    PCHECK(result == 0);
  }

  void signal()
  {
    PCHECK(sem_post(&semaphore) == 0);
  }

private:
  sem_t semaphore;
};
#endif // __MACH__


// Provides a "decomissionable" kernel semaphore which allows us to
// effectively flush all waiters and keep any future threads from
// waiting. In order to be able to decomission the semaphore we need
// to keep around the number of waiters so we can signal them all.
class DecomissionableKernelSemaphore : public KernelSemaphore
{
public:
  void wait()
  {
    // NOTE: we must check `commissioned` AFTER we have incremented
    // `waiters` otherwise we might race with `decomission()` and fail
    // to properly get signaled.
    waiters.fetch_add(1);

    if (!comissioned.load()) {
      waiters.fetch_sub(1);
      return;
    }

    KernelSemaphore::wait();

    waiters.fetch_sub(1);
  }

  void decomission()
  {
    comissioned.store(false);

    // Now signal all the waiters so they wake up and stop
    // waiting. Note that this may do more `signal()` than necessary
    // but since no future threads will wait that doesn't matter (it
    // would only matter if we cared about the value of the semaphore
    // which in the current implementation we don't).
    for (size_t i = waiters.load(); i > 0; i--) {
      signal();
    }
  }

  bool decomissioned() const
  {
    return !comissioned.load();
  }

  size_t capacity() const
  {
    // The semaphore probably doesn't actually support this many but
    // who knows how to get this value otherwise.
    return SIZE_MAX;
  }

private:
  std::atomic<bool> comissioned = ATOMIC_VAR_INIT(true);
  std::atomic<size_t> waiters = ATOMIC_VAR_INIT(0);
};


// Benchmarks have shown that the performance of semaphores on Linux
// have not been great compared to the performance of semaphores on OS
// X. One notable performance improvement is to wake up threads in a
// last-in-first-out model rather than a queue. We provide an
// approximation of this using a fixed array that waiters atomically
// add themselves into and signalers atomically remove waiters
// from. The actual mechanism for signaling the waiting threads are
// individual thread-local semaphores.
THREAD_LOCAL KernelSemaphore* __semaphore__ = nullptr;

#define _semaphore_                                                     \
  (__semaphore__ == nullptr                                             \
   ? __semaphore__ = new KernelSemaphore()                              \
   : __semaphore__)

class DecomissionableFixedSizeLastInFirstOutSemaphore
{
public:
  DecomissionableFixedSizeLastInFirstOutSemaphore()
  {
    for (size_t i = 0; i < SEMAPHORES; i++) {
      semaphores[i] = nullptr;
    }
  }

  void signal()
  {
    count.fetch_add(1);

    while (waiters.load() > 0 && count.load() > 0) {
      for (size_t i = 0; i < SEMAPHORES; i++) {
        // Don't bother signaling if there is nobody to signal
        // (`waiters` == 0) or nothing to do (`count` == 0).
        if (waiters.load() == 0 || count.load() == 0) {
          return;
        }

        // Try and find and then signal a waiter.
        KernelSemaphore* semaphore = semaphores[i].load();
        if (semaphore != nullptr) {
          if (!semaphores[i].compare_exchange_strong(semaphore, nullptr)) {
            continue;
          }

          semaphore->signal();

          // NOTE: we decrement `waiters` _here_ rather than in `wait`
          // so that future signalers won't bother looping here
          // (potentially for a long time) trying to find a waiter
          // that might have already been signaled but just hasn't
          // woken up yet.
          waiters.fetch_sub(1);
          return;
        }
      }
    }
  }

  void wait()
  {
    do {
      size_t old = count.load();
      while (old > 0) {
      CAS:
        if (!count.compare_exchange_strong(old, old - 1)) {
          continue;
        }
        return;
      }

      // Need to actually wait (slow path).
      waiters.fetch_add(1);

      // NOTE: we must check `commissioned` AFTER we have
      // incremented `waiters` otherwise we might race with
      // `decomission()` and fail to properly get signaled.
      if (!comissioned.load()) {
        waiters.fetch_sub(1);
        return;
      }

      bool done = false;
      while (!done) {
        for (size_t i = 0; i < SEMAPHORES; i++) {
          KernelSemaphore* semaphore = semaphores[i].load();
          if (semaphore == nullptr) {
            // NOTE: we _must_ check one last time if we should really
            // wait because there is a race that `signal()` was
            // completely executed in between when we checked `count`
            // and when we incremented `waiters` and hence we could
            // wait forever. We delay this check until the 11th hour
            // so that we can also benefit from the possibility that
            // more things have been enqueued while we were looking
            // for a slot in the array.
            if ((old = count.load()) > 0) {
              waiters.fetch_sub(1);
              goto CAS;
            }
            if (semaphores[i].compare_exchange_strong(semaphore, _semaphore_)) {
              done = true;
              break;
            }
          }
        }
      }

      // TODO(benh): To make this be wait-free for the signalers we
      // need to enqueue semaphore before we increment `waiters`. The
      // reason we can't do that right now is because we don't know
      // how to remove ourselves from `semaphores` if, after checking
      // `count` (which we need to do due to the race between
      // signaling and waiting) we determine that we don't need to
      // wait (because then we have our semaphore stuck in the
      // queue). A solution here could be to have a fixed size queue
      // that we can just remove ourselves from, but then note that
      // we'll need to set the semaphore back to zero in the event
      // that it got signaled so the next time we don't _not_ wait.

      _semaphore_->wait();
    } while (true);
  }

  void decomission()
  {
    comissioned.store(false);

    // Now signal all the waiters so they wake up and stop
    // waiting. Note that this may do more `signal()` than necessary
    // but since no future threads will wait that doesn't matter (it
    // would only matter if we cared about the value of the semaphore
    // which in the current implementation we don't).
    for (size_t i = waiters.load(); i > 0; i--) {
      signal();
    }
  }

  bool decomissioned() const
  {
    return !comissioned.load();
  }

  size_t capacity() const
  {
    return SEMAPHORES;
  }

private:
  static constexpr size_t SEMAPHORES = 128;
  std::atomic<bool> comissioned = ATOMIC_VAR_INIT(true);
  std::atomic<size_t> count = ATOMIC_VAR_INIT(0);
  std::atomic<size_t> waiters = ATOMIC_VAR_INIT(0);
  std::array<std::atomic<KernelSemaphore*>, SEMAPHORES> semaphores;
};

#endif // __PROCESS_SEMAPHORE_HPP__
