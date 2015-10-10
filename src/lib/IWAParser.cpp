/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "IWAParser.h"

#include <cassert>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

#include "IWAObjectType.h"
#include "IWASnappyStream.h"
#include "IWORKCollector.h"
#include "IWORKPath.h"
#include "IWORKTypes.h"

namespace libetonyek
{

using boost::make_shared;
using boost::optional;

using std::deque;
using std::make_pair;
using std::pair;
using std::string;

IWAParser::ObjectRecord::ObjectRecord()
  : m_stream()
  , m_type(0)
  , m_headerRange(0, 0)
  , m_dataRange(0, 0)
{
}

IWAParser::ObjectRecord::ObjectRecord(const RVNGInputStreamPtr_t &stream, const unsigned type,
                                      const long pos, const unsigned long headerLen, const unsigned long dataLen)
  : m_stream(stream)
  , m_type(type)
  , m_headerRange(pos, pos + long(headerLen))
  , m_dataRange(m_headerRange.second, m_headerRange.second + long(dataLen))
{
}

IWAParser::IWAParser(const RVNGInputStreamPtr_t &fragments, const RVNGInputStreamPtr_t &package, IWORKCollector &collector)
  : m_fragments(fragments)
  , m_package(package)
  , m_collector(collector)
{
}

bool IWAParser::parse()
{
  parseObjectIndex();
  return parseDocument();
}

IWAParser::ObjectMessage::ObjectMessage(IWAParser &parser, const unsigned id, const unsigned type)
  : m_parser(parser)
  , m_message()
  , m_id(id)
  , m_type(0)
{
  std::deque<unsigned>::const_iterator it = find(m_parser.m_visited.begin(), m_parser.m_visited.end(), m_id);
  if (it == m_parser.m_visited.end())
  {
    optional<IWAMessage> msg;
    m_parser.queryObject(m_id, m_type, msg);
    if (msg)
    {
      if ((m_type == type) || (type == 0))
      {
        m_message = msg;
        m_parser.m_visited.push_back(m_id);
      }
      else
      {
        ETONYEK_DEBUG_MSG(("IWAParser::ObjectMessage::ObjectMessage: type mismatch for object %u: expected %u, got %u\n", id, type, m_type));
      }
    }
  }
}

IWAParser::ObjectMessage::~ObjectMessage()
{
  if (m_message)
  {
    assert(!m_parser.m_visited.empty());
    assert(m_parser.m_visited.back() == m_id);
    m_parser.m_visited.pop_back();
  }
}

IWAParser::ObjectMessage::operator bool() const
{
  return bool(m_message);
}

const IWAMessage &IWAParser::ObjectMessage::get() const
{
  return m_message.get();
}

unsigned IWAParser::ObjectMessage::getType() const
{
  return m_type;
}

void IWAParser::queryObject(const unsigned id, unsigned &type, boost::optional<IWAMessage> &msg) const
{
  const RecordMap_t::const_iterator recIt = m_fragmentObjectMap.find(id);
  if (recIt == m_fragmentObjectMap.end())
  {
    ETONYEK_DEBUG_MSG(("IWAParser::queryObject: object %u not found\n", id));
    return;
  }
  if (!recIt->second.second.m_stream)
    const_cast<IWAParser *>(this)->scanFragment(recIt->second.first);
  if (recIt->second.second.m_stream)
  {
    const ObjectRecord &objRecord = recIt->second.second;
    msg = IWAMessage(objRecord.m_stream, objRecord.m_dataRange.first, objRecord.m_dataRange.second);
    type = objRecord.m_type;
  }
}

boost::optional<unsigned> IWAParser::readRef(const IWAMessage &msg, const unsigned field)
{
  if (msg.message(field))
    return msg.message(field).uint32(1).optional();
  return boost::none;
}

std::deque<unsigned> IWAParser::readRefs(const IWAMessage &msg, const unsigned field)
{
  std::deque<unsigned> refs;
  if (msg.message(field))
  {
    const std::deque<IWAMessage> &objs = msg.message(field).repeated();
    for (std::deque<IWAMessage>::const_iterator it = objs.begin(); it != objs.end(); ++it)
    {
      if (it->uint32(1))
        refs.push_back(it->uint32(1).get());
    }
  }
  return refs;
}

boost::optional<IWORKPosition> IWAParser::readPosition(const IWAMessage &msg, const unsigned field)
{
  if (msg.message(field))
  {
    const optional<float> &x = msg.message(field).float_(1).optional();
    const optional<float> &y = msg.message(field).float_(2).optional();
    return IWORKPosition(get_optional_value_or(x, 0), get_optional_value_or(y, 0));
  }
  return boost::none;
}

boost::optional<IWORKSize> IWAParser::readSize(const IWAMessage &msg, const unsigned field)
{
  if (msg.message(field))
  {
    const optional<float> &w = msg.message(field).float_(1).optional();
    const optional<float> &h = msg.message(field).float_(2).optional();
    return IWORKSize(get_optional_value_or(w, 0), get_optional_value_or(h, 0));
  }
  return boost::none;
}

bool IWAParser::dispatchShape(const unsigned id)
{
  const ObjectMessage msg(*this, id);
  if (!msg)
    return false;

  switch (msg.getType())
  {
  case IWAObjectType::DrawableShape :
    return parseDrawableShape(get(msg));
  case IWAObjectType::Group :
    return parseGroup(get(msg));
  }

  return false;
}

bool IWAParser::parseDrawableShape(const IWAMessage &msg)
{
  m_collector.startLevel();
  m_collector.startText();

  const optional<IWAMessage> &shape = msg.message(1).optional();
  if (shape)
  {
    const optional<IWAMessage> &placement = get(shape).message(1).optional();
    if (placement)
      parseShapePlacement(get(placement));

    // const optional<unsigned> styleRef = readRef(get(shape), 2);

    const optional<IWAMessage> &path = get(shape).message(3).optional();
    if (path)
    {
      if (get(path).message(3)) // point path
      {
        const IWAMessage &pointPath = get(path).message(3).get();
        const optional<unsigned> &type = pointPath.uint32(1).optional();
        const optional<IWORKPosition> &point = readPosition(pointPath, 2);
        const optional<IWORKSize> &size = readSize(pointPath, 3);
        if (type && point && size)
        {
          switch (get(type))
          {
          case 1 :
          case 10 :
            m_collector.collectArrowPath(get(size), get(point).m_x, get(point).m_y, get(type) == 10);
            break;
          case 100 :
            m_collector.collectStarPath(get(size), get(point).m_x, get(point).m_y);
            break;
          default :
            ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: unknown point path type %u\n", get(type)));
            break;
          }
        }
      }
      else if (get(path).message(4)) // scalar path
      {
        const IWAMessage &scalarPath = get(path).message(4).get();
        const optional<unsigned> &type = scalarPath.uint32(1).optional();
        const optional<float> &value = scalarPath.float_(2).optional();
        const optional<IWORKSize> &size = readSize(scalarPath, 3);
        if (type && value && size)
        {
          switch (get(type))
          {
          case 0 :
            m_collector.collectRoundedRectanglePath(get(size), get(value));
            break;
          case 1 :
            m_collector.collectPolygonPath(get(size), get(value));
            break;
          default :
            ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: unknown scalar path type %u\n", get(type)));
            break;
          }
        }
      }
      else if (get(path).message(5)) // bezier path
      {
        const optional<IWAMessage> &bezier = get(path).message(5).get().message(3).optional();
        if (bezier)
        {
          const IWORKPathPtr_t bezierPath(new IWORKPath());
          const deque<IWAMessage> &elements = get(bezier).message(1).repeated();
          bool closed = false;
          bool closingMove = false;
          for (deque<IWAMessage>::const_iterator it = elements.begin(); it != elements.end() && !closed; ++it)
          {
            const optional<unsigned> &type = it->uint32(1).optional();
            if (type)
            {
              switch (get(type))
              {
              case 1 :
              case 2 :
              {
                const optional<IWORKPosition> &coords = readPosition(*it, 2);
                if (!coords)
                {
                  ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: missing coordinates for %c element\n", get(type) == 1 ? 'M' : 'L'));
                  break;
                }
                if (closed)
                {
                  if (closingMove)
                  {
                    ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: unexpected bezier path element after the closing move\n"));
                  }
                  else if (get(type) != 1)
                  {
                    ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: unexpected element %cafter close\n", get(type)));
                  }
                  closingMove = true;
                }
                else
                {
                  if (get(type) == 1)
                    bezierPath->appendMoveTo(get(coords).m_x, get(coords).m_y);
                  else
                    bezierPath->appendLineTo(get(coords).m_x, get(coords).m_y);
                }
                break;
              }
              case 4 :
              {
                if (it->message(2))
                {
                  const std::deque<IWAMessage> &positions = it->message(2).repeated();
                  if (positions.size() >= 3)
                  {
                    if (positions.size() > 3)
                    {
                      ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: a curve has got %u control coords\n", unsigned(positions.size())));
                    }
                    const optional<float> &x = positions[0].float_(1).optional();
                    const optional<float> &y = positions[0].float_(2).optional();
                    const optional<float> &x1 = positions[1].float_(1).optional();
                    const optional<float> &y1 = positions[1].float_(2).optional();
                    const optional<float> &x2 = positions[2].float_(1).optional();
                    const optional<float> &y2 = positions[2].float_(2).optional();
                    bezierPath->appendCurveTo(get_optional_value_or(x, 0), get_optional_value_or(y, 0),
                                              get_optional_value_or(x1, 0), get_optional_value_or(y1, 0),
                                              get_optional_value_or(x2, 0), get_optional_value_or(y2, 0));
                  }
                  else
                  {
                    ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: %u is not enough coords for a curve\n", unsigned(positions.size())));
                  }
                }
                break;
              }
              case 5 :
                bezierPath->appendClose();
                closed = true;
                break;
              default :
                ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: unknown bezier path element type %u\n", get(type)));
              }
            }
          }
          m_collector.collectBezier(bezierPath);
          m_collector.collectBezierPath();
        }
      }
      else if (get(path).message(6)) // callout2 path
      {
        const IWAMessage &callout2Path = get(path).message(6).get();
        const optional<IWORKSize> &size = readSize(callout2Path, 1);
        const optional<IWORKPosition> &tailPos = readPosition(callout2Path, 2);
        const optional<float> &tailSize = callout2Path.float_(3).optional();
        if (size && tailPos && tailSize)
        {
          const optional<float> &cornerRadius = callout2Path.float_(4).optional();
          const optional<bool> &tailAtCenter = callout2Path.bool_(5).optional();
          m_collector.collectCalloutPath(get(size), get_optional_value_or(cornerRadius, 0),
                                         get(tailSize), get(tailPos).m_x, get(tailPos).m_y,
                                         get_optional_value_or(tailAtCenter, false));
        }
      }
      else if (get(path).message(7)) // connection path
      {
        ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: connection path is not supported yet\n"));
      }
    }

    m_collector.collectShape();
  }

  // const optional<unsigned> &textRef = readRef(msg, 2);

  m_collector.endText();
  m_collector.endLevel();

  return true;
}

bool IWAParser::parseGroup(const IWAMessage &msg)
{
  if (msg.message(1))
    parseShapePlacement(get(msg.message(1)));
  if (!msg.message(2).empty())
  {
    m_collector.startLevel();
    m_collector.startGroup();
    const deque<unsigned> &shapeRefs = readRefs(msg, 2);
    std::for_each(shapeRefs.begin(), shapeRefs.end(), bind(&IWAParser::dispatchShape, this, _1));
    m_collector.endGroup();
    m_collector.endLevel();
  }

  return true;
}

bool IWAParser::parseShapePlacement(const IWAMessage &msg)
{
  const IWORKGeometryPtr_t geometry(new IWORKGeometry());

  const optional<IWAMessage> &g = msg.message(1).optional();
  if (g)
  {
    const optional<IWORKPosition> &pos = readPosition(get(g), 1);
    if (pos)
      geometry->m_position = get(pos);
    const optional<IWORKSize> &size = readSize(get(g), 2);
    if (size)
    {
      geometry->m_naturalSize = get(size);
      geometry->m_size = get(size);
    }

    if (get(g).uint32(3))
    {
      switch (get(get(g).uint32(3)))
      {
      case 3 : // normal
        break;
      case 7 : // horizontal flip
        geometry->m_horizontalFlip = true;
        break;
      default :
        ETONYEK_DEBUG_MSG(("IWAParser::parseDrawableShape: unknown transformation %u\n", get(get(g).uint32(3))));
        break;
      }
    }
    if (get(g).float_(4))
      geometry->m_angle = deg2rad(get(get(g).float_(4)));
  }
  geometry->m_aspectRatioLocked = msg.bool_(7).optional();

  m_collector.collectGeometry(geometry);

  return true;
}

void IWAParser::parseObjectIndex()
{
  m_fragmentMap[2] = make_pair(string("Index/Metadata.iwa"), RVNGInputStreamPtr_t());
  m_fragmentObjectMap[2] = make_pair(2, ObjectRecord());
  scanFragment(2);
  const RecordMap_t::const_iterator indexIt = m_fragmentObjectMap.find(2);
  if (indexIt == m_fragmentObjectMap.end())
  {
    // TODO: scan all fragment files
    ETONYEK_DEBUG_MSG(("IWAParser::parseObjectIndex: object index is broken, nothing will be parsed\n"));
  }
  else
  {
    const ObjectRecord &rec = indexIt->second.second;
    assert(bool(rec.m_stream));
    const IWAMessage objectIndex(rec.m_stream, rec.m_dataRange.first, rec.m_dataRange.second);
    const deque<IWAMessage> &fragments = objectIndex.message(3).repeated();
    for (deque<IWAMessage>::const_iterator it = fragments.begin(); it != fragments.end(); ++it)
    {
      if (it->uint32(1) && (it->string(2) || it->string(3)))
      {
        const unsigned pathIdx = it->string(3) ? 3 : 2;
        m_fragmentMap[it->uint32(1).get()] = make_pair("Index/" + it->string(pathIdx).get() + ".iwa", RVNGInputStreamPtr_t());
        m_fragmentObjectMap[it->uint32(1).get()] = make_pair(it->uint32(1).get(), ObjectRecord());
      }
      const deque<IWAMessage> &refs = it->message(6).repeated();
      for (deque<IWAMessage>::const_iterator refIt = refs.begin(); refIt != refs.end(); ++refIt)
      {
        if (refIt->uint32(1) && refIt->uint32(2))
          m_fragmentObjectMap[refIt->uint32(2).get()] = make_pair(refIt->uint32(1).get(), ObjectRecord());
      }
    }
    const deque<IWAMessage> &files = objectIndex.message(4).repeated();
    for (deque<IWAMessage>::const_iterator it = files.begin(); it != files.end(); ++it)
    {
      if (it->uint32(1) && it->string(3))
        m_fileMap[it->uint32(1).get()] = make_pair(it->string(3).get(), RVNGInputStreamPtr_t());
    }
  }
}

void IWAParser::scanFragment(const unsigned id)
{
  // scan the fragment file
  const FileMap_t::iterator fragmentIt = m_fragmentMap.find(id);
  if (fragmentIt != m_fragmentMap.end())
  {
    assert(!fragmentIt->second.second); // this could only happen if the fragment file had already been scanned
    if (m_fragments->existsSubStream(fragmentIt->second.first.c_str()))
    {
      const RVNGInputStreamPtr_t stream(m_fragments->getSubStreamByName(fragmentIt->second.first.c_str()));
      assert(bool(stream));
      fragmentIt->second.second = make_shared<IWASnappyStream>(stream);
      scanFragment(fragmentIt->first, fragmentIt->second.second);
    }
    else
    {
      ETONYEK_DEBUG_MSG(("IWAParser::scanFragment: file %s does not exist\n", fragmentIt->second.first.c_str()));
      m_fragmentMap.erase(fragmentIt); // avoid unnecessary repeats of the lookup
    }
  }
}

void IWAParser::scanFragment(const unsigned id, const RVNGInputStreamPtr_t &stream)
{
  try
  {
    while (!stream->isEnd())
    {
      // scan a single object
      const uint64_t headerLen = readUVar(stream);
      const long start = stream->tell();
      const IWAMessage header(stream, headerLen);
      if (!header.message(2) || !header.message(2).uint64(3))
        break;
      const uint64_t dataLen = header.message(2).uint64(3).get();
      if (header.uint32(1))
      {
        const optional<unsigned> type = header.message(2).uint32(1).optional();
        const ObjectRecord rec(stream, get_optional_value_or(type, 0), start, long(headerLen), long(dataLen));
        m_fragmentObjectMap[header.uint32(1).get()] = make_pair(id, rec);
      }
      if (stream->seek(start + long(headerLen) + long(dataLen), librevenge::RVNG_SEEK_SET) != 0)
        break;
    }
  }
  catch (...)
  {
    // just read as much as possible
  }

  // remove all objects from the fragment that have not been found
  RecordMap_t::iterator it = m_fragmentObjectMap.begin();
  while (it != m_fragmentObjectMap.end())
  {
    const RecordMap_t::iterator curIt = it;
    ++it;
    if ((curIt->second.first == id) && !curIt->second.second.m_stream)
    {
      ETONYEK_DEBUG_MSG(("IWAParser::scanFragment: object with ID %u was not found\n", curIt->first));
      m_fragmentObjectMap.erase(curIt);
    }
  }
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
