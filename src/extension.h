#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
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

class DiscordUserHandler : public IHandleTypeDispatch
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

class DiscordWebhookHandler : public IHandleTypeDispatch
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

class DiscordAutocompleteInteractionHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class DiscordSlashCommandHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class DiscordForumTagHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class DiscordGuildHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class HttpHeadersHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

class HttpCompletionHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object);
};

extern DiscordExtension g_DiscordExt;
extern ThreadSafeQueue<std::function<void()>> g_TaskQueue;

extern HandleType_t
	g_DiscordHandle,
	g_DiscordUserHandle,
	g_DiscordMessageHandle,
	g_DiscordChannelHandle,
	g_DiscordWebhookHandle,
	g_DiscordEmbedHandle,
	g_DiscordInteractionHandle,
	g_DiscordAutocompleteInteractionHandle,
	g_DiscordSlashCommandHandle,
	g_DiscordForumTagHandle,
	g_DiscordGuildHandle,
	g_HttpHeadersHandle,
	g_HttpCompletionHandle;

extern DiscordHandler g_DiscordHandler;
extern DiscordUserHandler g_DiscordUserHandler;
extern DiscordMessageHandler g_DiscordMessageHandler;
extern DiscordChannelHandler g_DiscordChannelHandler;
extern DiscordWebhookHandler g_DiscordWebhookHandler;
extern DiscordEmbedHandler g_DiscordEmbedHandler;
extern DiscordInteractionHandler g_DiscordInteractionHandler;
extern DiscordAutocompleteInteractionHandler g_DiscordAutocompleteInteractionHandler;
extern DiscordSlashCommandHandler g_DiscordSlashCommandHandler;
extern DiscordForumTagHandler g_DiscordForumTagHandler;
extern DiscordGuildHandler g_DiscordGuildHandler;
extern HttpHeadersHandler g_HttpHeadersHandler;
extern HttpCompletionHandler g_HttpCompletionHandler;

extern const sp_nativeinfo_t discord_natives[];

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
