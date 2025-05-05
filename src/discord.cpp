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

void AddAllowedMentionsToMessage(dpp::message* msg, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles)
{
	msg->set_allowed_mentions(allowed_mentions_mask & 1, allowed_mentions_mask & 2, allowed_mentions_mask & 4, allowed_mentions_mask & 8, users, roles);
}

bool DiscordClient::ExecuteWebhook(dpp::webhook wh, const char* message, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles)
{
	if (!m_isRunning) {
		return false;
	}

	dpp::message message_obj(message);
	AddAllowedMentionsToMessage(&message_obj, allowed_mentions_mask, users, roles);

	try {
		m_cluster->execute_webhook(wh, message_obj);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to execute webhook: %s", e.what());
		return false;
	}
}

bool DiscordClient::SendMessage(dpp::snowflake channel_id, const char* message, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles)
{
	if (!m_isRunning) {
		return false;
	}

	dpp::message message_obj(channel_id, message);
	AddAllowedMentionsToMessage(&message_obj, allowed_mentions_mask, users, roles);

	try {
		m_cluster->message_create(message_obj);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to send message: %s", e.what());
		return false;
	}
}

bool DiscordClient::SendMessageEmbed(dpp::snowflake channel_id, const char* message, const DiscordEmbed* embed, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles)
{
	if (!m_isRunning) {
		return false;
	}

	dpp::message message_obj(channel_id, message);
	AddAllowedMentionsToMessage(&message_obj, allowed_mentions_mask, users, roles);

	try {
		message_obj.add_embed(embed->GetEmbed());
		m_cluster->message_create(message_obj);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to send message with embed: %s", e.what());
		return false;
	}
}

bool DiscordClient::GetChannelWebhooks(dpp::snowflake channel_id, IForward *callback_forward, cell_t data)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->get_channel_webhooks(channel_id, [this, forward = callback_forward, value = data](const dpp::confirmation_callback_t& callback)
		{
			if (callback.is_error())
			{
				smutils->LogError(myself, "Failed to get channel webhooks: %s", callback.get_error().message.c_str());
				forwards->ReleaseForward(forward);
				return;
			}
			auto webhook_map = callback.get<dpp::webhook_map>();

			g_TaskQueue.Push([this, &forward, webhooks = webhook_map, value = value]() {
				if (forward && forward->GetFunctionCount() == 0)
				{
					return;
				}

				int webhook_count = webhooks.size();
				std::unique_ptr<cell_t[]> handles = std::make_unique<cell_t[]>(webhook_count);

				HandleError err;
				HandleSecurity sec(myself->GetIdentity(), myself->GetIdentity());
				int i = 0;
				for (auto pair : webhooks)
				{
					DiscordWebhook* wbk = new DiscordWebhook(pair.second);
					Handle_t webhookHandle = handlesys->CreateHandleEx(g_DiscordWebhookHandle, wbk, &sec, nullptr, &err);
					if (webhookHandle == BAD_HANDLE)
					{
						smutils->LogError(myself, "Could not create webhook handle (error %d)", err);
						continue;
					}
					handles[i++] = webhookHandle;
				}

				forward->PushCell(m_discord_handle);
				forward->PushArray(handles.get(), webhook_count);
				forward->PushCell(webhook_count);
				forward->PushCell(value);
				forward->Execute(nullptr);

				for (i = 0; i < webhook_count; i++)
				{
					handlesys->FreeHandle(handles[i], &sec);
				}

				forwards->ReleaseForward(forward);
			});
		});
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to get channel webhooks: %s", e.what());
		return false;
	}
}

bool DiscordClient::CreateWebhook(dpp::webhook wh, IForward *callback_forward, cell_t data)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->create_webhook(wh, [this, forward = callback_forward, value = data](const dpp::confirmation_callback_t& callback)
		{
			if (callback.is_error())
			{
				smutils->LogError(myself, "Failed to create webhook: %s", callback.get_error().message.c_str());
				forwards->ReleaseForward(forward);
				return;
			}
			auto webhook = callback.get<dpp::webhook>();

			g_TaskQueue.Push([this, &forward, webhook = new DiscordWebhook(webhook), value = value]() {
				if (forward && forward->GetFunctionCount() == 0)
				{
					return;
				}

				HandleError err;
				HandleSecurity sec(myself->GetIdentity(), myself->GetIdentity());
				Handle_t webhookHandle = handlesys->CreateHandleEx(g_DiscordWebhookHandle, webhook, &sec, nullptr, &err);
				if (webhookHandle == BAD_HANDLE)
				{
					smutils->LogError(myself, "Could not create webhook handle (error %d)", err);
					return;
				}

				forward->PushCell(m_discord_handle);
				forward->PushCell(webhookHandle);
				forward->PushCell(value);
				forward->Execute(nullptr);

				handlesys->FreeHandle(webhookHandle, &sec);

				forwards->ReleaseForward(forward);
            });
		});
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to get create webhook: %s", e.what());
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
				Handle_t channelHandle = handlesys->CreateHandleEx(g_DiscordChannelHandle, channel, &sec, nullptr, &err);
				if (channelHandle == BAD_HANDLE)
				{
					smutils->LogError(myself, "Could not create channel handle (error %d)", err);
					return;
				}

				forward->PushCell(m_discord_handle);
				forward->PushCell(channelHandle);
				forward->PushCell(value);
				forward->Execute(nullptr);

				handlesys->FreeHandle(channelHandle, &sec);

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

	m_cluster->on_autocomplete([this](const dpp::autocomplete_t& event) {
		g_TaskQueue.Push([this, event]() {
			if (g_pForwardAutocomplete && g_pForwardAutocomplete->GetFunctionCount()) {
				DiscordAutocompleteInteraction* interaction = new DiscordAutocompleteInteraction(event);

				HandleError err;
				HandleSecurity sec;
				sec.pOwner = myself->GetIdentity();
				sec.pIdentity = myself->GetIdentity();

				Handle_t interactionHandle = handlesys->CreateHandleEx(g_DiscordAutocompleteInteractionHandle,
						interaction,
						&sec,
						nullptr,
						&err);

				if (interactionHandle != BAD_HANDLE) {
					std::string str;
					for (auto & opt : event.options) {
						dpp::command_option_type type = opt.type;

						g_pForwardAutocomplete->PushCell(m_discord_handle);
						g_pForwardAutocomplete->PushCell(interactionHandle);
						g_pForwardAutocomplete->PushCell(opt.focused ? 1 : 0);
						g_pForwardAutocomplete->PushCell(type);
						g_pForwardAutocomplete->PushString(opt.name.c_str());
						g_pForwardAutocomplete->Execute(nullptr);
					}
	        	}

				handlesys->FreeHandle(interactionHandle, &sec);
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

static DiscordWebhook* GetWebhookPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordWebhook* webhook;
	if ((err = handlesys->ReadHandle(handle, g_DiscordWebhookHandle, &sec, (void**)&webhook)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord webhook handle %x (error %d)", handle, err);
		return nullptr;
	}

	return webhook;
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

static cell_t discord_GetChannelWebhooks(IPluginContext* pContext, const cell_t* params)
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

		IChangeableForward *forward = forwards->CreateForwardEx(nullptr, ET_Ignore, 4, nullptr, Param_Cell, Param_Array, Param_Cell, Param_Any);
		if (forward == nullptr || !forward->AddFunction(callback))
		{
			return pContext->ThrowNativeError("Could not create forward.");
		}

		cell_t data = params[4];
		return discord->GetChannelWebhooks(channelFlake, forward, data);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t discord_CreateWebhook(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	char* name;
	pContext->LocalToString(params[3], &name);

	try {
		dpp::snowflake channelFlake = std::stoull(channelId);
		dpp::webhook webhook;
		webhook.name = name;
		webhook.channel_id = channelFlake;

		IPluginFunction *callback = pContext->GetFunctionById(params[4]);

		IChangeableForward *forward = forwards->CreateForwardEx(nullptr, ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Any);
		if (forward == nullptr || !forward->AddFunction(callback))
		{
			return pContext->ThrowNativeError("Could not create forward.");
		}

		cell_t data = params[5];
		return discord->CreateWebhook(webhook, forward, data);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t discord_ExecuteWebhook(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[2]);

	char* message;
	pContext->LocalToString(params[3], &message);

	cell_t* users_array;
	cell_t* roles_array;

	pContext->LocalToPhysAddr(params[5], &users_array);
	pContext->LocalToPhysAddr(params[7], &roles_array);

	std::vector<dpp::snowflake> users(params[6]);
	std::vector<dpp::snowflake> roles(params[8]);

	for (int i = 0; i < users.size(); i++) {
		char* str;

		pContext->LocalToString(users_array[i], &str);
		try {
			users[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue; // Stub
		}
	}

	for (int i = 0; i < roles.size(); i++) {
		char* str;

		pContext->LocalToString(roles_array[i], &str);
		try {
			roles[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue; // Stub
		}
	}

	try {
		return discord->ExecuteWebhook(webhook->m_webhook, message, params[4], users, roles) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Failed to execute webhook: %s", e.what());
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

	cell_t* users_array;
	cell_t* roles_array;

	pContext->LocalToPhysAddr(params[5], &users_array);
	pContext->LocalToPhysAddr(params[7], &roles_array);

	std::vector<dpp::snowflake> users(params[6]);
	std::vector<dpp::snowflake> roles(params[8]);

	for (int i = 0; i < users.size(); i++) {
		char* str;

		pContext->LocalToString(users_array[i], &str);
		try {
			users[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue; // Stub
		}
	}

	for (int i = 0; i < roles.size(); i++) {
		char* str;

		pContext->LocalToString(roles_array[i], &str);
		try {
			roles[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue; // Stub
		}
	}

	try {
		dpp::snowflake channel = std::stoull(channelId);
		return discord->SendMessage(channel, message, params[4], users, roles) ? 1 : 0;
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

	cell_t* users_array;
	cell_t* roles_array;

	pContext->LocalToPhysAddr(params[6], &users_array);
	pContext->LocalToPhysAddr(params[8], &roles_array);

	std::vector<dpp::snowflake> users(params[7]);
	std::vector<dpp::snowflake> roles(params[9]);

	for (int i = 0; i < users.size(); i++) {
		char* str;

		pContext->LocalToString(users_array[i], &str);
		try {
			users[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue; // Stub
		}
	}

	for (int i = 0; i < roles.size(); i++) {
		char* str;

		pContext->LocalToString(roles_array[i], &str);
		try {
			roles[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue; // Stub
		}
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordEmbed* embed;
	if ((err = handlesys->ReadHandle(params[4], g_DiscordEmbedHandle, &sec, (void**)&embed)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord embed handle %x (error %d)", params[4], err);
	}

	try {
		dpp::snowflake channel = std::stoull(channelId);
		return discord->SendMessageEmbed(channel, message, embed, params[5], users, roles) ? 1 : 0;
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

		IChangeableForward *forward = forwards->CreateForwardEx(nullptr, ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Any);
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

static DiscordUser* GetUserPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordUser* user;
	if ((err = handlesys->ReadHandle(handle, g_DiscordUserHandle, &sec, (void**)&user)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord user handle %x (error %d)", handle, err);
		return nullptr;
	}

	return user;
}

// User natives
static cell_t user_GetId(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetId().c_str());
	return 1;
}

static cell_t user_GetUsername(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetUsername());
	return 1;
}

static cell_t user_GetDiscriminator(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->GetDiscriminator();
}

static cell_t user_GetGlobalName(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetGlobalName());
	return 1;
}

static cell_t user_GetAvatarUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[3], params[4], user->GetAvatarUrl(params[2] ? true : false).c_str());
	return 1;
}

static cell_t user_IsBot(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsBot() ? 1 : 0;
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

static cell_t message_GetAuthor(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	DiscordUser* pDiscordUser = message->GetAuthor();

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete pDiscordUser;
		pContext->ReportError("Could not create user handle (error %d)", err);
		return BAD_HANDLE;
	}

	return handle;
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

static cell_t message_GetAuthorDisplayName(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetAuthorDisplayName());
	return 1;
}

static cell_t message_GetAuthorNickname(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetAuthorNickname().c_str());
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

// Webhook natives
static cell_t webhook_CreateWebhook(IPluginContext* pContext, const cell_t* params)
{
	char* webhook_url;
	pContext->LocalToString(params[1], &webhook_url);

	dpp::webhook webhook;
	try
	{
    	webhook = dpp::webhook(webhook_url);
	}
	catch (const std::exception& e)
	{
		pContext->ReportError("Webhook url invalid: %s", e.what());
		return BAD_HANDLE;
	}

	DiscordWebhook* pDiscordWebhook = new DiscordWebhook(webhook);

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordWebhookHandle, pDiscordWebhook, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete pDiscordWebhook;
		pContext->ReportError("Could not create webhook handle (error %d)", err);
		return BAD_HANDLE;
	}

	return handle;
}

static cell_t webhook_GetId(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetId().c_str());
	return 1;
}

static cell_t webhook_GetUser(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	DiscordUser* pDiscordUser = webhook->GetUser();

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete pDiscordUser;
		pContext->ReportError("Could not create user handle (error %d)", err);
		return BAD_HANDLE;
	}

	return handle;
}

static cell_t webhook_GetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetName());
	return 1;
}

static cell_t webhook_SetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);
	webhook->SetName(name);
	return 1;
}

static cell_t webhook_GetAvatarUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetAvatarUrl());
	return 1;
}

static cell_t webhook_SetAvatarUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	char* avatar_url;
	pContext->LocalToString(params[2], &avatar_url);
	webhook->SetAvatarUrl(avatar_url);
	return 1;
}

static cell_t webhook_GetAvatarData(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetAvatarData().c_str());
	return 1;
}

static cell_t webhook_SetAvatarData(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	char* avatar_data;
	pContext->LocalToString(params[2], &avatar_data);
	webhook->SetAvatarData(avatar_data);
	return 1;
}

bool DiscordClient::RegisterSlashCommand(dpp::snowflake guild_id, const char* name, const char* description, const char* default_permissions)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::slashcommand command;
		command.set_name(name)
			.set_description(description)
			.set_application_id(m_cluster->me.id);

		if ((default_permissions != NULL) && (default_permissions[0] != '\0')) {
			command.set_default_permissions(std::stoull(default_permissions));
		}

		m_cluster->guild_command_create(command, guild_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to register slash command: %s", e.what());
		return false;
	}
}

bool DiscordClient::RegisterGlobalSlashCommand(const char* name, const char* description, const char* default_permissions)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		dpp::slashcommand command;
		command.set_name(name)
			.set_description(description)
			.set_application_id(m_cluster->me.id);

		if ((default_permissions != NULL) && (default_permissions[0] != '\0')) {
			command.set_default_permissions(std::stoull(default_permissions));
		}

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

	char* permissions;
	pContext->LocalToString(params[5], &permissions);

	try {
		dpp::snowflake guild = std::stoull(guildId);
		return discord->RegisterSlashCommand(guild, name, description, permissions) ? 1 : 0;
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

	char* permissions;
	pContext->LocalToString(params[4], &permissions);

	return discord->RegisterGlobalSlashCommand(name, description, permissions) ? 1 : 0;
}

static DiscordInteraction* GetInteractionPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordInteraction* interaction;
	if ((err = handlesys->ReadHandle(handle, g_DiscordInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord interaction handle %x (error %d)", handle, err);
		return nullptr;
	}

	return interaction;
}

// Interaction natives
static cell_t interaction_CreateResponse(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	interaction->CreateResponse(content);
	return 1;
}

static cell_t interaction_CreateResponseEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
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
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
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
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
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
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
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
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
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
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	interaction->DeferReply(params[2] ? true : false);
	return 1;
}

static cell_t interaction_EditResponse(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	interaction->EditResponse(content);
	return 1;
}

static cell_t interaction_CreateEphemeralResponse(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	interaction->CreateEphemeralResponse(content);
	return 1;
}

static cell_t interaction_CreateEphemeralResponseEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* content;
	pContext->LocalToString(params[2], &content);

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
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
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	const char* commandName = interaction->GetCommandName();
	pContext->StringToLocal(params[2], params[3], commandName);
	return 1;
}

static cell_t interaction_GetGuildId(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	std::string guildId = interaction->GetGuildId();
	pContext->StringToLocal(params[2], params[3], guildId.c_str());
	return 1;
}

static cell_t interaction_GetChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	std::string channelId = interaction->GetChannelId();
	pContext->StringToLocal(params[2], params[3], channelId.c_str());
	return 1;
}

static cell_t interaction_GetUser(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	DiscordUser* pDiscordUser = interaction->GetUser();

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete pDiscordUser;
		pContext->ReportError("Could not create user handle (error %d)", err);
		return BAD_HANDLE;
	}

	return handle;
}

static cell_t interaction_GetUserId(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	std::string userId = interaction->GetUserId();
	pContext->StringToLocal(params[2], params[3], userId.c_str());
	return 1;
}

static cell_t interaction_GetUserName(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	const char* userName = interaction->GetUserName();
	pContext->StringToLocal(params[2], params[3], userName);
	return 1;
}

static cell_t interaction_GetUserNickname(IPluginContext* pContext, const cell_t* params)
{
	DiscordInteraction* interaction = GetInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], interaction->GetUserNickname().c_str());
	return 1;
}

static DiscordAutocompleteInteraction* GetAutocompleteInteractionPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordAutocompleteInteraction* interaction;
	if ((err = handlesys->ReadHandle(handle, g_DiscordAutocompleteInteractionHandle, &sec, (void**)&interaction)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord autocomplete interaction handle %x (error %d)", handle, err);
		return nullptr;
	}

	return interaction;
}

// Autocomplete natives
static cell_t autocomplete_GetCommandName(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	const char* commandName = interaction->GetCommandName();
	pContext->StringToLocal(params[2], params[3], commandName);
	return 1;
}

static cell_t autocomplete_GetGuildId(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	std::string guildId = interaction->GetGuildId();
	pContext->StringToLocal(params[2], params[3], guildId.c_str());
	return 1;
}

static cell_t autocomplete_GetChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	std::string channelId = interaction->GetChannelId();
	pContext->StringToLocal(params[2], params[3], channelId.c_str());
	return 1;
}

static cell_t autocomplete_GetUser(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	DiscordUser* pDiscordUser = interaction->GetUser();

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete pDiscordUser;
		pContext->ReportError("Could not create user handle (error %d)", err);
		return BAD_HANDLE;
	}

	return handle;
}

static cell_t autocomplete_GetUserNickname(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], interaction->GetUserNickname().c_str());
	return 1;
}

static cell_t autocomplete_GetOptionValue(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	std::string value = interaction->GetOptionValue(name);
	pContext->StringToLocal(params[3], params[4], value.c_str());
	return 1;
}

static cell_t autocomplete_GetOptionValueInt(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	int64_t value = interaction->GetOptionValueInt(name);
	return value;
}

static cell_t autocomplete_GetOptionValueFloat(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	double value = interaction->GetOptionValueDouble(name);
	return sp_ftoc((float)value);
}

static cell_t autocomplete_GetOptionValueBool(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	bool value = interaction->GetOptionValueBool(name);
	return value;
}

static cell_t autocomplete_AddAutocompleteChoice(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	dpp::command_value value;
	dpp::command_option_type type = static_cast<dpp::command_option_type>(params[3]);
	switch(type) {
		case dpp::co_string:
		{
			char* str_value;
			pContext->LocalToString(params[4], &str_value);
			value = std::string(str_value);
			break;
		}
		case dpp::co_number:
			value = sp_ctof(params[4]);
			break;
		default:
			value = (int64_t)params[4];
			break;
	}

	interaction->m_response.add_autocomplete_choice(dpp::command_option_choice(name, value));
	return 1;
}

static cell_t autocomplete_AddAutocompleteChoiceString(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);

	char* str_value;
	pContext->LocalToString(params[4], &str_value);

	interaction->m_response.add_autocomplete_choice(dpp::command_option_choice(name, std::string(str_value)));
	return 1;
}

static cell_t autocomplete_CreateAutocompleteResponse(IPluginContext* pContext, const cell_t* params)
{
	DiscordAutocompleteInteraction* interaction = GetAutocompleteInteractionPointer(pContext, params[1]);
	if (!interaction) {
		return 0;
	}

	DiscordClient* discord = GetDiscordPointer(pContext, params[2]);
	if (!discord) {
		return 0;
	}

	discord->CreateAutocompleteResponse(interaction->m_command.id, interaction->m_command.token, interaction->m_response);
	return 1;
}

void DiscordClient::CreateAutocompleteResponse(dpp::snowflake id, const std::string &token, const dpp::interaction_response &response)
{
	m_cluster->interaction_response_create(id, token, response);
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

bool DiscordClient::RegisterSlashCommandWithOptions(dpp::snowflake guild_id, const char* name, const char* description, const char* default_permissions,
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

		if ((default_permissions != NULL) && (default_permissions[0] != '\0')) {
			command.set_default_permissions(std::stoull(default_permissions));
		}

		command.options = options;
		m_cluster->guild_command_create(command, guild_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to register slash command with options: %s", e.what());
		return false;
	}
}

bool DiscordClient::RegisterGlobalSlashCommandWithOptions(const char* name, const char* description, const char* default_permissions,
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

		if ((default_permissions != NULL) && (default_permissions[0] != '\0')) {
			command.set_default_permissions(std::stoull(default_permissions));
		}

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

	char* permissions;
	pContext->LocalToString(params[5], &permissions);

	cell_t* option_names_array;
	cell_t* option_descriptions_array;
	cell_t* option_types_array;
	cell_t* option_required_array;
	cell_t* option_autocomplete_array;

	pContext->LocalToPhysAddr(params[6], &option_names_array);
	pContext->LocalToPhysAddr(params[7], &option_descriptions_array);
	pContext->LocalToPhysAddr(params[8], &option_types_array);
	pContext->LocalToPhysAddr(params[9], &option_required_array);
	pContext->LocalToPhysAddr(params[10], &option_autocomplete_array);

	cell_t optionsSize = params[11];

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
		cmd_option.set_auto_complete(option_autocomplete_array[i]);

		options.push_back(cmd_option);
	}

	try {
		dpp::snowflake guild = std::stoull(guildId);
		return discord->RegisterSlashCommandWithOptions(guild, name, description, permissions, options) ? 1 : 0;
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

	char* permissions;
	pContext->LocalToString(params[4], &permissions);

	cell_t* option_names_array;
	cell_t* option_descriptions_array;
	cell_t* option_types_array;
	cell_t* option_required_array;
	cell_t* option_autocomplete_array;

	pContext->LocalToPhysAddr(params[5], &option_names_array);
	pContext->LocalToPhysAddr(params[6], &option_descriptions_array);
	pContext->LocalToPhysAddr(params[7], &option_types_array);
	pContext->LocalToPhysAddr(params[8], &option_required_array);
	pContext->LocalToPhysAddr(params[9], &option_autocomplete_array);

	cell_t optionsSize = params[10];

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
		cmd_option.set_auto_complete(option_autocomplete_array[i]);

		options.push_back(cmd_option);
	}

	return discord->RegisterGlobalSlashCommandWithOptions(name, description, permissions, options) ? 1 : 0;
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

bool DiscordClient::BulkDeleteGuildCommands(dpp::snowflake guild_id)
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->guild_bulk_command_delete(guild_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to bulk delete guild commands: %s", e.what());
		return false;
	}
}

bool DiscordClient::BulkDeleteGlobalCommands()
{
	if (!m_isRunning) {
		return false;
	}

	try {
		m_cluster->global_bulk_command_delete();
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to bulk delete global commands: %s", e.what());
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
		return discord->BulkDeleteGuildCommands(command) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid command ID format");
		return 0;
	}
}

static cell_t discord_BulkDeleteGuildCommands(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* guildId;
	pContext->LocalToString(params[2], &guildId);

	try {
		dpp::snowflake guild = std::stoull(guildId);
		return discord->BulkDeleteGuildCommands(guild) ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid guild ID format");
		return 0;
	}
}

static cell_t discord_BulkDeleteGlobalCommands(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	try {
		return discord->BulkDeleteGlobalCommands() ? 1 : 0;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Unable to bulk delete global commands: %s", e.what());
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
	{"Discord.CreateWebhook",      discord_CreateWebhook},
	{"Discord.GetChannelWebhooks",      discord_GetChannelWebhooks},
	{"Discord.ExecuteWebhook",      discord_ExecuteWebhook},
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
  	{"Discord.BulkDeleteGuildCommands", discord_BulkDeleteGuildCommands},
  	{"Discord.BulkDeleteGlobalCommands", discord_BulkDeleteGlobalCommands},

	// User
	{"DiscordUser.GetId",    user_GetId},
	{"DiscordUser.GetUsername",    user_GetUsername},
	{"DiscordUser.GetDiscriminator",    user_GetDiscriminator},
	{"DiscordUser.GetGlobalName",    user_GetGlobalName},
	{"DiscordUser.GetAvatarUrl",       user_GetAvatarUrl},
	{"DiscordUser.IsBot",    user_IsBot},

	// Message
	{"DiscordMessage.GetContent",    message_GetContent},
	{"DiscordMessage.GetMessageId",  message_GetMessageId},
	{"DiscordMessage.GetChannelId",  message_GetChannelId},
	{"DiscordMessage.GetGuildId",    message_GetGuildId},
	{"DiscordMessage.GetAuthor",       message_GetAuthor},
	{"DiscordMessage.GetAuthorId",   message_GetAuthorId},
	{"DiscordMessage.GetAuthorName", message_GetAuthorName},
	{"DiscordMessage.GetAuthorDisplayName", message_GetAuthorDisplayName},
	{"DiscordMessage.GetAuthorNickname", message_GetAuthorNickname},
	{"DiscordMessage.GetAuthorDiscriminator", message_GetAuthorDiscriminator},
	{"DiscordMessage.IsBot",         message_IsBot},

    // Channel
	{"DiscordChannel.GetName",       channel_GetName},

	// Webhook
	{"DiscordWebhook.DiscordWebhook",webhook_CreateWebhook},
	{"DiscordWebhook.GetId",       webhook_GetId},
	{"DiscordWebhook.GetUser",       webhook_GetUser},
	{"DiscordWebhook.GetName",       webhook_GetName},
	{"DiscordWebhook.SetName",       webhook_SetName},
	{"DiscordWebhook.GetAvatarUrl",       webhook_GetAvatarUrl},
	{"DiscordWebhook.SetAvatarUrl",       webhook_SetAvatarUrl},
	{"DiscordWebhook.GetAvatarData",       webhook_GetAvatarData},
	{"DiscordWebhook.SetAvatarData",       webhook_SetAvatarData},

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
	{"DiscordInteraction.GetUser",       interaction_GetUser},
	{"DiscordInteraction.GetUserNickname", interaction_GetUserNickname},
	{"DiscordInteraction.GetUserId", interaction_GetUserId},
	{"DiscordInteraction.GetUserName", interaction_GetUserName},

	// Autocomplete
	{"DiscordAutocompleteInteraction.GetCommandName", autocomplete_GetCommandName},
	{"DiscordAutocompleteInteraction.GetGuildId", autocomplete_GetGuildId},
	{"DiscordAutocompleteInteraction.GetChannelId", autocomplete_GetChannelId},
	{"DiscordAutocompleteInteraction.GetUser",       autocomplete_GetUser},
	{"DiscordAutocompleteInteraction.GetUserNickname", autocomplete_GetUserNickname},
	{"DiscordAutocompleteInteraction.GetOptionValue", autocomplete_GetOptionValue},
	{"DiscordAutocompleteInteraction.GetOptionValueInt", autocomplete_GetOptionValueInt},
	{"DiscordAutocompleteInteraction.GetOptionValueFloat", autocomplete_GetOptionValueFloat},
	{"DiscordAutocompleteInteraction.GetOptionValueBool", autocomplete_GetOptionValueBool},
	{"DiscordAutocompleteInteraction.CreateAutocompleteResponse", autocomplete_CreateAutocompleteResponse},
	{"DiscordAutocompleteInteraction.AddAutocompleteChoice", autocomplete_AddAutocompleteChoice},
	{"DiscordAutocompleteInteraction.AddAutocompleteChoiceString", autocomplete_AddAutocompleteChoiceString},
	{nullptr, nullptr}
};