/*------------------------------------------------------------------------
  Junction: Concurrent data structures in C++
  Copyright (c) 2016 Jeff Preshing

  Distributed under the Simplified BSD License.
  Original location: https://github.com/preshing/junction

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the LICENSE file for more information.
------------------------------------------------------------------------*/

#ifndef JUNCTION_QSBR_H
#define JUNCTION_QSBR_H

#include <junction/Core.h>
#include <turf/Mutex.h>
#include <turf/RaceDetector.h>
#include <vector>
#include <string.h>

namespace junction {

class JUNCTION_API QSBR {
private:
    struct Action {
        void (*func)(void*);
        uptr param[4]; // Size limit found experimentally. Verified by assert below.

        Action(void (*f)(void*), void* p, ureg paramSize) : func(f) {
            TURF_ASSERT(paramSize <= sizeof(param)); // Verify size limit.
            memcpy(&param, p, paramSize);
        }
        void operator()() {
            func(&param);
        }
    };

    struct Status {
        u16 inUse : 1;
        u16 wasIdle : 1;
        s16 nextFree : 14;

        Status() : inUse(1), wasIdle(0), nextFree(0) {
        }
    };

    turf::Mutex m_mutex;
    TURF_DEFINE_RACE_DETECTOR(m_flushRaceDetector)
    std::vector<Status> m_status;
    sreg m_freeIndex;
    sreg m_numContexts;
    sreg m_remaining;
    std::vector<Action> m_deferredActions;
    std::vector<Action> m_pendingActions;

    void onAllQuiescentStatesPassed(std::vector<Action>& callbacks);

public:
    typedef u16 Context;

    QSBR();
    ~QSBR();
    Context createContext();
    void destroyContext(Context context);

    //! Convenience for calling a member function to reclaim.
    template <class T>
    void enqueue_mem_fun(void (T::*pmf)(), T* target) {
        struct Closure {
            void (T::*pmf)();
            T* target;
            static void thunk(void* param) {
                Closure* self = (Closure*) param;
                TURF_CALL_MEMBER (*self->target, self->pmf)();
            }
        };
        Closure closure = {pmf, target};
        turf::LockGuard<turf::Mutex> guard(m_mutex);
        TURF_RACE_DETECT_GUARD(m_flushRaceDetector);
        m_deferredActions.push_back(Action(Closure::thunk, &closure, sizeof(closure)));
    }

    //! Add a nullary callable which encapsulates the reclamation. This is probably the best default going forward.
    template <typename Fn>
    void enqueue_callable(Fn&& f) {
        struct Closure {
            Fn f;
            static void thunk(void* param) {
                Closure* self = (Closure*) param;
                self->f();
            }
        };
        Closure closure = {std::forward<Fn>(f)};
        turf::LockGuard<turf::Mutex> guard(m_mutex);
        TURF_RACE_DETECT_GUARD(m_flushRaceDetector);
        m_deferredActions.push_back(Action(Closure::thunk, &closure, sizeof(closure)));
    }

    //! Add a unary callable with the target to be reclaimed. (TODO: bind? lambda?)
    template <typename Fn, typename T>
    void enqueue_callable(Fn&& f, T* target) {
        struct Closure {
            Fn f;
            T* target;
            static void thunk(void* param) {
                Closure* self = (Closure*) param;
                self->f(self->target);
            }
        };
        Closure closure = {std::forward<Fn>(f), target};
        turf::LockGuard<turf::Mutex> guard(m_mutex);
        TURF_RACE_DETECT_GUARD(m_flushRaceDetector);
        m_deferredActions.push_back(Action(Closure::thunk, &closure, sizeof(closure)));
    }

    //! Add a default constructible type which is the reclaimer and the target to be reclaimed (saves memory?)
    template <typename Fn, typename T>
    void enqueue_defaultable_callable(T* target) {
        struct Closure {
            T* target;
            static void thunk(void* param) {
                Closure* self = (Closure*) param;
                Fn()(self->target);
            }
        };
        Closure closure = {target};
        turf::LockGuard<turf::Mutex> guard(m_mutex);
        TURF_RACE_DETECT_GUARD(m_flushRaceDetector);
        m_deferredActions.push_back(Action(Closure::thunk, &closure, sizeof(closure)));
    }
    void update(Context context);
    void flush();
};

JUNCTION_API QSBR& DefaultQSBR();

} // junction

#endif // JUNCTION_QSBR_H
