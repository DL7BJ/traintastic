/**
 * server/src/hardware/programming/lncv/lncvprogrammer.cpp
 *
 * This file is part of the traintastic source code.
 *
 * Copyright (C) 2022 Reinder Feenstra
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "decoderprogrammer.hpp"
#include "decoderprogrammingcontroller.hpp"
#include "../../../core/method.tpp"

DecoderProgrammer::DecoderProgrammer(DecoderProgrammingController& controller)
  : m_controller{controller}
  , onReadResponse{*this, "on_read_response", EventFlags::Public}
{
  if(!m_controller.attachDecoderProgrammer(*this))
    throw std::runtime_error("decoder_programmer:programmer_not_available");
  m_interfaceItems.add(onReadResponse);
}

DecoderProgrammer::~DecoderProgrammer()
{
  m_controller.detachDecoderProgrammer(*this);
}

void DecoderProgrammer::readResponse(bool success, uint16_t cv, uint16_t value)
{
  fireEvent(onReadResponse, success, cv, value);
}
