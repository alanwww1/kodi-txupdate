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
#include "ConfigHandler.h"
#include <list>


class CProjectHandler
{
public:
  CProjectHandler();
  ~CProjectHandler();
  void SetProjectDir (std::string const &strDir) {m_strProjDir = strDir;}
  void LoadConfigToMem();
  bool FetchResourcesFromTransifex();
  bool FetchResourcesFromUpstream();
  bool CreateMergedResources();
  bool WriteResourcesToFile(std::string strProjRootDir);
  bool WriteResourcesToLOCGitRepos(std::string strProjRootDir);
  void UploadTXUpdateFiles(std::string strProjRootDir);
  void MigrateTranslators();
  void InitLCodeHandler();

protected:
  std::string GetResNameFromTXResName(std::string const &strTXResName);

  std::set<std::string> ParseResources(std::string strJSON);

  std::map<std::string, CResourceHandler> m_mapResources;
  typedef std::map<std::string, CResourceHandler>::iterator T_itmapRes;

  std::map<std::string, CResData> m_mapResData;
  typedef std::map<std::string, CResData>::iterator T_itResData;
  std::map<std::string, CBasicGITData> m_MapGitRepos;
  std::map<int, std::string> m_mapResOrder;
  std::string m_strProjDir;
};
