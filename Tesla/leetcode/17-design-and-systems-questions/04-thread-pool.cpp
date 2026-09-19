// COMPONENT   A thread pool: fixed workers, a task queue, futures, and clean shutdown.
// WHERE       Running independent pipeline stages or data-parallel chunks without paying
//             ~10-30 us of thread creation per task.
// WHY         std::async is not a pool (it may create a thread per call, and its future's
//             destructor BLOCKS -- course section 11). A pool amortizes thread creation
//             and bounds concurrency.
//
// DESIGN DECISIONS an interviewer will probe:
//   - Workers are std::jthread, so the destructor joins on EVERY exit path (C++20).
//   - The task is invoked OUTSIDE the lock. Holding a mutex across user code is how you
//     get deadlocks and latency spikes (course section 11).
//   - cv_.wait uses the PREDICATE form -- spurious and lost wakeups are real.
//   - Shutdown uses notify_ALL and drains the queue, so submitted work is not silently
//     dropped. If you prefer to discard, that is a policy decision you must state.
//   - submit() returns a std::future so exceptions propagate to the caller instead of
//     terminating a worker.
//   - The task is wrapped in a shared_ptr<packaged_task> because std::function requires a
//     COPYABLE target and packaged_task is move-only. That specific detail comes up every
//     time someone writes this from scratch.
//
// COMPLEXITY  submit is O(1) plus one allocation; the wake is one condition-variable
//             signal.
//
// FOLLOW-UPS  What would you change for a REAL-TIME path? -> everything that allocates or
//             blocks: no std::function (a fixed-size task slot or function_ref), no
//             std::queue + mutex (a preallocated lock-free MPMC ring), pin each worker to
//             a core with SCHED_FIFO, and make submit() fail rather than grow when full.
//             Often the right answer is not a pool at all but one thread per pipeline
//             stage, because that makes latency analyzable.
//             Work stealing? -> per-worker deques reduce contention; only add it after
//             measuring, since it complicates ordering and debugging.
//             How do you size it? -> hardware_concurrency for CPU-bound work; for mixed
//             work use Little's law (course section 14) and measure queue depth.

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class ThreadPool {
    // NOTE the declaration order: workers_ first only because the destructor joins them
    // explicitly before anything else is destroyed. See ~ThreadPool.
    std::vector<std::jthread>         workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex                mutex_;
    std::condition_variable           cv_;
    bool                              stopping_{false};
    std::atomic<std::size_t>          completed_{0};

public:
    explicit ThreadPool(unsigned threads = std::thread::hardware_concurrency()) {
        if (threads == 0) threads = 1;                  // hardware_concurrency may return 0
        workers_.reserve(threads);
        for (unsigned i = 0; i < threads; ++i) workers_.emplace_back([this] { run(); });
    }

    ~ThreadPool() {
        { std::lock_guard lock{mutex_}; stopping_ = true; }
        cv_.notify_all();                                // ALL: every worker must wake
        // JOIN EXPLICITLY HERE. Relying on ~jthread is a real bug: members are destroyed
        // in REVERSE declaration order, so mutex_, cv_ and tasks_ (declared after
        // workers_) would be destroyed while the workers are still using them. Either
        // join here, or declare workers_ LAST. This is the exact mistake that makes a
        // hand-rolled pool crash on shutdown.
        for (auto& w : workers_) if (w.joinable()) w.join();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    auto submit(F&& fn, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using Result = std::invoke_result_t<F, Args...>;
        // shared_ptr because std::function needs a COPYABLE target and packaged_task
        // is move-only.
        auto task = std::make_shared<std::packaged_task<Result()>>(
            [fn = std::forward<F>(fn),
             tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(fn, tuple);
            });
        std::future<Result> future = task->get_future();
        {
            std::lock_guard lock{mutex_};
            if (stopping_) throw std::runtime_error("submit on a stopping pool");
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_one();
        return future;
    }

    std::size_t pending() const {
        std::lock_guard lock{mutex_};
        return tasks_.size();
    }
    std::size_t completed() const noexcept { return completed_.load(std::memory_order_relaxed); }
    std::size_t worker_count() const noexcept { return workers_.size(); }

private:
    void run() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock lock{mutex_};
                cv_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });  // PREDICATE
                if (tasks_.empty()) return;              // stopping and drained
                job = std::move(tasks_.front());
                tasks_.pop();
            }
            job();                                       // OUTSIDE the lock
            completed_.fetch_add(1, std::memory_order_relaxed);
        }
    }
};

int main() {
    {
        ThreadPool pool{4};
        assert(pool.worker_count() == 4);

        // Results come back through futures, in the caller's order.
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 100; ++i)
            futures.push_back(pool.submit([](int x) { return x * x; }, i));
        long long total = 0;
        for (auto& f : futures) total += f.get();
        long long expected = 0;
        for (int i = 0; i < 100; ++i) expected += static_cast<long long>(i) * i;
        assert(total == expected);

        // An exception in a task propagates to the caller, it does not kill a worker.
        auto bad = pool.submit([]() -> int { throw std::runtime_error("boom"); });
        bool caught = false;
        try { (void)bad.get(); } catch (const std::runtime_error&) { caught = true; }
        assert(caught);

        // The pool survives, and void tasks work too.
        std::atomic<int> counter{0};
        std::vector<std::future<void>> voids;
        for (int i = 0; i < 50; ++i)
            voids.push_back(pool.submit([&counter] { counter.fetch_add(1); }));
        for (auto& f : voids) f.get();
        assert(counter.load() == 50);

        // Tasks really do run concurrently: a barrier that only clears with >= 2 workers.
        std::atomic<int> arrived{0};
        auto a = pool.submit([&] { arrived.fetch_add(1); while (arrived.load() < 2) {} return 1; });
        auto b = pool.submit([&] { arrived.fetch_add(1); while (arrived.load() < 2) {} return 2; });
        assert(a.get() + b.get() == 3);
    }   // destructor: stop, notify_all, drain, join

    // Submitted work is DRAINED on shutdown, not dropped.
    {
        std::atomic<int> done{0};
        {
            ThreadPool pool{2};
            for (int i = 0; i < 200; ++i)
                (void)pool.submit([&done] { done.fetch_add(1); });
        }                                                // destructor drains everything
        assert(done.load() == 200);
    }

    // A single-worker pool, and the hardware_concurrency == 0 guard.
    {
        ThreadPool one{1};
        assert(one.worker_count() == 1);
        assert(one.submit([] { return 7; }).get() == 7);
        ThreadPool zero{0};
        assert(zero.worker_count() == 1);                // clamped, not zero workers
    }
    return 0;
}
