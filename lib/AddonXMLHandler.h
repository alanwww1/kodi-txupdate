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

#include "POHandler.h"
#include "TinyXML/tinyxml.h"
#include "UpdateXMLHandler.h"
#include <set>

struct COtherAddonMetadata
{
  std::string strLanguage;
  std::string strPlatform;
  std::string strLicense;
  std::string strForum;
  std::string strWebsite;
  std::string strEmail;
  std::string strSource;
};

class CAddonXMLHandler
{
public:
  CAddonXMLHandler();
  ~CAddonXMLHandler();
  void SetXMLReasData (const CXMLResdata& XMLResData) {m_XMLResData = XMLResData;}
  bool UpdateAddonXMLFile (std::string strAddonXMLFilename, bool bUpdateVersion);
  bool UpdateAddonChangelogFile (std::string strFilename, std::string strFormat, bool bUpdate);
  bool FetchAddonChangelogFile ();
  void FetchAddonDataFiles();
  std::string GetResHeaderPretext () const {return m_strResourceData;}
 // std::map<std::string, CAddonXMLEntry> * GetMapAddonXMLData () {return &m_mapAddonXMLData;}
  const std::map<std::string, CAddonXMLEntry>& GetMapAddonXMLData () {return m_mapAddonXMLData;}
  void AddAddonXMLLangsToList(std::set<std::string>& listLangs);
  void SetMapAddonXMLData (const std::map<std::string, CAddonXMLEntry>& mapData) {m_mapAddonXMLData = mapData;}
  std::string GetStrAddonXMLFile() const {return m_strAddonXMLFile;}
  void SetAddonXMLEntry (const CAddonXMLEntry& AddonXMLEntry, const std::string& sLang) {m_mapAddonXMLData[sLang] = AddonXMLEntry;}
  const CAddonXMLEntry& GetAddonXMLEntry(const std::string& sLang) const {return m_mapAddonXMLData.at(sLang);}
  bool FindAddonXMLEntry(const std::string& sLang) const {return m_mapAddonXMLData.find(sLang) != m_mapAddonXMLData.end();}

  void SetStrAddonXMLFile(std::string const &strAddonXMLFile) {m_strAddonXMLFile = strAddonXMLFile;}
  std::string GetAddonVersion () const {return m_strAddonVersion;}
  void SetAddonVersion(std::string const &strAddonVersion) {m_strAddonVersion = strAddonVersion;}
  std::string GetAddonChangelogFile () const {return m_strChangelogFile;}
  void SetAddonChangelogFile(std::string const &strAddonChangelogFile) {m_strChangelogFile = strAddonChangelogFile;}
  std::string GetAddonLogFilename () const {return m_strLogFilename;}
  void SetAddonLogFilename(std::string const &strAddonLogFilename) {m_strLogFilename = strAddonLogFilename;}
  COtherAddonMetadata GetAddonMetaData () const {return m_AddonMetadata;}
  void SetAddonMetadata(COtherAddonMetadata const &MetaData) {m_AddonMetadata = MetaData;}

protected:
  bool FetchAddonXMLFileUpstr ();

  bool GetEncoding(const TiXmlDocument* pDoc, std::string& strEncoding);
  void BumpVersionNumber();
  void UpdateVersionNumber();
  void ParseAddonXMLVersionGITHUB(const std::string &strJSON);
  std::string CstrToString(const char * StrToEscape);
  std::string GetXMLEntry (std::string const &strprefix, size_t &pos1, size_t &pos2);
  void CleanWSBetweenXMLEntries (std::string &strXMLString);
  std::map<std::string, CAddonXMLEntry> m_mapAddonXMLData;
typedef std::map<std::string, CAddonXMLEntry>::iterator T_itAddonXMLData;
  std::string m_strResourceData;
  std::string m_strAddonXMLFile;
  std::string m_strAddonVersion;
  std::string m_strChangelogFile;
  std::string m_strLogFilename;
  COtherAddonMetadata m_AddonMetadata;
  CXMLResdata m_XMLResData;
};
