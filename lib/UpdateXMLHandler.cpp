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

#include "UpdateXMLHandler.h"
#include "Log.h"
#include <stdlib.h>
#include <sstream>
#include "HTTPUtils.h"


using namespace std;

CXMLResdata::CXMLResdata()
{}

CXMLResdata::~CXMLResdata()
{}

CUpdateXMLHandler::CUpdateXMLHandler()
{};

CUpdateXMLHandler::~CUpdateXMLHandler()
{};

void CUpdateXMLHandler::LoadUpdXMLToMem (std::string rootDir, std::map<std::string, CXMLResdata> & mapResData)
{
  std::string UpdateXMLFilename = rootDir  + DirSepChar + "kodi-txupdate.xml";
  TiXmlDocument xmlUpdateXML;

  if (!xmlUpdateXML.LoadFile(UpdateXMLFilename.c_str()))
  {
    CLog::Log(logERROR, "UpdXMLHandler: No 'kodi-txupdate.xml' file exists in the specified project dir. Cannot continue. "
                        "Please create one!");
  }

  CLog::Log(logINFO, "UpdXMLHandler: Succesfuly found the update.xml file in the specified project directory");

  TiXmlElement* pProjectRootElement = xmlUpdateXML.RootElement();
  if (!pProjectRootElement || pProjectRootElement->NoChildren() || pProjectRootElement->ValueTStr()!="project")
  {
    CLog::Log(logERROR, "UpdXMLHandler: No root element called \"project\" in xml file. Cannot continue. Please create it");
  }

  TiXmlElement* pDataRootElement = pProjectRootElement->FirstChildElement("projectdata");
  if (!pDataRootElement || pDataRootElement->NoChildren())
  {
    CLog::Log(logERROR, "UpdXMLHandler: No element called \"projectdata\" in xml file. Cannot continue. Please create it");
  }

  TiXmlElement * pData;
  std::string strHTTPCacheExp;
  size_t iHTTPCacheExp = DEFAULTCACHEEXPIRE;
  if ((pData = pDataRootElement->FirstChildElement("http_cache_expire")) && (strHTTPCacheExp = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found http cache expire time in kodi-txupdate.xml file: %s", strHTTPCacheExp.c_str());
    iHTTPCacheExp = strtol(&strHTTPCacheExp[0], NULL, 10);
  }
  g_HTTPHandler.SetHTTPCacheExpire(iHTTPCacheExp);

//TODO separate download TX projectname from upload TX projectname to handle project name changes
  std::string strProjName;
  std::string strTargetProjName;
  if ((pData = pDataRootElement->FirstChildElement("projectname")) && (strProjName = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found projectname in kodi-txupdate.xml file: %s",strProjName.c_str());
    strTargetProjName = strProjName;
  }
  else
    CLog::Log(logERROR, "UpdXMLHandler: No projectname specified in kodi-txupdate.xml file. Cannot continue. "
    "Please specify the Transifex projectname in the xml file");

  if ((pData = pDataRootElement->FirstChildElement("targetprojectname")) && (strTargetProjName = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found target projectname in kodi-txupdate.xml file: %s",strTargetProjName.c_str());
    strTargetProjName = strTargetProjName;
  }

  std::string strLongProjName;
  if ((pData = pDataRootElement->FirstChildElement("longprojectname")) && (strLongProjName = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found long projectname in kodi-txupdate.xml file: %s",strLongProjName.c_str());
    strLongProjName = strLongProjName;
  }
  else
    CLog::Log(logERROR, "UpdXMLHandler: No long projectname specified in kodi-txupdate.xml file. Cannot continue. "
    "Please specify the Transifex long projectname in the xml file");

  std::string strTXLangFormat = DEFAULTTXLFORMAT;
  if ((pData = pDataRootElement->FirstChildElement("txlcode")) && (strTXLangFormat = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found tx langformat in kodi-txupdate.xml file: %s",strTXLangFormat.c_str());
  }

  std::string strTargetTXLangFormat = DEFAULTTXLFORMAT;
  if ((pData = pDataRootElement->FirstChildElement("targettxlcode")) && (strTargetTXLangFormat = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found target tx langformat in kodi-txupdate.xml file: %s",strTXLangFormat.c_str());
  }

  std::string strDefUPSAddonLangFormatinXML = DEFAULTLANGFORMATINADDONXML;
  if ((pData = pDataRootElement->FirstChildElement("addonxmllangformatupstream")) && (strDefUPSAddonLangFormatinXML = pData->FirstChild()->Value()) != "")
    CLog::Log(logINFO, "UpdXMLHandler: Found upstream addon.xml langformat in kodi-txupdate.xml file: %s",strDefUPSAddonLangFormatinXML.c_str());

  std::string strDefLOCAddonLangFormatinXML = DEFAULTLANGFORMATINADDONXML;
  if ((pData = pDataRootElement->FirstChildElement("addonxmllangformatlocal")) && (strDefLOCAddonLangFormatinXML = pData->FirstChild()->Value()) != "")
    CLog::Log(logINFO, "UpdXMLHandler: Found local addon.xml langformat in kodi-txupdate.xml file: %s",strDefLOCAddonLangFormatinXML.c_str());

  std::string strLangDatabaseURL = DEFAULTLANGDATABASELINK;
  if ((pData = pDataRootElement->FirstChildElement("langdatabaseurl")) && (strLangDatabaseURL = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found language database URL in kodi-txupdate.xml file: %s",strLangDatabaseURL.c_str());
    strLangDatabaseURL = strLangDatabaseURL;
  }

  std::string strBaseLcode = DEFAULTBASELCODE;
  if ((pData = pDataRootElement->FirstChildElement("baselcode")) && (strBaseLcode = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: found base language code format in kodi-txupdate.xml file: %s",strBaseLcode.c_str());
  }

  int iMinComplPercent = DEFAULTMINCOMPLETION;
  std::string strMinCompletion;
  if ((pData = pDataRootElement->FirstChildElement("min_completion")) && (strMinCompletion = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found min completion percentage in kodi-txupdate.xml file: %s", strMinCompletion.c_str());
    iMinComplPercent = strtol(&strMinCompletion[0], NULL, 10);
  }

  std::string strMergedLangfileDir = DEFAULTMERGEDLANGDIR;
  if ((pData = pDataRootElement->FirstChildElement("merged_langfiledir")) && (strMergedLangfileDir = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found merged language file directory in kodi-txupdate.xml file: %s", strMergedLangfileDir.c_str());
    strMergedLangfileDir= strMergedLangfileDir;
  }

  std::string strSourcelcode = DEFAULTSOURCELCODE;
  if ((pData = pDataRootElement->FirstChildElement("sourcelcode")) && (strSourcelcode = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found sourcelcode in kodi-txupdate.xml file: %s", strSourcelcode.c_str());
    strSourcelcode = strSourcelcode;
  }

  std::string strTXUpdatelangfileDir = DEFAULTTXUPDLANGDIR;
  if ((pData = pDataRootElement->FirstChildElement("temptxupdate_langfiledir")) && (strTXUpdatelangfileDir = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found temp tx update language file directory in kodi-txupdate.xml file: %s", strTXUpdatelangfileDir.c_str());
    strTXUpdatelangfileDir = strTXUpdatelangfileDir;
  }

  std::string strSupportEmailAdd = DEFAULTSUPPORTMAIL;
  if ((pData = pDataRootElement->FirstChildElement("support_email")) && (strSupportEmailAdd = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found support email address in kodi-txupdate.xml file: %s", strSupportEmailAdd.c_str());
   strSupportEmailAdd = strSupportEmailAdd;
  }

  std::string strAttr;
  bool bForcePOComments = false;
  if ((pData = pDataRootElement->FirstChildElement("forcePOComm")) && (strAttr = pData->FirstChild()->Value()) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Forced PO file comments for non English languages.", strMergedLangfileDir.c_str());
    bForcePOComments = true;
  }

  bool bRebrand =false;
  if ((pData = pDataRootElement->FirstChildElement("rebrand")) && (strAttr = pData->FirstChild()->Value()) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Rebrand of XBMC strings to Kodi strings turned on.");
    bRebrand = true;
  }

  std::string strLangteamLFormat = DEFAULTLANGTEAMLFORMAT;
  if ((pData = pDataRootElement->FirstChildElement("langteamformat")) && (strLangteamLFormat = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found language team format to put into PO files in kodi-txupdate.xml file: %s", strLangteamLFormat.c_str());
    strLangteamLFormat = strLangteamLFormat;
  }

  bool bForceTXUpdate = false;
  if ((pData = pDataRootElement->FirstChildElement("ForceTXUpd")) && (strAttr = pData->FirstChild()->Value()) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Create of TX update files is forced.");
    bForceTXUpdate = true;
  }

  TiXmlElement* pRootElement = pProjectRootElement->FirstChildElement("resources");
  if (!pRootElement || pRootElement->NoChildren())
  {
    CLog::Log(logERROR, "UpdXMLHandler: No element called \"resources\" in xml file. Cannot continue. Please create it");
  }

  const TiXmlElement *pChildResElement = pRootElement->FirstChildElement("resource");
  if (!pChildResElement || pChildResElement->NoChildren())
  {
    CLog::Log(logERROR, "UpdXMLHandler: No xml element called \"resource\" exists in the xml file. Cannot continue. Please create at least one");
  }

  std::string strType;
  while (pChildResElement && pChildResElement->FirstChild())
  {
    CXMLResdata currResData;
    currResData.strProjectName = strProjName;
    currResData.strTargetProjectName = strTargetProjName;
    currResData.strTargetProjectNameLong = strLongProjName;
    currResData.iMinComplPercent = iMinComplPercent;
    currResData.strMergedLangfileDir = strMergedLangfileDir;
    currResData.strTXUpdateLangfilesDir = strTXUpdatelangfileDir;
    currResData.bForceTXUpd = bForceTXUpdate;
    currResData.bRebrand = bRebrand;
    currResData.bForceComm = bForcePOComments;
    currResData.strLangteamLFormat = strLangteamLFormat;
    currResData.strSupportEmailAdd = strSupportEmailAdd;
    currResData.strSourceLcode = strSourcelcode;
    currResData.LangDatabaseURL = strLangDatabaseURL;
    currResData.strTargTXLFormat = strTargetTXLangFormat;
    currResData.strDefTXLFormat = strTargetTXLangFormat;
    currResData.strBaseLCode = strBaseLcode;

    std::string strResName;
    if (!pChildResElement->Attribute("name") || (strResName = pChildResElement->Attribute("name")) == "")
    {
      CLog::Log(logERROR, "UpdXMLHandler: No name specified for resource. Cannot continue. Please specify it.");
    }
    currResData.strResName =strResName;

    if (pChildResElement->FirstChild())
    {
      const TiXmlElement *pChildTXNameElement = pChildResElement->FirstChildElement("TXname");
      if (pChildTXNameElement && pChildTXNameElement->FirstChild())
      {
        currResData.strTXName = pChildTXNameElement->FirstChild()->Value();
        currResData.strTargetTXName =currResData.strTXName;
      }
      if (currResData.strTXName.empty())
        CLog::Log(logERROR, "UpdXMLHandler: Transifex resource name is empty or missing for resource %s", strResName.c_str());

      const TiXmlElement *pChildTTXNameElement = pChildResElement->FirstChildElement("TargetTXname");
      if (pChildTTXNameElement && pChildTTXNameElement->FirstChild())
        currResData.strTargetTXName = pChildTTXNameElement->FirstChild()->Value();

      const TiXmlElement *pChildURLElement = pChildResElement->FirstChildElement("upstreamLangURL");
      if (pChildURLElement && pChildURLElement->FirstChild())
        currResData.strUPSLangURL = pChildURLElement->FirstChild()->Value();
      if ((currResData.bHasOnlyAddonXML = currResData.strUPSLangURL.empty()))
        CLog::Log(logINFO, "UpdXMLHandler: UpstreamURL entry is empty for resource %s, which means we have no language files for this addon", strResName.c_str());
      else if (!GetParamsFromURLorPath (currResData.strUPSLangURL, currResData.strUPSLangFormat, currResData.strUPSLangFileName,
                                        currResData.strUPSLangURLRoot, '/'))
        CLog::Log(logERROR, "UpdXMLHandler: UpstreamURL format is wrong for resource %s", strResName.c_str());
      if (!currResData.strUPSLangURLRoot.empty() && currResData.strUPSLangURLRoot.find (".github") == std::string::npos)
        CLog::Log(logERROR, "UpdXMLHandler: Only github is supported as upstream repository for resource %s", strResName.c_str());

      const TiXmlElement *pChildURLSRCElement = pChildResElement->FirstChildElement("upstreamLangSRCURL");
      if (pChildURLSRCElement && pChildURLSRCElement->FirstChild())
        currResData.strUPSSourceLangURL = pChildURLSRCElement->FirstChild()->Value();

      const TiXmlElement *pChildURLSRCAddonElement = pChildResElement->FirstChildElement("upstreamAddonSRCURL");
      if (pChildURLSRCAddonElement && pChildURLSRCAddonElement->FirstChild())
        currResData.strUPSSourceLangAddonURL = pChildURLSRCAddonElement->FirstChild()->Value();

      const TiXmlElement *pChildAddonURLElement = pChildResElement->FirstChildElement("upstreamAddonURL");
      if (pChildAddonURLElement && pChildAddonURLElement->FirstChild())
        currResData.strUPSAddonURL = pChildAddonURLElement->FirstChild()->Value();
      if (currResData.strUPSAddonURL.empty())
        currResData.strUPSAddonURL = currResData.strUPSLangURL.substr(0,currResData.strUPSLangURL.find(currResData.strResName)
                                                                      + currResData.strResName.size()) + "/addon.xml";
      if (currResData.strUPSAddonURL.empty())
        CLog::Log(logERROR, "UpdXMLHandler: Unable to determine the URL for the addon.xml file for resource %s", strResName.c_str());
      GetParamsFromURLorPath (currResData.strUPSAddonURL, currResData.strUPSAddonLangFormat, currResData.strUPSAddonXMLFilename,
                                currResData.strUPSAddonURLRoot, '/');
      if (!currResData.strUPSAddonURL.empty() && currResData.strUPSAddonURL.find (".github") == std::string::npos)
          CLog::Log(logERROR, "UpdXMLHandler: Only github is supported as upstream repository for resource %s", strResName.c_str());
      std::string strUPSAddonLangFormatinXML;
      if (pChildAddonURLElement &&  pChildAddonURLElement->Attribute("addonxmllangformat"))
        strUPSAddonLangFormatinXML = pChildAddonURLElement->Attribute("addonxmllangformat");
      if (strUPSAddonLangFormatinXML != "")
        currResData.strUPSAddonLangFormatinXML = strUPSAddonLangFormatinXML;
      else
        currResData.strUPSAddonLangFormatinXML = strDefUPSAddonLangFormatinXML;
      currResData.bIsLanguageAddon = !currResData.strUPSAddonLangFormat.empty();


      const TiXmlElement *pChildChglogElement = pChildResElement->FirstChildElement("changelogFormat");
      if (pChildChglogElement && pChildChglogElement->FirstChild())
        currResData.strChangelogFormat = pChildChglogElement->FirstChild()->Value();

      const TiXmlElement *pChildChglogUElement = pChildResElement->FirstChildElement("upstreamChangelogURL");
      if (pChildChglogUElement && pChildChglogUElement->FirstChild())
        currResData.strUPSChangelogURL = pChildChglogUElement->FirstChild()->Value();
      else if (!currResData.strChangelogFormat.empty())
        currResData.strUPSChangelogURL = currResData.strUPSAddonURLRoot + "changelog.txt";
      if (!currResData.strChangelogFormat.empty())
      GetParamsFromURLorPath (currResData.strUPSChangelogURL, currResData.strUPSChangelogName,
                                currResData.strUPSChangelogURLRoot, '/');

//TODO if not defined automatic creation fails for language-addons
      const TiXmlElement *pChildLocLangElement = pChildResElement->FirstChildElement("localLangPath");
      if (pChildLocLangElement && pChildLocLangElement->FirstChild())
        currResData.strLOCLangPath = pChildLocLangElement->FirstChild()->Value();
      if (currResData.strLOCLangPath.empty() && !currResData.bHasOnlyAddonXML)
        currResData.strLOCLangPath = currResData.strUPSLangURL.substr(currResData.strUPSAddonURLRoot.size()-1);
      if (!currResData.bHasOnlyAddonXML)
        currResData.strLOCLangPath = currResData.strResName + DirSepChar + currResData.strLOCLangPath;
      if (!currResData.bHasOnlyAddonXML && !GetParamsFromURLorPath (currResData.strLOCLangPath, currResData.strLOCLangFormat,
          currResData.strLOCLangFileName, currResData.strLOCLangPathRoot, DirSepChar))
        CLog::Log(logERROR, "UpdXMLHandler: Local langpath format is wrong for resource %s", strResName.c_str());

      const TiXmlElement *pChildLocAddonElement = pChildResElement->FirstChildElement("localAddonPath");
      if (pChildLocAddonElement && pChildLocAddonElement->FirstChild())
        currResData.strLOCAddonPath = pChildLocAddonElement->FirstChild()->Value();
      if (currResData.strLOCAddonPath.empty())
        currResData.strLOCAddonPath = currResData.strUPSAddonXMLFilename;
      currResData.strLOCAddonPath = currResData.strResName + DirSepChar + currResData.strLOCAddonPath;
      GetParamsFromURLorPath (currResData.strLOCAddonPath, currResData.strLOCAddonLangFormat, currResData.strLOCAddonXMLFilename,
                              currResData.strLOCAddonPathRoot, DirSepChar);
      std::string strLOCAddonLangFormatinXML;
      if (pChildLocAddonElement && pChildLocAddonElement->Attribute("addonxmllangformat"))
        strLOCAddonLangFormatinXML = pChildLocAddonElement->Attribute("addonxmllangformat");
      if (strLOCAddonLangFormatinXML != "")
        currResData.strLOCAddonLangFormatinXML = strLOCAddonLangFormatinXML;
      else
        currResData.strLOCAddonLangFormatinXML = strDefLOCAddonLangFormatinXML;

      const TiXmlElement *pChildChglogLElement = pChildResElement->FirstChildElement("localChangelogPath");
      if (pChildChglogLElement && pChildChglogLElement->FirstChild())
        currResData.strLOCChangelogPath = pChildChglogLElement->FirstChild()->Value();
      else
	currResData.strLOCChangelogPath = currResData.strLOCAddonPathRoot + "changelog.txt";
      GetParamsFromURLorPath (currResData.strLOCChangelogPath, currResData.strLOCChangelogName,
                                currResData.strLOCChangelogPathRoot, '/');

      mapResData[strResName] = currResData;
      CLog::Log(logINFO, "UpdXMLHandler: found resource in update.xml file: %s, Type: %s, SubDir: %s",
                strResName.c_str(), strType.c_str(), currResData.strLOCAddonPath.c_str());
    }
    pChildResElement = pChildResElement->NextSiblingElement("resource");
  }

  return;
};

bool CUpdateXMLHandler::GetParamsFromURLorPath (string const &strURL, string &strLangFormat, string &strFileName,
                                                 string &strURLRoot, const char strSeparator)
{
  if (strURL.empty())
    return false;

  size_t pos0, posStart, posEnd;

  pos0 = strURL.find_last_of("$");
  if (((posStart = strURL.find("$("), pos0) != std::string::npos) && ((posEnd = strURL.find(")",posStart)) != std::string::npos))
    strLangFormat = strURL.substr(posStart, posEnd - posStart +1);

  return GetParamsFromURLorPath (strURL, strFileName, strURLRoot, strSeparator);
}

bool CUpdateXMLHandler::GetParamsFromURLorPath (string const &strURL, string &strFileName,
                                                 string &strURLRoot, const char strSeparator)
{
  if (strURL.empty())
    return false;

  if (strURL.find(strSeparator) == std::string::npos)
    return false;

  strFileName = strURL.substr(strURL.find_last_of(strSeparator)+1);
  strURLRoot = g_CharsetUtils.GetRoot(strURL, strFileName);
  return true;
}