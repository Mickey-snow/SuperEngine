#include "utilities/shutdown_signal.hpp"

#include <csignal>

namespace {

volatile std::sig_atomic_t g_shutdown_requested = 0;

void HandleShutdownSignal(int) { g_shutdown_requested = 1; }

}  // namespace

void InstallShutdownSignalHandlers() {
  std::signal(SIGINT, HandleShutdownSignal);
  std::signal(SIGTERM, HandleShutdownSignal);
}

bool ConsumeShutdownSignal() {
  if (!g_shutdown_requested)
    return false;

  g_shutdown_requested = 0;
  return true;
}

void RequestShutdownForTesting() { g_shutdown_requested = 1; }

void ClearShutdownSignalForTesting() { g_shutdown_requested = 0; }
