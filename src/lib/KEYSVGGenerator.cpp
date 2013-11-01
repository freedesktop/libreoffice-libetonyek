/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <locale.h>
#include <sstream>
#include <string>
#include "KEYSVGGenerator.h"

namespace libetonyek
{

namespace
{

static std::string doubleToString(const double value)
{
  WPXProperty *prop = WPXPropertyFactory::newDoubleProp(value);
  std::string retVal = prop->getStr().cstr();
  delete prop;
  return retVal;
}

static unsigned stringToColour(const ::WPXString &s)
{
  std::string str(s.cstr());
  if (str[0] == '#')
  {
    if (str.length() != 7)
      return 0;
    else
      str.erase(str.begin());
  }
  else
    return 0;

  std::istringstream istr(str);
  unsigned val = 0;
  istr >> std::hex >> val;
  return val;
}

}

KEYSVGGenerator::KEYSVGGenerator(KEYStringVector &vec): m_gradient(), m_style(), m_gradientIndex(1), m_patternIndex(1), m_shadowIndex(1), m_outputSink(), m_vec(vec)
{
}

KEYSVGGenerator::~KEYSVGGenerator()
{
}

void KEYSVGGenerator::startDocument(const ::WPXPropertyList &)
{
}

void KEYSVGGenerator::endDocument()
{
}

void KEYSVGGenerator::setDocumentMetaData(const ::WPXPropertyList &)
{
}

void KEYSVGGenerator::startSlide(const WPXPropertyList &propList)
{
  m_outputSink << "<svg:svg version=\"1.1\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" ";
  if (propList["svg:width"])
    m_outputSink << "width=\"" << doubleToString(72*(propList["svg:width"]->getDouble())) << "\" ";
  if (propList["svg:height"])
    m_outputSink << "height=\"" << doubleToString(72*(propList["svg:height"]->getDouble())) << "\"";
  m_outputSink << " >\n";
}

void KEYSVGGenerator::endSlide()
{
  m_outputSink << "</svg:svg>\n";
  m_vec.append(m_outputSink.str().c_str());
  m_outputSink.str("");
}

void KEYSVGGenerator::setStyle(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &gradient)
{
  m_style.clear();
  m_style = propList;

  m_gradient = gradient;
  if(m_style["draw:shadow"] && m_style["draw:shadow"]->getStr() == "visible")
  {
    unsigned shadowColour = 0;
    double shadowRed = 0.0;
    double shadowGreen = 0.0;
    double shadowBlue = 0.0;
    if (m_style["draw:shadow-color"])
    {
      shadowColour = stringToColour(m_style["draw:shadow-color"]->getStr());
      shadowRed = (double)((shadowColour & 0x00ff0000) >> 16)/255.0;
      shadowGreen = (double)((shadowColour & 0x0000ff00) >> 8)/255.0;
      shadowBlue = (double)(shadowColour & 0x000000ff)/255.0;
    }
    m_outputSink << "<svg:defs>\n";
    m_outputSink << "<svg:filter filterUnits=\"userSpaceOnUse\" id=\"shadow" << m_shadowIndex++ << "\">";
    m_outputSink << "<svg:feOffset in=\"SourceGraphic\" result=\"offset\" ";
    m_outputSink << "dx=\"" << doubleToString(72*m_style["draw:shadow-offset-x"]->getDouble()) << "\" ";
    m_outputSink << "dy=\"" << doubleToString(72*m_style["draw:shadow-offset-y"]->getDouble()) << "\"/>";
    m_outputSink << "<svg:feColorMatrix in=\"offset\" result=\"offset-color\" type=\"matrix\" values=\"";
    m_outputSink << "0 0 0 0 " << doubleToString(shadowRed) ;
    m_outputSink << " 0 0 0 0 " << doubleToString(shadowGreen);
    m_outputSink << " 0 0 0 0 " << doubleToString(shadowBlue);
    if(m_style["draw:opacity"] && m_style["draw:opacity"]->getDouble() < 1)
      m_outputSink << " 0 0 0 "   << doubleToString(m_style["draw:shadow-opacity"]->getDouble()/m_style["draw:opacity"]->getDouble()) << " 0\"/>";
    else
      m_outputSink << " 0 0 0 "   << doubleToString(m_style["draw:shadow-opacity"]->getDouble()) << " 0\"/>";
    m_outputSink << "<svg:feMerge><svg:feMergeNode in=\"offset-color\" /><svg:feMergeNode in=\"SourceGraphic\" /></svg:feMerge></svg:filter></svg:defs>";
  }

  if(m_style["draw:fill"] && m_style["draw:fill"]->getStr() == "gradient")
  {
    double angle = (m_style["draw:angle"] ? m_style["draw:angle"]->getDouble() : 0.0);
    angle *= -1.0;
    while(angle < 0)
      angle += 360;
    while(angle > 360)
      angle -= 360;

    if (!m_gradient.count())
    {
      if (m_style["draw:style"] &&
          (m_style["draw:style"]->getStr() == "radial" ||
           m_style["draw:style"]->getStr() == "rectangular" ||
           m_style["draw:style"]->getStr() == "square" ||
           m_style["draw:style"]->getStr() == "ellipsoid"))
      {
        m_outputSink << "<svg:defs>\n";
        m_outputSink << "  <svg:radialGradient id=\"grad" << m_gradientIndex++ << "\"";

        if (m_style["svg:cx"])
          m_outputSink << " cx=\"" << m_style["svg:cx"]->getStr().cstr() << "\"";
        else if (m_style["draw:cx"])
          m_outputSink << " cx=\"" << m_style["draw:cx"]->getStr().cstr() << "\"";

        if (m_style["svg:cy"])
          m_outputSink << " cy=\"" << m_style["svg:cy"]->getStr().cstr() << "\"";
        else if (m_style["draw:cy"])
          m_outputSink << " cy=\"" << m_style["draw:cy"]->getStr().cstr() << "\"";
        m_outputSink << " r=\"" << (1 - (m_style["draw:border"] ? m_style["draw:border"]->getDouble() : 0))*100.0 << "%\" >\n";
        m_outputSink << " >\n";

        if (m_style["draw:start-color"] && m_style["draw:end-color"])
        {
          m_outputSink << "    <svg:stop offset=\"0%\"";
          m_outputSink << " stop-color=\"" << m_style["draw:end-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << (m_style["libwpg:end-opacity"] ? m_style["libwpg:end-opacity"]->getDouble() : 1) << "\" />" << std::endl;

          m_outputSink << "    <svg:stop offset=\"100%\"";
          m_outputSink << " stop-color=\"" << m_style["draw:start-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << (m_style["libwpg:start-opacity"] ? m_style["libwpg:start-opacity"]->getDouble() : 1) << "\" />" << std::endl;
        }
        m_outputSink << "  </svg:radialGradient>\n";
        m_outputSink << "</svg:defs>\n";
      }
      else if (m_style["draw:style"] && m_style["draw:style"]->getStr() == "linear")
      {
        m_outputSink << "<svg:defs>\n";
        m_outputSink << "  <svg:linearGradient id=\"grad" << m_gradientIndex++ << "\" >\n";

        if (m_style["draw:start-color"] && m_style["draw:end-color"])
        {
          m_outputSink << "    <svg:stop offset=\"0%\"";
          m_outputSink << " stop-color=\"" << m_style["draw:start-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << (m_style["libwpg:start-opacity"] ? m_style["libwpg:start-opacity"]->getDouble() : 1) << "\" />" << std::endl;

          m_outputSink << "    <svg:stop offset=\"100%\"";
          m_outputSink << " stop-color=\"" << m_style["draw:end-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << (m_style["libwpg:end-opacity"] ? m_style["libwpg:end-opacity"]->getDouble() : 1) << "\" />" << std::endl;
        }
        m_outputSink << "  </svg:linearGradient>\n";

        // not a simple horizontal gradient
        if(angle != 270)
        {
          m_outputSink << "  <svg:linearGradient xlink:href=\"#grad" << m_gradientIndex-1 << "\"";
          m_outputSink << " id=\"grad" << m_gradientIndex++ << "\" ";
          m_outputSink << "x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\" ";
          m_outputSink << "gradientTransform=\"rotate(" << angle << " .5 .5)\" ";
          m_outputSink << "gradientUnits=\"objectBoundingBox\" >\n";
          m_outputSink << "  </svg:linearGradient>\n";
        }

        m_outputSink << "</svg:defs>\n";
      }
      else if (m_style["draw:style"] && m_style["draw:style"]->getStr() == "axial")
      {
        m_outputSink << "<svg:defs>\n";
        m_outputSink << "  <svg:linearGradient id=\"grad" << m_gradientIndex++ << "\" >\n";

        if (m_style["draw:start-color"] && m_style["draw:end-color"])
        {
          m_outputSink << "    <svg:stop offset=\"0%\"";
          m_outputSink << " stop-color=\"" << m_style["draw:end-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << (m_style["libwpg:end-opacity"] ? m_style["libwpg:end-opacity"]->getDouble() : 1) << "\" />" << std::endl;

          m_outputSink << "    <svg:stop offset=\"50%\"";
          m_outputSink << " stop-color=\"" << m_style["draw:start-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << (m_style["libwpg:start-opacity"] ? m_style["libwpg:start-opacity"]->getDouble() : 1) << "\" />" << std::endl;

          m_outputSink << "    <svg:stop offset=\"100%\"";
          m_outputSink << " stop-color=\"" << m_style["draw:end-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << (m_style["libwpg:end-opacity"] ? m_style["libwpg:end-opacity"]->getDouble() : 1) << "\" />" << std::endl;
        }
        m_outputSink << "  </svg:linearGradient>\n";

        // not a simple horizontal gradient
        if(angle != 270)
        {
          m_outputSink << "  <svg:linearGradient xlink:href=\"#grad" << m_gradientIndex-1 << "\"";
          m_outputSink << " id=\"grad" << m_gradientIndex++ << "\" ";
          m_outputSink << "x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\" ";
          m_outputSink << "gradientTransform=\"rotate(" << angle << " .5 .5)\" ";
          m_outputSink << "gradientUnits=\"objectBoundingBox\" >\n";
          m_outputSink << "  </svg:linearGradient>\n";
        }

        m_outputSink << "</svg:defs>\n";
      }
    }
    else
    {
      if (m_style["draw:style"] && m_style["draw:style"]->getStr() == "radial")
      {
        m_outputSink << "<svg:defs>\n";
        m_outputSink << "  <svg:radialGradient id=\"grad" << m_gradientIndex++ << "\" cx=\"" << m_style["svg:cx"]->getStr().cstr() << "\" cy=\"" << m_style["svg:cy"]->getStr().cstr() << "\" r=\"" << m_style["svg:r"]->getStr().cstr() << "\" >\n";
        for(unsigned c = 0; c < m_gradient.count(); c++)
        {
          m_outputSink << "    <svg:stop offset=\"" << m_gradient[c]["svg:offset"]->getStr().cstr() << "\"";

          m_outputSink << " stop-color=\"" << m_gradient[c]["svg:stop-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << m_gradient[c]["svg:stop-opacity"]->getDouble() << "\" />" << std::endl;

        }
        m_outputSink << "  </svg:radialGradient>\n";
        m_outputSink << "</svg:defs>\n";
      }
      else
      {
        m_outputSink << "<svg:defs>\n";
        m_outputSink << "  <svg:linearGradient id=\"grad" << m_gradientIndex++ << "\" >\n";
        for(unsigned c = 0; c < m_gradient.count(); c++)
        {
          m_outputSink << "    <svg:stop offset=\"" << m_gradient[c]["svg:offset"]->getStr().cstr() << "\"";

          m_outputSink << " stop-color=\"" << m_gradient[c]["svg:stop-color"]->getStr().cstr() << "\"";
          m_outputSink << " stop-opacity=\"" << m_gradient[c]["svg:stop-opacity"]->getDouble() << "\" />" << std::endl;

        }
        m_outputSink << "  </svg:linearGradient>\n";

        // not a simple horizontal gradient
        if(angle != 270)
        {
          m_outputSink << "  <svg:linearGradient xlink:href=\"#grad" << m_gradientIndex-1 << "\"";
          m_outputSink << " id=\"grad" << m_gradientIndex++ << "\" ";
          m_outputSink << "x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\" ";
          m_outputSink << "gradientTransform=\"rotate(" << angle << " .5 .5)\" ";
          m_outputSink << "gradientUnits=\"objectBoundingBox\" >\n";
          m_outputSink << "  </svg:linearGradient>\n";
        }

        m_outputSink << "</svg:defs>\n";
      }
    }
  }
  else if(m_style["draw:fill"] && m_style["draw:fill"]->getStr() == "bitmap")
  {
    if (m_style["draw:fill-image"] && m_style["libwpg:mime-type"])
    {
      m_outputSink << "<svg:defs>\n";
      m_outputSink << "  <svg:pattern id=\"img" << m_patternIndex++ << "\" patternUnits=\"userSpaceOnUse\" ";
      if (m_style["svg:width"])
        m_outputSink << "width=\"" << doubleToString(72*(m_style["svg:width"]->getDouble())) << "\" ";
      else
        m_outputSink << "width=\"100\" ";

      if (m_style["svg:height"])
        m_outputSink << "height=\"" << doubleToString(72*(m_style["svg:height"]->getDouble())) << "\">" << std::endl;
      else
        m_outputSink << "height=\"100\">" << std::endl;
      m_outputSink << "<svg:image ";

      if (m_style["svg:x"])
        m_outputSink << "x=\"" << doubleToString(72*(m_style["svg:x"]->getDouble())) << "\" ";
      else
        m_outputSink << "x=\"0\" ";

      if (m_style["svg:y"])
        m_outputSink << "y=\"" << doubleToString(72*(m_style["svg:y"]->getDouble())) << "\" ";
      else
        m_outputSink << "y=\"0\" ";

      if (m_style["svg:width"])
        m_outputSink << "width=\"" << doubleToString(72*(m_style["svg:width"]->getDouble())) << "\" ";
      else
        m_outputSink << "width=\"100\" ";

      if (m_style["svg:height"])
        m_outputSink << "height=\"" << doubleToString(72*(m_style["svg:height"]->getDouble())) << "\" ";
      else
        m_outputSink << "height=\"100\" ";

      m_outputSink << "xlink:href=\"data:" << m_style["libwpg:mime-type"]->getStr().cstr() << ";base64,";
      m_outputSink << m_style["draw:fill-image"]->getStr().cstr();
      m_outputSink << "\" />\n";
      m_outputSink << "  </svg:pattern>\n";
      m_outputSink << "</svg:defs>\n";
    }
  }
}

void KEYSVGGenerator::startLayer(const ::WPXPropertyList &propList)
{
  m_outputSink << "<svg:g id=\"Layer" << propList["svg:id"]->getInt() << "\"";
  if (propList["svg:fill-rule"])
    m_outputSink << " fill-rule=\"" << propList["svg:fill-rule"]->getStr().cstr() << "\"";
  m_outputSink << " >\n";
}

void KEYSVGGenerator::endLayer()
{
  m_outputSink << "</svg:g>\n";
}

void KEYSVGGenerator::startGroup(const ::WPXPropertyList &/*propList*/)
{
  // TODO: handle svg:id
  m_outputSink << "<svg:g>\n";
}

void KEYSVGGenerator::endGroup()
{
  m_outputSink << "</svg:g>\n";
}

void KEYSVGGenerator::drawRectangle(const ::WPXPropertyList &propList)
{
  m_outputSink << "<svg:rect ";
  m_outputSink << "x=\"" << doubleToString(72*propList["svg:x"]->getDouble()) << "\" y=\"" << doubleToString(72*propList["svg:y"]->getDouble()) << "\" ";
  m_outputSink << "width=\"" << doubleToString(72*propList["svg:width"]->getDouble()) << "\" height=\"" << doubleToString(72*propList["svg:height"]->getDouble()) << "\" ";
  if((propList["svg:rx"] && propList["svg:rx"]->getInt() !=0) || (propList["svg:ry"] && propList["svg:ry"]->getInt() !=0))
    m_outputSink << "rx=\"" << doubleToString(72*propList["svg:rx"]->getDouble()) << "\" ry=\"" << doubleToString(72*propList["svg:ry"]->getDouble()) << "\" ";
  writeStyle();
  m_outputSink << "/>\n";
}

void KEYSVGGenerator::drawEllipse(const WPXPropertyList &propList)
{
  m_outputSink << "<svg:ellipse ";
  m_outputSink << "cx=\"" << doubleToString(72*propList["svg:cx"]->getDouble()) << "\" cy=\"" << doubleToString(72*propList["svg:cy"]->getDouble()) << "\" ";
  m_outputSink << "rx=\"" << doubleToString(72*propList["svg:rx"]->getDouble()) << "\" ry=\"" << doubleToString(72*propList["svg:ry"]->getDouble()) << "\" ";
  writeStyle();
  if (propList["libwpg:rotate"] && propList["libwpg:rotate"]->getDouble() != 0.0)
    m_outputSink << " transform=\" translate(" << doubleToString(72*propList["svg:cx"]->getDouble()) << ", " << doubleToString(72*propList["svg:cy"]->getDouble())
                 << ") rotate(" << doubleToString(-propList["libwpg:rotate"]->getDouble())
                 << ") translate(" << doubleToString(-72*propList["svg:cx"]->getDouble())
                 << ", " << doubleToString(-72*propList["svg:cy"]->getDouble())
                 << ")\" ";
  m_outputSink << "/>\n";
}

void KEYSVGGenerator::drawPolyline(const ::WPXPropertyListVector &vertices)
{
  drawPolySomething(vertices, false);
}

void KEYSVGGenerator::drawPolygon(const ::WPXPropertyListVector &vertices)
{
  drawPolySomething(vertices, true);
}

void KEYSVGGenerator::drawPolySomething(const ::WPXPropertyListVector &vertices, bool isClosed)
{
  if(vertices.count() < 2)
    return;

  if(vertices.count() == 2)
  {
    m_outputSink << "<svg:line ";
    m_outputSink << "x1=\"" << doubleToString(72*(vertices[0]["svg:x"]->getDouble())) << "\"  y1=\"" << doubleToString(72*(vertices[0]["svg:y"]->getDouble())) << "\" ";
    m_outputSink << "x2=\"" << doubleToString(72*(vertices[1]["svg:x"]->getDouble())) << "\"  y2=\"" << doubleToString(72*(vertices[1]["svg:y"]->getDouble())) << "\"\n";
    writeStyle();
    m_outputSink << "/>\n";
  }
  else
  {
    if (isClosed)
      m_outputSink << "<svg:polygon ";
    else
      m_outputSink << "<svg:polyline ";

    m_outputSink << "points=\"";
    for(unsigned i = 0; i < vertices.count(); i++)
    {
      m_outputSink << doubleToString(72*(vertices[i]["svg:x"]->getDouble())) << " " << doubleToString(72*(vertices[i]["svg:y"]->getDouble()));
      if (i < vertices.count()-1)
        m_outputSink << ", ";
    }
    m_outputSink << "\"\n";
    writeStyle(isClosed);
    m_outputSink << "/>\n";
  }
}

void KEYSVGGenerator::drawPath(const ::WPXPropertyListVector &path)
{
  m_outputSink << "<svg:path d=\" ";
  bool isClosed = false;
  unsigned i=0;
  for(i=0; i < path.count(); i++)
  {
    WPXPropertyList propList = path[i];
    if (propList["libwpg:path-action"] && propList["libwpg:path-action"]->getStr() == "M")
    {
      m_outputSink << "\nM";
      m_outputSink << doubleToString(72*(propList["svg:x"]->getDouble())) << "," << doubleToString(72*(propList["svg:y"]->getDouble()));
    }
    else if (propList["libwpg:path-action"] && propList["libwpg:path-action"]->getStr() == "L")
    {
      m_outputSink << "\nL";
      m_outputSink << doubleToString(72*(propList["svg:x"]->getDouble())) << "," << doubleToString(72*(propList["svg:y"]->getDouble()));
    }
    else if (propList["libwpg:path-action"] && propList["libwpg:path-action"]->getStr() == "C")
    {
      m_outputSink << "\nC";
      m_outputSink << doubleToString(72*(propList["svg:x1"]->getDouble())) << "," << doubleToString(72*(propList["svg:y1"]->getDouble())) << " ";
      m_outputSink << doubleToString(72*(propList["svg:x2"]->getDouble())) << "," << doubleToString(72*(propList["svg:y2"]->getDouble())) << " ";
      m_outputSink << doubleToString(72*(propList["svg:x"]->getDouble())) << "," << doubleToString(72*(propList["svg:y"]->getDouble()));
    }
    else if (propList["libwpg:path-action"] && propList["libwpg:path-action"]->getStr() == "Q")
    {
      m_outputSink << "\nQ";
      m_outputSink << doubleToString(72*(propList["svg:x1"]->getDouble())) << "," << doubleToString(72*(propList["svg:y1"]->getDouble())) << " ";
      m_outputSink << doubleToString(72*(propList["svg:x"]->getDouble())) << "," << doubleToString(72*(propList["svg:y"]->getDouble()));
    }
    else if (propList["libwpg:path-action"] && propList["libwpg:path-action"]->getStr() == "A")
    {
      m_outputSink << "\nA";
      m_outputSink << doubleToString(72*(propList["svg:rx"]->getDouble())) << "," << doubleToString(72*(propList["svg:ry"]->getDouble())) << " ";
      m_outputSink << doubleToString(propList["libwpg:rotate"] ? propList["libwpg:rotate"]->getDouble() : 0) << " ";
      m_outputSink << (propList["libwpg:large-arc"] ? propList["libwpg:large-arc"]->getInt() : 1) << ",";
      m_outputSink << (propList["libwpg:sweep"] ? propList["libwpg:sweep"]->getInt() : 1) << " ";
      m_outputSink << doubleToString(72*(propList["svg:x"]->getDouble())) << "," << doubleToString(72*(propList["svg:y"]->getDouble()));
    }
    else if ((i >= path.count()-1 && i > 2) && propList["libwpg:path-action"] && propList["libwpg:path-action"]->getStr() == "Z" )
    {
      isClosed = true;
      m_outputSink << "\nZ";
    }
  }

  m_outputSink << "\" \n";
  writeStyle(isClosed);
  m_outputSink << "/>\n";
}

void KEYSVGGenerator::drawGraphicObject(const ::WPXPropertyList &propList, const ::WPXBinaryData &binaryData)
{
  if (!propList["libwpg:mime-type"] || propList["libwpg:mime-type"]->getStr().len() <= 0)
    return;
  WPXString base64 = binaryData.getBase64Data();
  m_outputSink << "<svg:image ";
  if (propList["svg:x"] && propList["svg:y"] && propList["svg:width"] && propList["svg:height"])
    m_outputSink << "x=\"" << doubleToString(72*(propList["svg:x"]->getDouble())) << "\" y=\"" << doubleToString(72*(propList["svg:y"]->getDouble())) << "\" ";
  m_outputSink << "width=\"" << doubleToString(72*(propList["svg:width"]->getDouble())) << "\" height=\"" << doubleToString(72*(propList["svg:height"]->getDouble())) << "\" ";
  m_outputSink << "xlink:href=\"data:" << propList["libwpg:mime-type"]->getStr().cstr() << ";base64,";
  m_outputSink << base64.cstr();
  m_outputSink << "\" />\n";
}

void KEYSVGGenerator::drawConnector(const ::WPXPropertyList &/*propList*/, const ::WPXPropertyListVector &/*path*/)
{
  // TODO: implement me
}

void KEYSVGGenerator::startTextObject(const ::WPXPropertyList &propList, const ::WPXPropertyListVector & /* path */)
{
  m_outputSink << "<svg:text ";
  if (propList["svg:x"] && propList["svg:y"])
    m_outputSink << "x=\"" << doubleToString(72*(propList["svg:x"]->getDouble())) << "\" y=\"" << doubleToString(72*(propList["svg:y"]->getDouble())) << "\"";
  if (propList["libwpg:rotate"] && propList["libwpg:rotate"]->getDouble() != 0.0)
    m_outputSink << " transform=\"translate(" << doubleToString(72*propList["svg:x"]->getDouble()) << ", " << doubleToString(72*propList["svg:y"]->getDouble())
                 << ") rotate(" << doubleToString(-propList["libwpg:rotate"]->getDouble())
                 << ") translate(" << doubleToString(-72*propList["svg:x"]->getDouble())
                 << ", " << doubleToString(-72*propList["svg:y"]->getDouble())
                 << ")\"";
  m_outputSink << ">\n";

}

void KEYSVGGenerator::endTextObject()
{
  m_outputSink << "</svg:text>\n";
}

void KEYSVGGenerator::openSpan(const ::WPXPropertyList &propList)
{
  m_outputSink << "<svg:tspan ";
  if (propList["style:font-name"])
    m_outputSink << "font-family=\"" << propList["style:font-name"]->getStr().cstr() << "\" ";
  if (propList["fo:font-style"])
    m_outputSink << "font-style=\"" << propList["fo:font-style"]->getStr().cstr() << "\" ";
  if (propList["fo:font-weight"])
    m_outputSink << "font-weight=\"" << propList["fo:font-weight"]->getStr().cstr() << "\" ";
  if (propList["fo:font-variant"])
    m_outputSink << "font-variant=\"" << propList["fo:font-variant"]->getStr().cstr() << "\" ";
  if (propList["fo:font-size"])
    m_outputSink << "font-size=\"" << doubleToString(propList["fo:font-size"]->getDouble()) << "\" ";
  if (propList["fo:color"])
    m_outputSink << "fill=\"" << propList["fo:color"]->getStr().cstr() << "\" ";
  if (propList["fo:text-transform"])
    m_outputSink << "text-transform=\"" << propList["fo:text-transform"]->getStr().cstr() << "\" ";
  if (propList["svg:fill-opacity"])
    m_outputSink << "fill-opacity=\"" << doubleToString(propList["svg:fill-opacity"]->getDouble()) << "\" ";
  if (propList["svg:stroke-opacity"])
    m_outputSink << "stroke-opacity=\"" << doubleToString(propList["svg:stroke-opacity"]->getDouble()) << "\" ";
  m_outputSink << ">\n";
}

void KEYSVGGenerator::closeSpan()
{
  m_outputSink << "</svg:tspan>\n";
}

void KEYSVGGenerator::insertTab()
{
  m_outputSink << '\t';
}

void KEYSVGGenerator::insertSpace()
{
  m_outputSink << ' ';
}

void KEYSVGGenerator::insertText(const ::WPXString &str)
{
  WPXString tempUTF8(str, true);
  m_outputSink << tempUTF8.cstr() << "\n";
}

void KEYSVGGenerator::insertLineBreak()
{
  m_outputSink << '\n';
}

void KEYSVGGenerator::insertField(const WPXString &/*type*/, const ::WPXPropertyList &/*propList*/)
{
  // TODO: implement me
}

void KEYSVGGenerator::openOrderedListLevel(const ::WPXPropertyList &)
{
}

void KEYSVGGenerator::openUnorderedListLevel(const ::WPXPropertyList &)
{
}

void KEYSVGGenerator::closeOrderedListLevel()
{
}

void KEYSVGGenerator::closeUnorderedListLevel()
{
}

void KEYSVGGenerator::openListElement(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &tabStops)
{
  openParagraph(propList, tabStops);
}

void KEYSVGGenerator::closeListElement()
{
  closeParagraph();
}

void KEYSVGGenerator::openParagraph(const ::WPXPropertyList &, const ::WPXPropertyListVector &)
{
}

void KEYSVGGenerator::closeParagraph()
{
  m_outputSink << '\n';
}

void KEYSVGGenerator::openTable(const ::WPXPropertyList &/*propList*/, const ::WPXPropertyListVector &/*columns*/)
{
  // TODO: implement me
}

void KEYSVGGenerator::openTableRow(const ::WPXPropertyList &/*propList*/)
{
  // TODO: implement me
}

void KEYSVGGenerator::closeTableRow()
{
  // TODO: implement me
}

void KEYSVGGenerator::openTableCell(const ::WPXPropertyList &/*propList*/)
{
  // TODO: implement me
}

void KEYSVGGenerator::closeTableCell()
{
  // TODO: implement me
}

void KEYSVGGenerator::insertCoveredTableCell(const ::WPXPropertyList &/*propList*/)
{
  // TODO: implement me
}

void KEYSVGGenerator::closeTable()
{
  // TODO: implement me
}


void KEYSVGGenerator::startComment(const ::WPXPropertyList &/*propList*/)
{
  // TODO: implement me
}

void KEYSVGGenerator::endComment()
{
  // TODO: implement me
}


void KEYSVGGenerator::startNotes(const ::WPXPropertyList &/*propList*/)
{
  // TODO: implement me
}

void KEYSVGGenerator::endNotes()
{
  // TODO: implement me
}

// create "style" attribute based on current pen and brush
void KEYSVGGenerator::writeStyle(bool /* isClosed */)
{
  m_outputSink << "style=\"";

  if (m_style["svg:stroke-width"])
  {
    double width = m_style["svg:stroke-width"]->getDouble();
    if (width == 0.0 && m_style["draw:stroke"] && m_style["draw:stroke"]->getStr() != "none")
      width = 0.2 / 72.0; // reasonable hairline
    m_outputSink << "stroke-width: " << doubleToString(72*width) << "; ";
  }
  if ((m_style["draw:stroke"] && m_style["draw:stroke"]->getStr() != "none"))
  {
    if (m_style["svg:stroke-color"])
      m_outputSink << "stroke: " << m_style["svg:stroke-color"]->getStr().cstr()  << "; ";
    if(m_style["svg:stroke-opacity"] &&  m_style["svg:stroke-opacity"]->getInt()!= 1)
      m_outputSink << "stroke-opacity: " << doubleToString(m_style["svg:stroke-opacity"]->getDouble()) << "; ";
  }

  if (m_style["draw:stroke"] && m_style["draw:stroke"]->getStr() == "solid")
    m_outputSink << "stroke-dasharray:  solid; ";
  else if (m_style["draw:stroke"] && m_style["draw:stroke"]->getStr() == "dash")
  {
    int dots1 = m_style["draw:dots1"]->getInt();
    int dots2 = m_style["draw:dots2"]->getInt();
    double dots1len = m_style["draw:dots1-length"]->getDouble();
    double dots2len = m_style["draw:dots2-length"]->getDouble();
    double gap = m_style["draw:distance"]->getDouble();
    m_outputSink << "stroke-dasharray: ";
    for (int i = 0; i < dots1; i++)
    {
      if (i)
        m_outputSink << ", ";
      m_outputSink << (int)dots1len;
      m_outputSink << ", ";
      m_outputSink << (int)gap;
    }
    for (int j = 0; j < dots2; j++)
    {
      m_outputSink << ", ";
      m_outputSink << (int)dots2len;
      m_outputSink << ", ";
      m_outputSink << (int)gap;
    }
    m_outputSink << "; ";
  }

  if (m_style["svg:stroke-linecap"])
    m_outputSink << "stroke-linecap: " << m_style["svg:stroke-linecap"]->getStr().cstr() << "; ";

  if (m_style["svg:stroke-linejoin"])
    m_outputSink << "stroke-linejoin: " << m_style["svg:stroke-linejoin"]->getStr().cstr() << "; ";

  if(m_style["draw:fill"] && m_style["draw:fill"]->getStr() == "none")
    m_outputSink << "fill: none; ";
  else if(m_style["svg:fill-rule"])
    m_outputSink << "fill-rule: " << m_style["svg:fill-rule"]->getStr().cstr() << "; ";

  if(m_style["draw:fill"] && m_style["draw:fill"]->getStr() == "gradient")
    m_outputSink << "fill: url(#grad" << m_gradientIndex-1 << "); ";

  if(m_style["draw:fill"] && m_style["draw:fill"]->getStr() == "bitmap")
    m_outputSink << "fill: url(#img" << m_patternIndex-1 << "); ";

  if(m_style["draw:shadow"] && m_style["draw:shadow"]->getStr() == "visible")
    m_outputSink << "filter:url(#shadow" << m_shadowIndex-1 << "); ";

  if(m_style["draw:fill"] && m_style["draw:fill"]->getStr() == "solid")
    if (m_style["draw:fill-color"])
      m_outputSink << "fill: " << m_style["draw:fill-color"]->getStr().cstr() << "; ";
  if(m_style["draw:opacity"] && m_style["draw:opacity"]->getDouble() < 1)
    m_outputSink << "fill-opacity: " << doubleToString(m_style["draw:opacity"]->getDouble()) << "; ";
  m_outputSink << "\""; // style
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */