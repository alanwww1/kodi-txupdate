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

#include "Log.h"
#include "FileUtils/FileUtils.h"

static int m_numWarnings;

using namespace std;

CLog::CLog()
{
  m_numWarnings = 0;
}

CLog::~CLog()
{}

void CLog::Log(TLogLevel loglevel, const char *format, ... )
{

  if (loglevel == logLINEFEED)
  {
    printf("\n");
    return;
  }

  if (loglevel == logWARNING)
    m_numWarnings++;

  std::string strLogType;

  va_list va;
  va_start(va, format);

  std::string strFormat = format;

  printf(strFormat.c_str(), va);
  printf("\n");
  va_end(va);

  if (loglevel == logERROR || loglevel == logWARNING)
  {
    va_list va1;
    va_start(va1, format);
    char cstrLogMessage[1024];
    vsprintf(cstrLogMessage, format, va1);
    va_end(va1);
    if (loglevel == logERROR)
    {
      printf ("\nError message thrown: %s\n\n", cstrLogMessage);
      throw 1;
    }
    else
      printf ("\n%sWarning log message: %s%s\n\n", KRED, cstrLogMessage, RESET);
  }

  return;
};

void CLog::ResetWarnCounter()
{
  m_numWarnings = 0;
};

int CLog::GetWarnCount()
{
  return m_numWarnings;
};
