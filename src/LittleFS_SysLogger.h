/*
**  Program   : LittleFS_SysLogger.h
**
**  Version   : 2.0.1   (18-12-2022)
**
**  Copyright (c) 2022 .. 2023 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.
***************************************************************************/

#ifndef _LITTLEFS_SYSLOGGER_H
#define _LITTLEFS_SYSLOGGER_H

#include <FS.h>
#include <LittleFS.h>

class ESPSL {

  #define _DODEBUG
  #define _MAXLINEWIDTH 150
  #define _MINLINEWIDTH  50
  #define _MINNUMLINES   10
  #define _KEYLEN         9
  
public:
  ESPSL();

  boolean   begin(uint16_t depth,  uint16_t lineWidth);
  boolean   begin(uint16_t depth,  uint16_t lineWidth, boolean mode);
  void      status();
  boolean   write(const char*);
  boolean   writef(const char *fmt, ...);
  char     *buildD(const char *fmt, ...);
  boolean   writeDbg(const char *dbg, const char *fmt, ...);
  void      startReading(int16_t startLine, int16_t numLines);    // Returns last line read
  void      startReading(int16_t startLine);                      // Returns last line read
  bool      readNextLine(char *lineOut, int lineOutLen);
  String    dumpLogFile();
  boolean   removeSysLog();
  uint32_t  getLastLineID();
  void      setOutput(HardwareSerial *serIn, int baud);
  void      setOutput(Stream *serIn);
  void      setDebugLvl(int8_t debugLvl);
    
private:

  const char *_sysLogFile = "/sysLog.dat";
  HardwareSerial  *_Serial;
  Stream          *_Stream;
  boolean         _streamOn;
  boolean         _serialOn;

  File        _sysLog;
  uint32_t    _lastUsedLineID;
  uint32_t    _oldestLineID;
  int32_t     _noLines;
  int32_t     _lineWidth;
  int32_t     _recLength;
  uint32_t    _readPointer;
  uint32_t    _readEnd;
  int8_t      _debugLvl = 0;
  char        globalBuff[_MAXLINEWIDTH +15];
  int8_t      _emptyID = -1;
  
  boolean     create(uint16_t depth, uint16_t lineWidth);
  boolean     init();
  const char *rtrim(char *);
  boolean     checkSysLogFileSize(const char* func, int32_t cSize);
  void        fixLineWidth(char *inLine, int lineLen);
  void        fixRecLen(char *inLine, int32_t key, int recLen);
  int32_t     sysLogFileSize();
  void        print(const char*);
  void        println(const char*);
  void        printf(const char *fmt, ...);
  void        flush();
  int8_t      getDebugLvl();
  boolean     _Debug(int8_t Lvl);

};

#endif

/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* 
***************************************************************************/
