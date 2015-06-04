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

#include "ResourceHandler.h"
#include "UpdateXMLHandler.h"
#include <list>


class CProjectHandler
{
public:
  CProjectHandler();
  ~CProjectHandler();
  void SetProjectDir (std::string const &strDir) {m_strProjDir = strDir;}
  void LoadUpdXMLToMem();
  bool FetchResourcesFromTransifex();
  bool FetchResourcesFromUpstream();
  bool CreateMergedResources();
  bool WriteResourcesToFile(std::string strProjRootDir);
  void UploadTXUpdateFiles(std::string strProjRootDir);
  void MigrateTranslators();
  void InitLCodeHandler();

protected:
  const CPOEntry * SafeGetPOEntry(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                                  std::string &strLangCode, size_t numID);
  const CPOEntry * SafeGetPOEntry(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                                  std::string &strLangCode, CPOEntry const &currPOEntryEN);
  CPOHandler * SafeGetPOHandler(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                                std::string &strLangCode);
  std::list<std::string> CreateMergedLanguageList(std::string strResname);
  std::map<std::string, CResourceHandler> * ChoosePrefResMap(std::string strResname);
  std::list<std::string> CreateResourceList();
  CAddonXMLEntry * const GetAddonDataFromXML(std::map<std::string, CResourceHandler> * pmapRes,
                                             const std::string &strResname, const std::string &strLangCode) const;
  void MergeAddonXMLEntry(CAddonXMLEntry const &EntryToMerge, CAddonXMLEntry &MergedAddonXMLEntry,
                                           CAddonXMLEntry const &SourceENEntry, CAddonXMLEntry const &CurrENEntry, bool UpstrToMerge,
                                           bool &bResChangedFromUpstream);
  bool FindResInList(std::list<std::string> const &listResourceNamesTX, std::string strTXResName);
  std::list<std::string> GetLangsFromDir(std::string const &strLangDir);
  void CheckPOEntrySyntax(const CPOEntry * pPOEntry, std::string const &strLangCode, const CPOEntry * pcurrPOEntryEN,
                          const CXMLResdata& XMLResData);
  std::string GetEntryContent(const CPOEntry * pPOEntry, std::string const &strLangCode, const CXMLResdata& XMLResData);
  void CheckCharCount(const CPOEntry * pPOEntry, std::string const &strLangCode, const CPOEntry * pcurrPOEntryEN, char chrToCheck,
                      const CXMLResdata& XMLResData);
  void PrintChangedLangs(std::list<std::string> lChangedLangs);
  std::string GetResNameFromTXResName(std::string const &strTXResName);

  std::map<std::string, CResourceHandler> m_mapResourcesTX;
  std::map<std::string, CResourceHandler> m_mapResourcesUpstr;

  std::map<std::string, CResourceHandler> m_mapResMerged;
  std::map<std::string, CResourceHandler> m_mapResUpdateTX;
  typedef std::map<std::string, CResourceHandler>::iterator T_itmapRes;
  std::map<std::string, std::string> m_mapResourceNamesTX;
  int m_resCount;

  std::string m_strProjDir;
  std::map<std::string, CXMLResdata> m_mapResData;
  typedef std::map<std::string, CXMLResdata>::iterator T_itResData;
};
