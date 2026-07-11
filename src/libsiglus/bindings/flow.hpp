// -----------------------------------------------------------------------
// Internal helpers shared by Siglus flow-control bindings.
// -----------------------------------------------------------------------

#pragma once

#include <string>

namespace serilang {
class Code;
class VM;
}  // namespace serilang

namespace libsiglus::binding {
class Loader;

serilang::Code* MakeSceneEntryThunk(serilang::VM& vm,
                                    Loader& loader,
                                    std::string scene_name,
                                    const std::string& entry_name);

}  // namespace libsiglus::binding
