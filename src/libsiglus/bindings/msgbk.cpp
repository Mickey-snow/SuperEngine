// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
// -----------------------------------------------------------------------

#include "libsiglus/bindings/registry.hpp"

#include "libsiglus/bindings/util.hpp"
#include "srbind/srbind.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <memory>
#include <string>
#include <vector>

namespace libsiglus::binding {

namespace sb = srbind;
namespace sr = serilang;

namespace {

struct MsgbkEntry {
  std::string msg;
  std::string original_name;
  std::string display_name;
  std::string debug_msg;
  std::vector<int> koe_no_list;
  std::vector<int> chara_no_list;
  int scene_no = -1;
  int line_no = -1;
  bool pct_flag = false;
};

class MsgbkState {
 public:
  void InsertMsg(std::vector<sr::Value> raw_args) {
    auto packet = CallPacket::DecodeFrom(std::move(raw_args));
    GoNextMsg();
    AddMsg(packet);
  }

  void AddMsg(std::vector<sr::Value> raw_args) {
    AddMsg(CallPacket::DecodeFrom(std::move(raw_args)));
  }

  void AddNamae(std::vector<sr::Value> raw_args) {
    auto packet = CallPacket::DecodeFrom(std::move(raw_args));
    if (packet.args.empty())
      return;

    const std::string name = AsString(packet.args[0]);
    if (name.empty())
      return;

    ReadyMsg();
    entries_[insert_pos_].original_name = name;
    entries_[insert_pos_].display_name = name;
    last_pos_ = insert_pos_;
  }

  void AddKoe(std::vector<sr::Value> raw_args) {
    auto packet = CallPacket::DecodeFrom(std::move(raw_args));
    if (packet.args.empty())
      return;

    const int koe_no = AsInt(packet.args[0]).value_or(0);
    if (koe_no < 0)
      return;

    int chara_no = -1;
    if (packet.args.size() > 1)
      chara_no = AsInt(packet.args[1]).value_or(-1);

    ReadyMsg();
    entries_[insert_pos_].koe_no_list.push_back(koe_no);
    entries_[insert_pos_].chara_no_list.push_back(chara_no);
    last_pos_ = insert_pos_;
  }

  void GoNextMsg() {
    if (new_msg_)
      return;

    const MsgbkEntry& entry = entries_[insert_pos_];
    if (!entry.pct_flag && entry.msg.empty())
      return;

    insert_pos_ = (insert_pos_ + 1) % entries_.size();
    new_msg_ = true;
  }

 private:
  void AddMsg(const CallPacket& packet) {
    if (packet.args.empty())
      return;

    const std::string msg = AsString(packet.args[0]);
    if (msg.empty())
      return;

    ReadyMsg();
    MsgbkEntry& entry = entries_[insert_pos_];
    entry.msg += msg;
    entry.debug_msg = msg;
    last_pos_ = insert_pos_;
  }

  void ReadyMsg() {
    if (!new_msg_)
      return;

    new_msg_ = false;
    if (history_count_ < entries_.size())
      ++history_count_;
    else
      start_pos_ = (start_pos_ + 1) % entries_.size();

    entries_[insert_pos_] = MsgbkEntry{};
  }

  static constexpr std::size_t kMaxHistory = 256;
  std::vector<MsgbkEntry> entries_ = std::vector<MsgbkEntry>(kMaxHistory);
  std::size_t history_count_ = 0;
  std::size_t start_pos_ = 0;
  std::size_t last_pos_ = 0;
  std::size_t insert_pos_ = 0;
  bool new_msg_ = true;
};

}  // namespace

void BindMsgbk(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;
  sb::module_ m(vm, "msgbk");
  auto state = std::make_shared<MsgbkState>();

  m.def(
      "insert_msg",
      [state](std::vector<sr::Value> args) {
        state->InsertMsg(std::move(args));
      },
      sb::vararg);
  m.def(
      "add_koe",
      [state](std::vector<sr::Value> args) { state->AddKoe(std::move(args)); },
      sb::vararg);
  m.def(
      "add_namae",
      [state](std::vector<sr::Value> args) {
        state->AddNamae(std::move(args));
      },
      sb::vararg);
  m.def(
      "add_msg",
      [state](std::vector<sr::Value> args) { state->AddMsg(std::move(args)); },
      sb::vararg);
  m.def("go_next_msg", [state] { state->GoNextMsg(); });
}

RLVM_REGISTER(SiglusBindingRegistry, "msgbk", BindMsgbk)

}  // namespace libsiglus::binding
