/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <string.h>

#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>
#include <librevenge/librevenge.h>
#include <libetonyek/libetonyek.h>

#ifndef VERSION
#define VERSION "UNKNOWN VERSION"
#endif

#define TOOL "key2xhtml"

namespace
{

int printUsage()
{
  printf("`" TOOL "' converts Apple Keynote presentations to SVG.\n");
  printf("\n");
  printf("Usage: " TOOL " [OPTION] INPUT\n");
  printf("\n");
  printf("Options:\n");
  printf("\t--help                show this help message\n");
  printf("\t--version             show version information\n");
  printf("\n");
  printf("Report bugs to <https://bugs.documentfoundation.org/>.\n");
  return -1;
}

int printVersion()
{
  printf(TOOL " " VERSION "\n");
  return 0;
}

} // anonymous namespace

int main(int argc, char *argv[]) try
{
  if (argc < 2)
    return printUsage();

  char *file = nullptr;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "--version"))
      return printVersion();
    else if (!file && strncmp(argv[i], "--", 2))
      file = argv[i];
    else
      return printUsage();
  }

  if (!file)
    return printUsage();

  using std::shared_ptr;
  using libetonyek::EtonyekDocument;

  shared_ptr<librevenge::RVNGInputStream> input;
  if (librevenge::RVNGDirectoryStream::isDirectory(file))
    input.reset(new librevenge::RVNGDirectoryStream(file));
  else
    input.reset(new librevenge::RVNGFileStream(file));

  EtonyekDocument::Type type = EtonyekDocument::TYPE_UNKNOWN;
  const EtonyekDocument::Confidence confidence = EtonyekDocument::isSupported(input.get(), &type);
  if ((EtonyekDocument::CONFIDENCE_NONE == confidence) || (EtonyekDocument::TYPE_KEYNOTE != type))
  {
    std::cerr << "ERROR: Unsupported file format!" << std::endl;
    return 1;
  }

  if (EtonyekDocument::CONFIDENCE_SUPPORTED_PART == confidence)
    input.reset(librevenge::RVNGDirectoryStream::createForParent(file));

  librevenge::RVNGStringVector output;
  librevenge::RVNGSVGPresentationGenerator generator(output);
  if (!EtonyekDocument::parse(input.get(), &generator))
  {
    std::cerr << "ERROR: SVG Generation failed!" << std::endl;
    return 1;
  }

  if (output.empty())
  {
    std::cerr << "ERROR: No SVG document generated!" << std::endl;
    return 1;
  }

  std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
  std::cout << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << std::endl;
  std::cout << "<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">" << std::endl;
  std::cout << "<body>" << std::endl;
  std::cout << "<?import namespace=\"svg\" urn=\"http://www.w3.org/2000/svg\"?>" << std::endl;

  for (unsigned k = 0; k<output.size(); ++k)
  {
    if (k>0)
      std::cout << "<hr/>\n";

    std::cout << "<!-- \n";
    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    std::cout << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"";
    std::cout << " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    std::cout << " -->\n";

    std::cout << output[k].cstr() << std::endl;
  }

  std::cout << "</body>" << std::endl;
  std::cout << "</html>" << std::endl;

  return 0;
}
catch (...)
{
  fprintf(stderr, "ERROR: uncaught exception!\n");
  return 1;
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
