#include "base/memory_services.hpp"

#include "machine/rlmachine.h"

MemoryServices::MemoryServices(RLMachine& machine) : machine_(machine) {}

int* MemoryServices::IntLBank() { return machine_.CurrentIntLBank(); }

std::vector<std::string>& MemoryServices::StrKBank() {
  return machine_.CurrentStrKBank();
}
