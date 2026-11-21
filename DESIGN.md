# M5 Design

## 1. M4 fixes

none

## 2. Threads and shared data

The GUI thread runs MainWindow, the widgets, and the corpus states. BuildWorker::run is the one build thread. submit starts it on the first build, and ~MainWindow calls stop, which joins it. RemoteAddServer is still its own thread.

Shared under mutex_: pending_ (the latest BuildRequest, or empty), cancelRequested_, and stopRequested_. The GUI writes them in submit, cancel, and stop. The worker reads them in run and waitForDocument, and waits on condition_. That set cannot be smaller. The request has to cross threads, and cancel and stop have to be visible while the worker is blocked. The Workspace in a request is a copy. A finished build comes back as a BuildResultEvent on Qt's queue, which Qt locks. States and widgets stay on the GUI thread. RemoteAddServer::stopping_ is still an atomic, and quitSignalled is still a sig_atomic_t.

## 3. States

applyTransition installs whatever the handler returns. Exit from any state is ~MainWindow calling BuildWorker::stop.

NotBuilt means nothing is current yet. Build calls beginBuild and goes to Building. A workspace change stays here. Reset stays NotBuilt.

Building means this version is in the worker. Another build click is ignored. A workspace change calls cancel and goes to OutOfDate. Reset goes to NotBuilt. A finish goes to Current if versionMatches. A failure goes to Failed if versionMatches. An older result is ignored.

Current means the core matches the workspace. Build calls beginBuild and goes to Building. A workspace change goes to OutOfDate. Reset goes to NotBuilt.

OutOfDate means the workspace moved on. Build calls beginBuild and goes to Building. Another workspace change stays here and calls cancel. Reset goes to NotBuilt.

Failed means the last build threw. Build calls beginBuild and goes to Building. A workspace change goes to OutOfDate. Reset goes to NotBuilt.

NotBuilt, Current, OutOfDate, and Failed ignore a finished or failed build.

## 4. Trace

Reset during a build. GUI: Reset, reset_application, BuildingState::onReset, clearForReset (version++, BuildWorker::cancel), applyTransition to NotBuilt. cancel clears pending_, sets cancelRequested_, and notifies condition_. Worker: blocked in waitForDocument, wakes, stopThisBuild is true, runBuild returns, run waits. GUI: add a file, Build, NotBuiltState::onBuildRequested, beginBuild, handOffBuild, submit. The worker takes that request. The old one is never built.

## 5. Decisions

1. Build during a build. Build stays off in BuildingState, so a second click for this workspace does nothing. After an add or reset, OutOfDateState or NotBuiltState turns it back on immediately. onBuildRequested calls beginBuild. submit replaces a request not yet taken and cancels the one in progress, so the next build does not wait out the old one.

2. Remote add mid-build. Same path as Add Document. remote_add_requested calls add_document_file, then BuildingState::onWorkspaceChanged. The build is cancelled, the new file shows up, status becomes "Out of date", and that core never becomes current.

3. Shared data is the set in section 2. The snapshot and the result event are not shared after the handoff.

## 6. Faults

AIWS_FAULT_NO_CANCEL is in BuildWorker::stopThisBuild, which then ignores cancelRequested_. The worker still finishes every document delay, so the next request waits. nextBuildDoesNotWaitOnCancelled catches it.

AIWS_FAULT_SILENT_SHUTDOWN is in BuildWorker::wakeForWork, which then ignores stopRequested_ while run is idle. stop joins forever. exitWhileIdleDoesNotHang catches it. Exit during a build still returns, because runBuild sets hitStop.

## 7. AI use
I used AI to double check my work. 