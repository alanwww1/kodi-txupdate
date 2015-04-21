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

#ifndef SETTINGS_H
#define SETTINGS_H

#pragma once

#include <string>

const size_t DEFAULTCACHEEXPIRE = 360; // 6 hours
const size_t DEFAULTMINCOMPLETION = 10; // %
const std::string DEFAULTMERGEDLANGDIR = "merged-langfiles";
const std::string DEFAULTTXUPDLANGDIR = "tempfiles_txupdate";
const std::string DEFAULTSOURCELCODE = "en_GB";
const std::string DEFAULTBASELCODE = "$(LCODE)";
const std::string DEFAULTTXLFORMAT = "$(LCODE)";

class CSettings
{
public:
  CSettings();
  ~CSettings();
  void SetProjectname(std::string strName);
  std::string GetProjectname();
  std::string GetProjectnameLong();
  void SetHTTPCacheExpire(size_t exptime);
  size_t GetHTTPCacheExpire();
  void SetMinCompletion(int complperc);
  int GetMinCompletion();
  void SetMergedLangfilesDir(std::string const &strMergedLangfilesDir);
  std::string GetMergedLangfilesDir();
  void SetTXUpdateLangfilesDir(std::string const &strTXUpdateLangfilesDir);
  std::string GetTXUpdateLangfilesDir();
  void SetForcePOComments(bool bForceComm);
  void SetRebrand(bool bRebrand);
  bool GetForcePOComments();
  bool GetRebrand();
  void SetSupportEmailAdd(std::string const &strEmailAdd);
  std::string GetSupportEmailAdd();
  std::string GetSourceLcode() {return m_strSourceLcode;}
  void SetSourceLcode(std::string strSourceLcode);
  bool GetForceTXUpdate();
  void SetForceTXUpdate(bool bForceTXUpd);
  void SetBaseLCode(std::string const &strBaseLCode);
  std::string GetBaseLCode();
  std::string GetDefaultTXLFormat();
  void SetDefaultTXLFormat(std::string const &strTXLFormat) {m_strDefTXLFormat = strTXLFormat;}
  void SetLangteamLFormat(std::string const &strLangteamLFormat) {m_strLangteamLFormat = strLangteamLFormat;}
  std::string GetLangteamLFormat() {return m_strLangteamLFormat;}
  std::string GetDefaultAddonLFormatinXML() {return m_DefaultAddonLFormatinXML;}
  std::string GetLangDatabaseURL() {return m_LangDatabaseURL;}
  void SetLangDatabaseURL(std::string const &strLangDatabaseURL) {m_LangDatabaseURL = strLangDatabaseURL;}

private:
  size_t m_CacheExpire;
  int m_minComplPercentage;
  std::string m_strProjectName;
  std::string m_strMergedLangfilesDir;
  std::string m_strTXUpdateLangfilesDir;
  std::string m_strSupportEmailAdd;
  std::string m_strProjectnameLong;
  std::string m_strSourceLcode;
  std::string m_strBaseLCode;
  std::string m_strDefTXLFormat;
  std::string m_strLangteamLFormat;
  std::string m_DefaultAddonLFormatinXML;
  std::string m_LangDatabaseURL;
  bool m_bForceComm;
  bool m_bRebrand;
  bool m_bForceTXUpd;
};

extern CSettings g_Settings;
#endif