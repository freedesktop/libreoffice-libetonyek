/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <boost/enable_shared_from_this.hpp>

#include "IWORKXMLContextBase.h"
#include "IWORKXMLReader.h"
#include "KEYParser.h"
#include "KEYCollector.h"
#include "KEYDefaults.h"

namespace libetonyek
{

namespace
{

class DiscardContext : public IWORKXMLContext, public boost::enable_shared_from_this<DiscardContext>
{
private:
  virtual void startOfElement();
  void attribute(int name, const char *value);
  virtual void endOfAttributes();
  IWORKXMLContextPtr_t element(int name);
  void text(const char *value);
  virtual void endOfElement();
};

void DiscardContext::startOfElement()
{
}

void DiscardContext::attribute(int, const char *)
{
}

void DiscardContext::endOfAttributes()
{
}

IWORKXMLContextPtr_t DiscardContext::element(int)
{
  return shared_from_this();
}

void DiscardContext::text(const char *)
{
}

void DiscardContext::endOfElement()
{
}

}

namespace
{

void processElement(const IWORKXMLReader &reader, const IWORKXMLContextPtr_t &context)
{
  context->startOfElement();

  IWORKXMLReader::AttributeIterator attr(reader);
  while (attr.next())
    context->attribute(getId(attr), attr.getValue());
  context->endOfAttributes();

  IWORKXMLReader::MixedIterator mixed(reader);
  while (mixed.next())
  {
    if (mixed.isElement())
    {
      IWORKXMLContextPtr_t subContext(context->element(getId(mixed)));
      if (!subContext)
        subContext.reset(new DiscardContext());
      processElement(mixed, subContext);
    }
    else if (mixed.isText())
    {
      context->text(mixed.getText());
    }
  }

  context->endOfElement();
}

}

KEYParser::KEYParser(const RVNGInputStreamPtr_t &input, const RVNGInputStreamPtr_t &package, KEYCollector *const collector, const KEYDefaults &defaults)
  : m_input(input)
  , m_package(package)
  , m_collector(collector)
  , m_defaults(defaults)
{
}

KEYParser::~KEYParser()
{
}

bool KEYParser::parse()
{
  IWORKXMLReader reader(m_input.get(), getTokenizer());
  IWORKXMLParserState state(*this, getTokenizer());
  processElement(reader, makeDocumentContext(state));

  return true;
}

RVNGInputStreamPtr_t &KEYParser::getInput()
{
  return m_input;
}

RVNGInputStreamPtr_t KEYParser::getInput() const
{
  return m_input;
}

RVNGInputStreamPtr_t &KEYParser::getPackage()
{
  return m_package;
}

RVNGInputStreamPtr_t KEYParser::getPackage() const
{
  return m_package;
}

KEYCollector *KEYParser::getCollector() const
{
  return m_collector;
}

const KEYDefaults &KEYParser::getDefaults() const
{
  return m_defaults;
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
