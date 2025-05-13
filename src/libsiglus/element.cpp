// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include "libsiglus/element.hpp"

#include "libsiglus/elm_details/memory.hpp"
#include "utilities/string_utilities.hpp"

#include <format>

namespace libsiglus {

// -----------------------------------------------------------------------
// class IElement
std::string IElement::ToDebugString() const {
  return std::format("({}:{})", ToString(type), root_id);
}
Element IElement::Parse(std::span<int>) const {
  throw std::runtime_error("IElement::Parse: no implementation found.");
}

// -----------------------------------------------------------------------
// class UserCommand
std::string UserCommand::ToDebugString() const {
  return std::format("{}<{}:{}>", name, scene, entry);
}
elm::Kind UserCommand::Kind() const noexcept { return elm::Kind::Usrcmd; }

// -----------------------------------------------------------------------
// class UserProperty
std::string UserProperty::ToDebugString() const {
  return std::format("{}<{}:{}>", name, scene, idx);
}
elm::Kind UserProperty::Kind() const noexcept { return elm::Kind::Usrprop; }

// -----------------------------------------------------------------------
// class UnknownElement
std::string UnknownElement::ToDebugString() const {
  return "???" +
         std::format("<{},{}>", root_id, Join(",", view_to_string(path)));
}
elm::Kind UnknownElement::Kind() const noexcept { return elm::Kind::Invalid; }

// -----------------------------------------------------------------------
// MakeElement
Element MakeElement(int elm, std::span<int> path) {
  enum Global {
    A = 25,
    B = 26,
    C = 27,
    D = 28,
    E = 29,
    F = 30,
    X = 137,
    G = 31,
    Z = 32,
    S = 34,
    M = 35,
    NAMAE_GLOBAL = 107,
    NAMAE_LOCAL = 106,
    NAMAE = 108,
    STAGE = 49,
    BACK = 37,
    FRONT = 38,
    NEXT = 73,
    MSGBK = 145,
    SCREEN = 70,
    COUNTER = 40,
    FRAME_ACTION = 79,
    FRAME_ACTION_CH = 53,
    TIMEWAIT = 54,
    TIMEWAIT_KEY = 55,
    MATH = 39,
    DATABASE = 105,
    CGTABLE = 78,
    BGMTABLE = 123,
    G00BUF = 124,
    MASK = 135,
    EDITBOX = 97,
    FILE = 48,
    BGM = 42,
    KOE_ST = 82,
    PCM = 43,
    PCMCH = 44,
    PCMEVENT = 52,
    SE = 45,
    MOV = 20,
    INPUT = 86,
    MOUSE = 46,
    KEY = 24,
    SCRIPT = 64,
    SYSCOM = 63,
    SYSTEM = 92,
    CALL = 98,
    CUR_CALL = 83,
    EXCALL = 65,
    STEAM = 166,
    EXKOE = 87,
    EXKOE_PLAY_WAIT = 88,
    EXKOE_PLAY_WAIT_KEY = 89,
    NOP = 60,
    OWARI = 2,
    RETURNMENU = 3,
    JUMP = 4,
    FARCALL = 5,
    GET_SCENE_NAME = 131,
    GET_LINE_NO = 158,
    SET_TITLE = 74,
    GET_TITLE = 75,
    SAVEPOINT = 36,
    CLEAR_SAVEPOINT = 113,
    CHECK_SAVEPOINT = 112,
    SELPOINT = 1,
    CLEAR_SELPOINT = 111,
    CHECK_SELPOINT = 110,
    STACK_SELPOINT = 149,
    DROP_SELPOINT = 150,
    DISP = 96,
    FRAME = 6,
    CAPTURE = 80,
    CAPTURE_FROM_FILE = 163,
    CAPTURE_FREE = 81,
    CAPTURE_FOR_OBJECT = 130,
    CAPTURE_FOR_OBJECT_FREE = 136,
    CAPTURE_FOR_TWEET = 164,
    CAPTURE_FREE_FOR_TWEET = 165,
    CAPTURE_FOR_LOCAL_SAVE = 144,
    WIPE = 7,
    WIPE_ALL = 23,
    MASK_WIPE = 50,
    MASK_WIPE_ALL = 51,
    WIPE_END = 33,
    WAIT_WIPE = 103,
    CHECK_WIPE = 109,
    MESSAGE_BOX = 0,
    SET_MWND = 8,
    GET_MWND = 116,
    SET_SEL_MWND = 71,
    GET_SEL_MWND = 117,
    SET_WAKU = 22,
    OPEN = 9,
    OPEN_WAIT = 58,
    OPEN_NOWAIT = 59,
    CLOSE = 10,
    CLOSE_WAIT = 56,
    CLOSE_NOWAIT = 57,
    END_CLOSE = 125,
    MSG_BLOCK = 84,
    MSG_PP_BLOCK = 121,
    CLEAR = 11,
    SET_NAMAE = 156,
    PRINT = 12,
    RUBY = 61,
    MSGBTN = 47,
    NL = 15,
    NLI = 62,
    INDENT = 119,
    CLEAR_INDENT = 94,
    WAIT_MSG = 21,
    PP = 13,
    R = 14,
    PAGE = 115,
    REP_POS = 151,
    SIZE = 16,
    COLOR = 17,
    MULTI_MSG = 95,
    NEXT_MSG = 93,
    START_SLIDE_MSG = 120,
    END_SLIDE_MSG = 122,
    KOE = 18,
    KOE_PLAY_WAIT = 90,
    KOE_PLAY_WAIT_KEY = 91,
    KOE_SET_VOLUME = 152,
    KOE_SET_VOLUME_MAX = 153,
    KOE_SET_VOLUME_MIN = 154,
    KOE_GET_VOLUME = 155,
    KOE_STOP = 68,
    KOE_WAIT = 85,
    KOE_WAIT_KEY = 99,
    KOE_CHECK = 69,
    KOE_CHECK_GET_KOE_NO = 159,
    KOE_CHECK_GET_CHARA_NO = 160,
    KOE_CHECK_IS_EX_KOE = 161,
    CLEAR_FACE = 72,
    SET_FACE = 41,
    CLEAR_MSGBK = 118,
    INSERT_MSGBK_IMG = 114,
    SEL = 19,
    SEL_CANCEL = 101,
    SELMSG = 100,
    SELMSG_CANCEL = 102,
    SELBTN = 76,
    SELBTN_READY = 77,
    SELBTN_CANCEL = 126,
    SELBTN_CANCEL_READY = 128,
    SELBTN_START = 127,
    GET_LAST_SEL_MSG = 162,
    INIT_CALL_STACK = 141,
    DEL_CALL_STACK = 138,
    SET_CALL_STACK_CNT = 140,
    GET_CALL_STACK_CNT = 139,
    __FOG_NAME = 132,
    __FOG_X = 142,
    __FOG_X_EVE = 143,
    __FOG_NEAR = 133,
    __FOG_FAR = 134,
    __TEST = 104
  };

  switch (elm) {
    case A:
    case B:
    case C:
    case D:
    case E:
    case F:
    case X:
    case G:
    case Z:
    case S:
    case M:
    case NAMAE_LOCAL:
    case NAMAE_GLOBAL: {
      static elm::Memory memory;
      memory.root_id = elm;
      return memory.Parse(path);
    }
  }

  auto uke = std::make_unique<UnknownElement>();
  uke->root_id = elm;
  uke->path = std::vector(path.begin(), path.end());

  return uke;
}

}  // namespace libsiglus
