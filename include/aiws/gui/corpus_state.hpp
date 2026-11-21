#pragma once

#include <QString>

#include <cstdint>
#include <memory>

namespace aiws {
class ProcessingCore;
}

namespace aiws::gui {

class MainWindow;

// what a corpus event does is decided by whichever state is current
class CorpusState {
public:
    virtual ~CorpusState() = default;

    virtual std::unique_ptr<CorpusState> onBuildRequested(MainWindow& window) = 0;
    virtual std::unique_ptr<CorpusState> onWorkspaceChanged(MainWindow& window) = 0;
    virtual std::unique_ptr<CorpusState> onReset(MainWindow& window) = 0;

    virtual std::unique_ptr<CorpusState> onBuildFinished(
        MainWindow& window, std::unique_ptr<ProcessingCore> core, std::uint64_t version);
    virtual std::unique_ptr<CorpusState> onBuildFailed(MainWindow& window,
                                                       const QString& message,
                                                       std::uint64_t version);

    virtual QString corpusStatus(const MainWindow& window) const = 0;
    virtual bool allowsQueries() const;
    virtual bool allowsBuild() const;
};

class NotBuiltState final : public CorpusState {
public:
    std::unique_ptr<CorpusState> onBuildRequested(MainWindow& window) override;
    std::unique_ptr<CorpusState> onWorkspaceChanged(MainWindow& window) override;
    std::unique_ptr<CorpusState> onReset(MainWindow& window) override;
    QString corpusStatus(const MainWindow& window) const override;
};

class BuildingState final : public CorpusState {
public:
    std::unique_ptr<CorpusState> onBuildRequested(MainWindow& window) override;
    std::unique_ptr<CorpusState> onWorkspaceChanged(MainWindow& window) override;
    std::unique_ptr<CorpusState> onReset(MainWindow& window) override;
    std::unique_ptr<CorpusState> onBuildFinished(MainWindow& window,
                                                 std::unique_ptr<ProcessingCore> core,
                                                 std::uint64_t version) override;
    std::unique_ptr<CorpusState> onBuildFailed(MainWindow& window, const QString& message,
                                               std::uint64_t version) override;
    QString corpusStatus(const MainWindow& window) const override;
    bool allowsBuild() const override;
};

class CurrentState final : public CorpusState {
public:
    std::unique_ptr<CorpusState> onBuildRequested(MainWindow& window) override;
    std::unique_ptr<CorpusState> onWorkspaceChanged(MainWindow& window) override;
    std::unique_ptr<CorpusState> onReset(MainWindow& window) override;
    QString corpusStatus(const MainWindow& window) const override;
    bool allowsQueries() const override;
};

class OutOfDateState final : public CorpusState {
public:
    std::unique_ptr<CorpusState> onBuildRequested(MainWindow& window) override;
    std::unique_ptr<CorpusState> onWorkspaceChanged(MainWindow& window) override;
    std::unique_ptr<CorpusState> onReset(MainWindow& window) override;
    QString corpusStatus(const MainWindow& window) const override;
};

class FailedState final : public CorpusState {
public:
    std::unique_ptr<CorpusState> onBuildRequested(MainWindow& window) override;
    std::unique_ptr<CorpusState> onWorkspaceChanged(MainWindow& window) override;
    std::unique_ptr<CorpusState> onReset(MainWindow& window) override;
    QString corpusStatus(const MainWindow& window) const override;
};

}  // namespace aiws::gui
