/*
 *      Copyright (C) 2014 Team Kodi
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "POUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include "../HTTPUtils.h"

using namespace std;

CPOEntry::CPOEntry()
{
  Type = UNKNOWN_FOUND;
}

CPOEntry::~CPOEntry()
{}


bool CPOEntry::operator==(const CPOEntry& poentry) const
{
  bool bhasMatch = true;
  if (!poentry.msgCtxt.empty())
    bhasMatch = bhasMatch && (poentry.msgCtxt == msgCtxt);
  if (!poentry.msgID.empty())
    bhasMatch = bhasMatch && (poentry.msgID == msgID);
  if (!poentry.msgIDPlur.empty())
    bhasMatch = bhasMatch && (poentry.msgIDPlur == msgIDPlur);
  if (!poentry.msgStr.empty())
    bhasMatch = bhasMatch && (poentry.msgStr == msgStr);
  if (!poentry.msgStrPlural.empty())
    bhasMatch = bhasMatch && (poentry.msgStrPlural == msgStrPlural);
//  if (!poentry.Type == ID_FOUND)
//    bhasMatch = bhasMatch && (poentry.numID == numID);
  if (poentry.Type != UNKNOWN_FOUND && poentry.Type != 0)
    bhasMatch = bhasMatch && (poentry.Type == Type);
  return bhasMatch;
};

