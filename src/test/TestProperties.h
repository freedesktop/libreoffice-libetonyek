/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "IWORKPropertyInfo.h"

namespace libetonyek
{

namespace property
{

struct Answer {};
struct Antwort {};

}

template<>
struct IWORKPropertyInfo<property::Answer>
{
  typedef int ValueType;
  static const IWORKPropertyID_t id;
};

template<>
struct IWORKPropertyInfo<property::Antwort>
{
  typedef int ValueType;
  static const IWORKPropertyID_t id;
};

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
