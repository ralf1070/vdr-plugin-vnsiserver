/*
 *      vdr-plugin-vnsi - KODI server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *      Copyright (C) 2015 Team KODI
 *
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <vdr/thread.h>
#include <vdr/tools.h>
#include <vdr/receiver.h>
#include <vdr/status.h>

#include "config.h"
#include "channelscancontrol.h"
#include "vnsisocket.h"
#include "ICommandQueue.h"
#include "ICommandVisitor.h"

#include <map>
#include <string>
#include <mutex>
#include <list>
#include <condition_variable>
#include <memory>

#define VNSI_EPG_AGAIN 1
#define VNSI_EPG_PAUSE 2

class cChannel;
class cDevice;
class cLiveStreamer;
class cRequestPacket;
class cResponsePacket;
class cRecPlayer;
class cCmdControl;
class cVnsiOsdProvider;
class CVNSITimers;

class cVNSIClient : public cThread
                  , public cStatus
				  , public ICommandQueue
				  , public ICommandVisitor
{

public:
  cVNSIClient(int fd, unsigned int id, const char *ClientAdr, CVNSITimers &timers);
  virtual ~cVNSIClient();

  cVNSIClient(const cVNSIClient &) = delete;
  cVNSIClient &operator=(const cVNSIClient &) = delete;

  unsigned int GetID() { return m_Id; }

  static bool InhibidDataUpdates() { return m_inhibidDataUpdates; }

  void ChannelsChange();
  void RecordingsChange();
  void SignalTimerChange();
  void EpgChange();

  /****************************************************************************
   * ICommandQueue virtual overrides
   ***************************************************************************/
  virtual void
  enqueue( const ICommandSharedPtr& command ) override;

  /****************************************************************************
   * ICommandQueue virtual overrides
   ***************************************************************************/
  virtual void
  visit( cRequestPacket& command ) override;

  virtual void
  visit( SocketError& command ) override;

  virtual void
  visit( StatusRecording& command ) override;

  virtual void
  visit( StatusOsdStatusMessage& command ) override;

  virtual void
  visit( StatusChannelChange& command ) override;

  virtual void
  visit( StatusChannelsChange& command ) override;

  virtual void
  visit( StatusRecordingsChange& command ) override;

  virtual void
  visit( StatusSignalTimerChange& command ) override;

  virtual void
  visit( StatusEpgChange& command ) override;

private:
  /****************************************************************************
   * cThread virtual overrides
   ***************************************************************************/
  void
  Action() override;

  /****************************************************************************
   * cStatus virtual overrides
   ***************************************************************************/
  void
  Recording( const cDevice* Device,
		     const char* Name,
			 const char* FileName,
			 bool On ) override;

  void
  OsdStatusMessage( const char* Message ) override;

#if VDRVERSNUM >= 20104
  void
  ChannelChange( const cChannel* Channel ) override;
#endif

  /***************************************************************************/
  bool StartChannelStreaming(cResponsePacket &resp, const cChannel *channel, int32_t priority, uint8_t timeshift, uint32_t timeout);
  void StopChannelStreaming();

  bool processRequest(cRequestPacket &req);
  bool process_Login(cRequestPacket &r);
  bool process_GetTime(cRequestPacket &r);
  bool process_EnableStatusInterface(cRequestPacket &r);
  bool process_Ping(cRequestPacket &r);
  bool process_GetSetup(cRequestPacket &r);
  bool process_StoreSetup(cRequestPacket &r);
  bool process_GetSocket(cRequestPacket &r);
  bool process_InvalidateSocket(cRequestPacket &r);

  bool processChannelStream_Open(cRequestPacket &r);
  bool processChannelStream_Close(cRequestPacket &req);
  bool processChannelStream_Seek(cRequestPacket &r);
  bool processChannelStream_StatusSocket(cRequestPacket &r);
  bool processChannelStream_StatusRequest(cRequestPacket &r);

  bool processRecStream_Open(cRequestPacket &r);
  bool processRecStream_Close(cRequestPacket &r);
  bool processRecStream_GetBlock(cRequestPacket &r);
  bool processRecStream_PositionFromFrameNumber(cRequestPacket &r);
  bool processRecStream_FrameNumberFromPosition(cRequestPacket &r);
  bool processRecStream_GetIFrame(cRequestPacket &r);
  bool processRecStream_GetLength(cRequestPacket &r);

  bool processCHANNELS_GroupsCount(cRequestPacket &r);
  bool processCHANNELS_ChannelsCount(cRequestPacket &r);
  bool processCHANNELS_GroupList(cRequestPacket &r);
  bool processCHANNELS_GetChannels(cRequestPacket &r);
  bool processCHANNELS_GetGroupMembers(cRequestPacket &r);
  bool processCHANNELS_GetCaids(cRequestPacket &r);
  bool processCHANNELS_GetWhitelist(cRequestPacket &r);
  bool processCHANNELS_GetBlacklist(cRequestPacket &r);
  bool processCHANNELS_SetWhitelist(cRequestPacket &r);
  bool processCHANNELS_SetBlacklist(cRequestPacket &r);

  void CreateChannelGroups(bool automatic);

  bool processTIMER_GetCount(cRequestPacket &r);
  bool processTIMER_Get(cRequestPacket &r);
  bool processTIMER_GetList(cRequestPacket &r);
  bool processTIMER_Add(cRequestPacket &r);
  bool processTIMER_Delete(cRequestPacket &r);
  bool processTIMER_Update(cRequestPacket &r);
  bool processTIMER_GetTypes(cRequestPacket &r);

  bool processRECORDINGS_GetDiskSpace(cRequestPacket &r);
  bool processRECORDINGS_GetCount(cRequestPacket &r);
  bool processRECORDINGS_GetList(cRequestPacket &r);
  bool processRECORDINGS_GetInfo(cRequestPacket &r);
  bool processRECORDINGS_Rename(cRequestPacket &r);
  bool processRECORDINGS_Delete(cRequestPacket &r);
  bool processRECORDINGS_Move(cRequestPacket &r);
  bool processRECORDINGS_GetEdl(cRequestPacket &r);
  bool processRECORDINGS_DELETED_Supported(cRequestPacket &r);
  bool processRECORDINGS_DELETED_GetCount(cRequestPacket &r);
  bool processRECORDINGS_DELETED_GetList(cRequestPacket &r);
  bool processRECORDINGS_DELETED_Delete(cRequestPacket &r);
  bool processRECORDINGS_DELETED_Undelete(cRequestPacket &r);
  bool processRECORDINGS_DELETED_DeleteAll(cRequestPacket &r);

  bool processEPG_GetForChannel(cRequestPacket &r);

  bool processSCAN_ScanSupported(cRequestPacket &r);
  bool processSCAN_GetSupportedTypes(cRequestPacket &r);
  bool processSCAN_GetCountries(cRequestPacket &r);
  bool processSCAN_GetSatellites(cRequestPacket &r);
  bool processSCAN_Start(cRequestPacket &r);
  bool processSCAN_Stop(cRequestPacket &r);

  bool Undelete(cRecording* recording, cRecordings* reclist, cRecordings* dellist);

  bool processOSD_Connect(cRequestPacket &req);
  bool processOSD_Disconnect();
  bool processOSD_Hitkey(cRequestPacket &req);

  cString CreatePiconRef(const cChannel* channel);

  // Static callback functions to interact with wirbelscan plugin over
  // the plugin service interface
  friend class CScanControl;

  void processSCAN_SetPercentage(int percent);
  void processSCAN_SetSignalStrength(int strength, bool locked);
  void processSCAN_SetDeviceInfo(const char *Info);
  void processSCAN_SetTransponder(const char *Info);
  void processSCAN_NewChannel(const char *Name, bool isRadio, bool isEncrypted, bool isHD);
  void processSCAN_IsFinished();
  void processSCAN_SetStatus(int status);

  struct ChannelGroup
  {
    bool automatic;
    bool radio;
    std::string name;
  };

  std::map<std::string, ChannelGroup> m_channelgroups[2];

  const unsigned int 	m_Id;
  bool 					m_loggedIn = false;
  bool 					m_StatusInterfaceEnabled;
  std::unique_ptr<cLiveStreamer>		m_Streamer;
  bool 					m_isStreaming = false;
  bool 					m_bSupportRDS = false;
  const cString 		m_ClientAddress;
  std::unique_ptr<cRecPlayer>m_RecPlayer;
  cCharSetConv 			m_toUTF8;
  uint32_t 				m_protocolVersion;
  static cMutex 		m_timerLock;
  std::unique_ptr<cVnsiOsdProvider>		m_Osd;
  CScanControl 			m_ChannelScanControl;
  static bool 			m_inhibidDataUpdates;

  struct sEpgUpdate
  {
    int attempts = 0;
    time_t lastEvent = 0;
    time_t lastTrigger = 0;
  };
  std::map<int, sEpgUpdate> 	m_epgUpdate;
  CVNSITimers&					m_vnsiTimers;

  std::mutex 					m_QueueMutex;
  std::condition_variable		m_QueueCondVar;
  std::list<ICommandSharedPtr> 	m_Queue;
  VNSISocket 					m_socket;
};
