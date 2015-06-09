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
#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include "../Log.h"
#include "../Langcodes.h"
#include "../FileUtils/FileUtils.h"
#include "../CharsetUtils/CharsetUtils.h"
// #include "../UpdateXMLHandler.h"

enum
{
  ID_FOUND = 200, // We have an entry with a numeric (previously XML) identification number.
  MSGID_FOUND = 201, // We have a classic gettext entry with textual msgid. No numeric ID.
  MSGID_PLURAL_FOUND = 202, // We have a classic gettext entry with textual msgid in plural form.
  COMMENT_ENTRY_FOUND = 203, // We have a separate comment entry
  HEADER_FOUND = 204, // We have a header entry
  UNKNOWN_FOUND = 205 // Unknown entrytype found
};

enum Boolean
{
  ISSOURCELANG=true
};

struct CAddonXMLEntry
{
  std::string strSummary;
  std::string strDescription;
  std::string strDisclaimer;
};

// Struct to collect all important data of the current processed entry.
class CPOEntry
{
public:
  CPOEntry();
  ~CPOEntry();
  int Type;
  uint32_t numID;
  std::string msgCtxt;
  std::string msgID;
  std::string msgIDPlur;
  std::string msgStr;
  std::vector<std::string> msgStrPlural;
  std::vector<std::string> extractedComm;   // #. extracted comment
  std::vector<std::string> referenceComm;   // #: reference
  std::vector<std::string> translatorComm;  // # translator comment
  std::vector<std::string> interlineComm;   // #comment between lines
  std::string Content;
  bool operator == (const CPOEntry &poentry) const;
};

