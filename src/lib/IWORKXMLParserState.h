/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libe-book project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IWORKXMLPARSERSTATE_H_INCLUDED
#define IWORKXMLPARSERSTATE_H_INCLUDED

#include "libetonyek_utils.h"
#include "IWORKXMLReader.h"

namespace libetonyek
{

class KEYCollector;
class KEYDefaults;
class KEYParser;

class IWORKXMLParserState
{
  // not copyable
  IWORKXMLParserState(const IWORKXMLParserState &);
  IWORKXMLParserState& operator=(const IWORKXMLParserState &);

public:
  IWORKXMLParserState(KEYParser &parser, const IWORKXMLReader::TokenizerFunction_t &tokenizer);

  KEYParser &getParser();
  KEYCollector *getCollector() const;
  const KEYDefaults &getDefaults() const;
  const IWORKXMLReader::TokenizerFunction_t &getTokenizer() const;

private:
  KEYParser &m_parser;
  const IWORKXMLReader::TokenizerFunction_t m_tokenizer;
};

}

#endif // IWORKXMLPARSERSTATE_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
