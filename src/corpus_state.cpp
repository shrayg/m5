#include "aiws/gui/corpus_state.hpp"

#include "aiws/gui/main_window.hpp"
#include "aiws/processing_core.hpp"

namespace aiws::gui {

std::unique_ptr<CorpusState> CorpusState::onBuildFinished(MainWindow&,
                                                         std::unique_ptr<ProcessingCore>,
                                                         std::uint64_t) {
    return nullptr;
}

std::unique_ptr<CorpusState> CorpusState::onBuildFailed(MainWindow&, const QString&,
                                                        std::uint64_t) {
    return nullptr;
}

bool CorpusState::allowsQueries() const { return false; }

bool CorpusState::allowsBuild() const { return true; }

std::unique_ptr<CorpusState> NotBuiltState::onBuildRequested(MainWindow& window) {
    return window.beginBuild();
}

std::unique_ptr<CorpusState> NotBuiltState::onWorkspaceChanged(MainWindow& window) {
    window.discardCurrentWork();
    return nullptr;
}

std::unique_ptr<CorpusState> NotBuiltState::onReset(MainWindow& window) {
    window.clearForReset();
    window.setStatusText("Application reset.");
    return std::make_unique<NotBuiltState>();
}

QString NotBuiltState::corpusStatus(const MainWindow&) const { return "Not built"; }

std::unique_ptr<CorpusState> BuildingState::onBuildRequested(MainWindow&) {
    return nullptr;
}

std::unique_ptr<CorpusState> BuildingState::onWorkspaceChanged(MainWindow& window) {
    window.discardCurrentWork();
    return std::make_unique<OutOfDateState>();
}

std::unique_ptr<CorpusState> BuildingState::onReset(MainWindow& window) {
    window.clearForReset();
    window.setStatusText("Application reset.");
    return std::make_unique<NotBuiltState>();
}

std::unique_ptr<CorpusState> BuildingState::onBuildFinished(
    MainWindow& window, std::unique_ptr<ProcessingCore> core, std::uint64_t version) {
    if (!window.versionMatches(version) || core == nullptr) {
        return nullptr;
    }
    window.installCore(std::move(core));
    window.setStatusText("Corpus built.");
    return std::make_unique<CurrentState>();
}

std::unique_ptr<CorpusState> BuildingState::onBuildFailed(MainWindow& window,
                                                          const QString& message,
                                                          std::uint64_t version) {
    if (!window.versionMatches(version)) {
        return nullptr;
    }
    window.setStatusText(QString("Build failed: ") + message);
    return std::make_unique<FailedState>();
}

QString BuildingState::corpusStatus(const MainWindow&) const { return "Building..."; }

bool BuildingState::allowsBuild() const { return false; }

std::unique_ptr<CorpusState> CurrentState::onBuildRequested(MainWindow& window) {
    return window.beginBuild();
}

std::unique_ptr<CorpusState> CurrentState::onWorkspaceChanged(MainWindow& window) {
    window.discardCurrentWork();
    return std::make_unique<OutOfDateState>();
}

std::unique_ptr<CorpusState> CurrentState::onReset(MainWindow& window) {
    window.clearForReset();
    window.setStatusText("Application reset.");
    return std::make_unique<NotBuiltState>();
}

QString CurrentState::corpusStatus(const MainWindow& window) const {
    return window.chunkStatusText();
}

bool CurrentState::allowsQueries() const { return true; }

std::unique_ptr<CorpusState> OutOfDateState::onBuildRequested(MainWindow& window) {
    return window.beginBuild();
}

std::unique_ptr<CorpusState> OutOfDateState::onWorkspaceChanged(MainWindow& window) {
    window.discardCurrentWork();
    return nullptr;
}

std::unique_ptr<CorpusState> OutOfDateState::onReset(MainWindow& window) {
    window.clearForReset();
    window.setStatusText("Application reset.");
    return std::make_unique<NotBuiltState>();
}

QString OutOfDateState::corpusStatus(const MainWindow&) const { return "Out of date"; }

std::unique_ptr<CorpusState> FailedState::onBuildRequested(MainWindow& window) {
    return window.beginBuild();
}

std::unique_ptr<CorpusState> FailedState::onWorkspaceChanged(MainWindow& window) {
    window.discardCurrentWork();
    return std::make_unique<OutOfDateState>();
}

std::unique_ptr<CorpusState> FailedState::onReset(MainWindow& window) {
    window.clearForReset();
    window.setStatusText("Application reset.");
    return std::make_unique<NotBuiltState>();
}

QString FailedState::corpusStatus(const MainWindow&) const { return "Build failed"; }

}  // namespace aiws::gui
