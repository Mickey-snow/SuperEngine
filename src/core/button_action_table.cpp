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
//
// -----------------------------------------------------------------------

#include "core/button_action_table.hpp"

#include "core/gameexe.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

std::vector<ButtonActionTable::Entry> MakeDefaultEntries(
    std::size_t action_count,
    bool use_siglus_defaults) {
  std::vector<ButtonActionTable::Entry> result(action_count);

  if (use_siglus_defaults) {
    for (auto& entry : result) {
      entry.hit.rep_bright = 32;
      entry.push.rep_pos = Point(1, 1);
      entry.push.rep_bright = 32;
    }
  }

  return result;
}

std::optional<std::size_t> ParseIndex(std::string_view text) {
  std::size_t result = 0;
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), result);
  if (error != std::errc() || end != text.data() + text.size())
    return std::nullopt;
  return result;
}

enum class StateType { Normal, Hit, Push, Select, Disable };

std::optional<StateType> ParseState(std::string_view state) {
  if (state == "NORMAL")
    return StateType::Normal;
  if (state == "HIT")
    return StateType::Hit;
  if (state == "PUSH")
    return StateType::Push;
  if (state == "SELECT")
    return StateType::Select;
  if (state == "DISABLE")
    return StateType::Disable;
  return std::nullopt;
}

struct ParsedKey {
  std::size_t action;
  StateType state;
};

std::optional<ParsedKey> ParseKey(const GameexeInterpretObject& record,
                                  std::string_view root) {
  const auto parts = record.GetKeyParts();
  if (parts.size() != 4 || parts[0] != root || parts[1] != "ACTION")
    return std::nullopt;

  const auto action = ParseIndex(parts[2]);
  const auto state = ParseState(parts[3]);
  if (!action || !state)
    return std::nullopt;

  return ParsedKey{*action, *state};
}

ButtonActionTable::State& GetState(ButtonActionTable::Entry& entry,
                                   StateType state) {
  switch (state) {
    case StateType::Normal:
      return entry.normal;
    case StateType::Hit:
      return entry.hit;
    case StateType::Push:
      return entry.push;
    case StateType::Select:
      return entry.select;
    case StateType::Disable:
      return entry.disable;
  }

  throw std::logic_error("Unknown button state");
}

}  // namespace

ButtonActionTable ButtonActionTable::ParseSiglus(Gameexe& gexe) {
  constexpr int kDefaultActionCount = 16;

  const int action_count =
      gexe("BUTTON.ACTION.CNT").Int().value_or(kDefaultActionCount);
  if (action_count < 0)
    throw std::runtime_error("Siglus BUTTON.ACTION.CNT must not be negative");

  ButtonActionTable result;
  result.entry_ = MakeDefaultEntries(action_count, true);
  result.cnt_ = result.entry_.size();

  for (auto record : gexe.Filter("BUTTON.ACTION.")) {
    const auto key = ParseKey(record, "BUTTON");
    if (!key || key->action >= static_cast<std::size_t>(action_count))
      continue;

    const auto values = record.IntVec();
    if (!values || values->size() != 6)
      continue;

    GetState(result.entry_[key->action], key->state) = State{
        .pattern = (*values)[0],
        .rep_pos = Point((*values)[1], (*values)[2]),
        .rep_tr = std::clamp((*values)[3], 0, 255),
        .rep_bright = std::clamp((*values)[4], 0, 255),
        .rep_dark = std::clamp((*values)[5], 0, 255),
    };
  }

  return result;
}

ButtonActionTable ButtonActionTable::ParseReallive(Gameexe& gexe) {
  struct ParsedRecord {
    ParsedKey key;
    State state;
  };

  std::vector<ParsedRecord> records;
  std::size_t action_count = 0;
  for (auto record : gexe.Filter("BTNOBJ.ACTION.")) {
    const auto key = ParseKey(record, "BTNOBJ");
    const auto values = record.IntVec();
    if (!key || !values || values->size() < 4)
      continue;

    records.emplace_back(*key, State{
                                   .pattern = (*values)[0],
                                   .rep_pos = Point((*values)[2], (*values)[3]),
                                   .rep_tr = 255,
                                   .rep_bright = 0,
                                   .rep_dark = 0,
                               });
    action_count = std::max(action_count, key->action + 1);
  }

  ButtonActionTable result;
  result.entry_ = MakeDefaultEntries(action_count, false);
  result.cnt_ = result.entry_.size();
  for (const auto& record : records)
    GetState(result.entry_[record.key.action], record.key.state) = record.state;

  return result;
}
