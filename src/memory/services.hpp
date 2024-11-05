#ifndef SRC_MEMORY_SERVICES_HPP_
#define SRC_MEMORY_SERVICES_HPP_

#include <string>
#include <vector>

class RLMachine;

class IMemoryServices {
 public:
  virtual ~IMemoryServices() = default;

  virtual int* IntLBank() = 0;
  virtual std::vector<std::string>& StrKBank() = 0;
};

class MemoryServices : public IMemoryServices {
 public:
  MemoryServices(RLMachine&);

  virtual int* IntLBank() override;
  
  virtual std::vector<std::string>& StrKBank() override;

 private:
  RLMachine& machine_;
};

#endif  // SRC_MEMORY_SERVICES_HPP_
