#include "aiws/build_worker.hpp"

#include <QCoreApplication>

#include <chrono>
#include <stdexcept>
#include <utility>

namespace aiws {

BuildResultEvent::BuildResultEvent(std::uint64_t version,
                                   std::unique_ptr<ProcessingCore> core,
                                   std::exception_ptr error)
    : QEvent(eventType()),
      version(version),
      core(std::move(core)),
      error(std::move(error)) {}

QEvent::Type BuildResultEvent::eventType() {
    static const int value = QEvent::registerEventType();
    return static_cast<QEvent::Type>(value);
}

BuildWorker::BuildWorker() = default;

BuildWorker::~BuildWorker() { stop(); }

void BuildWorker::setReceiver(QObject* receiver) noexcept { receiver_ = receiver; }

void BuildWorker::ensureStarted() {
    if (!thread_.joinable()) {
        thread_ = std::thread([this] { run(); });
    }
}

void BuildWorker::submit(BuildRequest request) {
    ensureStarted();
    std::lock_guard lock(mutex_);
    pending_ = std::move(request);
    cancelRequested_ = true;
    condition_.notify_all();
}

void BuildWorker::cancel() noexcept {
    std::lock_guard lock(mutex_);
    pending_.reset();
    cancelRequested_ = true;
    condition_.notify_all();
}

void BuildWorker::stop() {
    {
        std::lock_guard lock(mutex_);
        stopRequested_ = true;
        cancelRequested_ = true;
        condition_.notify_all();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool BuildWorker::wakeForWork() const {
#ifdef AIWS_FAULT_SILENT_SHUTDOWN
    // fault: exit while the worker is idle never wakes it, so join hangs
    return pending_.has_value();
#else
    return stopRequested_ || pending_.has_value();
#endif
}

bool BuildWorker::stopThisBuild() const {
#ifdef AIWS_FAULT_NO_CANCEL
    // fault: a build nobody wants anymore still runs every document
    return stopRequested_;
#else
    return stopRequested_ || cancelRequested_;
#endif
}

bool BuildWorker::waitForDocument(int delayMs) {
    std::unique_lock lock(mutex_);
    if (delayMs > 0) {
        condition_.wait_for(lock, std::chrono::milliseconds(delayMs), [this] {
            return stopThisBuild();
        });
    }
    return !stopThisBuild();
}

BuildWorker::Result BuildWorker::runBuild(const BuildRequest& request) {
    Result result;
    const std::size_t documentCount = request.snapshot.documents().size();
    for (std::size_t index = 0; index < documentCount; ++index) {
        if (!waitForDocument(request.delayMs)) {
            std::lock_guard lock(mutex_);
            result.hitStop = stopRequested_;
            return result;
        }
    }

    {
        std::lock_guard lock(mutex_);
        if (stopThisBuild()) {
            result.hitStop = stopRequested_;
            return result;
        }
    }

    try {
        auto core = request.factory != nullptr ? request.factory()
                                               : std::make_unique<ProcessingCore>();
        if (core == nullptr) {
            throw std::runtime_error("no processing core to build");
        }
        core->rebuild(request.snapshot);
        result.core = std::move(core);
    } catch (...) {
        result.error = std::current_exception();
    }

    std::lock_guard lock(mutex_);
    if (stopThisBuild()) {
        result.core.reset();
        result.error = nullptr;
        result.hitStop = stopRequested_;
    }
    return result;
}

void BuildWorker::run() {
    for (;;) {
        BuildRequest request;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this] { return wakeForWork(); });
#ifndef AIWS_FAULT_SILENT_SHUTDOWN
            if (stopRequested_) {
                return;
            }
#endif
            if (!pending_.has_value()) {
                continue;
            }
            request = std::move(*pending_);
            pending_.reset();
            cancelRequested_ = false;
        }

        Result result = runBuild(request);

        bool deliver = result.core != nullptr || static_cast<bool>(result.error);
        bool leave = false;
        {
            std::lock_guard lock(mutex_);
            if (stopRequested_) {
                deliver = false;
#ifdef AIWS_FAULT_SILENT_SHUTDOWN
                leave = result.hitStop;
#else
                leave = true;
#endif
            }
        }

        if (deliver && receiver_ != nullptr) {
            QCoreApplication::postEvent(
                receiver_,
                new BuildResultEvent(request.version, std::move(result.core),
                                     std::move(result.error)));
        }
        if (leave) {
            return;
        }
    }
}

}  // namespace aiws
