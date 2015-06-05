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

#include "ProjectHandler.h"
#include <list>
#include "HTTPUtils.h"
#include "jsoncpp/json/json.h"
#include <algorithm>
#include "UpdateXMLHandler.h"

CProjectHandler::CProjectHandler()
{};

CProjectHandler::~CProjectHandler()
{};

void CProjectHandler::LoadUpdXMLToMem()
{
  CUpdateXMLHandler UpdateXMLHandler;
  UpdateXMLHandler.LoadUpdXMLToMem (m_strProjDir, m_mapResData);
}

bool CProjectHandler::FetchResourcesFromTransifex()
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  printf ("TXresourcelist");

  const std::string& strProjectName = m_mapResData.begin()->second.strProjectName; //TODO handle different projectnames accross the update xml file
  std::string strtemp = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/" + strProjectName + "/resources/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ProjectHandler::FetchResourcesFromTransifex: error getting resources from transifex.net");

  printf ("\n\n");
  char cstrtemp[strtemp.size()];
  strcpy(cstrtemp, strtemp.c_str());

  std::list<std::string> listResourceNamesTX = ParseResources(strtemp);

  for (std::list<std::string>::iterator it = listResourceNamesTX.begin(); it != listResourceNamesTX.end(); it++)
  {
    printf("%s%s%s (", KMAG, it->c_str(), RESET);
    CLog::Log(logLINEFEED, "");
    CLog::Log(logINFO, "ProjHandler: ****** FETCH Resource from TRANSIFEX: %s", it->c_str());
    CLog::IncIdent(4);

    std::string strResname = GetResNameFromTXResName(*it);
    if (strResname.empty())
    {
      printf(" )\n");
      CLog::Log(logWARNING, "ProjHandler: found resource on Transifex which is not in kodi-txupdate.xml: %s", it->c_str());
      CLog::DecIdent(4);
      continue;
    }

    CXMLResdata XMLResdata = m_mapResData[strResname];
    CResourceHandler ResourceHandler(XMLResdata);

    m_mapResourcesTX[strResname]=ResourceHandler;
    m_mapResourcesTX[strResname].FetchPOFilesTXToMem(XMLResdata, "https://www.transifex.com/api/2/project/" + strProjectName +
                                              "/resource/" + *it + "/");
    CLog::DecIdent(4);
    printf(" )\n");
  }
  return true;
};

bool CProjectHandler::FetchResourcesFromUpstream()
{
  for (std::map<std::string, CXMLResdata>::iterator it = m_mapResData.begin(); it != m_mapResData.end(); it++)
  {
    const CXMLResdata& XMLResData = it->second;
    CResourceHandler ResourceHandler(XMLResData);

    printf("%s%s%s (", KMAG, it->first.c_str(), RESET);
    CLog::Log(logLINEFEED, "");
    CLog::Log(logINFO, "ProjHandler: ****** FETCH Resource from UPSTREAM: %s ******", it->first.c_str());

    CLog::IncIdent(4);
    m_mapResourcesUpstr[it->first] = ResourceHandler;
    m_mapResourcesUpstr[it->first].FetchPOFilesUpstreamToMem(XMLResData);
    CLog::DecIdent(4);
    printf(" )\n");
  }
  return true;
};

bool CProjectHandler::WriteResourcesToFile(std::string strProjRootDir)
{
  std::string strPrefixDir;

  //TODO
  const CXMLResdata& XMLResData = m_mapResData.begin()->second;

  strPrefixDir = XMLResData.strMergedLangfileDir;
  CLog::Log(logINFO, "Deleting merged language file directory");
  g_File.DeleteDirectory(strProjRootDir + strPrefixDir);
  for (T_itmapRes itmapResources = m_mapResMerged.begin(); itmapResources != m_mapResMerged.end(); itmapResources++)
  {
    printf("Writing merged resources to HDD: %s%s%s\n", KMAG, itmapResources->first.c_str(), RESET);
    std::list<std::string> lChangedLangsFromUpstream = m_mapResMerged[itmapResources->first].GetChangedLangsFromUpstream();
    std::list<std::string> lChangedAddXMLLangsFromUpstream = m_mapResMerged[itmapResources->first].GetChangedLangsInAddXMLFromUpstream();
    if (!lChangedAddXMLLangsFromUpstream.empty())
    {
      printf("    Changed Langs in addon.xml file from upstream: ");
      PrintChangedLangs(lChangedAddXMLLangsFromUpstream);
      printf("\n");
    }
    if (!lChangedLangsFromUpstream.empty())
    {
      printf("    Changed Langs in strings files from upstream: ");
      PrintChangedLangs(lChangedLangsFromUpstream);
      printf ("\n");
    }

    CLog::Log(logLINEFEED, "");
    CLog::Log(logINFO, "ProjHandler: *** Write Merged Resource: %s ***", itmapResources->first.c_str());
    CLog::IncIdent(4);
    CXMLResdata XMLResdata = m_mapResData[itmapResources->first];
    m_mapResMerged[itmapResources->first].WritePOToFiles (strProjRootDir, strPrefixDir, itmapResources->first, XMLResdata, false);
    CLog::DecIdent(4);
  }
  printf ("\n\n");

  strPrefixDir = XMLResData.strTXUpdateLangfilesDir;
  CLog::Log(logINFO, "Deleting tx update language file directory");
  g_File.DeleteDirectory(strProjRootDir + strPrefixDir);
  for (T_itmapRes itmapResources = m_mapResUpdateTX.begin(); itmapResources != m_mapResUpdateTX.end(); itmapResources++)
  {
    printf("Writing update TX resources to HDD: %s%s%s\n", KMAG, itmapResources->first.c_str(), RESET);
    CLog::Log(logLINEFEED, "");
    CLog::Log(logINFO, "ProjHandler: *** Write UpdTX Resource: %s ***", itmapResources->first.c_str());
    CLog::IncIdent(4);
    CXMLResdata XMLResdata = m_mapResData[itmapResources->first];
    m_mapResUpdateTX[itmapResources->first].WritePOToFiles (strProjRootDir, strPrefixDir, itmapResources->first, XMLResdata, true);
    CLog::DecIdent(4);
  }

  return true;
};

bool CProjectHandler::CreateMergedResources()
{
  CLog::Log(logINFO, "CreateMergedResources started");

  std::list<std::string> listMergedResource = CreateResourceList();

  m_mapResMerged.clear();
  m_mapResUpdateTX.clear();

  for (std::list<std::string>::iterator itResAvail = listMergedResource.begin(); itResAvail != listMergedResource.end(); itResAvail++)
  {
    CXMLResdata XMLResData = m_mapResData[*itResAvail];

    printf("Merging resource: %s%s%s\n", KMAG, itResAvail->c_str(), RESET);
    CLog::SetSyntaxAddon(*itResAvail);
    CLog::Log(logINFO, "CreateMergedResources: Merging resource:%s", itResAvail->c_str());
    CLog::IncIdent(4);

    CResourceHandler mergedResHandler(XMLResData), updTXResHandler(XMLResData);
    std::list<std::string> lAddXMLLangsChgedFromUpstream, lLangsChgedFromUpstream;

    // Get available pretext for Resource Header. we use the upstream one
    std::string strResPreHeader;
    if (m_mapResourcesUpstr.find(*itResAvail) != m_mapResourcesUpstr.end())
      strResPreHeader = m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetResHeaderPretext();
    else
      CLog::Log(logERROR, "CreateMergedResources: Not able to read addon data for header text");

    CAddonXMLEntry * pENAddonXMLEntry;
    bool bIsResourceLangAddon = m_mapResourcesUpstr[*itResAvail].GetIfIsLangaddon();

    if ((pENAddonXMLEntry = GetAddonDataFromXML(&m_mapResourcesUpstr, *itResAvail, XMLResData.strSourceLcode)) != NULL)
    {
      mergedResHandler.GetXMLHandler()->SetStrAddonXMLFile(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetStrAddonXMLFile());
      mergedResHandler.GetXMLHandler()->SetAddonVersion(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetAddonVersion());
      mergedResHandler.GetXMLHandler()->SetAddonChangelogFile(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetAddonChangelogFile());
      mergedResHandler.GetXMLHandler()->SetAddonLogFilename(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetAddonLogFilename());
      mergedResHandler.GetXMLHandler()->SetAddonMetadata(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetAddonMetaData());
      updTXResHandler.GetXMLHandler()->SetStrAddonXMLFile(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetStrAddonXMLFile());
      updTXResHandler.GetXMLHandler()->SetAddonVersion(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetAddonVersion());
      updTXResHandler.GetXMLHandler()->SetAddonChangelogFile(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetAddonChangelogFile());
      updTXResHandler.GetXMLHandler()->SetAddonLogFilename(m_mapResourcesUpstr[*itResAvail].GetXMLHandler()->GetAddonLogFilename());
    }
    else if (!bIsResourceLangAddon)
      CLog::Log(logERROR, "CreateMergedResources: No Upstream AddonXML file found as source for merging");

    std::list<std::string> listMergedLangs = CreateMergedLanguageList(*itResAvail);

    CPOHandler * pcurrPOHandlerEN = m_mapResourcesUpstr[*itResAvail].GetPOData(XMLResData.strSourceLcode);

    for (std::list<std::string>::iterator itlang = listMergedLangs.begin(); itlang != listMergedLangs.end(); itlang++)
    {
      CLog::SetSyntaxLang(*itlang);
      std::string strLangCode = *itlang;
      CPOHandler mergedPOHandler(XMLResData), updTXPOHandler (XMLResData);
      const CPOEntry* pPOEntryTX;
      const CPOEntry* pPOEntryUpstr;
      bool bResChangedInAddXMLFromUpstream = false; bool bResChangedFromUpstream = false;

      mergedPOHandler.SetIfIsSourceLang(strLangCode == XMLResData.strSourceLcode);
      updTXPOHandler.SetIfIsSourceLang(strLangCode == XMLResData.strSourceLcode);
      updTXPOHandler.SetIfPOIsUpdTX(true);

      CAddonXMLEntry MergedAddonXMLEntry, MergedAddonXMLEntryTX;
      CAddonXMLEntry * pAddonXMLEntry;

      // Get addon.xml file translatable strings from Transifex to the merged Entry
      if (m_mapResourcesTX.find(*itResAvail) != m_mapResourcesTX.end() && m_mapResourcesTX[*itResAvail].GetPOData(*itlang))
      {
        CAddonXMLEntry AddonXMLEntryInPO, AddonENXMLEntryInPO;
        m_mapResourcesTX[*itResAvail].GetPOData(*itlang)->GetAddonMetaData(AddonXMLEntryInPO, AddonENXMLEntryInPO);
        MergeAddonXMLEntry(AddonXMLEntryInPO, MergedAddonXMLEntry, *pENAddonXMLEntry, AddonENXMLEntryInPO, false,
                           bResChangedInAddXMLFromUpstream);
      }
      // Save these strings from Transifex for later use
      MergedAddonXMLEntryTX = MergedAddonXMLEntry;

      // Get the addon.xml file translatable strings from upstream merged into the merged entry
      if ((pAddonXMLEntry = GetAddonDataFromXML(&m_mapResourcesUpstr, *itResAvail, *itlang)) != NULL)
        MergeAddonXMLEntry(*pAddonXMLEntry, MergedAddonXMLEntry, *pENAddonXMLEntry,
                           *GetAddonDataFromXML(&m_mapResourcesUpstr, *itResAvail, XMLResData.strSourceLcode), true, bResChangedInAddXMLFromUpstream);
      else if (!MergedAddonXMLEntryTX.strDescription.empty() || !MergedAddonXMLEntryTX.strSummary.empty() ||
               !MergedAddonXMLEntryTX.strDisclaimer.empty())
        bResChangedInAddXMLFromUpstream = true;

      if (!bIsResourceLangAddon)
      {
        mergedResHandler.GetXMLHandler()->GetMapAddonXMLData()->operator[](*itlang) = MergedAddonXMLEntry;
        updTXResHandler.GetXMLHandler()->GetMapAddonXMLData()->operator[](*itlang) = MergedAddonXMLEntry;
        updTXPOHandler.SetAddonMetaData(MergedAddonXMLEntry, MergedAddonXMLEntryTX, *pENAddonXMLEntry, *itlang); // add addonxml data as PO  classic entries
      }

      for (size_t POEntryIdx = 0; pcurrPOHandlerEN && POEntryIdx != pcurrPOHandlerEN->GetNumEntriesCount(); POEntryIdx++)
      {
        size_t numID = pcurrPOHandlerEN->GetNumPOEntryByIdx(POEntryIdx)->numID;

        CPOEntry currPOEntryEN = *(pcurrPOHandlerEN->GetNumPOEntryByIdx(POEntryIdx));
        currPOEntryEN.msgStr.clear();
        CPOEntry* pcurrPOEntryEN = &currPOEntryEN;

        pPOEntryTX = SafeGetPOEntry(m_mapResourcesTX, *itResAvail, strLangCode, numID);
        pPOEntryUpstr = SafeGetPOEntry(m_mapResourcesUpstr, *itResAvail, strLangCode, numID);

        CheckPOEntrySyntax(pPOEntryTX, strLangCode, pcurrPOEntryEN, XMLResData);

        if (strLangCode == XMLResData.strSourceLcode) // Source language entry
        {
          mergedPOHandler.AddNumPOEntryByID(numID, *pcurrPOEntryEN, *pcurrPOEntryEN, true);
          updTXPOHandler.AddNumPOEntryByID(numID, *pcurrPOEntryEN, *pcurrPOEntryEN, true);
        }
        //1. Tx entry single
        else if (pPOEntryTX && pcurrPOEntryEN->msgIDPlur.empty() && !pPOEntryTX->msgStr.empty() &&
                 pPOEntryTX->msgID == pcurrPOEntryEN->msgID)
        {
          mergedPOHandler.AddNumPOEntryByID(numID, *pPOEntryTX, *pcurrPOEntryEN, true);
          if (XMLResData.bForceTXUpd)
            updTXPOHandler.AddNumPOEntryByID(numID, *pPOEntryTX, *pcurrPOEntryEN, true);
          //Check if the string is new on TX or changed from the upstream one -> Addon version bump later
          if ((!pPOEntryUpstr || pPOEntryUpstr->msgStr.empty() ||
              (!pPOEntryUpstr->msgID.empty() && pPOEntryUpstr->msgID != pcurrPOEntryEN->msgID)) &&
              (!pPOEntryUpstr || pPOEntryUpstr->msgStr != pPOEntryTX->msgStr))
              bResChangedFromUpstream = true;
        }
        //2. Tx entry plural
        else if (pPOEntryTX && !pcurrPOEntryEN->msgIDPlur.empty() && !pPOEntryTX->msgStrPlural.empty() &&
                 pPOEntryTX->msgIDPlur == pcurrPOEntryEN->msgIDPlur)
        {
          mergedPOHandler.AddNumPOEntryByID(numID, *pPOEntryTX, *pcurrPOEntryEN, true);
          if (XMLResData.bForceTXUpd)
            updTXPOHandler.AddNumPOEntryByID(numID, *pPOEntryTX, *pcurrPOEntryEN, true);
          //Check if the string is new on TX or changed from the upstream one -> Addon version bump later
          if ((!pPOEntryUpstr || pPOEntryUpstr->msgStrPlural.empty() ||
              (!pPOEntryUpstr->msgID.empty() && pPOEntryUpstr->msgID != pcurrPOEntryEN->msgID)) &&
              (!pPOEntryUpstr || pPOEntryUpstr->msgStrPlural != pPOEntryTX->msgStrPlural))
              bResChangedFromUpstream = true;
        }
        //3. Upstr entry single
        else if (pPOEntryUpstr && pcurrPOEntryEN->msgIDPlur.empty() && !pPOEntryUpstr->msgStr.empty() &&
                (pPOEntryUpstr->msgID.empty() || pPOEntryUpstr->msgID == pcurrPOEntryEN->msgID)) //if it is empty it is from a strings.xml file
        {
          mergedPOHandler.AddNumPOEntryByID(numID, *pPOEntryUpstr, *pcurrPOEntryEN, true);
          updTXPOHandler.AddNumPOEntryByID(numID, *pPOEntryUpstr, *pcurrPOEntryEN, false);
        }
        //4. Upstr entry plural
        else if (pPOEntryUpstr && !pcurrPOEntryEN->msgIDPlur.empty() && !pPOEntryUpstr->msgStrPlural.empty() &&
                 pPOEntryUpstr->msgIDPlur == pcurrPOEntryEN->msgIDPlur)
        {
          mergedPOHandler.AddNumPOEntryByID(numID, *pPOEntryUpstr, *pcurrPOEntryEN, true);
          updTXPOHandler.AddNumPOEntryByID(numID, *pPOEntryUpstr, *pcurrPOEntryEN, false);
        }

      }




      // Handle classic non-id based po entries
      for (size_t POEntryIdx = 0; pcurrPOHandlerEN && POEntryIdx != pcurrPOHandlerEN->GetClassEntriesCount(); POEntryIdx++)
      {

        CPOEntry currPOEntryEN = *(pcurrPOHandlerEN->GetClassicPOEntryByIdx(POEntryIdx));
        currPOEntryEN.msgStr.clear();
        CPOEntry* pcurrPOEntryEN = &currPOEntryEN;

        pPOEntryTX = SafeGetPOEntry(m_mapResourcesTX, *itResAvail, strLangCode, currPOEntryEN);
        pPOEntryUpstr = SafeGetPOEntry(m_mapResourcesUpstr, *itResAvail, strLangCode, currPOEntryEN);

        CheckPOEntrySyntax(pPOEntryTX, strLangCode, pcurrPOEntryEN, XMLResData);

        if (strLangCode == XMLResData.strSourceLcode)
        {
          mergedPOHandler.AddClassicEntry(*pcurrPOEntryEN, *pcurrPOEntryEN, true);
          updTXPOHandler.AddClassicEntry(*pcurrPOEntryEN, *pcurrPOEntryEN, true);
        }
        else if (pPOEntryTX && pcurrPOEntryEN->msgIDPlur.empty() && !pPOEntryTX->msgStr.empty()) // Tx entry single
        {
          mergedPOHandler.AddClassicEntry(*pPOEntryTX, *pcurrPOEntryEN, true);
          if (XMLResData.bForceTXUpd)
            updTXPOHandler.AddClassicEntry(*pPOEntryTX, *pcurrPOEntryEN, true);
          //Check if the string is new on TX or changed from the upstream one -> Addon version bump later
          if (!pPOEntryUpstr || pPOEntryUpstr->msgStr.empty())
          {
            CPOEntry POEntryToCompare = *pPOEntryTX;
            POEntryToCompare.msgIDPlur.clear();
            POEntryToCompare.Type = 0;
            if (!pPOEntryUpstr || !(*pPOEntryUpstr == POEntryToCompare))
              bResChangedFromUpstream = true;
          }
        }
        else if (pPOEntryTX && !pcurrPOEntryEN->msgIDPlur.empty() && !pPOEntryTX->msgStrPlural.empty()) // Tx entry plural
        {
          mergedPOHandler.AddClassicEntry(*pPOEntryTX, *pcurrPOEntryEN, true);
          if (XMLResData.bForceTXUpd)
            updTXPOHandler.AddClassicEntry(*pPOEntryTX, *pcurrPOEntryEN, true);
          //Check if the string is new on TX or changed from the upstream one -> Addon version bump later
          if (!pPOEntryUpstr || pPOEntryUpstr->msgStrPlural.empty())
          {
            CPOEntry POEntryToCompare = *pPOEntryTX;
            POEntryToCompare.msgID.clear();
            POEntryToCompare.Type = 0;
            if (!pPOEntryUpstr || !(*pPOEntryUpstr == POEntryToCompare))
              bResChangedFromUpstream = true;
          }
        }
        else if (pPOEntryUpstr && pcurrPOEntryEN->msgIDPlur.empty() && !pPOEntryUpstr->msgStr.empty()) // Upstr entry single
        {
          mergedPOHandler.AddClassicEntry(*pPOEntryUpstr, *pcurrPOEntryEN, true);
          updTXPOHandler.AddClassicEntry(*pPOEntryUpstr, *pcurrPOEntryEN, false);
        }
        else if (pPOEntryUpstr && !pcurrPOEntryEN->msgIDPlur.empty() && !pPOEntryUpstr->msgStrPlural.empty()) // Upstr entry plural
        {
          mergedPOHandler.AddClassicEntry(*pPOEntryUpstr, *pcurrPOEntryEN, true);
          updTXPOHandler.AddClassicEntry(*pPOEntryUpstr, *pcurrPOEntryEN, false);
        }
      }

      CPOHandler * pPOHandlerTX, * pPOHandlerUpstr;
      pPOHandlerTX = SafeGetPOHandler(m_mapResourcesTX, *itResAvail, strLangCode);
      pPOHandlerUpstr = SafeGetPOHandler(m_mapResourcesUpstr, *itResAvail, strLangCode);

      if (mergedPOHandler.GetNumEntriesCount() !=0 || mergedPOHandler.GetClassEntriesCount() !=0)
      {
        if (bIsResourceLangAddon && pPOHandlerUpstr) // Copy the individual addon.xml files from upstream to merged resource for language-addons
        {
          mergedPOHandler.SetLangAddonXMLString(pPOHandlerUpstr->GetLangAddonXMLString());
          if (bResChangedFromUpstream)
            mergedPOHandler.BumpLangAddonXMLVersion();  // bump minor version number of the language-addon
        }
        else if (bIsResourceLangAddon && !pPOHandlerUpstr)
          CLog::Log(logWARNING, "Warning: No addon xml file exist for resource: %s and language: %s\nPlease create one upstream to be able to use this new language",
                    itResAvail->c_str(), strLangCode.c_str());

        mergedPOHandler.SetPreHeader(strResPreHeader);
        mergedPOHandler.SetHeaderNEW(*itlang);
        mergedResHandler.AddPOData(mergedPOHandler, strLangCode);
      }

      if ((updTXPOHandler.GetNumEntriesCount() !=0 || updTXPOHandler.GetClassEntriesCount() !=0) &&
        (strLangCode != XMLResData.strSourceLcode || XMLResData.bForceTXUpd ||
        !g_HTTPHandler.ComparePOFilesInMem(&updTXPOHandler, pPOHandlerTX, strLangCode == XMLResData.strSourceLcode)))
      {
        updTXPOHandler.SetPreHeader(strResPreHeader);
        updTXPOHandler.SetHeaderNEW(*itlang);
        updTXResHandler.AddPOData(updTXPOHandler, strLangCode);
      }

      CLog::LogTable(logINFO, "merged", "\t\t\t%s\t\t%i\t\t%i\t\t%i\t\t%i", strLangCode.c_str(), mergedPOHandler.GetNumEntriesCount(),
                     mergedPOHandler.GetClassEntriesCount(), updTXPOHandler.GetNumEntriesCount(), updTXPOHandler.GetClassEntriesCount());

     //store what languages changed from upstream in strings.po and addon.xml files
     if (bResChangedFromUpstream)
       lLangsChgedFromUpstream.push_back(strLangCode);
     if (bResChangedInAddXMLFromUpstream)
       lAddXMLLangsChgedFromUpstream.push_back(strLangCode);
    }
    CLog::LogTable(logADDTABLEHEADER, "merged", "--------------------------------------------------------------------------------------------\n");
    CLog::LogTable(logADDTABLEHEADER, "merged", "MergedPOHandler:\tLang\t\tmergedID\tmergedClass\tupdID\t\tupdClass\n");
    CLog::LogTable(logADDTABLEHEADER, "merged", "--------------------------------------------------------------------------------------------\n");
    CLog::LogTable(logCLOSETABLE, "merged",   "");

    mergedResHandler.SetChangedLangsFromUpstream(lLangsChgedFromUpstream);
    mergedResHandler.SetChangedLangsInAddXMLFromUpstream(lAddXMLLangsChgedFromUpstream);

    if (mergedResHandler.GetLangsCount() != 0 || !mergedResHandler.GetXMLHandler()->GetMapAddonXMLData()->empty())
      m_mapResMerged[*itResAvail] = mergedResHandler;
    if (updTXResHandler.GetLangsCount() != 0 || !updTXResHandler.GetXMLHandler()->GetMapAddonXMLData()->empty())
      m_mapResUpdateTX[*itResAvail] = updTXResHandler;
    CLog::DecIdent(4);
  }
  return true;
}

std::list<std::string> CProjectHandler::CreateMergedLanguageList(std::string strResname)
{
  std::list<std::string> listMergedLangs;

  if (m_mapResourcesTX.find(strResname) != m_mapResourcesTX.end())
  {

    // Add languages exist in transifex PO files
    for (size_t i =0; i != m_mapResourcesTX[strResname].GetLangsCount(); i++)
    {
      std::string strMLCode = m_mapResourcesTX[strResname].GetLangCodeFromPos(i);
      if (std::find(listMergedLangs.begin(), listMergedLangs.end(), strMLCode) == listMergedLangs.end())
        listMergedLangs.push_back(strMLCode);
    }
  }

  if (m_mapResourcesUpstr.find(strResname) != m_mapResourcesUpstr.end())
  {

    // Add languages exist in upstream PO or XML files
    for (size_t i =0; i != m_mapResourcesUpstr[strResname].GetLangsCount(); i++)
    {
      std::string strMLCode = m_mapResourcesUpstr[strResname].GetLangCodeFromPos(i);
      if (std::find(listMergedLangs.begin(), listMergedLangs.end(), strMLCode) == listMergedLangs.end())
        listMergedLangs.push_back(strMLCode);
    }

    // Add languages only exist in addon.xml files
    std::map<std::string, CAddonXMLEntry> * pMapUpstAddonXMLData = m_mapResourcesUpstr[strResname].GetXMLHandler()->GetMapAddonXMLData();
    for (std::map<std::string, CAddonXMLEntry>::iterator it = pMapUpstAddonXMLData->begin(); it != pMapUpstAddonXMLData->end(); it++)
    {
      if (std::find(listMergedLangs.begin(), listMergedLangs.end(), it->first) == listMergedLangs.end())
        listMergedLangs.push_back(it->first);
    }
  }

  return listMergedLangs;
}

std::list<std::string> CProjectHandler::CreateResourceList()
{
  std::list<std::string> listMergedResources;
  for (T_itmapRes it = m_mapResourcesUpstr.begin(); it != m_mapResourcesUpstr.end(); it++)
  {
    if (std::find(listMergedResources.begin(), listMergedResources.end(), it->first) == listMergedResources.end())
      listMergedResources.push_back(it->first);
  }

  for (T_itmapRes it = m_mapResourcesTX.begin(); it != m_mapResourcesTX.end(); it++)
  {
    if (std::find(listMergedResources.begin(), listMergedResources.end(), it->first) == listMergedResources.end())
      listMergedResources.push_back(it->first);
  }

  return listMergedResources;
}

CAddonXMLEntry * const CProjectHandler::GetAddonDataFromXML(std::map<std::string, CResourceHandler> * pmapRes,
                                                            const std::string &strResname, const std::string &strLangCode) const
{
  if (pmapRes->find(strResname) == pmapRes->end())
    return NULL;

  CResourceHandler * pRes = &(pmapRes->operator[](strResname));
  if (pRes->GetXMLHandler()->GetMapAddonXMLData()->find(strLangCode) == pRes->GetXMLHandler()->GetMapAddonXMLData()->end())
    return NULL;

  return &(pRes->GetXMLHandler()->GetMapAddonXMLData()->operator[](strLangCode));
}

const CPOEntry * CProjectHandler::SafeGetPOEntry(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                          std::string &strLangCode, size_t numID)
{
  if (mapResHandl.find(strResname) == mapResHandl.end())
    return NULL;
  if (!mapResHandl[strResname].GetPOData(strLangCode))
    return NULL;
  return mapResHandl[strResname].GetPOData(strLangCode)->GetNumPOEntryByID(numID);
}

const CPOEntry * CProjectHandler::SafeGetPOEntry(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                                                 std::string &strLangCode, CPOEntry const &currPOEntryEN)
{
  CPOEntry POEntryToFind;
  POEntryToFind.Type = currPOEntryEN.Type;
  POEntryToFind.msgCtxt = currPOEntryEN.msgCtxt;
  POEntryToFind.msgID = currPOEntryEN.msgID;
  POEntryToFind.msgIDPlur = currPOEntryEN.msgIDPlur;

  if (mapResHandl.find(strResname) == mapResHandl.end())
    return NULL;
  if (!mapResHandl[strResname].GetPOData(strLangCode))
    return NULL;
  return mapResHandl[strResname].GetPOData(strLangCode)->PLookforClassicEntry(POEntryToFind);
}

CPOHandler * CProjectHandler::SafeGetPOHandler(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                                                 std::string &strLangCode)
{
  if (mapResHandl.find(strResname) == mapResHandl.end())
    return NULL;
  return mapResHandl[strResname].GetPOData(strLangCode);
}

void CProjectHandler::MergeAddonXMLEntry(CAddonXMLEntry const &EntryToMerge, CAddonXMLEntry &MergedAddonXMLEntry,
                                         CAddonXMLEntry const &SourceENEntry, CAddonXMLEntry const &CurrENEntry, bool UpstrToMerge,
                                         bool &bResChangedFromUpstream)
{
  if (!EntryToMerge.strDescription.empty() && MergedAddonXMLEntry.strDescription.empty() &&
      CurrENEntry.strDescription == SourceENEntry.strDescription)
    MergedAddonXMLEntry.strDescription = EntryToMerge.strDescription;
  else if (UpstrToMerge && !MergedAddonXMLEntry.strDescription.empty() &&
           EntryToMerge.strDescription != MergedAddonXMLEntry.strDescription &&
           CurrENEntry.strDescription == SourceENEntry.strDescription)
  {
    bResChangedFromUpstream = true;
  }

  if (!EntryToMerge.strDisclaimer.empty() && MergedAddonXMLEntry.strDisclaimer.empty() &&
      CurrENEntry.strDisclaimer == SourceENEntry.strDisclaimer)
    MergedAddonXMLEntry.strDisclaimer = EntryToMerge.strDisclaimer;
  else if (UpstrToMerge && !MergedAddonXMLEntry.strDisclaimer.empty() &&
           EntryToMerge.strDisclaimer != MergedAddonXMLEntry.strDisclaimer &&
           CurrENEntry.strDisclaimer == SourceENEntry.strDisclaimer)
  {
    bResChangedFromUpstream = true;
  }

  if (!EntryToMerge.strSummary.empty() && MergedAddonXMLEntry.strSummary.empty() &&
      CurrENEntry.strSummary == SourceENEntry.strSummary)
    MergedAddonXMLEntry.strSummary = EntryToMerge.strSummary;
    else if (UpstrToMerge && !MergedAddonXMLEntry.strSummary.empty() &&
           EntryToMerge.strSummary != MergedAddonXMLEntry.strSummary &&
           CurrENEntry.strSummary == SourceENEntry.strSummary)
  {
    bResChangedFromUpstream = true;
  }
}

void CProjectHandler::UploadTXUpdateFiles(std::string strProjRootDir)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  printf ("TXresourcelist");

  //TODO
  const CXMLResdata& XMLResData = m_mapResData.begin()->second;
  const std::string& strTargetProjectName = XMLResData.strTargetProjectName;

  std::string strtemp = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/" + strTargetProjectName + "/resources/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ProjectHandler::FetchResourcesFromTransifex: error getting resources from transifex.net");

  printf ("\n\n");

  std::list<std::string> listResourceNamesTX = ParseResources(strtemp);

  std::string strPrefixDir = XMLResData.strTXUpdateLangfilesDir;

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  for (T_itResData itResData = m_mapResData.begin(); itResData != m_mapResData.end(); itResData++)
  {
    std::string strResourceDir, strLangDir;
    const CXMLResdata& XMLResData = itResData->second;
    std::string strResname = itResData->first;
    bool bNewResource = false;

    strResourceDir = strProjRootDir + strPrefixDir + DirSepChar + strResname + DirSepChar;
    strLangDir = strResourceDir;

    CLog::Log(logINFO, "CProjectHandler::UploadTXUpdateFiles: Uploading resource: %s, from langdir: %s",itResData->first.c_str(), strLangDir.c_str());
    printf ("Uploading files for resource: %s%s%s", KMAG, itResData->first.c_str(), RESET);

    std::list<std::string> listLangCodes = GetLangsFromDir(strLangDir);

    if (listLangCodes.empty()) // No update needed for the specific resource (not even an English one)
    {
      CLog::Log(logINFO, "CProjectHandler::GetLangsFromDir: no English directory found at langdir: %s,"
      " skipping upload for this resource.", strLangDir.c_str());
      printf (", no upload was requested.\n");
      continue;
    }

    if (!FindResInList(listResourceNamesTX, itResData->second.strTargetTXName))
    {
      CLog::Log(logINFO, "CProjectHandler::UploadTXUpdateFiles: No resource %s exists on Transifex. Creating it now.", itResData->first.c_str());
      // We create the new resource on transifex and also upload the English source file at once
      g_HTTPHandler.Cleanup();
      g_HTTPHandler.ReInit();
      size_t straddednew;
      g_HTTPHandler.CreateNewResource(itResData->second.strTargetTXName,
                                      strLangDir + XMLResData.strSourceLcode + DirSepChar + "strings.po",
                                      "https://www.transifex.com/api/2/project/" + strTargetProjectName + "/resources/",
                                      straddednew, "https://www.transifex.com/api/2/project/" + strTargetProjectName +
                                      "/resource/" + XMLResData.strTargetTXName + "/translation/" +
                                      g_LCodeHandler.GetLangFromLCode(XMLResData.strSourceLcode, XMLResData.strTargetTXName) + "/");

      CLog::Log(logINFO, "CProjectHandler::UploadTXUpdateFiles: Resource %s was succesfully created with %i Source language strings.",
                itResData->first.c_str(), straddednew);
      printf (", newly created on Transifex with %s%lu%s English strings.\n", KGRN, straddednew, RESET);

      g_HTTPHandler.Cleanup();
      g_HTTPHandler.ReInit();
      bNewResource = true;
      //TODO change directory to the right location of the cache file (even if it is different)
      g_HTTPHandler.DeleteCachedFile("https://www.transifex.com/api/2/project/" + strTargetProjectName + "/resources/", "GET");
    }

    printf ("\n");

    for (std::list<std::string>::const_iterator it = listLangCodes.begin(); it!=listLangCodes.end(); it++)
    {
      if (bNewResource && *it == XMLResData.strSourceLcode) // Let's not upload the Source language file again
        continue;
      std::string strFilePath = strLangDir + *it + DirSepChar + "strings.po";
      std::string strLangAlias = g_LCodeHandler.GetLangFromLCode(*it, XMLResData.strTargTXLFormat);

      bool buploaded = false;
      size_t stradded, strupd;
      if (strLangAlias == g_LCodeHandler.GetLangFromLCode(XMLResData.strSourceLcode, XMLResData.strTargTXLFormat))
        g_HTTPHandler.PutFileToURL(strFilePath, "https://www.transifex.com/api/2/project/" + strTargetProjectName +
                                                "/resource/" + XMLResData.strTargetTXName + "/content/",
                                                buploaded, stradded, strupd);
      else
        g_HTTPHandler.PutFileToURL(strFilePath, "https://www.transifex.com/api/2/project/" + strTargetProjectName +
                                                "/resource/" + XMLResData.strTargetTXName + "/translation/" + strLangAlias + "/",
                                                buploaded, stradded, strupd);
      if (buploaded)
      {
        printf ("\tlangcode: %s%s%s:\t added strings:%s%lu%s, updated strings:%s%lu%s\n", KCYN, strLangAlias.c_str(), RESET, KCYN, stradded, RESET, KCYN, strupd, RESET);

        g_HTTPHandler.DeleteCachedFile("https://www.transifex.com/api/2/project/" + strTargetProjectName +
                                       "/resource/" + strResname + "/stats/", "GET");
        g_HTTPHandler.DeleteCachedFile("https://www.transifex.com/api/2/project/" +strTargetProjectName +
        "/resource/" + strResname + "/translation/" + strLangAlias + "/?file", "GET");
      }
      else
        printf ("\tlangcode: %s:\t no change, skipping.\n", it->c_str());
    }
  }
}

bool CProjectHandler::FindResInList(std::list<std::string> const &listResourceNamesTX, std::string strTXResName)
{
  for (std::list<std::string>::const_iterator it = listResourceNamesTX.begin(); it!=listResourceNamesTX.end(); it++)
  {
    if (*it == strTXResName)
      return true;
  }
  return false;
}

std::list<std::string> CProjectHandler::GetLangsFromDir(std::string const &strLangDir)
{
  //TODO
  const CXMLResdata& XMLResData = m_mapResData.begin()->second;
  std::list<std::string> listDirs;
  bool bEnglishExists = true;
  if (!g_File.DirExists(strLangDir + XMLResData.strSourceLcode))
    bEnglishExists = false;

  DIR* Dir;
  struct dirent *DirEntry;
  Dir = opendir(strLangDir.c_str());

  while(Dir && (DirEntry=readdir(Dir)))
  {
    if (DirEntry->d_type == DT_DIR && DirEntry->d_name[0] != '.')
    {
      std::string strDirname = DirEntry->d_name;
      if (strDirname != XMLResData.strSourceLcode)
      {
        std::string strFoundLangCode = DirEntry->d_name;
        listDirs.push_back(strFoundLangCode);
      }
    }
  }

  listDirs.sort();
  if (bEnglishExists)
    listDirs.push_front(XMLResData.strSourceLcode);

  return listDirs;
};

void CProjectHandler::CheckPOEntrySyntax(const CPOEntry * pPOEntry, std::string const &strLangCode, const CPOEntry * pcurrPOEntryEN,
                                         const CXMLResdata& XMLResData)
{
  if (!pPOEntry)
    return;

  CheckCharCount(pPOEntry, strLangCode, pcurrPOEntryEN, '%', XMLResData);
  CheckCharCount(pPOEntry, strLangCode, pcurrPOEntryEN, '\n', XMLResData);

  return;
}

std::string CProjectHandler::GetEntryContent(const CPOEntry * pPOEntry, std::string const &strLangCode, const CXMLResdata& XMLResData)
{
  if (!pPOEntry)
    return "";

  std::string strReturn;
  strReturn += "\n";

  if (pPOEntry->Type == ID_FOUND)
    strReturn += "msgctxt \"#" + g_CharsetUtils.IntToStr(pPOEntry->numID) + "\"\n";
  else if (!pPOEntry->msgCtxt.empty())
    strReturn += "msgctxt \"" + g_CharsetUtils.EscapeStringCPP(pPOEntry->msgCtxt) + "\"\n";

  strReturn += "msgid \""  + g_CharsetUtils.EscapeStringCPP(pPOEntry->msgID) +  "\"\n";

  if (strLangCode != XMLResData.strSourceLcode)
    strReturn += "msgstr \"" + g_CharsetUtils.EscapeStringCPP(pPOEntry->msgStr) + "\"\n";
  else
    strReturn += "msgstr \"\"\n";

  return strReturn;
}

void CProjectHandler::CheckCharCount(const CPOEntry * pPOEntry, std::string const &strLangCode, const CPOEntry * pcurrPOEntryEN, char chrToCheck,
                                     const CXMLResdata& XMLResData)
{
  // check '%' count in msgid and msgstr entries
  size_t count = g_CharsetUtils.GetCharCountInStr(pcurrPOEntryEN->msgID, chrToCheck);
  if (!pPOEntry->msgIDPlur.empty() && count != g_CharsetUtils.GetCharCountInStr(pPOEntry->msgIDPlur, chrToCheck))
    CLog::SyntaxLog(logWARNING, "Warning: count missmatch of char \"%s\"%s",
                    g_CharsetUtils.EscapeStringCPP(g_CharsetUtils.ChrToStr(chrToCheck)).c_str(),
                    GetEntryContent(pPOEntry, strLangCode, XMLResData).c_str());

  if (strLangCode != XMLResData.strSourceLcode)
  {
    if (!pPOEntry->msgStr.empty() && count != g_CharsetUtils.GetCharCountInStr(pPOEntry->msgStr, chrToCheck))
      CLog::SyntaxLog(logWARNING, "Warning: count missmatch of char \"%s\"%s",
                      g_CharsetUtils.EscapeStringCPP(g_CharsetUtils.ChrToStr(chrToCheck)).c_str(),
                      GetEntryContent(pPOEntry, strLangCode, XMLResData).c_str());

      for (std::vector<std::string>::const_iterator it =  pPOEntry->msgStrPlural.begin() ; it != pPOEntry->msgStrPlural.end() ; it++)
      {
        if (count != g_CharsetUtils.GetCharCountInStr(*it, '%'))
          CLog::SyntaxLog(logWARNING, "Warning: count missmatch of char \"%s\"%s",
                          g_CharsetUtils.EscapeStringCPP(g_CharsetUtils.ChrToStr(chrToCheck)).c_str(),
                          GetEntryContent(pPOEntry, strLangCode, XMLResData).c_str());
      }
  }
}

void CProjectHandler::PrintChangedLangs(std::list<std::string> lChangedLangs)
{
  std::list<std::string>::iterator itLangs;
  std::size_t counter = 0;
  printf ("%s", KCYN);
  for (itLangs = lChangedLangs.begin() ; itLangs != lChangedLangs.end(); itLangs++)
  {
    printf ("%s ", itLangs->c_str());
    counter++;
    if (counter > 10)
    {
      printf ("+ %i langs ", (int)lChangedLangs.size() - 11);
      break;
    }
  }
  printf ("%s", RESET);
}

std::string CProjectHandler::GetResNameFromTXResName(std::string const &strTXResName)
{
  for (T_itResData itResData = m_mapResData.begin(); itResData != m_mapResData.end(); itResData++)
  {
    if (itResData->second.strTXName == strTXResName)
      return itResData->first;
  }
  return "";
}

void CProjectHandler::MigrateTranslators()
{
  //TODO
  CXMLResdata XMLResdata = m_mapResData.begin()->second;
  const std::string& strProjectName = XMLResdata.strProjectName;
  const std::string& strTargetProjectName = XMLResdata.strTargetProjectName;
  const std::string& strTargetTXLangFormat = XMLResdata.strTargTXLFormat;

  if (strProjectName.empty() || strTargetProjectName.empty() || strProjectName == strTargetProjectName)
    CLog::Log(logERROR, "Cannot tranfer translators database. Wrong projectname and/or target projectname,");

  std::map<std::string, std::string> mapCoordinators, mapReviewers, mapTranslators;

  printf("\n%sCoordinators:%s\n", KGRN, RESET);
  mapCoordinators = g_LCodeHandler.GetTranslatorsDatabase("coordinators", strProjectName, XMLResdata);

  printf("\n%sReviewers:%s\n", KGRN, RESET);
  mapReviewers = g_LCodeHandler.GetTranslatorsDatabase("reviewers", strProjectName, XMLResdata);

  printf("\n%sTranslators:%s\n", KGRN, RESET);
  mapTranslators = g_LCodeHandler.GetTranslatorsDatabase("translators", strProjectName, XMLResdata);

  printf("\n%s", KGRN);
  printf("-----------------------------\n");
  printf("PUSH TRANSLATION GROUPS TO TX\n");
  printf("-----------------------------%s\n", RESET);

  g_LCodeHandler.UploadTranslatorsDatabase(mapCoordinators, mapReviewers, mapTranslators, strTargetProjectName, strTargetTXLangFormat);
}

void CProjectHandler::InitLCodeHandler()
{
//TODO
  CXMLResdata XMLResdata = m_mapResData.begin()->second;
  g_LCodeHandler.Init(XMLResdata.LangDatabaseURL, XMLResdata);
}

std::list<std::string> CProjectHandler::ParseResources(std::string strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string resName;
  std::list<std::string> listResources;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data");
    return listResources;
  }

  for(Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++)
  {
    Json::Value valu = *itr;
    resName = valu.get("slug", "unknown").asString();

    if (resName.size() == 0 || resName == "unknown")
      CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data while iterating");
    listResources.push_back(resName);
    CLog::Log(logINFO, "JSONHandler: found resource on Transifex server: %s", resName.c_str());
  };
  CLog::Log(logINFO, "JSONHandler: Found %i resources at Transifex server", listResources.size());
  return listResources;
};
