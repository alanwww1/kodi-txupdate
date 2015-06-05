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

#include "JSONHandler.h"
#include "Log.h"
#include <list>
#include <algorithm>
#include <stdlib.h>
#include "Fileversioning.h"
#include "CharsetUtils/CharsetUtils.h"

CJSONHandler g_Json;

using namespace std;

CJSONHandler::CJSONHandler()
{};

CJSONHandler::~CJSONHandler()
{};





std::string CJSONHandler::CreateJSONStrFromPOStr(std::string const &strPO)
{
  Json::Value root;
  root["content"] = strPO;
  root["mimetype"] = std::string("text/x-po");
  Json::StyledWriter writer;
  std::string strJSON = writer.write(root);
  return strJSON;
};

std::string CJSONHandler::CreateNewresJSONStrFromPOStr(std::string strTXResname, std::string const &strPO)
{
  Json::Value root;
  root["content"] = strPO;
  root["slug"] = std::string(strTXResname);
  root["name"] = std::string(strTXResname);
  root["i18n_type"] = std::string("PO");
  Json::StyledWriter writer;
  std::string strJSON = writer.write(root);
  return strJSON;
};

void CJSONHandler::ParseUploadedStringsData(std::string const &strJSON, size_t &stradded, size_t &strupd)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStringsData: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root.get("strings_added", 0).asInt();
  strupd = root.get("strings_updated", 0).asInt();
  return;
};

void CJSONHandler::ParseUploadedStrForNewRes(std::string const &strJSON, size_t &stradded)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStrForNewRes: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root[0].asInt();

  return;
};

void CJSONHandler::ParseAddonXMLVersionGITHUB(const std::string &strJSON, const std::string &strURL, const std::string &strAddXMLFilename, const std::string &strChlogname)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string strName, strVersion;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
    CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid JSON data downloaded from Github");

  const Json::Value JFiles = root;

  for(Json::ValueIterator itr = JFiles.begin() ; itr !=JFiles.end() ; itr++)
  {
    Json::Value JValu = *itr;
    std::string strType =JValu.get("type", "unknown").asString();

    if (strType == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid JSON data downloaded from Github");

    strName =JValu.get("name", "unknown").asString();

    if (strName == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid JSON data downloaded from Github");

    if (strType == "file" && ((!strAddXMLFilename.empty() && strName == strAddXMLFilename) ||
        (!strChlogname.empty() && strName == strChlogname)))
    {
      strVersion =JValu.get("sha", "unknown").asString();

      if (strVersion == "unknown")
        CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid sha JSON data downloaded from Github");

      g_Fileversion.SetVersionForURL(strURL + strName, strVersion);
    }
  };
};

void CJSONHandler::ParseLangDatabaseVersion(const std::string &strJSON, const std::string &strURL)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string strName, strVersion;

  std::string strDatabaseFilename = strURL.substr(strURL.rfind("/")+1,std::string::npos);

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
    CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid JSON data downloaded from Github");

  const Json::Value JFiles = root;

  for(Json::ValueIterator itr = JFiles.begin() ; itr !=JFiles.end() ; itr++)
  {
    Json::Value JValu = *itr;
    std::string strType =JValu.get("type", "unknown").asString();

    if (strType == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseLangDatabaseVersion: no valid JSON data downloaded from Github");

    strName =JValu.get("name", "unknown").asString();

    if (strName == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseLangDatabaseVersion: no valid JSON data downloaded from Github");

    if (strType == "file" && strName == strDatabaseFilename)
    {
      strVersion =JValu.get("sha", "unknown").asString();

      if (strVersion == "unknown")
        CLog::Log(logERROR, "CJSONHandler::ParseLangDatabaseVersion: no valid sha JSON data downloaded from Github");

      g_Fileversion.SetVersionForURL(strURL, strVersion);
    }
  };
};
