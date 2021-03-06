/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IWORKBINARYELEMENT_H_INCLUDED
#define IWORKBINARYELEMENT_H_INCLUDED

#include <boost/optional.hpp>

#include "IWORKTypes.h"
#include "IWORKXMLContextBase.h"

namespace libetonyek
{

class IWORKBinaryElement : public IWORKXMLElementContextBase
{
public:
  IWORKBinaryElement(IWORKXMLParserState &state, IWORKMediaContentPtr_t &value);

private:
  IWORKXMLContextPtr_t element(int name) override;
  void endOfElement() override;

private:
  IWORKMediaContentPtr_t &m_value;
  boost::optional<IWORKSize> m_size;
  IWORKDataPtr_t m_data;
  boost::optional<IWORKColor> m_fillColor;
};

}

#endif // IWORKBINARYELEMENT_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
