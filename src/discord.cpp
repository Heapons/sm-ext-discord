#include "extension.h"

// Discord Client Implementation
DiscordClient::DiscordClient(const char* token) : m_isRunning(false), m_discord_handle(0)
{
	m_cluster = std::make_unique<dpp::cluster>(token, dpp::i_default_intents | dpp::i_message_content);
}

DiscordClient::~DiscordClient()
{
	Stop();
}

bool DiscordClient::Initialize()
{
	try {
		SetupEventHandlers();
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to initialize Discord bot: %s", e.what());
		return false;
	}
}

void DiscordClient::RunBot()
{
	try {
		m_cluster->start(false);
	}
	catch (const std::exception& e) {
		g_TaskQueue.Push([this, error = std::string(e.what())]() {
			smutils->LogError(myself, "Failed to run Discord bot: %s", error.c_str());
			});
	}
}

void DiscordClient::Start()
{
	if (!m_cluster || m_isRunning) {
		return;
	}

	m_isRunning = true;
	m_thread = std::make_unique<std::thread>(&DiscordClient::RunBot, this);
	smutils->LogMessage(myself, "Discord bot started successfully");
}

void DiscordClient::Stop()
{
	if (!m_cluster || !m_isRunning) {
		return;
	}

	m_isRunning = false;

	try {
		if (m_cluster) {
			m_cluster->shutdown();
		}

		if (m_thread && m_thread->joinable()) {
			m_thread->join();
		}

		m_thread.reset();

		m_cluster.reset();

		smutils->LogMessage(myself, "Discord bot stopped successfully");
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Error during Discord bot shutdown: %s", e.what());
	}
}

bool DiscordClient::SetPresence(dpp::presence presence)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->set_presence(presence);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to set presence: %s", e.what());
		return false;
	}
}

bool DiscordClient::SendMessage(dpp::snowflake channel_id, const char* message)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->message_create(dpp::message(channel_id, message));
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to send message: %s", e.what());
		return false;
	}
}

bool DiscordClient::SendMessageEmbed(dpp::snowflake channel_id, const char* message, const DiscordEmbed* embed)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::message msg(channel_id, message);
		msg.add_embed(embed->GetEmbed());
		m_cluster->message_create(msg);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to send message with embed: %s", e.what());
		return false;
	}
}

bool DiscordClient::GetChannel(dpp::snowflake channel_id, IForward *callback_forward, cell_t data)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->channel_get(channel_id, [this, forward = callback_forward, value = data](const dpp::confirmation_callback_t& callback)
		{
			if (callback.is_error())
			{
				smutils->LogError(myself, "Failed to get channel: %s", callback.get_error().message.c_str());
				forwards->ReleaseForward(forward);
				return;
			}
			auto channel = callback.get<dpp::channel>();

			g_TaskQueue.Push([this, &forward, channel = new DiscordChannel(channel), value = value]() {
				if (forward && forward->GetFunctionCount() == 0)
				{
					return;
				}

				HandleError err;
				HandleSecurity sec(myself->GetIdentity(), myself->GetIdentity());
				Handle_t hndlResponse = handlesys->CreateHandleEx(g_DiscordChannelHandle, channel, &sec, nullptr, &err);
				if (hndlResponse == BAD_HANDLE)
				{
					smutils->LogError(myself, "Could not create channel fetch handle (error %d)", err);
					return;
				}

				forward->PushCell(m_discord_handle);
				forward->PushCell(hndlResponse);
				forward->PushCell(value);
				forward->Execute(nullptr);

				handlesys->FreeHandle(hndlResponse, &sec);

				forwards->ReleaseForward(forward);
            });
		});
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to get channel: %s", e.what());
		return false;
	}
}

void DiscordClient::SetupEventHandlers()
{
	if (!m_cluster) {
		return;
	}

	m_cluster->on_ready([this](const dpp::ready_t& event) {
		UpdateBotInfo();
		g_TaskQueue.Push([this]() {
			if (g_pForwardReady && g_pForwardReady->GetFunctionCount()) {
				g_pForwardReady->PushCell(m_discord_handle);
				g_pForwardReady->Execute(nullptr);
			}
			});
		});

	m_cluster->on_message_create([this](const dpp::message_create_t& event) {
		g_TaskQueue.Push([this, msg = event.msg]() {
			if (g_pForwardMessage && g_pForwardMessage->GetFunctionCount()) {
				DiscordMessage* message = new DiscordMessage(msg);
				HandleError err;
				HandleSecurity sec;
				sec.pOwner = myself->GetIdentity();
				sec.pIdentity = myself->GetIdentity();

				Handle_t messageHandle = handlesys->CreateHandleEx(g_DiscordMessageHandle,
					message,
					&sec,
					nullptr,
					&err);

				if (messageHandle != BAD_HANDLE) {
					g_pForwardMessage->PushCell(m_discord_handle);
					g_pForwardMessage->PushCell(messageHandle);
					g_pForwardMessage->Execute(nullptr);

					handlesys->FreeHandle(messageHandle, &sec);
				}
			}
			});
		});

	m_cluster->on_log([this](const dpp::log_t& event) {
		g_TaskQueue.Push([this, message = event.message]() {
			if (g_pForwardError && g_pForwardError->GetFunctionCount()) {
				g_pForwardError->PushCell(m_discord_handle);
				g_pForwardError->PushString(message.c_str());
				g_pForwardError->Execute(nullptr);
			}
			});
		});

	m_cluster->on_slashcommand([this](const dpp::slashcommand_t& event) {
		g_TaskQueue.Push([this, event]() {
			if (g_pForwardSlashCommand && g_pForwardSlashCommand->GetFunctionCount()) {
				DiscordInteraction* interaction = new DiscordInteraction(event);

				HandleError err;
				HandleSecurity sec;
				sec.pOwner = myself->GetIdentity();
				sec.pIdentity = myself->GetIdentity();

				Handle_t interactionHandle = handlesys->CreateHandleEx(g_DiscordInteractionHandle,
					interaction,
					&sec,
					nullptr,
					&err);

				if (interactionHandle != BAD_HANDLE) {
					g_pForwardSlashCommand->PushCell(m_discord_handle);
					g_pForwardSlashCommand->PushCell(interactionHandle);
					g_pForwardSlashCommand->Execute(nullptr);

					handlesys->FreeHandle(interactionHandle, &sec);
				}
			}
			});
		});
}

// Natives Implementation
static DiscordClient* GetDiscordPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordClient* discord;
	if ((err = handlesys->ReadHandle(handle, g_DiscordHandle, &sec, (void**)&discord)) != HandleError_None)
	{
		pContext->ReportError("Invalid Discord handle %x (error %d)", handle, err);
		return nullptr;
	}

	return discord;
}

static cell_t discord_CreateClient(IPluginContext* pContext, const cell_t* params)
{
	char* token;
	pContext->LocalToString(params[1], &token);

	DiscordClient* pDiscordClient = new DiscordClient(token);

	if (!pDiscordClient->Initialize())
	{
		delete pDiscordClient;
		return BAD_HANDLE;
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordHandle, pDiscordClient, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete pDiscordClient;
		pContext->ReportError("Could not create Discord handle (error %d)", err);
		return BAD_HANDLE;
	}

	pDiscordClient->SetHandle(handle);
	return handle;
}

static cell_t discord_Start(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	discord->Start();
	return 1;
}

static cell_t discord_Stop(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	discord->Stop();
	return 1;
}

static cell_t discord_GetBotId(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	const char* botId = discord->GetBotId();
	if (!botId) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], botId);
	return 1;
}

static cell_t discord_GetBotName(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	const char* botName = discord->GetBotName();
	if (!botName) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], botName);
	return 1;
}

static cell_t discord_GetBotDiscriminator(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	const char* discriminator = discord->GetBotDiscriminator();
	if (!discriminator) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], discriminator);
	return 1;
}

static cell_t discord_GetBotAvatarUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	const char* avatarUrl = discord->GetBotAvatarUrl();
	if (!avatarUrl) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], avatarUrl);
	return 1;
}

static cell_t discord_SetPresence(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* status_text;
	pContext->LocalToString(params[4], &status_text);

	try {
        dpp::presence presence(static_cast<dpp::presence_status>(params[2]), static_cast<dpp::activity_type>(params[3]), status_text);
		return discord->SetPresence(presence);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Unable to create presence object: %s", e.what());
		return 0;
	}
}

static cell_t discord_SendMessage(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	char* message;
	pContext->LocalToString(params[3], &message);

	try {
		dpp::snowflake channel = std::stoull(channelId);
		return discord->SendMessage(channel, message) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t discord_SendMessageEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	char* message;
	pContext->LocalToString(params[3], &message);

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordEmbed* embed;
	if ((err = handlesys->ReadHandle(params[4], g_DiscordEmbedHandle, &sec, (void**)&embed)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord embed handle %x (error %d)", params[4], err);
	}

	try {
		dpp::snowflake channel = std::stoull(channelId);
		return discord->SendMessageEmbed(channel, message, embed) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t discord_GetChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	try {
		dpp::snowflake channelFlake = std::stoull(channelId);

		IPluginFunction *callback = pContext->GetFunctionById(params[3]);

		IChangeableForward *forward = forwards->CreateForwardEx(nullptr, ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Cell);
		if (forward == nullptr || !forward->AddFunction(callback))
		{
			return pContext->ThrowNativeError("Could not create forward.");
		}

		cell_t data = params[4];
		return discord->GetChannel(channelFlake, forward, data);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t discord_IsRunning(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	return discord->IsRunning() ? 1 : 0;
}

// Embed natives
static cell_t embed_CreateEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = new DiscordEmbed();

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordEmbedHandle, embed, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete embed;
		return pContext->ThrowNativeError("Could not create Discord embed handle (error %d)", err);
	}

	return handle;
}

static DiscordEmbed* GetEmbedPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordEmbed* embed;
	if ((err = handlesys->ReadHandle(handle, g_DiscordEmbedHandle, &sec, (void**)&embed)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord embed handle %x (error %d)", handle, err);
		return nullptr;
	}

	return embed;
}

static cell_t embed_SetTitle(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	char* title;
	pContext->LocalToString(params[2], &title);

	embed->SetTitle(title);
	return 1;
}

static cell_t embed_SetDescription(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	char* desc;
	pContext->LocalToString(params[2], &desc);

	embed->SetDescription(desc);
	return 1;
}

static cell_t embed_SetColor(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	embed->SetColor(params[2]);
	return 1;
}

static cell_t embed_SetUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	char* url;
	pContext->LocalToString(params[2], &url);

	embed->SetUrl(url);
	return 1;
}

static cell_t embed_SetAuthor(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	char* url = nullptr;
	if (params[3] != 0) {
		pContext->LocalToString(params[3], &url);
	}

	char* icon_url = nullptr;
	if (params[4] != 0) {
		pContext->LocalToString(params[4], &icon_url);
	}

	embed->SetAuthor(name, url, icon_url);
	return 1;
}

static cell_t embed_SetFooter(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	char* text;
	pContext->LocalToString(params[2], &text);

	char* icon_url = nullptr;
	if (params[3] != 0) {
		pContext->LocalToString(params[3], &icon_url);
	}

	embed->SetFooter(text, icon_url);
	return 1;
}

static cell_t embed_AddField(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	char* value;
	pContext->LocalToString(params[3], &value);

	bool inLine = params[4] ? true : false;

	embed->AddField(name, value, inLine);
	return 1;
}

static cell_t embed_SetThumbnail(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	char* url;
	pContext->LocalToString(params[2], &url);

	embed->SetThumbnail(url);
	return 1;
}

static cell_t embed_SetImage(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	char* url;
	pContext->LocalToString(params[2], &url);

	embed->SetImage(url);
	return 1;
}

static DiscordMessage* GetMessagePointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordMessage* message;
	if ((err = handlesys->ReadHandle(handle, g_DiscordMessageHandle, &sec, (void**)&message)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord message handle %x (error %d)", handle, err);
		return nullptr;
	}

	return message;
}

// Message natives
static cell_t message_GetContent(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetContent());
	return 1;
}

static cell_t message_GetMessageId(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetMessageId().c_str());
	return 1;
}

static cell_t message_GetChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetChannelId().c_str());
	return 1;
}

static cell_t message_GetGuildId(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetGuildId().c_str());
	return 1;
}

static cell_t message_GetAuthorId(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetAuthorId().c_str());
	return 1;
}

static cell_t message_GetAuthorName(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetAuthorName());
	return 1;
}

static cell_t message_GetAuthorDiscriminator(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->GetAuthorDiscriminator();
}

static cell_t message_IsBot(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->IsBot() ? 1 : 0;
}

static DiscordChannel* GetChannelPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordChannel* channel;
	if ((err = handlesys->ReadHandle(handle, g_DiscordChannelHandle, &sec, (void**)&channel)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord channel handle %x (error %d)", handle, err);
		return nullptr;
	}

	return channel;
}

// Channel natives
static cell_t channel_GetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], channel->GetName());
	return 1;
}

bool DiscordClient::RegisterSlashCommand(dpp::snowflake guild_id, const char* name, const char* description)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::slashcommand command;
		command.set_name(name)
			.set_description(description)
			.set_application_id(m_cluster->me.id);

		m_cluster->guild_command_create(command, guild_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to register slash command: %s", e.what());
		return false;
	}
}

bool DiscordClient::RegisterGlobalSlashCommand(const char* name, const char* description)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::slashcommand command;
		command.set_name(name)
			.set_description(description)
			.set_application_id(m_cluster->me.id);

		m_cluster->global_command_create(command);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to register global slash command: %s", e.what());
		return false;
	}
}

static cell_t discord_RegisterSlashCommand(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* guildId;
	pContext->LocalToString(params[2], &guildId);

	char* name;
	pContext->LocalToString(params[3], &name);

	char* description;
	pContext->LocalToString(params[4], &description);

	try {
		dpp::snowflake guild = std::stoull(guildId);
		return discord->RegisterSlashCommand(guild, name, description) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid guild ID format: %s", guildId);
		return 0;
	}
}

static cell_t discord_RegisterGlobalSlashCommand(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	char* description;
	pContext->LocalToString(params[3], &description);

	return discord->RegisterGlobalSlashCommand(name, description) ? 1 : 0;
}

static cell_t interaction_CreateResponse(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	interaction->CreateResponse(content);
	return 1;
}

static cell_t interaction_CreateResponseEmbed(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	DiscordEmbed* embed;
	if ((err = handlesys->ReadHandle(params[3], g_DiscordEmbedHandle, &sec, (void**)&embed)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord embed handle %x (error %d)", params[3], err);
	}

	interaction->CreateResponseEmbed(content, embed);
	return 1;
}

static cell_t interaction_GetOptionValue(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	std::string value;
	if (!interaction->GetOptionValue(name, value)) {
		return 0;
	}

	pContext->StringToLocal(params[3], params[4], value.c_str());
	return 1;
}

// TODO: process int64_t
static cell_t interaction_GetOptionValueInt(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	int64_t value;
	if (!interaction->GetOptionValueInt(name, value)) {
		return 0;
	}

	return value;
}

static cell_t interaction_GetOptionValueFloat(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	double value;
	if (!interaction->GetOptionValueDouble(name, value)) {
		return 0;
	}

	return sp_ftoc((float)value);
}

static cell_t interaction_GetOptionValueBool(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	bool value;
	if (!interaction->GetOptionValueBool(name, value)) {
		return 0;
	}

	return value ? 1 : 0;
}

static cell_t interaction_DeferReply(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	interaction->DeferReply(params[2] ? true : false);
	return 1;
}

static cell_t interaction_EditResponse(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	interaction->EditResponse(content);
	return 1;
}

static cell_t interaction_CreateEphemeralResponse(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	interaction->CreateEphemeralResponse(content);
	return 1;
}

static cell_t interaction_CreateEphemeralResponseEmbed(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	DiscordEmbed* embed;
	if ((err = handlesys->ReadHandle(params[3], g_DiscordEmbedHandle, &sec, (void**)&embed)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord embed handle %x (error %d)", params[3], err);
	}

	interaction->CreateEphemeralResponseEmbed(content, embed);
	return 1;
}

static cell_t interaction_GetCommandName(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	const char* commandName = interaction->GetCommandName();
	pContext->StringToLocal(params[2], params[3], commandName);
	return 1;
}

static cell_t interaction_GetGuildId(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	std::string guildId = interaction->GetGuildId();
	pContext->StringToLocal(params[2], params[3], guildId.c_str());
	return 1;
}

static cell_t interaction_GetChannelId(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	std::string channelId = interaction->GetChannelId();
	pContext->StringToLocal(params[2], params[3], channelId.c_str());
	return 1;
}

static cell_t interaction_GetUserId(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	std::string userId = interaction->GetUserId();
	pContext->StringToLocal(params[2], params[3], userId.c_str());
	return 1;
}

static cell_t interaction_GetUserName(IPluginContext* pContext, const cell_t* params)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(params[1], g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", params[1], err);
	}

	const char* userName = interaction->GetUserName();
	pContext->StringToLocal(params[2], params[3], userName);
	return 1;
}

bool DiscordClient::EditMessage(dpp::snowflake channel_id, dpp::snowflake message_id, const char* content)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::message msg;
		msg.id = message_id;
		msg.channel_id = channel_id;
		msg.content = content;
		m_cluster->message_edit(msg);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to edit message: %s", e.what());
		return false;
	}
}

bool DiscordClient::EditMessageEmbed(dpp::snowflake channel_id, dpp::snowflake message_id, const char* content, const DiscordEmbed* embed)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::message msg;
		msg.id = message_id;
		msg.channel_id = channel_id;
		msg.content = content;
		msg.add_embed(embed->GetEmbed());
		m_cluster->message_edit(msg);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to edit message with embed: %s", e.what());
		return false;
	}
}

bool DiscordClient::DeleteMessage(dpp::snowflake channel_id, dpp::snowflake message_id)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->message_delete(message_id, channel_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to delete message: %s", e.what());
		return false;
	}
}

static cell_t discord_EditMessage(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	char* messageId;
	pContext->LocalToString(params[3], &messageId);

	char* content;
	pContext->LocalToString(params[4], &content);

	try {
		dpp::snowflake channel = std::stoull(channelId);
		dpp::snowflake message = std::stoull(messageId);
		return discord->EditMessage(channel, message, content) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid ID format");
		return 0;
	}
}

static cell_t discord_EditMessageEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	char* messageId;
	pContext->LocalToString(params[3], &messageId);

	char* content;
	pContext->LocalToString(params[4], &content);

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordEmbed* embed;
	if ((err = handlesys->ReadHandle(params[5], g_DiscordEmbedHandle, &sec, (void**)&embed)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord embed handle %x (error %d)", params[5], err);
	}

	try {
		dpp::snowflake channel = std::stoull(channelId);
		dpp::snowflake message = std::stoull(messageId);
		return discord->EditMessageEmbed(channel, message, content, embed) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid ID format");
		return 0;
	}
}

static cell_t discord_DeleteMessage(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	char* messageId;
	pContext->LocalToString(params[3], &messageId);

	try {
		dpp::snowflake channel = std::stoull(channelId);
		dpp::snowflake message = std::stoull(messageId);
		return discord->DeleteMessage(channel, message) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid ID format");
		return 0;
	}
}

bool DiscordClient::RegisterSlashCommandWithOptions(dpp::snowflake guild_id, const char* name, const char* description,
	const std::vector<dpp::command_option>& options)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::slashcommand command;
		command.set_name(name)
			.set_description(description)
			.set_application_id(m_cluster->me.id);

		command.options = options;
		m_cluster->guild_command_create(command, guild_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to register slash command with options: %s", e.what());
		return false;
	}
}

bool DiscordClient::RegisterGlobalSlashCommandWithOptions(const char* name, const char* description,
	const std::vector<dpp::command_option>& options)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::slashcommand command;
		command.set_name(name)
			.set_description(description)
			.set_application_id(m_cluster->me.id);

		command.options = options;
		m_cluster->global_command_create(command);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to register global slash command with options: %s", e.what());
		return false;
	}
}

static cell_t discord_RegisterSlashCommandWithOptions(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* guildId;
	pContext->LocalToString(params[2], &guildId);

	char* name;
	pContext->LocalToString(params[3], &name);

	char* description;
	pContext->LocalToString(params[4], &description);

	cell_t* option_names_array;
	cell_t* option_descriptions_array;
	cell_t* option_types_array;
	cell_t* option_required_array;

	pContext->LocalToPhysAddr(params[5], &option_names_array);
	pContext->LocalToPhysAddr(params[6], &option_descriptions_array);
	pContext->LocalToPhysAddr(params[7], &option_types_array);
	pContext->LocalToPhysAddr(params[8], &option_required_array);

	cell_t optionsSize = params[9];

	std::vector<dpp::command_option> options;

	for (cell_t i = 0; i < optionsSize; i++) {
		char* option_name;
		char* option_description;

		pContext->LocalToString(option_names_array[i], &option_name);
		pContext->LocalToString(option_descriptions_array[i], &option_description);

		dpp::command_option cmd_option(
			static_cast<dpp::command_option_type>(option_types_array[i]),
			option_name,
			option_description,
			option_required_array[i]
		);

		options.push_back(cmd_option);
	}

	try {
		dpp::snowflake guild = std::stoull(guildId);
		return discord->RegisterSlashCommandWithOptions(guild, name, description, options) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid guild ID format: %s", guildId);
		return 0;
	}
}

static cell_t discord_RegisterGlobalSlashCommandWithOptions(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	char* description;
	pContext->LocalToString(params[3], &description);

	cell_t* option_names_array;
	cell_t* option_descriptions_array;
	cell_t* option_types_array;
	cell_t* option_required_array;

	pContext->LocalToPhysAddr(params[4], &option_names_array);
	pContext->LocalToPhysAddr(params[5], &option_descriptions_array);
	pContext->LocalToPhysAddr(params[6], &option_types_array);
	pContext->LocalToPhysAddr(params[7], &option_required_array);

	cell_t optionsSize = params[8];

	std::vector<dpp::command_option> options;

	for (cell_t i = 0; i < optionsSize; i++) {
		char* option_name;
		char* option_description;

		pContext->LocalToString(option_names_array[i], &option_name);
		pContext->LocalToString(option_descriptions_array[i], &option_description);

		dpp::command_option cmd_option(
			static_cast<dpp::command_option_type>(option_types_array[i]),
			option_name,
			option_description,
			option_required_array[i]
		);

		options.push_back(cmd_option);
	}

	return discord->RegisterGlobalSlashCommandWithOptions(name, description, options) ? 1 : 0;
}

bool DiscordClient::DeleteGuildCommand(dpp::snowflake guild_id, dpp::snowflake command_id)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->guild_command_delete(command_id, guild_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to delete guild command: %s", e.what());
		return false;
	}
}

bool DiscordClient::DeleteGlobalCommand(dpp::snowflake command_id)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->global_command_delete(command_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to delete global command: %s", e.what());
		return false;
	}
}

static cell_t discord_DeleteGuildCommand(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* guildId;
	pContext->LocalToString(params[2], &guildId);

	char* commandId;
	pContext->LocalToString(params[3], &commandId);

	try {
		dpp::snowflake guild = std::stoull(guildId);
		dpp::snowflake command = std::stoull(commandId);
		return discord->DeleteGuildCommand(guild, command) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid ID format");
		return 0;
	}
}

static cell_t discord_DeleteGlobalCommand(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* commandId;
	pContext->LocalToString(params[2], &commandId);

	try {
		dpp::snowflake command = std::stoull(commandId);
		return discord->DeleteGlobalCommand(command) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid command ID format");
		return 0;
	}
}

const sp_nativeinfo_t discord_natives[] = {
	// Discord
	{"Discord.Discord",          discord_CreateClient},
	{"Discord.Start",            discord_Start},
	{"Discord.Stop",             discord_Stop},
	{"Discord.GetBotId",           discord_GetBotId},
	{"Discord.GetBotName",         discord_GetBotName},
	{"Discord.GetBotDiscriminator", discord_GetBotDiscriminator},
	{"Discord.GetBotAvatarUrl",    discord_GetBotAvatarUrl},
	{"Discord.SetPresence",      discord_SetPresence},
	{"Discord.SendMessage",      discord_SendMessage},
	{"Discord.SendMessageEmbed", discord_SendMessageEmbed},
	{"Discord.GetChannel", discord_GetChannel},
	{"Discord.IsRunning",        discord_IsRunning},
	{"Discord.RegisterSlashCommand", discord_RegisterSlashCommand},
	{"Discord.RegisterGlobalSlashCommand", discord_RegisterGlobalSlashCommand},
	{"Discord.EditMessage", discord_EditMessage},
	{"Discord.EditMessageEmbed", discord_EditMessageEmbed},
	{"Discord.DeleteMessage", discord_DeleteMessage},
	{"Discord.RegisterSlashCommandWithOptions", discord_RegisterSlashCommandWithOptions},
  	{"Discord.RegisterGlobalSlashCommandWithOptions", discord_RegisterGlobalSlashCommandWithOptions},
	{"Discord.DeleteGuildCommand", discord_DeleteGuildCommand},
  	{"Discord.DeleteGlobalCommand", discord_DeleteGlobalCommand},

	// Message
	{"DiscordMessage.GetContent",    message_GetContent},
	{"DiscordMessage.GetMessageId",  message_GetMessageId},
	{"DiscordMessage.GetChannelId",  message_GetChannelId},
	{"DiscordMessage.GetGuildId",    message_GetGuildId},
	{"DiscordMessage.GetAuthorId",   message_GetAuthorId},
	{"DiscordMessage.GetAuthorName", message_GetAuthorName},
	{"DiscordMessage.GetAuthorDiscriminator", message_GetAuthorDiscriminator},
	{"DiscordMessage.IsBot",         message_IsBot},

    // Channel
    {"DiscordChannel.GetName",       channel_GetName},

	// Embed
	{"DiscordEmbed.DiscordEmbed", embed_CreateEmbed},
	{"DiscordEmbed.SetTitle",     embed_SetTitle},
	{"DiscordEmbed.SetDescription", embed_SetDescription},
	{"DiscordEmbed.SetColor",     embed_SetColor},
	{"DiscordEmbed.SetUrl",       embed_SetUrl},
	{"DiscordEmbed.SetAuthor",    embed_SetAuthor},
	{"DiscordEmbed.SetFooter",    embed_SetFooter},
	{"DiscordEmbed.AddField",     embed_AddField},
	{"DiscordEmbed.SetThumbnail", embed_SetThumbnail},
	{"DiscordEmbed.SetImage",     embed_SetImage},

	// Slash Command
	{"DiscordInteraction.CreateResponse", interaction_CreateResponse},
	{"DiscordInteraction.CreateResponseEmbed", interaction_CreateResponseEmbed},
	{"DiscordInteraction.GetOptionValue", interaction_GetOptionValue},
	{"DiscordInteraction.GetOptionValueInt", interaction_GetOptionValueInt},
	{"DiscordInteraction.GetOptionValueFloat", interaction_GetOptionValueFloat},
	{"DiscordInteraction.GetOptionValueBool", interaction_GetOptionValueBool},
	{"DiscordInteraction.DeferReply", interaction_DeferReply},
	{"DiscordInteraction.EditResponse", interaction_EditResponse},
	{"DiscordInteraction.CreateEphemeralResponse", interaction_CreateEphemeralResponse},
	{"DiscordInteraction.CreateEphemeralResponseEmbed", interaction_CreateEphemeralResponseEmbed},
	{"DiscordInteraction.GetCommandName", interaction_GetCommandName},
	{"DiscordInteraction.GetGuildId", interaction_GetGuildId},
	{"DiscordInteraction.GetChannelId", interaction_GetChannelId},
	{"DiscordInteraction.GetUserId", interaction_GetUserId},
	{"DiscordInteraction.GetUserName", interaction_GetUserName},
	{nullptr, nullptr}
};