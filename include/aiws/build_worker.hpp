#pragma once

#include "aiws/processing_core.hpp"
#include "aiws/workspace.hpp"

#include <QEvent>
#include <QObject>

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace aiws {

// result of one build, posted back onto the gui thread
class BuildResultEvent : public QEvent {
public:
    BuildResultEvent(std::uint64_t version,
                     std::unique_ptr<ProcessingCore> core,
                     std::exception_ptr error);

    static QEvent::Type eventType();

    std::uint64_t version;
    std::unique_ptr<ProcessingCore> core;
    std::exception_ptr error;
};

// the one build thread, from the first build until exit.
// every public method runs on the gui thread.
class BuildWorker {
public:
    using CoreFactory = std::unique_ptr<ProcessingCore> (*)();

    struct BuildRequest {
        std::uint64_t version = 0;
        Workspace snapshot;
        int delayMs = 0;
        CoreFactory factory = nullptr;
    };

    BuildWorker();
    ~BuildWorker();

    BuildWorker(const BuildWorker&) = delete;
    BuildWorker& operator=(const BuildWorker&) = delete;

    // call this before the first submit
    void setReceiver(QObject* receiver) noexcept;

    // replaces any request the worker has not started, and cancels the one it has
    void submit(BuildRequest request);

    // drops a request that has not started and stops the one that has
    void cancel() noexcept;

    // wakes the worker and joins it
    void stop();

private:
    struct Result {
        std::unique_ptr<ProcessingCore> core;
        std::exception_ptr error;
        bool hitStop = false;
    };

    void ensureStarted();
    void run();
    Result runBuild(const BuildRequest& request);
    bool waitForDocument(int delayMs);

    // caller holds mutex_
    bool wakeForWork() const;
    bool stopThisBuild() const;

    QObject* receiver_{nullptr};
    std::mutex mutex_;
    std::condition_variable condition_;
    std::optional<BuildRequest> pending_;
    bool cancelRequested_{false};
    bool stopRequested_{false};
    std::thread thread_;
};

}  // namespace aiws
