//      ircclient.cpp:
//
//      Copyright (C) 2018  The ZetaGlest team <https://github.com/ZetaGlest>
//
//      ZetaGlest is a fork of MegaGlest <https://megaglest.org>
//
//      This program is free software: you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation, either version 3 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "ircclient.h"
#include "util.h"
#include "platform_common.h"
#include "socket.h"
#include "cache_manager.h"

#if !defined(DISABLE_IRCCLIENT)

#include <libircclient.h>
// upstream moved some defines into new headers as of 1.6
#ifndef LIBIRCCLIENT_PRE1_6
#include <libirc_rfcnumeric.h>
#endif

#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "conversion.h"
#include "versions.h"

using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared {
	namespace PlatformCommon {

		const char *IRCThread::globalCacheContainerName = NULL;
		const int IRC_SERVER_PORT = 6667;
		//bool IRCThread::debugEnabled = true;
		bool IRCThread::debugEnabled = false;

#if !defined(DISABLE_IRCCLIENT)
		void addlog(const char * fmt, ...) {
			//FILE * fp;
			char buf[8096];
			va_list va_alist;

			va_start(va_alist, fmt);
#if defined (WIN32)
			_vsnprintf(buf, sizeof(buf), fmt, va_alist);
#else
			vsnprintf(buf, sizeof(buf), fmt, va_alist);
#endif
			va_end(va_alist);

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: %s\n", buf);

			//if(SystemFlags::VERBOSE_MODE_ENABLED == true) {
			//    if ( (fp = fopen ("irctest.log", "ab")) != 0 ) {
			//        fprintf (fp, "%s\n", buf);
			//        fclose (fp);
			//    }
			//}
		}

		void dump_event(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
			char buf[512] = "";
			unsigned int cnt = 0;
			buf[0] = '\0';

			for (cnt = 0; cnt < count; cnt++) {
				if (cnt) {
					strcat(buf, "|");
				}
#ifdef WIN32
				strncat(buf, params[cnt], min((int) strlen(params[cnt]), 511));
#else
				strncat(buf, params[cnt], std::min((int) strlen(params[cnt]), 511));
#endif
			}

			addlog("Event \"%s\", origin: \"%s\", params: %d [%s]", event, origin ? origin : "NULL", cnt, buf);

			IRCThread *ctx = (IRCThread *) irc_get_ctx(session);
			if (ctx != NULL) {
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
				if (ctx->getQuitStatus()) {
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
					return;
				}
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
				if (difftime(time(NULL), ctx->getLastNickListUpdate()) >= 7) {
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
					ctx->setLastNickListUpdate(time(NULL));
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
					ctx->GetIRCConnectedNickList(ctx->getChannel(), false);
				}
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
			}
		}

		void get_nickname(const char *sourceNick, char *destNick, size_t maxDestBufferSize) {
			string sourceNickStr = sourceNick;
			if (sourceNickStr != "" && sourceNickStr[0] == '@') {
				sourceNickStr.erase(0, 1);
			}

			irc_target_get_nick(sourceNickStr.c_str(), destNick, maxDestBufferSize);
		}

		void event_notice(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
			dump_event(session, event, origin, params, count);

			if (origin == NULL) {
				return;
			}

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("NOTICE from '%s': %s", origin, params[1]);

			//if(strcasecmp (origin, "nickserv")) {
			//	return;
			//}

			if (strstr(params[1], "This nick is not registered") == params[1]) {
				//std::string regcmd = "REGISTER " + gCfg.irc_nickserv_pass + " NOMAIL";
				//gLog.Add (CLog::INFO, "Registering our nick with NICKSERV");
				//irc_cmd_msg (session, "nickserv", regcmd.c_str());
			} else if (strstr(params[1], "This nickname is registered and protected") == params[1]) {
				//std::string identcmd = "IDENTIFY " + gCfg.irc_nickserv_pass;
				//gLog.Add (CLog::INFO, "Identifying our nick with NICKSERV");
				//irc_cmd_msg (session, "nickserv", identcmd.c_str());

		//	    IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
		//		if(ctx != NULL) {
		//			if(ctx->getExecute_cmd_onconnect() != "") {
		//				irc_cmd_msg(session, "nickserv",  ctx->getExecute_cmd_onconnect().c_str());
		//			}
		//		}
			} else if (strstr(params[1], "Password accepted - you are now recognized") == params[1]) {
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("Nickserv authentication succeed.");
			}
		}

		void event_join(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

			dump_event(session, event, origin, params, count);

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

			IRCThread *ctx = (IRCThread *) irc_get_ctx(session);

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

			if (ctx != NULL) {

				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

				if (ctx->getHasJoinedChannel() == false) {

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					irc_cmd_user_mode(session, "+i");

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					// code to execute when someone joins the lobby can be put here.
					ctx->setHasJoinedChannel(true);

					/* The discord-irc bot<https://github.com/reactiflux/discord-irc>
					 * already prints this message because
					 * it's sent using NOTICE or whatever from IRC directly
					 *
					 * The only reason it would be useful is to provide an extra alert
					 * to users in the channel on IRC, if they have extra notifications
					 * enabled
					 *
					const char *announceJoin = { "has joined" };
					irc_cmd_msg(session, "#zetaglest-lobby", announceJoin);
					*/

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					//ctx->GetIRCConnectedNickList(ctx->getChannel(),true);
					ctx->GetIRCConnectedNickList(ctx->getChannel(), false);

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
				} else {
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					char realNick[128] = "";
					get_nickname(origin, realNick, 127);
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: user joined channel realNick [%s] origin [%s]\n", realNick, origin);
					bool foundNick = false;

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					MutexSafeWrapper safeMutex(ctx->getMutexNickList(), string(__FILE__) + "_" + intToStr(__LINE__));
					std::vector<string> nickList = ctx->getCachedNickList();
					for (unsigned int i = 0;
						i < nickList.size(); ++i) {
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: looking for match [%s] realNick [%s]\n", nickList[i].c_str(), realNick);

						if (nickList[i] == realNick) {
							foundNick = true;
							break;
						}
					}
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					if (foundNick == false) {
						nickList.push_back(realNick);
					}

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
				}

				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

				if (ctx->getWantToLeaveChannel() == true) {
					ctx->leaveChannel();
				}

				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
			}
		}

		void event_connect(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
			IRCThread *ctx = (IRCThread *) irc_get_ctx(session);

			dump_event(session, event, origin, params, count);

			//IRC: Event "433", origin: "leguin.freenode.net", params: 3 [*|softcoder|Nickname is already in use.]
		//	printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
		//	if(strstr (params[1], "Nickname is already in use") == params[1]) {
		//		IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
		//		if(ctx != NULL) {
		//			if(ctx->getExecute_cmd_onconnect() != "") {
		//				irc_cmd_msg(session, "nickserv",  ctx->getExecute_cmd_onconnect().c_str());
		//			}
		//		}
		//	}
		//	else
			if (ctx != NULL) {
				irc_cmd_join(session, ctx->getChannel().c_str(), 0);
			}
		}

		void event_privmsg(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
			dump_event(session, event, origin, params, count);

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("'%s' said me (%s): %s\n", origin ? origin : "someone", params[0], params[1]);
		}

		void dcc_recv_callback(irc_session_t * session, irc_dcc_t id, int status, void * ctx, const char * data, unsigned int length) {

			switch (status) {
				case LIBIRC_ERR_CLOSED:
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("DCC %d: chat closed\n", id);
					break;

				case 0:
					if (!data) {
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("DCC %d: chat connected\n", id);
						irc_dcc_msg(session, id, "Hehe");
					} else {
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("DCC %d: %s\n", id, data);

						static int count = 1;
						char buf[12] = "";
						sprintf(buf, "DCC [%d]: %d", id, count++);
						irc_dcc_msg(session, id, buf);
					}
					break;

				default:
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("DCC %d: error %s\n", id, irc_strerror(status));
					break;
			}
		}

		void dcc_file_recv_callback(irc_session_t * session, irc_dcc_t id, int status, void * ctx, const char * data, unsigned int length) {
			if (status == 0 && length == 0) {
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("File sent successfully\n");

				if (ctx) {
					fclose((FILE*) ctx);
				}
			} else if (status) {
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("File sent error: %d\n", status);

				if (ctx) {
					fclose((FILE*) ctx);
				}
			} else {
				if (ctx) {
					fwrite(data, 1, length, (FILE*) ctx);
				}
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("File sent progress: %u\n", length);
			}
		}

		void event_channel(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
			//IRC: Event "433", origin: "leguin.freenode.net", params: 3 [*|softcoder|Nickname is already in use.]
			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("In [%s::%s] Line: %d count = %u origin = %s\n", __FILE__, __FUNCTION__, __LINE__, count, (origin ? origin : "null"));

			if (count != 2)
				return;

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: '%s' said in channel %s: %s\n", origin ? origin : "someone", params[0], params[1]);

			if (!origin) {
				return;
			}

			char realNick[128] = "";
			get_nickname(origin, realNick, 127);
			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: event signalled realNick [%s] origin [%s]\n", realNick, origin);

			IRCThread *ctx = (IRCThread *) irc_get_ctx(session);
			if (ctx != NULL) {
				MutexSafeWrapper safeMutex(ctx->getMutexIRCCB(), string(__FILE__) + "_" + intToStr(__LINE__));
				IRCCallbackInterface *cb = ctx->getCallbackObj(false);
				if (cb != NULL) {
					cb->IRC_CallbackEvent(IRC_evt_chatText, realNick, params, count);
				}
			}

			//	if ( !strcmp (params[1], "quit") )
			//		irc_cmd_quit (session, "of course, Master!");
			//
			//	if ( !strcmp (params[1], "help") ) {
			//		irc_cmd_msg (session, params[0], "quit, help, dcc chat, dcc send, ctcp");
			//	}
			//
			//	if ( !strcmp (params[1], "ctcp") ) {
			//		irc_cmd_ctcp_request (session, realNick, "PING 223");
			//		irc_cmd_ctcp_request (session, realNick, "FINGER");
			//		irc_cmd_ctcp_request (session, realNick, "VERSION");
			//		irc_cmd_ctcp_request (session, realNick, "TIME");
			//	}
			//
			//	if ( !strcmp (params[1], "dcc chat") ) {
			//		irc_dcc_t dccid;
			//		irc_dcc_chat (session, 0, realNick, dcc_recv_callback, &dccid);
			//		if(SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf ("DCC chat ID: %d\n", dccid);
			//	}
			//
			//	if ( !strcmp (params[1], "dcc send") ) {
			//		irc_dcc_t dccid;
			//		irc_dcc_sendfile (session, 0, realNick, "irctest.c", dcc_file_recv_callback, &dccid);
			//		if(SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf ("DCC send ID: %d\n", dccid);
			//	}
			//
			//	if ( !strcmp (params[1], "topic") ) {
			//		irc_cmd_topic (session, params[0], 0);
			//    }
			//	else if ( strstr (params[1], "topic ") == params[1] ) {
			//		irc_cmd_topic (session, params[0], params[1] + 6);
			//    }
			//
			//	if ( strstr (params[1], "mode ") == params[1] )
			//		irc_cmd_channel_mode (session, params[0], params[1] + 5);
			//
			//	if ( strstr (params[1], "nick ") == params[1] )
			//		irc_cmd_nick (session, params[1] + 5);
			//
			//	if ( strstr (params[1], "whois ") == params[1] )
			//		irc_cmd_whois (session, params[1] + 5);
		}

		void irc_event_dcc_chat(irc_session_t * session, const char * nick, const char * addr, irc_dcc_t dccid) {
			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("DCC chat [%d] requested from '%s' (%s)\n", dccid, nick, addr);

			irc_dcc_accept(session, dccid, 0, dcc_recv_callback);
		}

		void irc_event_dcc_send(irc_session_t * session, const char * nick, const char * addr, const char * filename, unsigned long size, irc_dcc_t dccid) {
			FILE * fp;
			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("DCC send [%d] requested from '%s' (%s): %s (%lu bytes)\n", dccid, nick, addr, filename, size);

			if ((fp = fopen("file", "wb")) == 0) {
				abort();
			}
			irc_dcc_accept(session, dccid, fp, dcc_file_recv_callback);
		}

		void event_leave(irc_session_t *session, const char *event, const char *origin, const char ** params, unsigned count) {
			char buf[24] = "";
			sprintf(buf, "%s", event);

			// someone left the channel.
			if (origin) {
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: user left channel [%s]\n", origin);

				char realNick[128] = "";
				get_nickname(origin, realNick, 127);
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: user left channel realNick [%s] origin [%s]\n", realNick, origin);

				IRCThread *ctx = (IRCThread *) irc_get_ctx(session);
				if (ctx != NULL) {
					MutexSafeWrapper safeMutex(ctx->getMutexNickList(), string(__FILE__) + "_" + intToStr(__LINE__));
					std::vector<string> &nickList = ctx->getCachedNickList();
					for (unsigned int i = 0;
						i < nickList.size(); ++i) {
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: lookingfor match [%s] realNick [%s]\n", nickList[i].c_str(), realNick);

						if (nickList[i] == realNick) {
							nickList.erase(nickList.begin() + i);
							break;
						}
					}
				}
			}

			dump_event(session, buf, origin, params, count);
		}
		void event_numeric(irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count) {
			char buf[24] = "";
			sprintf(buf, "%u", event);

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d event = %u\n", __LINE__, event);

			switch (event) {
				case LIBIRC_RFC_ERR_NICKNAMEINUSE:
				case LIBIRC_RFC_ERR_NICKCOLLISION:
					//irc_auto_rename_nick(session);
					//IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
					//if(ctx != NULL) {
		//			{
		//			//IRC: Event "433", origin: "leguin.freenode.net", params: 3 [*|softcoder|Nickname is already in use.]
		//			printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
		//			//if(strstr (params[1], "Nickname is already in use") == params[1]) {
		//				IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
		//				if(ctx != NULL) {
		//					if(ctx->getExecute_cmd_onconnect() != "") {
		//						irc_cmd_msg(session, "nickserv",  ctx->getExecute_cmd_onconnect().c_str());
		//					}
		//				}
		//			//}
		//			}
					break;

				case LIBIRC_RFC_ERR_NOTREGISTERED:
					//irc_auto_rename_nick(session);
					//IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
					//if(ctx != NULL) {
		//			{
					//===> IRC: Event "451", origin: "leguin.freenode.net", params: 2 [*|You have not registered]
		//			printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
		//			//if(strstr (params[1], "Nickname is already in use") == params[1]) {
		//				IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
		//				if(ctx != NULL) {
		//					//if(ctx->getExecute_cmd_onconnect() != "") {
		//						//irc_cmd_msg(session, "nickserv",  ctx->getExecute_cmd_onconnect().c_str());
		//						string cmd = "REGISTER " + ctx->getNick() + " NOMAIL";
		//						irc_cmd_msg (session, "nickserv", cmd.c_str());
		//					//}
		//				}
		//			//}
		//			}
					break;

				case LIBIRC_RFC_RPL_TOPIC:
					break;
				case LIBIRC_RFC_RPL_NAMREPLY:
				{
					if (event == LIBIRC_RFC_RPL_NAMREPLY) {

						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: LIBIRC_RFC_RPL_NAMREPLY count = %u\n", count);

						std::vector<string> nickList;
						if (count >= 4) {
							for (unsigned int i = 3; i < count && params[i]; ++i) {
								vector<string> tokens;
								Tokenize(params[i], tokens, " ");

								for (unsigned int j = 0; j < tokens.size(); ++j) {

									char realNick[128] = "";
									get_nickname(tokens[j].c_str(), realNick, 127);
									if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: LIBIRC_RFC_RPL_NAMREPLY user joined channel realNick [%s] tokens[j] [%s]\n", realNick, tokens[j].c_str());

									// Only show ZetaGlest users in the user list
									//if(strncmp(&realNick[0],"ZG_",3) == 0) {
									nickList.push_back(realNick);
									//}
								}
							}
						}

						IRCThread *ctx = (IRCThread *) irc_get_ctx(session);
						if (ctx != NULL) {
							MutexSafeWrapper safeMutex(ctx->getMutexNickList(), string(__FILE__) + "_" + intToStr(__LINE__));
							ctx->setCachedNickList(nickList);
						}
					}
					break;
				}
				case LIBIRC_RFC_RPL_ENDOFNAMES:
				{
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("!!!!! ===> IRC: Line: %d event = %u\n", __LINE__, event);

					IRCThread *ctx = (IRCThread *) irc_get_ctx(session);
					if (ctx != NULL) {
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d event = %u\n", __LINE__, event);

						ctx->setEventDataDone(true);
					}
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d event = %u\n", __LINE__, event);
				}
				break;
			}

			dump_event(session, buf, origin, params, count);
		}

#endif

		bool IRCThread::getEventDataDone() {
			MutexSafeWrapper safeMutex(&mutexEventDataDone, string(__FILE__) + "_" + intToStr(__LINE__));
			bool result = eventDataDone;
			safeMutex.ReleaseLock();

			return result;
		}
		void IRCThread::setEventDataDone(bool value) {
			MutexSafeWrapper safeMutex(&mutexEventDataDone, string(__FILE__) + "_" + intToStr(__LINE__));
			eventDataDone = value;
		}

		IRCThread::IRCThread(const std::vector<string> &argv, IRCCallbackInterface *callbackObj) : BaseThread() {
			uniqueID = "IRCThread";
			this->argv = argv;
			this->callbackObj = callbackObj;
			ircSession = NULL;
			eventData.clear();
			setEventDataDone(false);
			hasJoinedChannel = false;
			lastNickListUpdate = time(NULL);
			wantToLeaveChannel = false;
			playerName = "";
		}

		void IRCThread::disconnect() {
#if !defined(DISABLE_IRCCLIENT)

			MutexSafeWrapper safeMutex(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
			bool validSession = (ircSession != NULL);
			safeMutex.ReleaseLock();

			if (validSession == true) {
				setCallbackObj(NULL);
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Quitting Channel\n");

				MutexSafeWrapper safeMutex1(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
				if (ircSession != NULL) {
					irc_disconnect(ircSession);
				}
				safeMutex1.ReleaseLock();

				BaseThread::signalQuit();
				hasJoinedChannel = false;
			}
#else
			BaseThread::signalQuit();
#endif
		}

		void IRCThread::signalQuit() {
			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: signalQuit [%p]\n", ircSession);

#if !defined(DISABLE_IRCCLIENT)

			MutexSafeWrapper safeMutex(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
			bool validSession = (ircSession != NULL);
			safeMutex.ReleaseLock();

			if (validSession == true) {
				setCallbackObj(NULL);
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Quitting Channel\n");

				MutexSafeWrapper safeMutex1(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
				if (ircSession != NULL) {
					irc_cmd_quit(ircSession, "ZG Bot is closing!");
				}
				safeMutex1.ReleaseLock();
				hasJoinedChannel = false;
			}
			BaseThread::signalQuit();

#else
			BaseThread::signalQuit();
#endif
		}

		bool IRCThread::shutdownAndWait() {
			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: shutdownAndWait [%p]\n", ircSession);

			signalQuit();
			return BaseThread::shutdownAndWait();
		}

		void IRCThread::SendIRCCmdMessage(string target, string msg) {
			MutexSafeWrapper safeMutex(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
			bool validSession = (ircSession != NULL);
			safeMutex.ReleaseLock();

			if (validSession == true && hasJoinedChannel == true) {
				if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s Line: %d] sending IRC command to [%s] cmd [%s]\n", __FILE__, __FUNCTION__, __LINE__, target.c_str(), msg.c_str());

#if !defined(DISABLE_IRCCLIENT)
				MutexSafeWrapper safeMutex1(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
				int ret = 0;
				if (ircSession != NULL) {
					ret = irc_cmd_msg(ircSession, target.c_str(), msg.c_str());
				}
				safeMutex1.ReleaseLock();

				if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s Line: %d] sending IRC command to [%s] cmd [%s] ret = %d\n", __FILE__, __FUNCTION__, __LINE__, target.c_str(), msg.c_str(), ret);
#endif
			}
		}

		std::vector<string> IRCThread::GetIRCConnectedNickList(string target, bool waitForCompletion) {
			setEventDataDone(false);

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
			MutexSafeWrapper safeMutexSession(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
			bool validSession = (ircSession != NULL);
			safeMutexSession.ReleaseLock();

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

			if (validSession == true && hasJoinedChannel == true) {
				if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s Line: %d] sending IRC nick list command to [%s]\n", __FILE__, __FUNCTION__, __LINE__, target.c_str());

#if !defined(DISABLE_IRCCLIENT)

				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

				MutexSafeWrapper safeMutex1(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));

				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
				int ret = irc_cmd_names(ircSession, target.c_str());
				safeMutex1.ReleaseLock();

				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
				if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s Line: %d] sending IRC nick list command to [%s] ret = %d\n", __FILE__, __FUNCTION__, __LINE__, target.c_str(), ret);

				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

				if (waitForCompletion == true) {

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					for (time_t tElapsed = time(NULL);
						getEventDataDone() == false &&
						this->getQuitStatus() == false &&
						difftime(time(NULL), tElapsed) <= 5;) {
						sleep(50);
					}

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
				}

				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
#endif
			}

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

			MutexSafeWrapper safeMutex(&mutexNickList, string(__FILE__) + "_" + intToStr(__LINE__));
			std::vector<string> nickList = eventData;
			safeMutex.ReleaseLock();

			if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

			return nickList;
		}

		bool IRCThread::isConnected(bool mutexLockRequired) {
			bool ret = false;
			if (this->getQuitStatus() == false) {
				MutexSafeWrapper safeMutex(NULL, string(__FILE__) + "_" + intToStr(__LINE__));
				int lockStatus = 0;
				if (mutexLockRequired == true) {
					lockStatus = safeMutex.setMutexAndTryLock(&mutexIRCSession);
				}
				bool validSession = (lockStatus == SDL_MUTEX_TIMEDOUT || (lockStatus == 0 && ircSession != NULL));
				if (mutexLockRequired == true && lockStatus == 0) {
					safeMutex.ReleaseLock();
				}

				if (validSession == true) {
#if !defined(DISABLE_IRCCLIENT)
					MutexSafeWrapper safeMutex1(NULL, string(__FILE__) + "_" + intToStr(__LINE__));
					if (ircSession != NULL) {
						lockStatus = 0;
						if (mutexLockRequired == true) {
							lockStatus = safeMutex1.setMutexAndTryLock(&mutexIRCSession);
						}
						ret = (lockStatus == SDL_MUTEX_TIMEDOUT || (lockStatus == 0 && irc_is_connected(ircSession) != 0));
					}
					if (mutexLockRequired == true && lockStatus == 0) {
						safeMutex1.ReleaseLock();
					}
				}
#endif
			}

			return ret;
		}

		std::vector<string> IRCThread::getNickList() {
			MutexSafeWrapper safeMutex(&mutexNickList, string(__FILE__) + "_" + intToStr(__LINE__));
			std::vector<string> nickList = eventData;
			safeMutex.ReleaseLock();

			return nickList;
		}

		IRCCallbackInterface * IRCThread::getCallbackObj(bool lockObj) {
			MutexSafeWrapper safeMutex(NULL, string(__FILE__) + "_" + intToStr(__LINE__));
			if (lockObj == true) {
				safeMutex.setMutex(&mutexIRCCB);
			}
			return callbackObj;
		}
		void IRCThread::setCallbackObj(IRCCallbackInterface *cb) {
			MutexSafeWrapper safeMutex(&mutexIRCCB, string(__FILE__) + "_" + intToStr(__LINE__));
			callbackObj = cb;
		}

		void IRCThread::execute() {
			{
				RunningStatusSafeWrapper runningStatus(this);
				if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s Line: %d] argv.size() = %d\n", __FILE__, __FUNCTION__, __LINE__, argv.size());
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: IRCThread::execute Line: %d\n", __LINE__);

				if (getQuitStatus() == true) {
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
					return;
				}

				if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "IRC thread is running\n");
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

				try {
#if !defined(DISABLE_IRCCLIENT)
					irc_callbacks_t	callbacks;

					MutexSafeWrapper safeMutex(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
					ircSession = NULL;
					safeMutex.ReleaseLock(true);

					if (argv.size() != 5) {
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC Usage: <server> <nick> <channel> : got params [%ld]\n", (long int) argv.size());
						return;
					}

					memset(&callbacks, 0, sizeof(callbacks));

					callbacks.event_connect = event_connect;
					callbacks.event_join = event_join;
					callbacks.event_nick = dump_event;
					callbacks.event_quit = dump_event;
					callbacks.event_part = event_leave;
					callbacks.event_mode = dump_event;
					callbacks.event_topic = dump_event;
					callbacks.event_kick = dump_event;
					callbacks.event_channel = event_channel;
					callbacks.event_privmsg = event_privmsg;
					callbacks.event_notice = event_notice;
					callbacks.event_invite = dump_event;
					callbacks.event_umode = dump_event;
					callbacks.event_ctcp_rep = dump_event;
					callbacks.event_ctcp_action = dump_event;
					callbacks.event_unknown = dump_event;
					callbacks.event_numeric = event_numeric;

					callbacks.event_dcc_chat_req = irc_event_dcc_chat;
					callbacks.event_dcc_send_req = irc_event_dcc_send;

					if (this->getQuitStatus() == true) {
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
						return;
					}
					safeMutex.Lock();
					ircSession = irc_create_session(&callbacks);
					bool validSession = (ircSession != NULL);

					if (validSession == false) {
						safeMutex.ReleaseLock();
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC Could not create session\n");
						return;
					}
					safeMutex.ReleaseLock(true);

					//            this->execute_cmd_onconnect = "";
					//            if(argv.size() >= 5) {
					//            	this->execute_cmd_onconnect = argv[4]; // /msg NickServ identify <password>.
					//            }

					//            this->password = "";
					//            if(argv.size() >= 5) {
					//            	this->password = argv[4];
					//            }
					this->username = argv[1];
					if (argv.size() >= 4) {
						this->username = argv[3];
					}
					this->channel = argv[2];
					this->nick = argv[1];

					safeMutex.Lock();
					irc_set_ctx(ircSession, this);
					safeMutex.ReleaseLock(true);

					if (this->getQuitStatus() == true) {
						return;
					}

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					safeMutex.Lock();
					if (irc_connect(ircSession, argv[0].c_str(), IRC_SERVER_PORT, 0, this->nick.c_str(), this->username.c_str(), (string("ZetaGlest ") + GAME_VERSION).c_str())) {
						safeMutex.ReleaseLock();
						if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC Could not connect: %s\n", irc_strerror(irc_errno(ircSession)));
						return;
					}
					safeMutex.ReleaseLock();

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					if (this->getQuitStatus() == true) {
						return;
					}

					if (this->getQuitStatus() == true) {
						return;
					}

					//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					for (int iAttempts = 1;
						this->getQuitStatus() == false && iAttempts <= 7; ++iAttempts) {
						//if(irc_run(ircSession)) {

						int run_result = irc_run_session(ircSession);
						if (run_result && this->getQuitStatus() == false) {
							//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);

							if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC Could not run the session: %s run_result = %d\n", irc_strerror(irc_errno(ircSession)), run_result);
							printf("===> IRC Could not run the session: %s run_result = %d\n", irc_strerror(irc_errno(ircSession)), run_result);
						}
					}

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);

					//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);
#else
					for (; this->getQuitStatus() == false;) {
						sleep(50);
					}
#endif
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC exiting IRC CLient!\n");
					//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);
				} catch (const exception &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugError, "In [%s::%s Line: %d] Error [%s]\n", __FILE__, __FUNCTION__, __LINE__, ex.what());
					if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s Line: %d] error [%s]\n", __FILE__, __FUNCTION__, __LINE__, ex.what());
				} catch (...) {
					SystemFlags::OutputDebug(SystemFlags::debugError, "In [%s::%s Line: %d] UNKNOWN Error\n", __FILE__, __FUNCTION__, __LINE__);
					if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s Line: %d] unknown error\n", __FILE__, __FUNCTION__, __LINE__);
				}

				if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s Line: %d] IRC thread is exiting\n", __FILE__, __FUNCTION__, __LINE__);
				if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
				//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);


				//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);
				// Delete ourself when the thread is done (no other actions can happen after this
				// such as the mutex which modifies the running status of this method
				MutexSafeWrapper safeMutex(&mutexIRCCB, string(__FILE__) + "_" + intToStr(__LINE__));
				IRCCallbackInterface *cb = getCallbackObj(false);
				if (cb != NULL) {
					//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);
					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC: Line: %d\n", __LINE__);
					cb->IRC_CallbackEvent(IRC_evt_exitThread, NULL, NULL, 0);
				}
				safeMutex.ReleaseLock();

			}
			//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);

			if (SystemFlags::VERBOSE_MODE_ENABLED) printf("In IRCThread() calling delete ...\n");

			//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);
			setDeleteAfterExecute(true);
		}

		int IRCThread::irc_run_session(irc_session_t * session) {
			//	if ( session->state != LIBIRC_STATE_CONNECTING )
			//	{
			//		session->lasterror = LIBIRC_ERR_STATE;
			//		return 1;
			//	}

			MutexSafeWrapper safeMutex(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));

			if (isConnected(false) == false) {
				//session->lasterror = LIBIRC_ERR_STATE;
				return 1;
			}
			safeMutex.ReleaseLock(true);

			bool isSessionConnected = true;
			while (isSessionConnected == true && this->getQuitStatus() == false) {
				struct timeval tv;
				fd_set in_set, out_set;
				int maxfd = 0;

				tv.tv_usec = 250000;
				tv.tv_sec = 0;

				// Init sets
				FD_ZERO(&in_set);
				FD_ZERO(&out_set);

				safeMutex.Lock();
				irc_add_select_descriptors(session, &in_set, &out_set, &maxfd);
				safeMutex.ReleaseLock(true);

				if (select(maxfd + 1, &in_set, &out_set, 0, &tv) < 0) {
					int lastSocketError = Socket::getLastSocketError();
					if (lastSocketError == PLATFORM_SOCKET_INTERRUPTED) {
						continue;
					}

					//session->lasterror = LIBIRC_ERR_TERMINATED;
					return 2;
				}

				safeMutex.Lock();
				if (irc_process_select_descriptors(session, &in_set, &out_set)) {
					safeMutex.ReleaseLock();
					return 3;
				}

				isSessionConnected = isConnected(false);
				safeMutex.ReleaseLock(true);
			}

			return 0;
		}

		IRCThread::~IRCThread() {
			//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);

			if (SystemFlags::VERBOSE_MODE_ENABLED) printf("In ~IRCThread() ...\n");

			if (IRCThread::globalCacheContainerName != NULL) {
				//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);

				IRCThread * &ircClient = CacheManager::getCachedItem< IRCThread * >(IRCThread::globalCacheContainerName);
				ircClient = NULL;
			}

			//printf("In ~IRCThread Line: %d [%p]\n",__LINE__,this);
		}

		void normalizeNick(char *nick) {
			// http://tools.ietf.org/html/rfc1459#section-2.3.1
		//	<target>     ::= <to> [ "," <target> ]
		//	   <to>         ::= <channel> | <user> '@' <servername> | <nick> | <mask>
		//	   <channel>    ::= ('#' | '&') <chstring>
		//	   <servername> ::= <host>
		//	   <host>       ::= see RFC 952 [DNS:4] for details on allowed hostnames
		//	   <nick>       ::= <letter> { <letter> | <number> | <special> }
		//	   <mask>       ::= ('#' | '$') <chstring>
		//	   <chstring>   ::= <any 8bit code except SPACE, BELL, NUL, CR, LF and
		//	                     comma (',')>
		//
		//	   Other parameter syntaxes are:
		//
		//	   <user>       ::= <nonwhite> { <nonwhite> }
		//	   <letter>     ::= 'a' ... 'z' | 'A' ... 'Z'
		//	   <number>     ::= '0' ... '9'
		//	   <special>    ::= '-' | '[' | ']' | '\' | '`' | '^' | '{' | '}'

			if (nick != NULL && strlen(nick) > 0) {
				char *newNick = new char[strlen(nick) + 1];
				memset(newNick, 0, strlen(nick) + 1);
				bool nickChanged = false;

				for (unsigned int i = 0; i < strlen(nick); ++i) {
					if (nick[i] == '-' || nick[i] == '[' || nick[i] == ']' || nick[i] == '_' ||
						nick[i] == '\\' || nick[i] == '`' || nick[i] == '^' ||
						nick[i] == '{' || nick[i] == '}' ||
						(nick[i] >= '0' && nick[i] <= '9') ||
						(nick[i] >= 'a' && nick[i] <= 'z') ||
						(nick[i] >= 'A' && nick[i] <= 'Z')) {
						strncat(newNick, &nick[i], 1);
					} else {
						strcat(newNick, "-");
						nickChanged = true;
					}
				}

				if (nickChanged == true) {
					strcpy(nick, newNick);
				}

				delete[] newNick;
			}
		}

		void IRCThread::connectToHost() {
			bool connectRequired = false;

			MutexSafeWrapper safeMutex(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
			bool validSession = (ircSession != NULL);
			safeMutex.ReleaseLock();

			if (validSession == false) {
				connectRequired = true;
			} else {
#if !defined(DISABLE_IRCCLIENT)

				MutexSafeWrapper safeMutex1(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
				int result = irc_is_connected(ircSession);
				if (result != 1) {
					connectRequired = true;
				}
				safeMutex1.ReleaseLock();
#endif
			}

			if (connectRequired == false) {
#if !defined(DISABLE_IRCCLIENT)
				MutexSafeWrapper safeMutex1(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
				if (irc_connect(ircSession, argv[0].c_str(), IRC_SERVER_PORT, 0, this->nick.c_str(), this->username.c_str(), "zetaglest")) {
					safeMutex1.ReleaseLock();

					if (SystemFlags::VERBOSE_MODE_ENABLED || IRCThread::debugEnabled) printf("===> IRC Could not connect: %s\n", irc_strerror(irc_errno(ircSession)));
					return;
				}
				safeMutex1.ReleaseLock();
#endif
			}
		}

		void IRCThread::joinChannel() {
			wantToLeaveChannel = false;
			connectToHost();

			MutexSafeWrapper safeMutex(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
			bool validSession = (ircSession != NULL);
			safeMutex.ReleaseLock();

			if (validSession == true) {
#if !defined(DISABLE_IRCCLIENT)

				MutexSafeWrapper safeMutex1(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
				IRCThread *ctx = (IRCThread *) irc_get_ctx(ircSession);
				if (ctx != NULL) {
					eventData.clear();
					setEventDataDone(false);
					hasJoinedChannel = false;
					lastNickListUpdate = time(NULL);

					irc_cmd_join(ircSession, ctx->getChannel().c_str(), 0);
					safeMutex.ReleaseLock();
				}
				safeMutex.ReleaseLock();
#endif
			}
		}

		void IRCThread::leaveChannel() {
			wantToLeaveChannel = true;

			MutexSafeWrapper safeMutex(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
			bool validSession = (ircSession != NULL);
			safeMutex.ReleaseLock();

			if (validSession == true) {
#if !defined(DISABLE_IRCCLIENT)

				MutexSafeWrapper safeMutex1(&mutexIRCSession, string(__FILE__) + "_" + intToStr(__LINE__));
				IRCThread *ctx = (IRCThread *) irc_get_ctx(ircSession);
				if (ctx != NULL) {
					irc_cmd_part(ircSession, ctx->getChannel().c_str());
					safeMutex.ReleaseLock();
				}
				safeMutex.ReleaseLock();
#endif
			}
		}

	}
}//end namespace
