#include "extension.h"

#define MAX_PROCESS 10

DiscordExtension g_DiscordExt;
SMEXT_LINK(&g_DiscordExt);

HandleType_t
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
  g_DiscordGuildHandle;

DiscordHandler g_DiscordHandler;
DiscordUserHandler g_DiscordUserHandler;
DiscordMessageHandler g_DiscordMessageHandler;
DiscordChannelHandler g_DiscordChannelHandler;
DiscordWebhookHandler g_DiscordWebhookHandler;
DiscordEmbedHandler g_DiscordEmbedHandler;
DiscordInteractionHandler g_DiscordInteractionHandler;
DiscordAutocompleteInteractionHandler g_DiscordAutocompleteInteractionHandler;
DiscordSlashCommandHandler g_DiscordSlashCommandHandler;
DiscordForumTagHandler g_DiscordForumTagHandler;
DiscordGuildHandler g_DiscordGuildHandler;

IForward* g_pForwardReady = nullptr;
IForward* g_pForwardMessage = nullptr;
IForward* g_pForwardError = nullptr;
IForward* g_pForwardSlashCommand = nullptr;
IForward* g_pForwardAutocomplete = nullptr;

ThreadSafeQueue<std::function<void()>> g_TaskQueue;

static void OnGameFrame(bool simulating) {
	std::function<void()> task;
	int count = 0;
	while (g_TaskQueue.TryPop(task) && count < MAX_PROCESS) {
		task();
		count++;
	}
}

bool DiscordExtension::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
	sharesys->AddNatives(myself, discord_natives);
	sharesys->RegisterLibrary(myself, "discord");

	HandleAccess haDefaults;
	handlesys->InitAccessDefaults(nullptr, &haDefaults);
	haDefaults.access[HandleAccess_Delete] = 0;

	g_DiscordHandle = handlesys->CreateType("Discord", &g_DiscordHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordUserHandle = handlesys->CreateType("DiscordUser", &g_DiscordUserHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordMessageHandle = handlesys->CreateType("DiscordMessage", &g_DiscordMessageHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordChannelHandle = handlesys->CreateType("DiscordChannel", &g_DiscordChannelHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordWebhookHandle = handlesys->CreateType("DiscordWebhook", &g_DiscordWebhookHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordEmbedHandle = handlesys->CreateType("DiscordEmbed", &g_DiscordEmbedHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordInteractionHandle = handlesys->CreateType("DiscordInteraction", &g_DiscordInteractionHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordAutocompleteInteractionHandle = handlesys->CreateType("DiscordAutocompleteInteraction", &g_DiscordAutocompleteInteractionHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordSlashCommandHandle = handlesys->CreateType("DiscordSlashCommand", &g_DiscordSlashCommandHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordForumTagHandle = handlesys->CreateType("DiscordForumTag", &g_DiscordForumTagHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);
	g_DiscordGuildHandle = handlesys->CreateType("DiscordGuild", &g_DiscordGuildHandler, 0, nullptr, &haDefaults, myself->GetIdentity(), nullptr);

	g_pForwardReady = forwards->CreateForward("Discord_OnReady", ET_Ignore, 1, nullptr, Param_Cell);
	g_pForwardMessage = forwards->CreateForward("Discord_OnMessage", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);
	g_pForwardError = forwards->CreateForward("Discord_OnError", ET_Ignore, 2, nullptr, Param_Cell, Param_String);
	g_pForwardSlashCommand = forwards->CreateForward("Discord_OnSlashCommand", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);
	g_pForwardAutocomplete = forwards->CreateForward("Discord_OnAutocomplete", ET_Ignore, 5, nullptr, Param_Cell, Param_Cell, Param_Cell, Param_Cell, Param_String);

  smutils->AddGameFrameHook(&OnGameFrame);

	return true;
}

void DiscordExtension::SDK_OnUnload()
{

	forwards->ReleaseForward(g_pForwardReady);
	forwards->ReleaseForward(g_pForwardMessage);
	forwards->ReleaseForward(g_pForwardError);
	forwards->ReleaseForward(g_pForwardSlashCommand);
	forwards->ReleaseForward(g_pForwardAutocomplete);

	handlesys->RemoveType(g_DiscordHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordUserHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordMessageHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordChannelHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordWebhookHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordEmbedHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordInteractionHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordAutocompleteInteractionHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordSlashCommandHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordForumTagHandle, myself->GetIdentity());
	handlesys->RemoveType(g_DiscordGuildHandle, myself->GetIdentity());

  smutils->RemoveGameFrameHook(&OnGameFrame);
}

void DiscordHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordClient* discord = (DiscordClient*)object;
	discord->Stop();
	delete discord;
}

void DiscordUserHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordUser* user = (DiscordUser*)object;
	delete user;
}

void DiscordMessageHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordMessage* message = (DiscordMessage*)object;
	delete message;
}

void DiscordChannelHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordChannel* channel = (DiscordChannel*)object;
	delete channel;
}

void DiscordWebhookHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordWebhook* webhook = (DiscordWebhook*)object;
	delete webhook;
}

void DiscordEmbedHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordEmbed* embed = (DiscordEmbed*)object;
	delete embed;
}

void DiscordInteractionHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordInteraction* interaction = (DiscordInteraction*)object;
	delete interaction;
}

void DiscordAutocompleteInteractionHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordAutocompleteInteraction* interaction = (DiscordAutocompleteInteraction*)object;
	delete interaction;
}

void DiscordSlashCommandHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordSlashCommand* command = (DiscordSlashCommand*)object;
	delete command;
}

void DiscordForumTagHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordForumTag* tag = (DiscordForumTag*)object;
	delete tag;
}

void DiscordGuildHandler::OnHandleDestroy(HandleType_t type, void* object)
{
	DiscordGuild* guild = (DiscordGuild*)object;
	delete guild;
}