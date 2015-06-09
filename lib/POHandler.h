/*
 *      Copyright (C) 2014 Team Kodi
 * 
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

#include <map>
#include "TinyXML/tinyxml.h"
#include "UpdateXMLHandler.h"
#include <vector>
#include <stdint.h>
#include <stdio.h>

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
  bool operator == (const CPOEntry &poentry) const;
};

typedef std::map<uint32_t, CPOEntry>::iterator itStrings;
typedef std::vector<CPOEntry>::iterator itClassicEntries;

class CPOHandler
{
public:
  CPOHandler();
  CPOHandler(const CXMLResdata& XMLResdata);
  ~CPOHandler();
  bool FetchPOURLToMem(std::string strURL, bool bSkipError);
  bool FetchXMLURLToMem (std::string strURL);
  void FetchLangAddonXML (const std::string &strURL);
  void WriteLangAddonXML(const std::string &strPath);
  bool ParsePOStrToMem (std::string const &strPOData, std::string const &strFilePath);
  bool WritePOFile(const std::string &strOutputPOFilename);
  bool WriteXMLFile(const std::string &strOutputPOFilename);
  bool LookforClassicEntry (CPOEntry &EntryToFind);
  const CPOEntry*  PLookforClassicEntry (CPOEntry &EntryToFind);
  bool AddClassicEntry (CPOEntry EntryToAdd, CPOEntry const &POEntryEN, bool bCopyComments);
  bool ModifyClassicEntry (CPOEntry &EntryToFind, CPOEntry EntryNewValue);
  bool DeleteClassicEntry (CPOEntry &EntryToFind);

//  const CPOEntry* GetNumPOEntryByID(uint32_t numid);
//  bool AddNumPOEntryByID(uint32_t numid, CPOEntry const &POEntry, CPOEntry const &POEntryEN, bool bCopyComments);
  const CPOEntry* GetClassicPOEntryByIdx(size_t pos) const;

//  const CPOEntry* GetNumPOEntryByIdx(size_t pos) const;
  void SetHeader (std::string strHeader) {m_strHeader = strHeader;}
  void SetHeaderNEW (std::string strLangCode);
  std::string GetHeader () {return m_strHeader;}
  void SetAddonMetaData (CAddonXMLEntry const &AddonXMLEntry, CAddonXMLEntry const &PrevAddonXMLEntry,
                         CAddonXMLEntry const &AddonXMLEntryEN, std::string const &strLang);
  void GetAddonMetaData (CAddonXMLEntry &AddonXMLEntry, CAddonXMLEntry &AddonXMLEntryEN);
  void SetPreHeader (std::string &strPreText);
//  size_t const GetNumEntriesCount() {return m_mapStrings.size();}
  size_t const GetClassEntriesCount() {return m_vecClassicEntries.size();}
  size_t const GetCommntEntriesCount() {return m_CommsCntr;}
  void SetIfIsSourceLang(bool bIsENLang) {m_bPOIsEnglish = bIsENLang;}
  void SetIfPOIsUpdTX(bool bIsUpdTX) {m_bPOIsUpdateTX = bIsUpdTX;}
  bool GetIfSourceIsXML () {return m_bIsXMLSource;}
  void SetLangAddonXMLString(std::string strXMLfile) {m_strLangAddonXML = strXMLfile;}
  std::string GetLangAddonXMLString () {return m_strLangAddonXML;}
  void BumpLangAddonXMLVersion();

protected:
  void ClearCPOEntry (CPOEntry &entry);
  bool ProcessPOFile();
  itStrings IterateToMapIndex(itStrings it, size_t index);
  bool GetXMLEncoding(const TiXmlDocument* pDoc, std::string& strEncoding);
  void GetXMLComment(std::string strXMLEncoding, const TiXmlNode *pCommentNode, CPOEntry &currEntry);
  int GetPluralNumOfVec(std::vector<std::string> &vecPluralStrings);
  void ParsePOHeader();

  std::string m_strHeader;
  std::string m_CurrentEntryText;
  std::string m_strLangCode;
  int m_nplurals;

//  std::map<uint32_t, CPOEntry> m_mapStrings;
  std::vector<CPOEntry> m_vecClassicEntries;

  CPOEntry m_prevCommEntry;
  bool m_bIsXMLSource;
  size_t m_CommsCntr;
  bool m_bPOIsEnglish;
  bool m_bPOIsUpdateTX;
  std::string m_strLangAddonXML;
  CXMLResdata m_XMLResData;





  bool SaveFile(const std::string &pofilename);
  bool GetNextEntry(bool bSkipError);
  void ParseEntry();
  void WriteHeader(const std::string &strHeader);
  void WritePOEntry(const CPOEntry &currEntry, unsigned int nplurals);
//  void SetIfIsEnglish(bool bIsENLang) {m_bIsForeignLang = !bIsENLang;}
//  void SetIfIsUpdDoc(bool bIsUpdTx) {m_bIsUpdateTxDoc = bIsUpdTx;}
  bool FetchURLToMem(const std::string &strURL, bool bSkipError);
  bool ParseStrToMem(const std::string &strPOData, std::string const &strFilePath);




  std::string IntToStr(int number);
  std::string UnescapeString(const std::string &strInput);
  bool FindLineStart(const std::string &strToFind);
  bool ParseNumID(const std::string &strLineToCheck, size_t xIDPos);
  void ConvertLineEnds(const std::string &filename);
  bool ReadStringLine(const std::string &line, std::string * pStrToAppend, int skip);
  const bool HasPrefix(const std::string &strLine, const std::string &strPrefix);
  void WriteLF();
  void WriteMultilineComment(std::vector<std::string> vecCommnts, std::string prefix);
  void ClearVariables();

  std::string m_strBuffer;
  size_t m_POfilelength;
  size_t m_CursorPos;
  size_t m_nextEntryPos;
  CPOEntry m_Entry;

  std::string m_strOutBuffer;
  bool m_bhasLFWritten;
  int m_previd;
  int m_writtenEntry;
};
