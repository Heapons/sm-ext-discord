#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include "queue.h"
#include "dpp/dpp.h"
#include "discord.h"

class DiscordExtension : public SDKExtension
{
public:
	virtual bool SDK_OnLoad(char* error, size_t maxlength, bool late);
	virtual void SDK_OnUnload();
};

class DiscordHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class DiscordMessageHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class DiscordChannelHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class DiscordEmbedHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class DiscordInteractionHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

extern DiscordExtension g_DiscordExt;
extern ThreadSafeQueue<std::function<void()>> g_TaskQueue;

extern IForward* g_pForwardReady;
extern IForward* g_pForwardMessage;
extern IForward* g_pForwardError;
extern IForward* g_pForwardSlashCommand;

extern HandleType_t g_DiscordHandle, g_DiscordMessageHandle, g_DiscordChannelHandle, g_DiscordEmbedHandle, g_DiscordInteractionHandle;
extern DiscordHandler g_DiscordHandler;
extern DiscordMessageHandler g_DiscordMessageHandler;
extern DiscordChannelHandler g_DiscordChannelHandler;
extern DiscordEmbedHandler g_DiscordEmbedHandler;
extern DiscordInteractionHandler g_DiscordInteractionHandler;

extern const sp_nativeinfo_t discord_natives[];

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
