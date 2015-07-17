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
#include "CharsetUtils/CharsetUtils.h"
#include "FileUtils/FileUtils.h"


using namespace std;

CXMLResdata::CXMLResdata()
{}

CXMLResdata::~CXMLResdata()
{}

CUpdateXMLHandler::CUpdateXMLHandler()
{};

CUpdateXMLHandler::~CUpdateXMLHandler()
{};
/*
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
    currResData.sProjRootDir = rootDir;
    currResData.strProjectName = strProjName;
    currResData.UPD.ProjectName = strTargetProjName;
    currResData.UPD.ProjectNameLong = strLongProjName;
    currResData.iMinComplPercent = iMinComplPercent;
    currResData.sMRGLFilesDir = strMergedLangfileDir;
    currResData.sUPDLFilesDir = strTXUpdatelangfileDir;
    currResData.bForceTXUpd = bForceTXUpdate;
    currResData.bRebrand = bRebrand;
    currResData.bForceComm = bForcePOComments;
    currResData.sLTeamLFormat = strLangteamLFormat;
    currResData.sSupportEmailAddr = strSupportEmailAdd;
    currResData.sSRCLCode = strSourcelcode;
    currResData.sLDatabaseURL = strLangDatabaseURL;
    currResData.strTargTXLFormat = strTargetTXLangFormat;
    currResData.strDefTXLFormat = strTargetTXLangFormat;
    currResData.sBaseLCode = strBaseLcode;

    std::string strResName;
    if (!pChildResElement->Attribute("name") || (strResName = pChildResElement->Attribute("name")) == "")
    {
      CLog::Log(logERROR, "UpdXMLHandler: No name specified for resource. Cannot continue. Please specify it.");
    }
    currResData.sResName =strResName;

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
        currResData.UPS.LURL = pChildURLElement->FirstChild()->Value();
      if ((currResData.bHasOnlyAddonXML = currResData.UPS.LURL.empty()))
        CLog::Log(logINFO, "UpdXMLHandler: UpstreamURL entry is empty for resource %s, which means we have no language files for this addon", strResName.c_str());
      else if (!GetParamsFromURLorPath (currResData.UPS.LURL, currResData.UPS.LForm, currResData.UPS.LFileName,
                                        currResData.strUPSLangURLRoot, '/'))
        CLog::Log(logERROR, "UpdXMLHandler: UpstreamURL format is wrong for resource %s", strResName.c_str());
      if (!currResData.strUPSLangURLRoot.empty() && currResData.strUPSLangURLRoot.find (".github") == std::string::npos)
        CLog::Log(logERROR, "UpdXMLHandler: Only github is supported as upstream repository for resource %s", strResName.c_str());

      const TiXmlElement *pChildURLSRCElement = pChildResElement->FirstChildElement("upstreamLangSRCURL");
      if (pChildURLSRCElement && pChildURLSRCElement->FirstChild())
        currResData.UPSSRC.LURL = pChildURLSRCElement->FirstChild()->Value();

      const TiXmlElement *pChildURLSRCAddonElement = pChildResElement->FirstChildElement("upstreamAddonSRCURL");
      if (pChildURLSRCAddonElement && pChildURLSRCAddonElement->FirstChild())
        currResData.UPSSRC.AXMLURL = pChildURLSRCAddonElement->FirstChild()->Value();

      const TiXmlElement *pChildAddonURLElement = pChildResElement->FirstChildElement("upstreamAddonURL");
      if (pChildAddonURLElement && pChildAddonURLElement->FirstChild())
        currResData.UPS.AXMLURL = pChildAddonURLElement->FirstChild()->Value();
      if (currResData.UPS.AXMLURL.empty())
        currResData.UPS.AXMLURL = currResData.UPS.LURL.substr(0,currResData.UPS.LURL.find(currResData.sResName)
                                                                      + currResData.sResName.size()) + "/addon.xml";
      if (currResData.UPS.AXMLURL.empty())
        CLog::Log(logERROR, "UpdXMLHandler: Unable to determine the URL for the addon.xml file for resource %s", strResName.c_str());
      GetParamsFromURLorPath (currResData.UPS.AXMLURL, currResData.UPS.ALForm, currResData.UPS.AXMLFileName,
                                currResData.UPS.AXMLURLRoot, '/');
      if (!currResData.UPS.AXMLURL.empty() && currResData.UPS.AXMLURL.find (".github") == std::string::npos)
          CLog::Log(logERROR, "UpdXMLHandler: Only github is supported as upstream repository for resource %s", strResName.c_str());
      std::string strUPSAddonLangFormatinXML;
      if (pChildAddonURLElement &&  pChildAddonURLElement->Attribute("addonxmllangformat"))
        strUPSAddonLangFormatinXML = pChildAddonURLElement->Attribute("addonxmllangformat");
      if (strUPSAddonLangFormatinXML != "")
        currResData.strUPSAddonLangFormatinXML = strUPSAddonLangFormatinXML;
      else
        currResData.strUPSAddonLangFormatinXML = strDefUPSAddonLangFormatinXML;
      currResData.bIsLanguageAddon = !currResData.UPS.ALForm.empty();


      const TiXmlElement *pChildChglogElement = pChildResElement->FirstChildElement("changelogFormat");
      if (pChildChglogElement && pChildChglogElement->FirstChild())
        currResData.sChgLogFormat = pChildChglogElement->FirstChild()->Value();

      const TiXmlElement *pChildChglogUElement = pChildResElement->FirstChildElement("upstreamChangelogURL");
      if (pChildChglogUElement && pChildChglogUElement->FirstChild())
        currResData.UPS.ChLogURL = pChildChglogUElement->FirstChild()->Value();
      else if (!currResData.sChgLogFormat.empty())
        currResData.UPS.ChLogURL = currResData.UPS.AXMLURLRoot + "changelog.txt";
      if (!currResData.sChgLogFormat.empty())
      GetParamsFromURLorPath (currResData.UPS.ChLogURL, currResData.strUPSChangelogName,
                                currResData.strUPSChangelogURLRoot, '/');

//TODO if not defined automatic creation fails for language-addons
      const TiXmlElement *pChildLocLangElement = pChildResElement->FirstChildElement("localLangPath");
      if (pChildLocLangElement && pChildLocLangElement->FirstChild())
        currResData.LOC.LURL = pChildLocLangElement->FirstChild()->Value();
      if (currResData.LOC.LURL.empty() && !currResData.bHasOnlyAddonXML)
        currResData.LOC.LURL = currResData.UPS.LURL.substr(currResData.UPS.AXMLURLRoot.size()-1);
      if (!currResData.bHasOnlyAddonXML)
        currResData.LOC.LURL = currResData.sResName + DirSepChar + currResData.LOC.LURL;
      if (!currResData.bHasOnlyAddonXML && !GetParamsFromURLorPath (currResData.LOC.LURL, currResData.strLOCLangFormat,
          currResData.strLOCLangFileName, currResData.strLOCLangPathRoot, DirSepChar))
        CLog::Log(logERROR, "UpdXMLHandler: Local langpath format is wrong for resource %s", strResName.c_str());

      const TiXmlElement *pChildLocAddonElement = pChildResElement->FirstChildElement("localAddonPath");
      if (pChildLocAddonElement && pChildLocAddonElement->FirstChild())
        currResData.LOC.AXMLURL = pChildLocAddonElement->FirstChild()->Value();
      if (currResData.LOC.AXMLURL.empty())
        currResData.LOC.AXMLURL = currResData.UPS.AXMLFileName;
      currResData.LOC.AXMLURL = currResData.sResName + DirSepChar + currResData.LOC.AXMLURL;
      GetParamsFromURLorPath (currResData.LOC.AXMLURL, currResData.strLOCAddonLangFormat, currResData.LOC.AXMLFileName,
                              currResData.strLOCAddonPathRoot, DirSepChar);
      std::string strLOCAddonLangFormatinXML;
      if (pChildLocAddonElement && pChildLocAddonElement->Attribute("addonxmllangformat"))
        strLOCAddonLangFormatinXML = pChildLocAddonElement->Attribute("addonxmllangformat");
      if (strLOCAddonLangFormatinXML != "")
        currResData.LOC.LFormInAXML = strLOCAddonLangFormatinXML;
      else
        currResData.LOC.LFormInAXML = strDefLOCAddonLangFormatinXML;

      const TiXmlElement *pChildChglogLElement = pChildResElement->FirstChildElement("localChangelogPath");
      if (pChildChglogLElement && pChildChglogLElement->FirstChild())
        currResData.LOC.ChLogURL = pChildChglogLElement->FirstChild()->Value();
      else
	currResData.LOC.ChLogURL = currResData.strLOCAddonPathRoot + "changelog.txt";
      GetParamsFromURLorPath (currResData.LOC.ChLogURL, currResData.strLOCChangelogName,
                                currResData.strLOCChangelogPathRoot, '/');

      mapResData[strResName] = currResData;
      CLog::Log(logINFO, "UpdXMLHandler: found resource in update.xml file: %s, Type: %s, SubDir: %s",
                strResName.c_str(), strType.c_str(), currResData.LOC.AXMLURL.c_str());
    }
    pChildResElement = pChildResElement->NextSiblingElement("resource");
  }

  return;
};
*/

void CUpdateXMLHandler::SubstituteExternalVariables(std::string& sVal)
{
  size_t iCurrPos = 0;
  size_t iNextPos = 0;
  while ((iNextPos = sVal.find('$', iCurrPos)) != std::string::npos)
  {
    // If the char is at the end of string, or if it is an internal var like $(LCODE), skip
    if (iNextPos + 1 == sVal.size() || sVal.at(iNextPos +1) == '(')
    {
      iCurrPos = iNextPos +1;
      continue;
    }
    size_t iVarLength = 1;
    while (iVarLength + iNextPos < sVal.size())
    {
      size_t iMatchedEntries = FindVariable(sVal.substr(iNextPos+1, iVarLength));
      if (iMatchedEntries == std::string::npos)
        CLog::Log(logERROR, "Undefined variable in value: %s", sVal.c_str());
      if (iMatchedEntries == 0)
        break;
      iVarLength++;
    }

    if ((iVarLength + iNextPos) == sVal.size())
      CLog::Log(logERROR, "Undefined variable in value: %s", sVal.c_str());

    std::string sVarToReplace = sVal.substr(iNextPos+1, iVarLength);
    std::string sReplaceString = m_MapOfVariables.at(sVarToReplace);
    sVal.replace(iNextPos,iVarLength +1, sReplaceString);

    iCurrPos = iNextPos + sReplaceString.size();
  }
}

size_t CUpdateXMLHandler::FindVariable(const std::string& sVar)
{
  size_t iMatchedEntries = 0;
  bool bExactMatchFound = false;

  for (std::map<std::string, std::string>::iterator it = m_MapOfVariables.begin(); it != m_MapOfVariables.end(); it++)
  {
    if (it->first.find(sVar) != std::string::npos)
    {
      iMatchedEntries++;
      if (sVar.size() == it->first.size())
        bExactMatchFound = true;
    }
  }
  if (iMatchedEntries == 0)
    return std::string::npos;
  if (iMatchedEntries == 1 && bExactMatchFound)
    return 0;
  return iMatchedEntries;
}

void CUpdateXMLHandler::SetExternalVariables(const std::string& sLine)
{
  size_t iPosVar1 = 0;
  size_t iPosVar2 = sLine.find(" = ", iPosVar1);

  if (iPosVar2 == std::string::npos)
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. variable = value format is wrong");

  std::string sVar = sLine.substr(iPosVar1, iPosVar2-iPosVar1);

  std::string sVal = sLine.substr(iPosVar2 + 3);

  SubstituteExternalVariables(sVal);

  m_MapOfVariables[sVar] = sVal;
}

void CUpdateXMLHandler::SetInternalVariables(const std::string& sLine, CXMLResdata& ResData)
{
  size_t iPosVar1 = 4;
  size_t iPosVar2 = sLine.find(" = ", iPosVar1);

  if (iPosVar2 == std::string::npos)
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. set variable = value format is wrong");

  std::string sVar = sLine.substr(iPosVar1, iPosVar2-iPosVar1);

  std::string sVal = sLine.substr(iPosVar2 + 3);

  SubstituteExternalVariables(sVal);

  if (sVar == "UPSOwner")
    ResData.UPS.Owner = sVal;
  else if (sVar == "UPSRepo")
    ResData.UPS.Repo = sVal;
  else if (sVar == "UPSBranch")
    ResData.UPS.Branch = sVal;

//To be deleted
  else if (sVar == "UPSLpath")
    ResData.UPS.LPath = sVal;
  else if (sVar == "UPSLForm")
    ResData.UPS.LForm = sVal;
  else if (sVar == "UPSAXMLPath")
    ResData.UPS.AXMLPath = sVal;
  else if (sVar == "UPSLFormInAXML")
    ResData.UPS.LFormInAXML = sVal;
  else if (sVar == "UPSLAXMLPath")
    ResData.UPS.LAXMLPath = sVal;
  else if (sVar == "UPSLAXMLFormat")
    ResData.UPS.LAXMLForm = sVal;
  else if (sVar == "UPSChLogURL")
    ResData.UPS.ChLogURL = sVal;
  else if (sVar == "UPSChLogURLRoot")
    ResData.UPS.ChLogURLRoot = sVal;
  else if (sVar == "UPSChLogName")
    ResData.UPS.ChLogName = sVal;

  else if (sVar == "LOCLpath")
    ResData.LOC.LPath = sVal;
  else if (sVar == "LOCLForm")
    ResData.LOC.LForm = sVal;
  else if (sVar == "LOCAXMLPath")
    ResData.LOC.AXMLPath = sVal;
  else if (sVar == "LOCLFormInAXML")
    ResData.LOC.LFormInAXML = sVal;
  else if (sVar == "LOCLAXMLPath")
    ResData.LOC.LAXMLPath = sVal;
  else if (sVar == "LOCLAXMLFormat")
    ResData.LOC.LAXMLForm = sVal;
  else if (sVar == "LOCChLogURL")
    ResData.LOC.ChLogURL = sVal;
  else if (sVar == "LOCChLogURLRoot")
    ResData.LOC.ChLogURLRoot = sVal;
  else if (sVar == "LOCChLogName")
    ResData.LOC.ChLogName = sVal;
//

  else if (sVar == "UPSLURL")
    ResData.UPS.LURL = sVal;
  else if (sVar == "UPSLURLRoot")
    ResData.UPS.LURLRoot = sVal;
  else if (sVar == "UPSAXMLURL")
    ResData.UPS.AXMLURL = sVal;
  else if (sVar == "UPSAXMLURLRoot")
    ResData.UPS.AXMLURLRoot = sVal;
  else if (sVar == "UPSALForm")
    ResData.UPS.ALForm = sVal;
  else if (sVar == "UPSAXMLFileName")
    ResData.UPS.AXMLFileName = sVal;
  else if (sVar == "UPSChLogPath")
    ResData.UPS.ChLogPath = sVal;
  else if (sVar == "UPSChLogPath")
    ResData.UPS.ChLogPath = sVal;


  if (sVar == "LOCOwner")
    ResData.LOC.Owner = sVal;
  else if (sVar == "LOCRepo")
    ResData.LOC.Repo = sVal;
  else if (sVar == "LOCBranch")
    ResData.LOC.Branch = sVal;

  else if (sVar == "LOCLpath")
    ResData.LOC.LPath = sVal;
  else if (sVar == "LOCLForm")
    ResData.LOC.LForm = sVal;
  else if (sVar == "LOCAXMLPath")
    ResData.LOC.AXMLPath = sVal;
  else if (sVar == "LOCLFormInAXML")
    ResData.LOC.LFormInAXML = sVal;
  else if (sVar == "LOCLAXMLPath")
    ResData.LOC.LAXMLPath = sVal;
  else if (sVar == "LOCLAXMLFormat")
    ResData.LOC.LAXMLForm = sVal;
  else if (sVar == "LOCChLogPath")
    ResData.LOC.ChLogPath = sVal;

  else if (sVar == "TRXProjectName")
    ResData.TRX.ProjectName = sVal;
  else if (sVar == "TRXLongProjectName")
    ResData.TRX.LongProjectName = sVal;
  else if (sVar == "TRXResName")
    ResData.TRX.ResName = sVal;
  else if (sVar == "TRXLForm")
    ResData.TRX.LForm = sVal;

  else if (sVar == "UPDProjectName")
    ResData.UPD.ProjectName = sVal;
  else if (sVar == "UPDLongProjectName")
    ResData.UPD.LongProjectName = sVal;
  else if (sVar == "UPDResName")
    ResData.UPD.ResName = sVal;
  else if (sVar == "UPDLForm")
    ResData.UPD.LForm = sVal;


  else if (sVar == "ResName")
    ResData.sResName = sVal;
  else if (sVar == "ChgLogFormat")
    ResData.sChgLogFormat = sVal;
  else if (sVar == "ProjRootDir")
    ResData.sProjRootDir = sVal;
  else if (sVar == "MRGLFilesDir")
    ResData.sMRGLFilesDir = sVal;
  else if (sVar == "MinComplPercent")
    ResData.iMinComplPercent = strtol(&sVal[0], NULL, 10);
  else if (sVar == "UPDLFilesDir")
    ResData.sUPDLFilesDir = sVal;
  else if (sVar == "SupportEmailAddr")
    ResData.sSupportEmailAddr = sVal;
  else if (sVar == "SRCLCode")
    ResData.sSRCLCode = sVal;
  else if (sVar == "BaseLCode")
    ResData.sBaseLCode = sVal;
  else if (sVar == "LTeamLFormat")
    ResData.sLTeamLFormat = sVal;
  else if (sVar == "LDatabaseURL")
    ResData.sLDatabaseURL = sVal;
  else if (sVar == "ForceComm")
    ResData.bForceComm = (sVal == "true");
  else if (sVar == "Rebrand")
    ResData.bRebrand = (sVal == "true");
  else if (sVar == "ForceTXUpd")
    ResData.bForceTXUpd = (sVal == "true");
  else if (sVar == "IsLangAddon")
    ResData.bIsLangAddon = (sVal == "true");
  else if (sVar == "HasOnlyAddonXML")
    ResData.bHasOnlyAddonXML = (sVal == "true");

  else
    CLog::Log(logERROR, "ConfHandler: Unreconised internal variable name");

  m_MapOfVariables[sVar] = sVal;
}


void CUpdateXMLHandler::LoadResDataToMem (std::string rootDir, std::map<std::string, CXMLResdata> & mapResData)
{
  std::string sConfFile = g_File.ReadFileToStr(rootDir + "kodi-txupdate.conf");
  if (sConfFile == "")
    CLog::Log(logERROR, "Confhandler: erroe: missing conf file.");

  size_t iPos1 = 0;
  size_t iPos2 = 0;

  while ((iPos2 = sConfFile.find('\n', iPos1)) != std::string::npos)
  {
    CXMLResdata ResData;
    std::string sLine = sConfFile.substr(iPos1, iPos2-iPos1);
    iPos1 = iPos2 +1;

    if (sLine.empty() || sLine.find('#') == 0) // If line is empty or a comment line, ignore
      continue;

    if (sLine.find("set ") == 0)
    {
      SetInternalVariables(sLine, ResData);
    }
    else
      SetExternalVariables(sLine);
  }
}

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