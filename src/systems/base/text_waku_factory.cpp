// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#include "systems/base/text_waku_factory.hpp"

#include "core/gameexe.hpp"
#include "systems/base/text_waku_normal.hpp"
#include "systems/base/text_waku_type4.hpp"

TextWakuFactory::TextWakuFactory(Gameexe& gexe) : gexe_(gexe) {}

std::unique_ptr<TextWaku> TextWakuFactory::CreateWaku(System& system,
                                                      TextWindow& window,
                                                      int setno,
                                                      int no) {
  auto waku = gexe_("WAKU", setno, "TYPE");

  if (waku.Int().value_or(5) == 5)
    return std::make_unique<TextWakuNormal>(system, window, setno, no);
  else
    return std::make_unique<TextWakuType4>(system, window, setno, no);
}
