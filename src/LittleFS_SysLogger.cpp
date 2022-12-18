/***************************************************************************
**  Program   : LittleFS_SysLogger.cpp
**
**  Version   : 2.0.1   (18-12-2022)
**
**  Copyright (c) 2022 .. 2023 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.
****************************************************************************
*/

#include "LittleFS_SysLogger.h"

//-- Constructor
ESPSL::ESPSL() 
{ 
  _Serial   = NULL;
  _serialOn = false;
  _Stream   = NULL;
  _streamOn = false;
}

//-------------------------------------------------------------------------------------
//-- begin object
boolean ESPSL::begin(uint16_t depth, uint16_t lineWidth) 
{
  uint32_t  tmpID, recKey;
  
#ifdef _DODEBUG
  if (_Debug(1)) printf("ESPSL::begin(%d, %d)..\n", depth, lineWidth);
#endif

  int16_t fileSize;

  if (lineWidth > _MAXLINEWIDTH) { lineWidth = _MAXLINEWIDTH; }
  if (lineWidth < _MINLINEWIDTH) { lineWidth = _MINLINEWIDTH; }
  memset(globalBuff, 0, (_MAXLINEWIDTH +15));
  
  //-- check if the file exists ---
  if (!LittleFS.exists(_sysLogFile)) 
  {
    create(depth, lineWidth);
  }
  
  //-- check if the file can be opened ---
  _sysLog  = LittleFS.open(_sysLogFile, "r+");    //-- open for reading and writing
  if (!_sysLog) 
  {
    printf("ESPSL(%d)::begin(): Some error opening [%s] .. bailing out!\r\n", __LINE__, _sysLogFile);
    return false;
  } //-- if (!_sysLog)

  if (_sysLog.available() > 0) 
  {
#ifdef _DODEBUG
    if (_Debug(3)) printf("ESPSL(%d)::begin(): read record [0]\r\n", __LINE__);
#endif
    if (!_sysLog.seek(0, SeekSet)) 
    {
      printf("ESPSL(%d)::begin(): seek to position [%04d] failed (now @%d)\r\n", __LINE__
                                                                               , 0
                                                                               , _sysLog.position());
    }

    int l = _sysLog.readBytesUntil('\n', globalBuff, _MAXLINEWIDTH) -1; 

#ifdef _DODEBUG
        if (_Debug(4)) printf("ESPSL(%d)::begin(): rec[0] [%s]\r\n", __LINE__, globalBuff);
#endif
        sscanf(globalBuff,"%u|%u;%d;%d;" 
                                , &recKey
                                , &tmpID
                                , &_noLines
                                , &_lineWidth);
    Serial.flush();
    if (_noLines    < _MINNUMLINES)  { _noLines    = _MINNUMLINES; }
    if (_lineWidth  < _MINLINEWIDTH) { _lineWidth  = _MINLINEWIDTH; }
    _recLength = _lineWidth + _KEYLEN;
    if (tmpID > _lastUsedLineID) { _lastUsedLineID = tmpID; }
#ifdef _DODEBUG
    if (_Debug(4)) printf("ESPSL(%d)::begin(): rec[%u] -> [%8d][%d][%d]\r\n", __LINE__
                                                                                , recKey
                                                                                , _lastUsedLineID
                                                                                , _noLines
                                                                                , _lineWidth);
#endif
  } 
  
  if ((depth != _noLines) || lineWidth != _lineWidth)
  {
    _sysLog.close();
    removeSysLog();
    create(depth, lineWidth);
    _sysLog  = LittleFS.open(_sysLogFile, "r+");    //-- open for reading and writing
    if (!_sysLog) 
    {
      printf("ESPSL(%d)::begin(): Some error opening [%s] .. bailing out!\r\n", __LINE__, _sysLogFile);
      return false;
    } //-- if (!_sysLog)

  }
  
  memset(globalBuff, 0, sizeof(globalBuff));
  
  checkSysLogFileSize("begin():", (_noLines + 1) * (_recLength +1));  //-- add '\n'
  
  if (_noLines != depth) 
  {
    printf("ESPSL(%d)::begin(): lines in file (%d) <> %d !!\r\n", __LINE__, _noLines, depth);
  }
  if (_lineWidth != lineWidth)
  {
    printf("ESPSL(%d)::begin(): lineWidth in file (%d) <> %d !!\r\n", __LINE__, _lineWidth, lineWidth);
  }

  init();
  printf("ESPSL(%d):: after init()\r\n", __LINE__);

  return true; // We're all setup!
  
} //-- begin()

//-------------------------------------------------------------------------------------
//-- begin object
boolean ESPSL::begin(uint16_t depth, uint16_t lineWidth, boolean mode) 
{
#ifdef _DODEBUG
  if (_Debug(1)) printf("ESPSL::begin(%d, %d, %d)..\n", depth, lineWidth, mode);
#endif

  if (mode) 
  {
    _sysLog.close();
    removeSysLog();
    create(depth, lineWidth);
  }
  return (begin(depth, lineWidth));
  
} // begin()

//-------------------------------------------------------------------------------------
//-- Create a SysLog file on LittleFS
boolean ESPSL::create(uint16_t depth, uint16_t lineWidth) 
{
  File createFile;
  
#ifdef _DODEBUG
  if (_Debug(1)) printf("ESPSL(%d)::create(%d, %d)..\n", __LINE__, depth, lineWidth);
#endif

  int32_t bytesWritten;
  
  _noLines  = depth;
  if (lineWidth > _MAXLINEWIDTH)
          _lineWidth  = _MAXLINEWIDTH;
  else if (lineWidth < _MINLINEWIDTH) 
          _lineWidth  = _MINLINEWIDTH;
  else    _lineWidth  = lineWidth;
  
  _recLength = _lineWidth + _KEYLEN;

  //_nextFree = 0;
  memset(globalBuff, 0, sizeof(globalBuff));  
  //-- check if the file exists and can be opened ---
  createFile  = LittleFS.open(_sysLogFile, "w");    //-- open for writing
  if (!createFile) 
  {
    printf("ESPSL(%d)::create(): Some error opening [%s] .. bailing out!\r\n", __LINE__, _sysLogFile);
    createFile.close();
    return false;
  } //-- if (!_sysLog)


  snprintf(globalBuff, _lineWidth, "%08d;%d;%d; META DATA LittleFS_SysLogger", 0, _noLines, _lineWidth);
  fixLineWidth(globalBuff, _lineWidth);
  fixRecLen(globalBuff, 0, _recLength);
  if (_Debug(1)) printf("ESPSL(%d)::create(): rec(0) [%s](%d bytes)\r\n", __LINE__, globalBuff, strlen(globalBuff));
  bytesWritten = createFile.println(globalBuff) -1; //-- skip '\n'
  createFile.flush();
  if (bytesWritten != _recLength) 
  {
    printf("ESPSL(%d)::create(): ERROR!! written [%d] bytes but should have been [%d] for record [0]\r\n"
                                            ,__LINE__ , bytesWritten, _recLength);

    createFile.close();
    return false;
  }
  
  int r;
  for (r=0; r < _noLines; r++) 
  {
    yield();
    snprintf(globalBuff, _lineWidth, "%08d; === empty log regel (%d) ===", -1, (r+1));
    fixLineWidth(globalBuff, _lineWidth);
    fixRecLen(globalBuff, r, _recLength);
    bytesWritten = createFile.println(globalBuff) -1; //-- skip '\n'
    if (bytesWritten != _recLength) 
    {
      printf("ESPSL(%d)::create(): ERROR!! written [%d] bytes but should have been [%d] for record [%d]\r\n"
                                            ,__LINE__ , bytesWritten, _recLength, r);
      createFile.close();
      return false;
    }
    
  } //-- for r ....
  
  createFile.close();
  _lastUsedLineID = 0;
  _oldestLineID   = 1;
  return (true);
  
} // create()

//-------------------------------------------------------------------------------------
//-- read SysLog file and find next line to write to
boolean ESPSL::init() 
{
  int32_t offset, recKey = 0;
  int32_t readKey;
  char logText[(_lineWidth + _KEYLEN +1)];
  memset(logText, 0, (_lineWidth + _KEYLEN +1));

#ifdef _DODEBUG
  if (_Debug(1)) printf("ESPSL(%d)::init()..\r\n", __LINE__);
#endif
      
  if (!_sysLog) 
  {
    printf("ESPSL(%d)::init(): Some error opening [%s] .. bailing out!\r\n", __LINE__, _sysLogFile);
    return false;
  } //-- if (!_sysLog)

  _oldestLineID   = 0;
  _lastUsedLineID = 0;
  recKey          = 0;

  while (_sysLog.available() > 0) 
  {
    recKey++;
    //printf("ESPSL(%d)::init(): available() -> recKey[%d]..", __LINE__, recKey);
    offset = (recKey * (_recLength +1)); 
    if (!_sysLog.seek(offset, SeekSet)) 
    {
      printf("ESPSL(%d)::init(): seek to position [%d/%04d] failed (now @%d)\r\n", __LINE__, recKey
                                                                                  , offset
                                                                                  , _sysLog.position());
      //_sysLog.close();
      return false;
    }
#ifdef _DODEBUG
    if (_Debug(4)) printf("ESPSL(%d)::init(): -> read record (recKey) [%d/%04d]\r\n", __LINE__, recKey, offset);
#endif
    int l = _sysLog.readBytesUntil('\n', globalBuff, _recLength);
        sscanf(globalBuff,"%u|%u;[^\0]" , &readKey, &_oldestLineID, logText);
    if (_oldestLineID > 0)
    {
      if (_oldestLineID >= _lastUsedLineID) { _lastUsedLineID = (_oldestLineID -1); }
    }
    //_sysLog.close();
    
#ifdef _DODEBUG
    if (_Debug(4)) printf("ESPSL(%d)::init(): testing readKey[%07u] -> [%08d][%s]\r\n", __LINE__
                                                                                , readKey
                                                                                , logText);
#endif
/***
    if (_oldestLineID < _lastUsedLineID) 
    {
#ifdef _DODEBUG
      if (_Debug(4)) printf("ESPSL(%d)::init(): ----> readKey[%07d] _oldest[%8d] _lastUsed[%8d]\r\n"
                                                      , __LINE__
                                                      , readKey
                                                      , _oldestLineID
                                                      , _lastUsedLineID);
#endif
      return true; 
    }
    else if (readKey >= _noLines) 
    {
      _lastUsedLineID++;
      _oldestLineID = (_lastUsedLineID - _noLines) +1;
#ifdef _DODEBUG
      if (_Debug(3)) printf("ESPSL(%d)::init(eof): -> readKey[%07d] _oldest[%8d] _lastUsed[%8d]\r\n"
                                                      , __LINE__
                                                      , readKey
                                                      , _oldestLineID
                                                      , _lastUsedLineID);
#endif
      return true; 
    }
    _lastUsedLineID = _oldestLineID;
    yield();
**/
  } //-- while ..
  if (_lastUsedLineID < 0) { _lastUsedLineID = 1; }
  _oldestLineID = _lastUsedLineID +1;
  return false;

} // init()


//-------------------------------------------------------------------------------------
boolean ESPSL::write(const char* logLine) 
{
#ifdef _DODEBUG
  if (_Debug(3)) printf("ESPSL(%d)::write(%s)..\r\n", __LINE__, logLine);
#endif

  int32_t   bytesWritten;
  uint16_t  offset, seekToLine;
  int       nextFree;

  //_sysLog  = LittleFS.open(_sysLogFile, "r+");    //-- open for reading and writing

#ifdef _DODEBUG
  if (_Debug(4)) printf("ESPSL(%d)::write(): oldest[%8d], last[%8d], nextFree Slot[%3d]\r\n"
                                                      , __LINE__
                                                      , _oldestLineID
                                                      , _lastUsedLineID
                                                      , nextFree);
#endif
  
  char lineBuff[(_recLength + _KEYLEN +1)];
  memset(globalBuff, 0, (_recLength + _KEYLEN + 1));

  snprintf(globalBuff, _lineWidth, "%08d;%s", _oldestLineID, logLine);
  fixLineWidth(globalBuff, _lineWidth); 
  _lastUsedLineID++;
  fixRecLen(globalBuff, _lastUsedLineID, _recLength);
  seekToLine = (_lastUsedLineID % _noLines) +1; //-- always skip rec. 0 (status rec)
  offset = (seekToLine * (_recLength +1));
#ifdef _DODEBUG
  if (_Debug(4)) printf("ESPSL(%d)::write() -> slot[%d], seek[%d/%04d] [%s]\r\n", __LINE__
                                                                                , _lastUsedLineID
                                                                                , seekToLine, offset
                                                                                , lineBuff);
#endif
  if (!_sysLog.seek(offset, SeekSet)) 
  {
    printf("ESPSL(%d)::write(): seek to position [%d/%04d] failed (now @%d)\r\n", __LINE__, seekToLine
                                                                                , offset
                                                                                , _sysLog.position());
    //_sysLog.close();
    return false;
  }
  bytesWritten = _sysLog.println(globalBuff) -1; // don't count '\n'
  _sysLog.flush();
  //_sysLog.close();

  if (bytesWritten != _recLength) 
  {
      printf("ESPSL(%d)::write(): ERROR!! written [%d] bytes but should have been [%d]\r\n"
                                       , __LINE__, bytesWritten, _recLength);
      return false;
  }

  _oldestLineID = _lastUsedLineID +1; //-- 1 after last
  nextFree = (_lastUsedLineID % _noLines) + 1;  //-- always skip rec "0"

  return true;

} // write()


//-------------------------------------------------------------------------------------
boolean ESPSL::writef(const char *fmt, ...) 
{
  char lineBuff[(_MAXLINEWIDTH + 101)];
  memset(lineBuff, 0, (_MAXLINEWIDTH + 101));

  #ifdef _DODEBUG
  if (_Debug(3)) printf("ESPSL(%d)::writef(%s)..\r\n", __LINE__, fmt);
#endif

  va_list args;
  va_start (args, fmt);
  vsnprintf (lineBuff, (_MAXLINEWIDTH +100), fmt, args);
  va_end (args);

  //-- remove control chars
  for(int i=0; ((i<strlen(lineBuff)) && (lineBuff[i]!=0)); i++)
  {
    if ((lineBuff[i] < ' ') || (lineBuff[i] > '~')) { lineBuff[i] = '^'; }
  }

  bool retVal = write(lineBuff);

  return retVal;

} // writef()


//-------------------------------------------------------------------------------------
boolean ESPSL::writeDbg(const char *dbg, const char *fmt, ...) 
{
#ifdef _DODEBUG
  if (_Debug(3)) printf("ESPSL(%d)::writeDbg(%s, %s)..\r\n", __LINE__, dbg, fmt);
#endif

  int       nextFree;
  char dbgStr[(_MAXLINEWIDTH + 101)];
  memset(dbgStr, 0, (_MAXLINEWIDTH + 101));
  char totBuf[(_MAXLINEWIDTH + 101)];
  memset(totBuf, 0, (_MAXLINEWIDTH + 101));

  sprintf(dbgStr, "%s", dbg);
  //printf("ESPSL(%d)::writeDbg([%s], *fmt)..\r\n", __LINE__, dbgStr);
  
  va_list args;
  va_start (args, fmt);
  vsnprintf (totBuf, (_MAXLINEWIDTH +100), fmt, args);
  va_end (args);
  
  while ((strlen(totBuf) > 0) && (strlen(dbgStr) + strlen(totBuf)) > (_lineWidth -1)) 
  {
    totBuf[strlen(totBuf)-1] = '\0';
    yield();
  }
  if ((strlen(dbgStr) + strlen(totBuf)) < _lineWidth) 
  {
    strlcat(dbgStr, totBuf, _lineWidth);
  }

  //-- remove control chars
  for(int i=0; ((i<strlen(dbgStr)) && (dbgStr[i]!=0)); i++)
  {
    if ((dbgStr[i] < ' ') || (dbgStr[i] > '~')) { dbgStr[i] = '^'; }
  }
  //printf("ESPSL(%d)::writeDbg(): dbgStr[%s]..\r\n", __LINE__, dbgStr);
  bool retVal = write(dbgStr);
  
  return retVal;

} // writeDbg()


//-------------------------------------------------------------------------------------
char *ESPSL::buildD(const char *fmt, ...) 
{
#ifdef _DODEBUG
  if (_Debug(3)) printf("ESPSL(%d)::buildD(%s)..\r\n", __LINE__, fmt);
#endif
  memset(globalBuff, 0, sizeof(globalBuff));
  
  va_list args;
  va_start (args, fmt);
  vsnprintf (globalBuff, (_MAXLINEWIDTH), fmt, args);
  va_end (args);

  //-- remove control chars
  for(int i=0; ((i<strlen(globalBuff)) && (globalBuff[i]!=0)); i++)
  {
    if ((globalBuff[i] < ' ') || (globalBuff[i] > '~')) { globalBuff[i] = '^'; }
  }

  return globalBuff;
  
} // buildD()


//-------------------------------------------------------------------------------------
//-- set pointer to startLine
void ESPSL::startReading(int16_t startLine, int16_t numLines) 
{
#ifdef _DODEBUG
  //if (_Debug(1)) 
  printf("ESPSL(%d)::startReading([%d], [%d])..\r\n", __LINE__, startLine, numLines);
#endif

  if (startLine < 0) 
  {
    _readPointer     = _lastUsedLineID + startLine +numLines;
    printf("ESPSL(%d)::startReading(start<0): _last[%d], start[%d], _readP[%d]\r\n"
                                              , __LINE__
                                              , _lastUsedLineID, startLine, _readPointer);
  }
  else if (startLine == 0) 
        _readPointer = _oldestLineID;
  else 
  {
    _readPointer = _oldestLineID + startLine;
    printf("ESPSL(%d)::startReading(start>0): _oldest[%d], start[%d], _readP[%d]\r\n"
                                              , __LINE__
                                              , _oldestLineID, startLine, _readPointer);
  }
  if (_readPointer > _lastUsedLineID) _readPointer = _oldestLineID;
  if (numLines == 0)
        _readEnd     = _lastUsedLineID;
  else  _readEnd     = (_readPointer + numLines) -1;
  if (_readEnd > _lastUsedLineID) _readEnd = _lastUsedLineID; 
#ifdef _DODEBUG
  //if (_Debug(3)) 
  printf("ESPSL(%d)::startReading(): _readPointer[%d], readEnd[%d], _oldest[%d], _last[%d], Start[%d], End[%d]\r\n"
                                              , __LINE__
                                              , _readPointer
                                              , _readEnd
                                              , _oldestLineID
                                              , _lastUsedLineID
                                              , _readPointer, _readEnd);
#endif

} // startReading()


//-------------------------------------------------------------------------------------
//-- set pointer to startLine
void ESPSL::startReading(int16_t startLine) 
{
#ifdef _DODEBUG
  //if (_Debug(1)) 
  printf("ESPSL(%d)::startReading([%d])..\r\n", __LINE__, startLine);
#endif
  startReading(startLine, 0);

} // startReading()

//-------------------------------------------------------------------------------------
//-- start reading from startLine
bool ESPSL::readNextLine(char *lineOut, int lineOutLen)
{
  uint32_t  offset;
  int32_t   dummyKey, recKey = _readPointer;
  int32_t   lineID;
  uint16_t  seekToLine;  
  File      tmpFile;
  char      lineIn[_recLength];
  
#ifdef _DODEBUG
  if (_Debug(1)) printf("ESPSL(%d)::readNextLine(%d) _recLength[%d]\r\n", __LINE__, _readPointer, _recLength);
#endif
  memset(globalBuff, 0, (_recLength+10));
  memset(lineIn, 0, _recLength);

  if (_readPointer > _readEnd) return false;
  
  if (!_sysLog)
  {
    printf("ESPSL(%d)::readNextLine(): _sysLog (%s) not open\r\n", __LINE__, _sysLogFile);
  }

  for (recKey = _readPointer; recKey <= _readEnd; recKey++) 
  {
    seekToLine = (recKey % _noLines) +1;
    offset     = (seekToLine * (_recLength +1));
    if (!_sysLog.seek(offset, SeekSet)) 
    {
      printf("ESPSL(%d)::readNextLine(): seek to position [%d/%04d] failed (now @%d)\r\n", __LINE__
                                                                                         , seekToLine
                                                                                         , offset
                                                                                         , _sysLog.position());
      return true;
    }

    int l = _sysLog.readBytesUntil('\n', globalBuff, _recLength);
    sscanf(globalBuff,"%u|%u;%[^\0]", &dummyKey, &lineID, lineIn);     

#ifdef _DODEBUG
    if (_Debug(4)) printf("ESPSL(%d)::readNextLine(): [%5d]->recNr[%07d][%10d]-> [%s]\r\n"
                                                            , __LINE__
                                                            , seekToLine
                                                            , recKey
                                                            , lineID
                                                            , lineIn);
#endif
    _readPointer++;
    if (lineID > _emptyID) 
    {
      strlcpy(lineOut, rtrim(lineIn), lineOutLen);
      return true;
#ifdef _DODEBUG
    } else 
    {
      if (_Debug(4)) printf("ESPSL(%d)::readNextLine(): SKIP[%s]\r\n", __LINE__, lineIn);
#endif
    }
    
  } //-- for ..

  return false;

} //  readNextLine()

//-------------------------------------------------------------------------------------
//-- start reading from startLine
String ESPSL::dumpLogFile() 
{
#ifdef _DODEBUG
  if (_Debug(1)) printf("ESPSL(%d)::dumpLogFile()..\r\n", __LINE__);
#endif

  int32_t       dummyKey, recKey;
  uint32_t      offset, seekToLine;
  memset(globalBuff, 0, sizeof(globalBuff));
      
  _sysLog  = LittleFS.open(_sysLogFile, "r+");    //-- open for reading and writing

  checkSysLogFileSize("dumpLogFile():", (_noLines + 1) * (_recLength +1));  //-- add '\n'

  for (recKey = 0; recKey < _noLines; recKey++) 
  {
    seekToLine = (recKey % _noLines)+1;
    offset = (seekToLine * (_recLength +1));
    if (!_sysLog.seek(offset, SeekSet)) 
    {
      printf("ESPSL(%d)::dumpLogFile(): seek to position [%d] (offset %d) failed (now @%d)\r\n", __LINE__
                                                                                              , seekToLine
                                                                                              , offset
                                                                                              , _sysLog.position());
      return "ERROR";
    }
    uint32_t  lineID;
    int l = _sysLog.readBytesUntil('\n', globalBuff, _recLength);
    if (_Debug(5)) printf("ESPSL(%d)::dumpLogFile():  >>>>> [%d] -> [%s]\r\n", __LINE__, l, globalBuff);
    sscanf(globalBuff,"%u|%u;%[^\0]", &dummyKey, &lineID, globalBuff);

    if (lineID == (_lastUsedLineID)) 
    {
            printf("(a)dumpLogFile(%d):: seek[%4d/%04d]ID[%8d]->[%s]\r\n", __LINE__, seekToLine, offset, lineID, globalBuff);

            println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    }
    else if (lineID == (_oldestLineID) ) //&& recKey < _noLines)
            printf("(b)dumpLogFile(%d):: seek[%4d/%04d]ID[%8d]->[%s]\r\n", __LINE__, seekToLine, offset, lineID, globalBuff);
    else if (lineID == _emptyID) 
            printf("(c)dumpLogFile(%d):: seek[%4d/%04d]ID[%8d]->[%s]\r\n", __LINE__, seekToLine, offset, lineID, globalBuff);
    else 
    {
       printf("(d)dumpLogFile(%d):: seek[%4d/%04d]ID[%8d]->[%s]\r\n", __LINE__, seekToLine, offset, lineID, globalBuff);
    }
  } //-- while ..

  return "EOF";

} // dumpLogFile

//-------------------------------------------------------------------------------------
//-- erase SysLog file from LittleFS
boolean ESPSL::removeSysLog() 
{
#ifdef _DODEBUG
  if (_Debug(1)) printf("ESPSL(%d)::removeSysLog()..\r\n", __LINE__);
#endif
  LittleFS.remove(_sysLogFile);
  return true;
  
} // removeSysLog()

//-------------------------------------------------------------------------------------
//-- returns ESPSL status info
void ESPSL::status() 
{
  printf("ESPSL::status():        _noLines[%8d]\r\n", _noLines);
  printf("ESPSL::status():      _lineWidth[%8d]\r\n", _lineWidth);
  if (_noLines > 0) 
  {
    printf("ESPSL::status():   _oldestLineID[%8d] (%2d)\r\n", _oldestLineID
                                                           , (_oldestLineID % _noLines)+1);
    printf("ESPSL::status(): _lastUsedLineID[%8d] (%2d)\r\n", _lastUsedLineID
                                                           , (_lastUsedLineID % _noLines)+1);
  }
  printf("ESPSL::status():       _debugLvl[%8d]\r\n", _debugLvl);
  
} // status()

//-------------------------------------------------------------------------------------
void ESPSL::setOutput(HardwareSerial *serIn, int baud)
{
  _Serial = serIn;
  _Serial->begin(baud);
  _Serial->printf("ESPSL(%d): Serial Output Ready..\r\n", __LINE__);
  _serialOn = true;
  
} // setOutput(hw, baud)

//-------------------------------------------------------------------------------------
void ESPSL::setOutput(Stream *serIn)
{
  _Stream = serIn;
  _Stream->printf("ESPSL(%d): Stream Output Ready..\r\n", __LINE__);
  _streamOn = true;
  
} // setOutput(Stream)

//-------------------------------------------------------------------------------------
void ESPSL::print(const char *line)
{
  if (_streamOn)  _Stream->print(line);
  if (_serialOn)  _Serial->print(line);
  
} // print()

//-------------------------------------------------------------------------------------
void ESPSL::println(const char *line)
{
  if (_streamOn)  _Stream->println(line);
  if (_serialOn)  _Serial->println(line);
  
} // println()

//-------------------------------------------------------------------------------------
void ESPSL::printf(const char *fmt, ...)
{
  char lineBuff[(_MAXLINEWIDTH + 101)];
  memset(lineBuff, 0, (_MAXLINEWIDTH + 101));

  va_list args;
  va_start (args, fmt);
  vsnprintf(lineBuff, (_MAXLINEWIDTH +100), fmt, args);
  va_end (args);

  if (_streamOn)  _Stream->print(lineBuff);
  if (_serialOn)  _Serial->print(lineBuff);

} // printf()

//-------------------------------------------------------------------------------------
void ESPSL::flush()
{
  if (_streamOn)  _Stream->flush();
  if (_serialOn)  _Serial->flush();
  
} // flush()
  
//-------------------------------------------------------------------------------------
//-- check fileSize of _sysLogFile
boolean ESPSL::checkSysLogFileSize(const char* func, int32_t cSize)
{
#ifdef _DODEBUG
  if (_Debug(4)) printf("ESPSL(%d)::checkSysLogFileSize(%d)..\r\n", __LINE__, cSize);
#endif
  int32_t fileSize = sysLogFileSize();
  if (fileSize != cSize) 
  {
    printf("ESPSL(%d)::%s -> [%s] size is [%d] but should be [%d] .. error!\r\n"
                                                          , __LINE__
                                                          , func
                                                          , _sysLogFile
                                                          , fileSize, cSize);
    return false;
  }
  return true;

} // checkSysLogSize()


//-------------------------------------------------------------------------------------
//-- returns lastUsed LineID
uint32_t ESPSL::getLastLineID()
{
  return _lastUsedLineID;
  
} // getLastLineID()

//-------------------------------------------------------------------------------------
//-- set Debug Level
void ESPSL::setDebugLvl(int8_t debugLvl)
{
  if (debugLvl >= 0 && debugLvl <= 9)
        _debugLvl = debugLvl;
  else  _debugLvl = 1;
  
} // setDebugLvl

//-------------------------------------------------------------------------------------
//-- returns debugLvl
int8_t ESPSL::getDebugLvl()
{
  return _debugLvl;
  
} // getDebugLvl


//-------------------------------------------------------------------------------------
//-- returns debugLvl
boolean ESPSL::_Debug(int8_t Lvl)
{
  if (getDebugLvl() > 0 && Lvl <= getDebugLvl()) 
        return true;
  else  return false;
  
} // _Debug()


//===========================================================================================
const char* ESPSL::rtrim(char *aChr)
{
  for(int p=strlen(aChr); p>0; p--)
  {
    if (aChr[p] < '!' || aChr[p] > '~') 
    {
      aChr[p] = '\0';
    }
    else
      break;
  }
  return aChr;
} // rtrim()

//===========================================================================================
void ESPSL::fixLineWidth(char *inLine, int lineLen) 
{
  int16_t newLen;
  
#ifdef _DODEBUG
  if (_Debug(4)) printf("ESPSL(%d)::fixLineWidth([%s], %d) ..\r\n", __LINE__, inLine, lineLen);
#endif
  char fixLine[(lineLen+1)];
  memset(fixLine, 0, (lineLen+1));

  //printf("ESPSL(%d)::fixLineWidth(): inLine[%s]\r\n", __LINE__, inLine);

  snprintf(fixLine, lineLen, "%s", inLine);
  //printf("ESPSL(%d)::fixLineWidth(): fixLine[%s]\r\n", __LINE__, fixLine);

  if (_Debug(3)) printf("ESPSL(%d):: [%s]\r\n", __LINE__, fixLine);
  //-- first: remove control chars
  for(int i=0; i<strlen(fixLine); i++)
  {
    if ((fixLine[i] < ' ') || (fixLine[i] > '~')) { fixLine[i] = '^'; }
  }
  //-- add spaces at the end
  do {
      strlcat(fixLine, "     ", lineLen);
      newLen = strlen(fixLine);
  } while(newLen < (lineLen-1));
  //printf("ESPSL(%d)::fixLineWidth(): fixLine[%s]\r\n", __LINE__, fixLine);
  
#ifdef _DODEBUG
  if (_Debug(3)) printf("ESPSL(%d):: [%s]\r\n", __LINE__, fixLine);
#endif

  memset(inLine, 0, lineLen);
  strlcpy(inLine, fixLine, lineLen);
  //printf("ESPSL(%d):: in[%s] -> out[%s] (len[%d])\r\n", __LINE__, fixLine, inLine, lineLen);
  
#ifdef _DODEBUG
  if (_Debug(4)) printf("ESPSL(%d)::fixLineWidth(): Length of inLine is [%d] bytes\r\n", __LINE__, strlen(inLine));
#endif
  //printf("ESPSL(%d)::fixLineWidth(): inLine[%s]\r\n", __LINE__, inLine);
  
} // fixLineWidth()

//===========================================================================================
void ESPSL::fixRecLen(char *recIn, int32_t recKey, int recLen) 
{
  int newLen;
  
#ifdef _DODEBUG
  if (_Debug(4)) printf("ESPSL(%d)::fixRecLen([%s], %u, %d) ..\r\n", __LINE__, recIn, recKey, recLen);
#endif
  char recOut[(recLen+1)];
  memset(recOut, 0, (recLen+1));

  snprintf(recOut, recLen, "%07u|%s", recKey, recIn);
#ifdef _DODEBUG
  if (_Debug(3)) printf("ESPSL(%d):: [%s]\r\n", __LINE__, recOut);
#endif
  //-- add spaces at the end
  do {
      strlcat(recOut, "     ", recLen);
      newLen = strlen(recOut);
  } while(newLen < (recLen-1));
  
#ifdef _DODEBUG
  if (_Debug(3)) printf("ESPSL(%d):: [%s]\r\n", __LINE__, recOut);
#endif

  strlcpy(recIn, recOut, recLen);

#ifdef _DODEBUG
  if (_Debug(4)) printf("ESPSL(%d)::fixRecLen(): Length of record is [%d] bytes\r\n", __LINE__, strlen(recIn));
#endif
  
  //printf("ESPSL(%d):: record[%s]\r\n", __LINE__, recIn);
  
} // fixRecLen()

//===========================================================================================
int32_t  ESPSL::sysLogFileSize()
{
/**
  {
    Dir dir = LittleFS.openDir("/");         // List files on LittleFS
    while (dir.next())  {
          yield();
          String fileName = dir.fileName();
          size_t fileSize = dir.fileSize();
#ifdef _DODEBUG
          if (_Debug(6)) printf("ESPSL)%d)::sysLogFileSize(): found: %s, size: %d\r\n"
                                                                             , __LINE__
                                                                             , fileName.c_str()
                                                                             , fileSize);
#endif
          if (fileName == _sysLogFile) {
#ifdef _DODEBUG
            if (_Debug(6)) printf("ESPSL(%d)::sysLogFileSize(): fileSize[%d]\r\n", __LINE__, fileSize);
#endif
            dir.close();
            return (int32_t)fileSize;
          }
    }
  dir.close();
  return 0;
  }
#else
**/
  {
      File root = LittleFS.open("/");
      File file = root.openNextFile();
      while(file)
      {
          String tmpS = file.name();
          String fileName = "/"+tmpS;
          size_t fileSize = file.size();
#ifdef _DODEBUG
          if (_Debug(4)) printf("ESPSL(%d)::FS Found: %s, size: %d\n", __LINE__, fileName.c_str(), fileSize);
#endif
          if (fileName == _sysLogFile) 
          {
#ifdef _DODEBUG
            if (_Debug(4)) printf("ESPSL(%d)::sysLogFileSize(): fileSize[%d]\r\n", __LINE__, fileSize);
#endif
            printf("ESPSL(%d)::sysLogFileSize(): file is still [%d]bytes\r\n", __LINE__, (int32_t)fileSize);
            return (int32_t)fileSize;
          }
          file = root.openNextFile();
      }
      return 0;
  }
/**
#endif
**/

} // sysLogFileSize()


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
