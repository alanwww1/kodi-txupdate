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

#include "POHandler.h"
#include "HTTPUtils.h"
#include <algorithm>
#include <list>
#include <sstream>
#include "CharsetUtils/CharsetUtils.h"
#include "Log.h"
#include "Langcodes.h"
#include "FileUtils/FileUtils.h"

CPOEntry::CPOEntry()
{
  Type = UNKNOWN;
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
  if (!poentry.Type == NUMID)
    bhasMatch = bhasMatch && (poentry.numID == numID);
  if (poentry.Type != UNKNOWN && poentry.Type != 0)
    bhasMatch = bhasMatch && (poentry.Type == Type);
  return bhasMatch;
};

bool CPOEntry::MatchMsgid(const CPOEntry& poentry) const
{
  bool bhasMatch = true;
  if (!poentry.msgCtxt.empty())
    bhasMatch = bhasMatch && (poentry.msgCtxt == msgCtxt);
  if (!poentry.msgID.empty())
    bhasMatch = bhasMatch && (poentry.msgID == msgID);
  if (!poentry.msgIDPlur.empty())
    bhasMatch = bhasMatch && (poentry.msgIDPlur == msgIDPlur);
  if (!poentry.Type == NUMID)
    bhasMatch = bhasMatch && (poentry.numID == numID);
  if (poentry.Type != UNKNOWN && poentry.Type != 0)
    bhasMatch = bhasMatch && (poentry.Type == Type);
  return bhasMatch;
};


CPOHandler::CPOHandler()
{};

CPOHandler::CPOHandler(const CXMLResdata& XMLResdata) : m_XMLResData(XMLResdata)
{
  m_POType = UNKNOWNPO;
};

CPOHandler::~CPOHandler()
{};

bool CPOHandler::FetchPOURLToMem (std::string strURL)
{
  ClearVariables();
  m_strBuffer = g_HTTPHandler.GetURLToSTR(strURL);

  return ProcessPOFile();
};

bool CPOHandler::ParsePOStrToMem (std::string const &strPOData)
{
  ClearVariables();
  m_strBuffer = strPOData;
  return ProcessPOFile();
};

bool CPOHandler::ProcessPOFile()
{
  if (m_strBuffer.empty())
    CLog::Log(logERROR, "CPODocument::ParseStrToMem: PO file to parse has a zero length.");

  m_strBuffer = "\n" + m_strBuffer;
  g_CharsetUtils.ConvertLineEnds(m_strBuffer);
  m_POfilelength = m_strBuffer.size();

  // we make sure, to have an LF at beginning and at end of buffer
  m_strBuffer[0] = '\n';
  if (*m_strBuffer.rbegin() != '\n')
  {
    m_strBuffer += "\n";
  }
  m_POfilelength = m_strBuffer.size();

  if (!GetNextEntry(true) || m_Entry.Type != MSGID || m_CurrentEntryText.find("msgid \"\"")  == std::string::npos ||
      m_CurrentEntryText.find("msgstr \"\"") == std::string::npos)
  {
    CLog::Log(logERROR, "POParser: unable to read PO file header.");
  }

  m_Entry.Type = HEADER;

  m_strHeader = m_CurrentEntryText.substr(1);

  // extract nplurals number from the PO file header
  size_t pos1, pos2;
  pos1 = m_strHeader.find("nplurals=",0)+9;
  if (pos1 == std::string::npos)
    CLog::Log(logERROR, "POHandler: No valid nplurals entry found in PO header");
  pos2 = m_strHeader.find(";",pos1);
  if (pos2 == std::string::npos)
    CLog::Log(logERROR, "POHandler: No valid nplurals entry found in PO header");
  std::stringstream ss;//create a stringstream
  ss << m_strHeader.substr(pos1, pos2-pos1);
  ss >> m_nplurals;

  m_mapPOData.clear();
  m_mapSequenceIndex.clear();
  m_mapClassicDataIndex.clear();
  m_CommsCntr = 0;

  bool bMultipleComment = false;
  std::vector<std::string> vecILCommnts;

  while ((GetNextEntry(false)))
  {
    ParseEntry();

    if (m_Entry.Type == COMMENT_ENTRY)
    {
      if (!vecILCommnts.empty())
        bMultipleComment = true;
      vecILCommnts = m_Entry.interlineComm;
      if (!bMultipleComment && !vecILCommnts.empty())
        m_CommsCntr++;
      continue;
    }

    if (m_Entry.Type == NUMID || m_Entry.Type == MSGID || m_Entry.Type == MSGID_PLURAL)
//    if (m_Entry.Type == MSGID_FOUND || m_Entry.Type == MSGID_PLURAL_FOUND)
    {
      if (bMultipleComment)
        CLog::Log(logWARNING, "POHandler: multiple comment entries found. Using only the last one "
        "before the real entry. Entry after comments: %s", m_CurrentEntryText.c_str());
      if (!m_Entry.interlineComm.empty())
        CLog::Log(logWARNING, "POParser: interline comments (eg. #comment) is not alowed inside "
        "a real po entry. Cleaned it. Problematic entry: %s", m_CurrentEntryText.c_str());
      m_Entry.interlineComm = vecILCommnts;
      bMultipleComment = false;
      vecILCommnts.clear();

      if (!m_Entry.msgIDPlur.empty())
      {
        if (GetPluralNumOfVec(m_Entry.msgStrPlural) != m_nplurals)
          m_Entry.msgStrPlural.clear(); // in case there is insufficient number of translated plurals we completely clear it
      }

      AddPOEntryToMaps(m_Entry);

      ClearCPOEntry(m_Entry);
    }
  }
  m_strBuffer.clear();
  return true;
};

void CPOHandler::ClearCPOEntry (CPOEntry &entry)
{
  entry.msgStrPlural.clear();
  entry.referenceComm.clear();
  entry.extractedComm.clear();
  entry.translatorComm.clear();
  entry.interlineComm.clear();
  entry.msgID.clear();
  entry.msgStr.clear();
  entry.msgIDPlur.clear();
  entry.msgCtxt.clear();
  entry.Type = UNKNOWN;
  m_CurrentEntryText.clear();
};


bool CPOHandler::WritePOFile(const std::string& strOutputPOFilename)
{
  ClearVariables();

    m_bhasLFWritten = false;

  m_strOutBuffer.clear();
  m_strOutBuffer = m_strHeader;

  for (T_itPOData it = m_mapPOData.begin(); it != m_mapPOData.end(); it++)
  {
    WritePOEntry(it->second);
  }

  std::string strDir = g_File.GetPath(strOutputPOFilename);
  g_File.MakeDir(strDir);

  // Initalize the output po document
  FILE * pPOTFile = fopen (strOutputPOFilename.c_str(),"wb");
  if (pPOTFile == NULL)
  {
    CLog::Log(logERROR, "POParser: Error opening output file: %s\n", strOutputPOFilename.c_str());
    return false;
  }
  if (m_strOutBuffer.find('\x00') != std::string::npos)
    CLog::Log(logERROR, "CPHandler::SaveFile: Unexpected zero byte in file: %s", strOutputPOFilename.c_str());
  fprintf(pPOTFile, "%s", m_strOutBuffer.c_str());
  fclose(pPOTFile);

  return true;
};

// Data manipulation functions

bool CPOHandler::FindEntry (const CPOEntry &EntryToFind)
{

  if (EntryToFind.Type == NUMID)
  {
    T_itPOData it = m_mapPOData.find(EntryToFind.numID + 0x100);
    if (it == m_mapPOData.end())
      return false;
    if (it->second.MatchMsgid(EntryToFind))
    {
      m_itLastFound = it;
      return true;
    }
    else
      return false;
  }
  else
  {
    std::string sKeyToFind = EntryToFind.msgCtxt;
    if (!EntryToFind.msgID.empty())
      sKeyToFind += "|" + EntryToFind.msgID;

    T_itClassicPOData it = m_mapClassicDataIndex.find(sKeyToFind);
    if (it == m_mapClassicDataIndex.end())
      return false;
    if (m_mapPOData[it->second].MatchMsgid(EntryToFind))
    {
      m_itLastFound = m_mapPOData.find(it->second);
      return true;
    }
    else
      return false;
  }
}

const CPOEntry*  CPOHandler::PLookforClassicEntry (CPOEntry &EntryToFind)
{
  if (EntryToFind.Type == NUMID)
  {
    T_itPOData it = m_mapPOData.find(EntryToFind.numID + 0x100);
    if (it == m_mapPOData.end())
      return NULL;
    if (it->second == EntryToFind)
    {
      return &(it->second);
    }
    else
      return NULL;
  }
  else
  {
    std::string sKeyToFind = EntryToFind.msgCtxt;
    if (!EntryToFind.msgID.empty())
      sKeyToFind += "|" + EntryToFind.msgID;

    T_itClassicPOData it = m_mapClassicDataIndex.find(sKeyToFind);
    if (it == m_mapClassicDataIndex.end())
      return NULL;
    if (m_mapPOData[it->second] == EntryToFind)
    {
      return &(m_mapPOData[it->second]);
    }
    else
      return NULL;
  }
}

bool CPOHandler::AddClassicEntry (CPOEntry EntryToAdd, CPOEntry const &POEntryEN, bool bCopyComments)
{
  if (bCopyComments)
  {
    EntryToAdd.extractedComm = POEntryEN.extractedComm;
    EntryToAdd.interlineComm = POEntryEN.interlineComm;
    EntryToAdd.referenceComm = POEntryEN.referenceComm;
    EntryToAdd.translatorComm = POEntryEN.translatorComm;
  }
  else
  {
    EntryToAdd.extractedComm.clear();
    EntryToAdd.interlineComm.clear();
    EntryToAdd.referenceComm.clear();
    EntryToAdd.translatorComm.clear();
  }

  AddPOEntryToMaps(EntryToAdd);
  return true;
};
void CPOHandler::AddAddonXMLEntries (const CAddonXMLEntry& AddonXMLEntry, const CAddonXMLEntry& AddonXMLEntrySRC)
{
  CPOEntry EntryToAdd;
  EntryToAdd.Type = MSGID;

  if (!AddonXMLEntry.strSummary.empty())
  {
    EntryToAdd.msgCtxt = "Addon Summary";
    EntryToAdd.msgID = AddonXMLEntrySRC.strSummary;
    EntryToAdd.msgStr = AddonXMLEntry.strSummary;
    AddPOEntryToMaps(EntryToAdd);
  }
  if (!AddonXMLEntry.strDescription.empty())
  {
    EntryToAdd.msgCtxt = "Addon Description";
    EntryToAdd.msgID = AddonXMLEntrySRC.strDescription;
    EntryToAdd.msgStr = AddonXMLEntry.strDescription;
    AddPOEntryToMaps(EntryToAdd);
  }
  if (!AddonXMLEntry.strDisclaimer.empty())
  {
    EntryToAdd.msgCtxt = "Addon Disclaimer";
    EntryToAdd.msgID = AddonXMLEntrySRC.strDisclaimer;
    EntryToAdd.msgStr = AddonXMLEntry.strDisclaimer;
    AddPOEntryToMaps(EntryToAdd);
  }
};

void CPOHandler::AddPOEntryToMaps (const CPOEntry& Entry)
{
  // store m_entry content in memory maps, separating the way it is stored for numid, msgid and addon data types
  // we also create two additional index maps for faster sequencial lookup and classic entry lookup
  size_t iEntryCounter = m_mapPOData.size();

  if (Entry.Type == NUMID)
  {
    unsigned long long iKey = Entry.numID + 0x100;
    if (m_mapPOData.find(iKey) != m_mapPOData.end())
      CLog::Log(logERROR, "POParser: duplicated numeric entry in PO file.");
    m_mapPOData[iKey] = Entry;
    m_mapSequenceIndex[iEntryCounter] = iKey;
  }
  else if (Entry.msgCtxt == "Addon Summary")
  {
    if (m_mapPOData.find(0) != m_mapPOData.end())
      CLog::Log(logERROR, "POParser: duplicated \"Addon Summary\" entry in PO file.");
    m_mapPOData[0] = Entry;
    m_mapClassicDataIndex["Addon Summary"] = 0;
    m_mapClassicDataIndex["Addon Summary|" + Entry.msgID] = 0;
    m_mapSequenceIndex[iEntryCounter] = 0;
  }
  else if (Entry.msgCtxt == "Addon Description")
  {
    if (m_mapPOData.find(1) != m_mapPOData.end())
      CLog::Log(logERROR, "POParser: duplicated \"Addon Description\" entry in PO file.");
    m_mapPOData[1] = Entry;
    m_mapClassicDataIndex["Addon Description"] = 1;
    m_mapClassicDataIndex["Addon Description|" + Entry.msgID] = 1;
    m_mapSequenceIndex[iEntryCounter] = 1;
  }
  else if (Entry.msgCtxt == "Addon Disclaimer")
  {
    if (m_mapPOData.find(2) != m_mapPOData.end())
      CLog::Log(logERROR, "POParser: duplicated \"Addon Disclaimer\" entry in PO file.");
    m_mapPOData[2] = Entry;
    m_mapClassicDataIndex["Addon Disclaimer"] = 2;
    m_mapClassicDataIndex["Addon Disclaimer|" + Entry.msgID] = 2;
    m_mapSequenceIndex[iEntryCounter] = 2;
  }
  else
  {
    unsigned long long iKey = iEntryCounter + 0x10000000000;
    if (m_mapPOData.find(iKey) != m_mapPOData.end())
      CLog::Log(logERROR, "POParser: duplicated classic PO entry in PO file.");
    m_mapPOData[iKey] = Entry;
    m_mapClassicDataIndex[Entry.msgCtxt + "|" + Entry.msgID] = 0;
    m_mapSequenceIndex[iEntryCounter] = iKey;
  }
}

bool CPOHandler::ModifyClassicEntry (CPOEntry &EntryToFind, CPOEntry EntryNewValue)
{

  std::string sKeyToFind = EntryToFind.msgCtxt;
  if (!EntryToFind.msgID.empty())
    sKeyToFind += "|" + EntryToFind.msgID;

  T_itClassicPOData it = m_mapClassicDataIndex.find(sKeyToFind);
  if (it == m_mapClassicDataIndex.end())
  {
    AddPOEntryToMaps(EntryNewValue);
    return false;
  }
  if (m_mapPOData[it->second] == EntryToFind)
  {
    m_mapPOData[it->second] = EntryNewValue;
    return true;
  }
  return false;
}

void CPOHandler::SetAddonMetaData (CAddonXMLEntry const &AddonXMLEntry, CAddonXMLEntry const &PrevAddonXMLEntry,
                                   CAddonXMLEntry const &AddonXMLEntryEN, std::string const &strLang)
{
  CPOEntry POEntryDesc, POEntryDiscl, POEntrySumm;
  POEntryDesc.Type = MSGID;
  POEntryDiscl.Type = MSGID;
  POEntrySumm.Type = MSGID;
  POEntryDesc.msgCtxt = "Addon Description";
  POEntryDiscl.msgCtxt = "Addon Disclaimer";
  POEntrySumm.msgCtxt = "Addon Summary";

  CPOEntry newPOEntryDesc = POEntryDesc;
  CPOEntry newPOEntryDisc = POEntryDiscl;
  CPOEntry newPOEntrySumm = POEntrySumm;

  newPOEntryDesc.msgID = AddonXMLEntryEN.strDescription;
  newPOEntryDisc.msgID = AddonXMLEntryEN.strDisclaimer;
  newPOEntrySumm.msgID = AddonXMLEntryEN.strSummary;

  if (strLang != m_XMLResData.strSourceLcode)
  {
    if (!AddonXMLEntry.strDescription.empty() && (m_XMLResData.bForceTXUpd || AddonXMLEntry.strDescription != PrevAddonXMLEntry.strDescription))
      newPOEntryDesc.msgStr = AddonXMLEntry.strDescription;
    if (!AddonXMLEntry.strDisclaimer.empty() && (m_XMLResData.bForceTXUpd || AddonXMLEntry.strDisclaimer != PrevAddonXMLEntry.strDisclaimer))
      newPOEntryDisc.msgStr = AddonXMLEntry.strDisclaimer;
    if (!AddonXMLEntry.strSummary.empty() && (m_XMLResData.bForceTXUpd || AddonXMLEntry.strSummary != PrevAddonXMLEntry.strSummary))
      newPOEntrySumm.msgStr = AddonXMLEntry.strSummary;
  }

  if (!newPOEntryDesc.msgID.empty() && (strLang == m_XMLResData.strSourceLcode || !newPOEntryDesc.msgStr.empty()))
    ModifyClassicEntry(POEntryDesc, newPOEntryDesc);
  if (!newPOEntryDisc.msgID.empty() && (strLang == m_XMLResData.strSourceLcode || !newPOEntryDisc.msgStr.empty()))
    ModifyClassicEntry(POEntryDiscl, newPOEntryDisc);
  if (!newPOEntrySumm.msgID.empty() && (strLang == m_XMLResData.strSourceLcode || !newPOEntrySumm.msgStr.empty()))
    ModifyClassicEntry(POEntrySumm, newPOEntrySumm);
  return;
}
/*
void CPOHandler::GetAddonMetaData (CAddonXMLEntry &AddonXMLEntry, CAddonXMLEntry &AddonXMLEntryEN)
{
  CAddonXMLEntry newAddonXMLEntry, newENAddonXMLEntry;
  CPOEntry POEntry;
  POEntry.msgCtxt = "Addon Summary";
  if (FindEntry(POEntry))
  {
    newAddonXMLEntry.strSummary = POEntry.msgStr;
    newENAddonXMLEntry.strSummary = POEntry.msgID;
  }

  ClearCPOEntry(POEntry);
  POEntry.msgCtxt = "Addon Description";
  if (FindEntry(POEntry))
  {
    newAddonXMLEntry.strDescription = POEntry.msgStr;
    newENAddonXMLEntry.strDescription = POEntry.msgID;
  }

  ClearCPOEntry(POEntry);
  POEntry.msgCtxt = "Addon Disclaimer";
  if (FindEntry(POEntry))
  {
    newAddonXMLEntry.strDisclaimer = POEntry.msgStr;
    newENAddonXMLEntry.strDisclaimer = POEntry.msgID;
  }
  AddonXMLEntry = newAddonXMLEntry;
  AddonXMLEntryEN = newENAddonXMLEntry;
  return;
}
*/

void CPOHandler::CreateHeader (const std::string &strPreText, const std::string& sLCode)
{
  m_strHeader = "# Kodi Media Center language file\n";
  m_strHeader += strPreText;

  m_sLCode = sLCode;
  std::stringstream ss;//create a stringstream
  m_nplurals = g_LCodeHandler.GetnPlurals(sLCode);
  ss << m_nplurals;
  std::string strnplurals = ss.str();

  m_strHeader += "msgid \"\"\n";
  m_strHeader += "msgstr \"\"\n";
  m_strHeader += "\"Project-Id-Version: " + m_XMLResData.strTargetProjectNameLong + "\\n\"\n";
  m_strHeader += "\"Report-Msgid-Bugs-To: " + m_XMLResData.strSupportEmailAdd + "\\n\"\n";
  m_strHeader += "\"POT-Creation-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n";
  m_strHeader += "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n";
  m_strHeader += "\"Last-Translator: Kodi Translation Team\\n\"\n";
  m_strHeader += "\"Language-Team: " + g_LCodeHandler.GetLangFromLCode(sLCode, m_XMLResData.strLangteamLFormat) +
                 " (http://www.transifex.com/projects/p/" + m_XMLResData.strTargetProjectName +"/language/"
                 + g_LCodeHandler.GetLangFromLCode(sLCode, m_XMLResData.strTargTXLFormat) +"/)" + "\\n\"\n";
  m_strHeader += "\"MIME-Version: 1.0\\n\"\n";
  m_strHeader += "\"Content-Type: text/plain; charset=UTF-8\\n\"\n";
  m_strHeader += "\"Content-Transfer-Encoding: 8bit\\n\"\n";
  m_strHeader +=  "\"Language: " + g_LCodeHandler.GetLangFromLCode(sLCode, m_XMLResData.strTargTXLFormat) + "\\n\"\n";
  m_strHeader +=  "\"Plural-Forms: nplurals=" + strnplurals + "; plural=" + g_LCodeHandler.GetPlurForm(sLCode) + ";\\n\"\n";
}

/*
bool CPOHandler::AddNumPOEntryByID(uint32_t numid, CPOEntry const &POEntry, CPOEntry const &POEntryEN, bool bCopyComments)
{
  if (m_mapStrings.find(numid) != m_mapStrings.end())
    return false;
  m_mapStrings[numid] = POEntry;
  m_mapStrings[numid].msgID = POEntryEN.msgID;
  if (bCopyComments)
  {
    m_mapStrings[numid].extractedComm = POEntryEN.extractedComm;
    m_mapStrings[numid].interlineComm = POEntryEN.interlineComm;
    m_mapStrings[numid].referenceComm = POEntryEN.referenceComm;
    m_mapStrings[numid].translatorComm = POEntryEN.translatorComm;
  }
  else
  {
    m_mapStrings[numid].extractedComm.clear();
    m_mapStrings[numid].interlineComm.clear();
    m_mapStrings[numid].referenceComm.clear();
    m_mapStrings[numid].translatorComm.clear();
  }
  return true;
}

const CPOEntry* CPOHandler::GetNumPOEntryByID(uint32_t numid)
{
  if (m_mapStrings.find(numid) == m_mapStrings.end())
    return NULL;
  return &(m_mapStrings[numid]);
}

const CPOEntry* CPOHandler::GetNumPOEntryByIdx(size_t pos) const
{
  std::map<uint32_t, CPOEntry>::const_iterator it_mapStrings;
  it_mapStrings = m_mapStrings.begin();
  advance(it_mapStrings, pos);
  return &(it_mapStrings->second);
}
*/
const CPOEntry* CPOHandler::GetClassicPOEntryByIdx(size_t pos)
{
  T_itSequenceIndex itseq = m_mapSequenceIndex.find(pos);
  if (itseq == m_mapSequenceIndex.end())
    CLog::Log(logERROR, "CPOHandler::GetClassicPOEntryByIdx: not found entry at position: %i", pos);

  T_itPOData it = m_mapPOData.find(itseq->second);
  return &(it->second);
}

unsigned int CPOHandler::GetPluralNumOfVec(std::vector<std::string> &vecPluralStrings)
{
  int num = 0;
  for (std::vector<std::string>::iterator it = vecPluralStrings.begin(); it != vecPluralStrings.end(); it++)
  {
    if (!it->empty())
      num++;
  }
  return num;
}

void CPOHandler::FetchLangAddonXML(const std::string &strURL)
{
  m_strLangAddonXML = g_HTTPHandler.GetURLToSTR(strURL);
  if (m_strLangAddonXML.empty())
    CLog::Log(logERROR, "CPOHandler::FetchLangAddonXML: http error reading XML file from url: %s", strURL.c_str());
}

void CPOHandler::WriteLangAddonXML(const std::string &strPath)
{
  if (!g_File.WriteFileFromStr(strPath, m_strLangAddonXML))
    CLog::Log(logERROR, "CPOHandler::WriteLangAddonXML: file write error, to output file: %s", strPath.c_str());
}

void CPOHandler::BumpLangAddonXMLVersion()
{
  size_t pos0, pos1, pos2;
  pos0 = m_strLangAddonXML.find("<addon");
  if (pos0 == std::string::npos)
    CLog::Log(logERROR, "CPOHandler::BumpLangAddonXMLVersion: Wrong Version format for language: %s", m_sLCode.c_str());

  pos1 = m_strLangAddonXML.find("version=\"", pos0);
  if (pos1 == std::string::npos)
    CLog::Log(logERROR, "CPOHandler::BumpLangAddonXMLVersion: unable to bump version number for language: %s", m_sLCode.c_str());

  pos1 += 9;
  pos2 = m_strLangAddonXML.find("\"", pos1);
  if (pos2 == std::string::npos)
    CLog::Log(logERROR, "CPOHandler::BumpLangAddonXMLVersion: unable to bump version number for language: %s", m_sLCode.c_str());

  std::string strVersion = m_strLangAddonXML.substr(pos1, pos2-pos1);

  size_t posLastDot = strVersion.find_last_of(".");
  if (posLastDot == std::string::npos)
    CLog::Log(logERROR, "CPOHandler::BumpLangAddonXMLVersion: Wrong Version format for language: %s", m_sLCode.c_str());

  std::string strLastNumber = strVersion.substr(posLastDot+1);
  if (strLastNumber.find_first_not_of("0123456789") != std::string::npos)
    CLog::Log(logERROR, "CPOHandler::BumpLangAddonXMLVersion: Wrong Version format for language: %s", m_sLCode.c_str());

  int LastNum = atoi (strLastNumber.c_str());
  strLastNumber = g_CharsetUtils.IntToStr(LastNum +1);
  strVersion = strVersion.substr(0, posLastDot +1) +strLastNumber;
  m_strLangAddonXML = m_strLangAddonXML.substr(0,pos1) + strVersion + m_strLangAddonXML.substr(pos2);
}

void CPOHandler::ClearVariables()
{
  m_CursorPos = 0;
  m_nextEntryPos = 0;
  m_POfilelength = 0;
  m_bhasLFWritten = false;
  m_previd = -1;
  m_writtenEntry = 0;
};




bool CPOHandler::GetNextEntry(bool bSkipError)
{
  do
  {
    // if we don't find LFLF, we reached the end of the buffer and the last entry to check
    // we indicate this with setting m_nextEntryPos to the end of the buffer
    if ((m_nextEntryPos = m_strBuffer.find("\n\n", m_CursorPos)) == std::string::npos)
      m_nextEntryPos = m_POfilelength-1;

    // now we read the actual entry into a temp string for further processing
    m_CurrentEntryText.assign(m_strBuffer, m_CursorPos, m_nextEntryPos - m_CursorPos +1);
    size_t oldCursorPos = m_CursorPos;
    m_CursorPos = m_nextEntryPos+1; // jump cursor to the second LF character

    if (FindLineStart ("\nmsgid "))
    {
      if (FindLineStart ("\nmsgctxt \"#"))
      {
        size_t ipos = m_CurrentEntryText.find("\nmsgctxt \"#");
        if (isdigit(m_CurrentEntryText[ipos+11]))
        {
          m_Entry.Type = NUMID; // we found an entry with a valid numeric id
          return true;
        }
      }

      if (FindLineStart ("\nmsgid_plural "))
      {
        m_Entry.Type = MSGID_PLURAL; // we found a pluralized entry
        return true;
      }

      m_Entry.Type = MSGID; // we found a normal entry, with no numeric id
      return true;
    }
    if (FindLineStart ("\n#"))
    {
      size_t ipos = m_CurrentEntryText.find("\n#");
      if (m_CurrentEntryText[ipos+2] != ' ' && m_CurrentEntryText[ipos+2] != '.' &&
          m_CurrentEntryText[ipos+2] != ':' && m_CurrentEntryText[ipos+2] != ',' &&
          m_CurrentEntryText[ipos+2] != '|')
      {
        m_Entry.Type = COMMENT_ENTRY; // we found a pluralized entry
        return true;
      }
    }
    if (m_nextEntryPos != m_POfilelength-1 && !bSkipError)
    {
      if (m_CurrentEntryText.find_first_not_of("\n") == std::string::npos)
        CLog::Log(logINFO, "POParser: Empty line(s) found at position: %i, ignored.", oldCursorPos);
      else
        CLog::Log(logWARNING, "POParser: unknown entry found at position %i, Failed entry: %s", oldCursorPos,
                  m_CurrentEntryText.substr(0,m_CurrentEntryText.size()-1).c_str());
    }
  }
  while (m_nextEntryPos != m_POfilelength-1);
  // we reached the end of buffer AND we have not found a valid entry

  return false;
};

void CPOHandler::ParseEntry()
{
  m_Entry.msgStrPlural.clear();
  m_Entry.translatorComm.clear();
  m_Entry.referenceComm.clear();
  m_Entry.interlineComm.clear();
  m_Entry.extractedComm.clear();
  m_Entry.numID = 0;
  m_Entry.msgID.clear();
  m_Entry.msgStr.clear();
  m_Entry.msgIDPlur.clear();
  m_Entry.msgCtxt.clear();

  size_t LineCursor = 1;
  size_t NextLineStart = 0;
  std::string strLine;
  std::string * pPlaceToParse = NULL;

  while ((NextLineStart = m_CurrentEntryText.find('\n', LineCursor)) != std::string::npos)
  {

    std::string strTemp;
    strTemp.assign(m_CurrentEntryText, LineCursor, NextLineStart - LineCursor +1);
    size_t strStart = strTemp.find_first_not_of("\n \t");
    size_t strEnd = strTemp.find_last_not_of("\n \t");

    if (strStart != std::string::npos && strEnd != std::string::npos)
      strLine.assign(strTemp, strStart, strEnd - strStart +1);
    else
      strLine = "";

    LineCursor = NextLineStart +1;

    if (pPlaceToParse && ReadStringLine(strLine, pPlaceToParse,0))
      continue; // we are reading a continous multilne string
    else
    {
      if (m_XMLResData.bRebrand && pPlaceToParse)
        g_CharsetUtils.reBrandXBMCToKodi(pPlaceToParse);
      pPlaceToParse= NULL; // end of reading the multiline string
    }

//    if (HasPrefix(strLine, "msgctxt") && !HasPrefix(strLine, "msgctxt \"#") && strLine.size() > 9)
    if (HasPrefix(strLine, "msgctxt") && strLine.size() > 9)
    {
      if (HasPrefix(strLine, "msgctxt \"#") && strLine.size() > 10 && isdigit(strLine[10]))
        ParseNumID(strLine, 10);

      pPlaceToParse = &m_Entry.msgCtxt;
      if (!ReadStringLine(strLine, pPlaceToParse,8))
      {
        CLog::Log(logWARNING, "POParser: wrong msgctxt format. Failed entry: %s", m_CurrentEntryText.c_str());
        pPlaceToParse = NULL;
      }
    }

    else if (HasPrefix(strLine, "msgid_plural") && strLine.size() > 14)
    {
      pPlaceToParse = &m_Entry.msgIDPlur;
      if (!ReadStringLine(strLine, pPlaceToParse,13))
      {
        CLog::Log(logWARNING, "POParser: wrong msgid_plural format. Failed entry: %s", m_CurrentEntryText.c_str());
        pPlaceToParse = NULL;
      }
    }

    else if (HasPrefix(strLine, "msgid") && strLine.size() > 7)
    {
      pPlaceToParse = &m_Entry.msgID;
      if (!ReadStringLine(strLine, pPlaceToParse,6))
      {
        CLog::Log(logWARNING, "POParser: wrong msgid format. Failed entry: %s", m_CurrentEntryText.c_str());
        pPlaceToParse = NULL;
      }
    }

    else if (HasPrefix(strLine, "msgstr[") && strLine[8] == ']'&& strLine.size() > 11)
    {
      m_Entry.msgStrPlural.push_back("");
      pPlaceToParse = &m_Entry.msgStrPlural[m_Entry.msgStrPlural.size()-1];
      if (!ReadStringLine(strLine, pPlaceToParse,10))
      {
        CLog::Log(logWARNING, "POParser: wrong msgstr[] format. Failed entry: %s", m_CurrentEntryText.c_str());
        pPlaceToParse = NULL;
      }
    }

    else if (HasPrefix(strLine, "msgstr") && strLine.size() > 8)
    {
      pPlaceToParse = &m_Entry.msgStr;
      if (!ReadStringLine(strLine, pPlaceToParse,7))
      {
        CLog::Log(logWARNING, "POParser: wrong msgstr format. Failed entry: %s", m_CurrentEntryText.c_str());
        pPlaceToParse = NULL;
      }
    }
    else if (HasPrefix(strLine, "#:") && strLine.size() > 2)
    {
      std::string strCommnt = strLine.substr(2);
      if (strCommnt.at(0) != ' ')
      {
        strCommnt = " " + strCommnt;
        CLog::SyntaxLog(logWARNING, "POParser: Wrong comment format. Space needed. Failed entry: %s", m_CurrentEntryText.c_str());
      }
      m_Entry.referenceComm.push_back(strCommnt);
    }

    else if (HasPrefix(strLine, "#.") && strLine.size() > 2)
    {
      std::string strCommnt = strLine.substr(2);
      if (m_XMLResData.bRebrand)
        g_CharsetUtils.reBrandXBMCToKodi(&strCommnt);
      if (strCommnt.at(0) != ' ')
      {
        strCommnt = " " + strCommnt;
        CLog::SyntaxLog(logWARNING, "POParser: Wrong comment format. Space needed. Failed entry: %s", m_CurrentEntryText.c_str());
      }
      m_Entry.extractedComm.push_back(strCommnt);
    }

    else if (HasPrefix(strLine, "#") && strLine.size() > 1 && strLine[1] != '.' &&
      strLine[1] != ':' && strLine[1] != ' ')
    {
      std::string strCommnt = strLine.substr(1);
      if (m_XMLResData.bRebrand)
        g_CharsetUtils.reBrandXBMCToKodi(&strCommnt);
      if (strCommnt.substr(0,5) != "empty")
        m_Entry.interlineComm.push_back(strCommnt);
    }
    else if (HasPrefix(strLine, "# "))
    {
      std::string strCommnt = strLine.substr(2);
      if (m_XMLResData.bRebrand)
        g_CharsetUtils.reBrandXBMCToKodi(&strCommnt);
      m_Entry.translatorComm.push_back(strCommnt);
    }
    else
      CLog::Log(logWARNING, "POParser: unknown line type found. Failed entry: %s", m_CurrentEntryText.c_str());
  }

  if (m_XMLResData.bRebrand && pPlaceToParse)
    g_CharsetUtils.reBrandXBMCToKodi(pPlaceToParse);
  if ((m_Entry.Type == MSGID || m_Entry.Type == MSGID_PLURAL) &&  m_Entry.msgID == "")
  {
    m_Entry.msgID = " ";
    CLog::Log(logWARNING, "POParser: empty msgid field corrected to a space char. Failed entry: %s", m_CurrentEntryText.c_str());
    CLog::SyntaxLog(logWARNING, "POParser: empty msgid field corrected to a space char. Failed entry: %s", m_CurrentEntryText.c_str());
  }
  return;
};

bool CPOHandler::ReadStringLine(const std::string &line, std::string * pStrToAppend, int skip)
{
  int linesize = line.size();
  if (line[linesize-1] != '\"' || line[skip] != '\"') return false;
  std::string strToAppend;
  strToAppend.append(line, skip + 1, linesize - skip - 2);
  if (!g_CharsetUtils.IsValidUTF8(strToAppend))
    CLog::Log(logERROR, "POHandler::ReadStringLine: wrong utf8 sequence found in string: %s", strToAppend.c_str());
  pStrToAppend->append(g_CharsetUtils.UnescapeCPPString(strToAppend));
  return true;
};

const bool CPOHandler::HasPrefix(const std::string &strLine, const std::string &strPrefix)
{
  if (strLine.length() < strPrefix.length())
    return false;
  else
    return strLine.compare(0, strPrefix.length(), strPrefix) == 0;
};


bool CPOHandler::FindLineStart(const std::string &strToFind)
{

  if (m_CurrentEntryText.find(strToFind) == std::string::npos)
    return false; // if we don't find the string or if we don't have at least one char after it

  return true;
};


bool CPOHandler::ParseNumID(const std::string &strLineToCheck, size_t xIDPos)
{
  if (isdigit(strLineToCheck.at(xIDPos))) // verify if the first char is digit
  {
    // we check for the numeric id for the fist 10 chars (uint32)
    m_Entry.numID = strtol(&strLineToCheck[xIDPos], NULL, 10);
    return true;
  }

  CLog::Log(logWARNING, "POParser: Found numeric id descriptor, but no valid id can be read, "
         "entry was handled as normal msgid entry");
  CLog::Log(logWARNING, "POParser: The problematic entry: %s", m_CurrentEntryText.c_str());
  return false;
};



// ********* SAVE part


void CPOHandler::WritePOEntry(const CPOEntry &currEntry)
{
  m_bhasLFWritten = false;


  if ((m_bIsSRCLang || m_XMLResData.bForceComm) && currEntry.Type == NUMID && m_POType == MERGEDPO)
  {
    int id = currEntry.numID;
    if (id-m_previd >= 2 && m_previd > -1 && m_bIsSRCLang)
    {
      WriteLF();
      if (id-m_previd == 2)
        m_strOutBuffer += "#empty string with id "  + g_CharsetUtils.IntToStr(id-1) + "\n";
      if (id-m_previd > 2)
        m_strOutBuffer += "#empty strings from id " + g_CharsetUtils.IntToStr(m_previd+1) + " to " + g_CharsetUtils.IntToStr(id-1) + "\n";
    }
    WriteMultilineComment(currEntry.interlineComm, "#");
    m_previd =id;
  }

  m_bhasLFWritten = false;

  if (m_bIsSRCLang || m_XMLResData.bForceComm)
  {
    WriteMultilineComment(currEntry.translatorComm, "# ");
    WriteMultilineComment(currEntry.extractedComm,  "#.");
    WriteMultilineComment(currEntry.referenceComm,  "#:");
  }

  WriteLF();
//  if (currEntry.Type == ID_FOUND)
//    m_strOutBuffer += "msgctxt \"#" + IntToStr(currEntry.numID) + "\"\n";
//  else
  if (!currEntry.msgCtxt.empty())
    m_strOutBuffer += "msgctxt \"" + g_CharsetUtils.EscapeStringCPP(currEntry.msgCtxt) + "\"\n";

  WriteLF();
  m_strOutBuffer += "msgid \""  + g_CharsetUtils.EscapeStringCPP(currEntry.msgID) +  "\"\n";
  if (!currEntry.msgIDPlur.empty())
  {
    m_strOutBuffer += "msgid_plural \""  + g_CharsetUtils.EscapeStringCPP(currEntry.msgIDPlur) +  "\"\n";
    if (!m_bIsSRCLang)
    { // we have a plural entry with non English strings
      for (unsigned int n = 0 ; n != m_nplurals ; n++)
      {
        std::string strValue;
        if (n < currEntry.msgStrPlural.size())
          strValue = currEntry.msgStrPlural[n];

        std::stringstream ss;//create a stringstream
        ss << n;
        std::string strnplurals = ss.str();
        m_strOutBuffer += "msgstr[" + strnplurals +"] \"" + g_CharsetUtils.EscapeStringCPP(strValue) + "\"\n";
      }
    }
    else
    { // we have a plural entry with English strings
      for (unsigned int n = 0 ; n != m_nplurals ; n++)
      {
        std::stringstream ss;//create a stringstream
        ss << n;
        std::string strnplurals = ss.str();
        m_strOutBuffer += "msgstr[" + strnplurals +"] \"\"\n";
      }
    }
  }
  else
  {
    if (!m_bIsSRCLang)
      m_strOutBuffer += "msgstr \"" + g_CharsetUtils.EscapeStringCPP(currEntry.msgStr) + "\"\n";
    else
      m_strOutBuffer += "msgstr \"\"\n";
  }

  m_writtenEntry++;
};

void CPOHandler::WriteLF()
{
  if (!m_bhasLFWritten)
  {
    m_bhasLFWritten = true;
    m_strOutBuffer += "\n";
  }
};

void CPOHandler::WriteMultilineComment(std::vector<std::string> vecCommnts, std::string prefix)
{
  if (vecCommnts.empty())
    return;

  for (size_t i=0; i < vecCommnts.size(); i++)
  {
    WriteLF();
    m_strOutBuffer += prefix + vecCommnts[i] + "\n";
  }
  return;
};
