#ifndef SRC_MEMORY_SERVICES_HPP_
#define SRC_MEMORY_SERVICES_HPP_

#include <string>
#include <vector>

class RLMachine;

// methods of this service locator will be called when read/write intL or strK
// memory is needed
class IMemoryServices {
 public:
  virtual ~IMemoryServices() = default;

  virtual int* IntLBank() = 0;
  virtual std::vector<std::string>& StrKBank() = 0;
};

// for dependency injection
class MemoryServices : public IMemoryServices {
 public:
  MemoryServices(RLMachine&);

  virtual int* IntLBank() override;

  virtual std::vector<std::string>& StrKBank() override;

 private:
  RLMachine& machine_;
};

#endif  // SRC_MEMORY_SERVICES_HPP_
