/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "IWORKXMLReader.h"
#include "KEY1Parser.h"
#include "KEY1Token.h"

namespace libetonyek
{

KEY1Parser::KEY1Parser(const RVNGInputStreamPtr_t &input, const RVNGInputStreamPtr_t &package, KEYCollector *const collector, const KEYDefaults &defaults)
  : KEYParser(input, package, collector, defaults)
{
}

KEY1Parser::~KEY1Parser()
{
}

IWORKXMLContextPtr_t KEY1Parser::makeDocumentContext(IWORKXMLParserState &state) const
{
  // TODO: implement me
  (void) state;
  return IWORKXMLContextPtr_t();
}

IWORKXMLReader::TokenizerFunction_t KEY1Parser::getTokenizer() const
{
  return KEY1Tokenizer();
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
