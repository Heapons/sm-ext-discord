#include "extension.h"

bool DiscordUser::HasPermission(const char* permission) const {
	if (!m_has_member || !permission) return false;
	
	try {
		uint64_t perm = std::stoull(permission);
		dpp::guild* g = dpp::find_guild(m_member.guild_id);
		if (!g) return false;
		
		uint64_t base_perms = g->base_permissions(&m_user);
		dpp::permission perms(base_perms);
		return perms.has(perm);
	}
	catch (const std::exception& e) {
		return false;
	}
}

uint64_t DiscordUser::GetPermissions() const {
	if (!m_has_member) return 0;
	
	dpp::guild* g = dpp::find_guild(m_member.guild_id);
	if (!g) return 0;
	
	uint64_t base_perms = g->base_permissions(&m_user);
	return base_perms;
}

bool DiscordUser::HasPermissionInChannel(dpp::snowflake channel_id, const char* permission) const {
	if (!m_has_member || !permission) return false;
	
	try {
		uint64_t perm = std::stoull(permission);
		dpp::guild* g = dpp::find_guild(m_member.guild_id);
		dpp::channel* c = dpp::find_channel(channel_id);
		if (!g || !c) return false;
		
		dpp::permission perms = g->permission_overwrites(m_member, *c);
		return perms.has(perm);
	}
	catch (const std::exception& e) {
		return false;
	}
}

uint64_t DiscordUser::GetPermissionsInChannel(dpp::snowflake channel_id) const {
	if (!m_has_member) return 0;
	
	dpp::guild* g = dpp::find_guild(m_member.guild_id);
	dpp::channel* c = dpp::find_channel(channel_id);
	if (!g || !c) return 0;
	
	dpp::permission perms = g->permission_overwrites(m_member, *c);
	return static_cast<uint64_t>(perms);
}

std::vector<dpp::snowflake> DiscordUser::GetRoles() const {
	if (!m_has_member) return std::vector<dpp::snowflake>();
	return m_member.get_roles();
}

bool DiscordUser::HasRole(dpp::snowflake role_id) const {
	if (!m_has_member) return false;
	
	std::vector<dpp::snowflake> roles = m_member.get_roles();
	for (const auto& role : roles) {
		if (role == role_id) return true;
	}
	return false;
}

bool DiscordUser::HasAnyRole(const std::vector<dpp::snowflake>& role_ids) const {
	if (!m_has_member) return false;
	
	std::vector<dpp::snowflake> user_roles = m_member.get_roles();
	for (const auto& target_role : role_ids) {
		for (const auto& user_role : user_roles) {
			if (user_role == target_role) return true;
		}
	}
	return false;
}

bool DiscordUser::HasAllRoles(const std::vector<dpp::snowflake>& role_ids) const {
	if (!m_has_member) return false;
	
	std::vector<dpp::snowflake> user_roles = m_member.get_roles();
	for (const auto& target_role : role_ids) {
		bool found = false;
		for (const auto& user_role : user_roles) {
			if (user_role == target_role) {
				found = true;
				break;
			}
		}
		if (!found) return false;
	}
	return true;
}

dpp::snowflake DiscordUser::GetHighestRole() const {
	if (!m_has_member) return 0;
	
	dpp::guild* g = dpp::find_guild(m_member.guild_id);
	if (!g) return 0;
	
	std::vector<dpp::snowflake> user_roles = m_member.get_roles();
	if (user_roles.empty()) return 0;
	
	dpp::snowflake highest_role = 0;
	int highest_position = -1;
	
	for (const auto& role_id : user_roles) {
		dpp::role* role = dpp::find_role(role_id);
		if (role && role->position > highest_position) {
			highest_position = role->position;
			highest_role = role_id;
		}
	}
	
	return highest_role;
}

std::string DiscordUser::GetRoleName(dpp::snowflake role_id) const {
	if (!m_has_member) return "";
	
	dpp::role* role = dpp::find_role(role_id);
	if (!role) return "";
	
	return role->name;
}

std::vector<std::string> DiscordUser::GetRoleNames() const {
	std::vector<std::string> role_names;
	if (!m_has_member) return role_names;
	
	std::vector<dpp::snowflake> user_roles = m_member.get_roles();
	for (const auto& role_id : user_roles) {
		dpp::role* role = dpp::find_role(role_id);
		if (role) {
			role_names.push_back(role->name);
		}
	}
	
	return role_names;
}

bool DiscordSlashCommand::Update(dpp::snowflake guild_id) {
	if (!m_client) return false;
	
	try {
		// Use provided guild_id or stored m_guild_id
		dpp::snowflake target_guild = (guild_id != 0) ? guild_id : m_guild_id;
		
		if (target_guild != 0) {
			// Update guild command
			m_client->GetCluster()->guild_command_edit(m_command, target_guild);
		} else {
			// Update global command
			m_client->GetCluster()->global_command_edit(m_command);
		}
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordSlashCommand::Delete(dpp::snowflake guild_id) {
	if (!m_client) return false;
	
	try {
		dpp::snowflake target_guild = (guild_id != 0) ? guild_id : m_guild_id;
		
		if (target_guild != 0) {
			return m_client->DeleteGuildCommand(target_guild, m_command.id);
		} else {
			return m_client->DeleteGlobalCommand(m_command.id);
		}
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordSlashCommand::ApplyPermissionOverrides(dpp::snowflake guild_id) {
	if (!m_client) return false;
	
	try {
		dpp::snowflake target_guild = (guild_id != 0) ? guild_id : m_guild_id;
		if (target_guild == 0) return false;
		
		m_command.permissions = m_permissions;
		
		m_client->GetCluster()->guild_command_edit_permissions(m_command, target_guild);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to apply command permission overrides: %s", e.what());
		return false;
	}
}

bool DiscordWebhook::Modify() {
	if (!m_client) return false;
	return m_client->ModifyWebhook(m_webhook.id, m_webhook.name, m_webhook.avatar_url);
}

bool DiscordWebhook::Delete() {
	if (!m_client) return false;
	return m_client->DeleteWebhook(m_webhook.id);
}

bool DiscordWebhook::Execute(const char* message, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles) {
	if (!m_client || !message) return false;
	return m_client->ExecuteWebhook(m_webhook, message, allowed_mentions_mask, users, roles);
}

bool DiscordWebhook::ExecuteEmbed(const char* message, const DiscordEmbed* embed, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles) {
	if (!m_client || !embed) return false;
	return m_client->ExecuteWebhookEmbed(m_webhook, message ? message : "", embed, allowed_mentions_mask, users, roles);
}

bool DiscordWebhook::CreateWebhook(DiscordClient* client, dpp::webhook wh, IForward *callback_forward, cell_t data)
{
	if (!client || !client->IsRunning()) {
		return false;
	}

	try {
		client->GetCluster()->create_webhook(wh, [client, forward = callback_forward, value = data](const dpp::confirmation_callback_t& callback)
		{
			if (callback.is_error())
			{
				smutils->LogError(myself, "Failed to create webhook: %s", callback.get_error().message.c_str());
				forwards->ReleaseForward(forward);
				return;
			}
			auto webhook = callback.get<dpp::webhook>();

			g_TaskQueue.Push([client, &forward, webhook = new DiscordWebhook(webhook, client), value = value]() {
				if (forward && forward->GetFunctionCount() == 0)
				{
					delete webhook;
					forwards->ReleaseForward(forward);
					return;
				}

				HandleError err;
				HandleSecurity sec(myself->GetIdentity(), myself->GetIdentity());
				Handle_t webhookHandle = handlesys->CreateHandleEx(g_DiscordWebhookHandle, webhook, &sec, nullptr, &err);
				if (webhookHandle == BAD_HANDLE)
				{
					smutils->LogError(myself, "Could not create webhook handle (error %d)", err);
					delete webhook;
					forwards->ReleaseForward(forward);
					return;
				}

				forward->PushCell(client->GetHandle());
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

bool DiscordWebhook::GetWebhook(DiscordClient* client, dpp::snowflake webhook_id, IForward* callback_forward, cell_t data) {
	if (!client || !client->IsRunning()) return false;
	
	try {
		client->GetCluster()->get_webhook(webhook_id, [client, forward = callback_forward, value = data](const dpp::confirmation_callback_t& callback) {
			if (callback.is_error()) {
				smutils->LogError(myself, "Failed to get webhook: %s", callback.get_error().message.c_str());
				forwards->ReleaseForward(forward);
				return;
			}
			auto webhook = callback.get<dpp::webhook>();

			g_TaskQueue.Push([client, &forward, webhook = new DiscordWebhook(webhook, client), value = value]() {
				if (forward && forward->GetFunctionCount() == 0) {
					delete webhook;
					forwards->ReleaseForward(forward);
					return;
				}
				
				HandleError err;
				HandleSecurity sec(myself->GetIdentity(), myself->GetIdentity());
				Handle_t webhookHandle = handlesys->CreateHandleEx(g_DiscordWebhookHandle, webhook, &sec, nullptr, &err);
				if (webhookHandle == BAD_HANDLE) {
					smutils->LogError(myself, "Could not create webhook handle (error %d)", err);
					delete webhook;
					forwards->ReleaseForward(forward);
					return;
				}
				
				forward->PushCell(client->GetHandle());
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
		smutils->LogError(myself, "Failed to get webhook: %s", e.what());
		return false;
	}
}

bool DiscordWebhook::GetChannelWebhooks(DiscordClient* client, dpp::snowflake channel_id, IForward *callback_forward, cell_t data)
{
	if (!client || !client->IsRunning()) {
		return false;
	}

	try {
		client->GetCluster()->get_channel_webhooks(channel_id, [client, forward = callback_forward, value = data](const dpp::confirmation_callback_t& callback)
		{
			if (callback.is_error())
			{
				smutils->LogError(myself, "Failed to get channel webhooks: %s", callback.get_error().message.c_str());
				forwards->ReleaseForward(forward);
				return;
			}
			auto webhook_map = callback.get<dpp::webhook_map>();

			g_TaskQueue.Push([client, &forward, webhooks = webhook_map, value = value]() {
				if (forward && forward->GetFunctionCount() == 0)
				{
					forwards->ReleaseForward(forward);
					return;
				}

				size_t webhook_count = webhooks.size();
				std::unique_ptr<cell_t[]> handles = std::make_unique<cell_t[]>(webhook_count);

				HandleError err;
				HandleSecurity sec(myself->GetIdentity(), myself->GetIdentity());
				size_t i = 0;
				for (auto pair : webhooks)
				{
					DiscordWebhook* wbk = new DiscordWebhook(pair.second, client);
					Handle_t webhookHandle = handlesys->CreateHandleEx(g_DiscordWebhookHandle, wbk, &sec, nullptr, &err);
					if (webhookHandle == BAD_HANDLE)
					{
						smutils->LogError(myself, "Could not create webhook handle (error %d)", err);
						delete wbk;
						continue;
					}
					handles[i++] = webhookHandle;
				}

				forward->PushCell(client->GetHandle());
				forward->PushArray(handles.get(), static_cast<unsigned int>(webhook_count));
				forward->PushCell(static_cast<unsigned int>(webhook_count));
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

bool DiscordUser::SetNickName(const char* nickname) {
	if (!m_client || !m_has_member) return false;
	
	try {
		dpp::guild_member updated_member = m_member;
		updated_member.set_nickname(nickname ? nickname : "");
		m_client->GetCluster()->guild_edit_member(updated_member);
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordUser::AddRole(dpp::snowflake role_id) {
	if (!m_client || !m_has_member) return false;
	
	try {
		m_client->GetCluster()->guild_member_add_role(m_member.guild_id, m_user.id, role_id);
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordUser::RemoveRole(dpp::snowflake role_id) {
	if (!m_client || !m_has_member) return false;
	
	try {
		m_client->GetCluster()->guild_member_remove_role(m_member.guild_id, m_user.id, role_id);
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordUser::KickFromGuild() {
	if (!m_client || !m_has_member) return false;
	
	try {
		m_client->GetCluster()->guild_member_kick(m_member.guild_id, m_user.id);
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordUser::BanFromGuild(const char* reason, int delete_message_days) {
	if (!m_client || !m_has_member) return false;
	
	try {
		m_client->GetCluster()->guild_ban_add(m_member.guild_id, m_user.id, delete_message_days);
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordUser::UnbanFromGuild() {
	if (!m_client || !m_has_member) return false;
	
	try {
		m_client->GetCluster()->guild_ban_delete(m_member.guild_id, m_user.id);
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordUser::SetTimeout(time_t timeout_until) {
	if (!m_client || !m_has_member) return false;
	
	try {
		dpp::guild_member updated_member = m_member;
		updated_member.communication_disabled_until = timeout_until;
		m_client->GetCluster()->guild_edit_member(updated_member);
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordUser::RemoveTimeout() {
	if (!m_client || !m_has_member) return false;
	
	try {
		dpp::guild_member updated_member = m_member;
		updated_member.communication_disabled_until = 0;
		m_client->GetCluster()->guild_edit_member(updated_member);
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordMessage::Edit(const char* new_content) {
	if (!m_client || !new_content) return false;
	return m_client->EditMessage(m_message.channel_id, m_message.id, new_content);
}

bool DiscordMessage::EditEmbed(const char* new_content, const DiscordEmbed* embed) {
	if (!m_client || !embed) return false;
	return m_client->EditMessageEmbed(m_message.channel_id, m_message.id, new_content ? new_content : "", embed);
}

bool DiscordMessage::Delete() {
	if (!m_client) return false;
	return m_client->DeleteMessage(m_message.channel_id, m_message.id);
}

bool DiscordMessage::Pin() {
	if (!m_client) return false;
	return m_client->PinMessage(m_message.channel_id, m_message.id);
}

bool DiscordMessage::Unpin() {
	if (!m_client) return false;
	return m_client->UnpinMessage(m_message.channel_id, m_message.id);
}

bool DiscordMessage::AddReaction(const char* emoji) {
	if (!m_client || !emoji) return false;
	return m_client->AddReaction(m_message.channel_id, m_message.id, emoji);
}

bool DiscordMessage::RemoveReaction(const char* emoji) {
	if (!m_client || !emoji) return false;
	return m_client->RemoveReaction(m_message.channel_id, m_message.id, emoji);
}

bool DiscordMessage::RemoveAllReactions() {
	if (!m_client) return false;
	return m_client->RemoveAllReactions(m_message.channel_id, m_message.id);
}

bool DiscordMessage::Reply(const char* content) {
	if (!m_client || !content) return false;
	
	try {
		dpp::message reply_msg;
		reply_msg.set_content(content);
		reply_msg.set_channel_id(m_message.channel_id);
		reply_msg.message_reference.message_id = m_message.id;
		reply_msg.message_reference.channel_id = m_message.channel_id;
		reply_msg.message_reference.guild_id = m_message.guild_id;
		m_client->GetCluster()->message_create(reply_msg);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to reply to message: %s", e.what());
		return false;
	}
}

bool DiscordMessage::ReplyEmbed(const char* content, const DiscordEmbed* embed) {
	if (!m_client || !embed) return false;
	
	try {
		dpp::message reply_msg;
		if (content && strlen(content) > 0) {
			reply_msg.set_content(content);
		}
		reply_msg.set_channel_id(m_message.channel_id);
		reply_msg.message_reference.message_id = m_message.id;
		reply_msg.message_reference.channel_id = m_message.channel_id;
		reply_msg.message_reference.guild_id = m_message.guild_id;
		reply_msg.add_embed(embed->GetEmbed());
		m_client->GetCluster()->message_create(reply_msg);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to reply with embed to message: %s", e.what());
		return false;
	}
}

bool DiscordMessage::Crosspost() {
	if (!m_client) return false;
	
	try {
		m_client->GetCluster()->message_crosspost(m_message.id, m_message.channel_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to crosspost message: %s", e.what());
		return false;
	}
}

bool DiscordMessage::CreateThread(const char* name, int auto_archive_duration) {
	if (!m_client || !name) return false;
	
	try {
		m_client->GetCluster()->thread_create_with_message(
			name,
			m_message.channel_id,
			m_message.id,
			auto_archive_duration,
			0
		);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to create thread from message: %s", e.what());
		return false;
	}
}

bool DiscordChannel::HasUserPermission(const dpp::user& user, const char* permission) const {
	if (!permission) return false;
	
	try {
		uint64_t perm = std::stoull(permission);
		dpp::guild* g = dpp::find_guild(m_channel.guild_id);
		if (!g) return false;
		
		uint64_t base_perms = g->base_permissions(&user);
		dpp::permission perms = g->permission_overwrites(base_perms, &user, &m_channel);
		return perms.has(perm);
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordChannel::HasMemberPermission(const dpp::guild_member& member, const char* permission) const {
	if (!permission) return false;
	
	try {
		uint64_t perm = std::stoull(permission);
		dpp::guild* g = dpp::find_guild(m_channel.guild_id);
		if (!g) return false;
		
		dpp::permission perms = g->permission_overwrites(member, m_channel);
		return perms.has(perm);
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool DiscordChannel::SetName(const char* name) {
	if (!m_client || !name) return false;
	return m_client->ModifyChannel(m_channel.id, std::string(name));
}

bool DiscordChannel::SetTopic(const char* topic) {
	if (!m_client) return false;
	return m_client->ModifyChannel(m_channel.id, "", topic ? std::string(topic) : "");
}

bool DiscordChannel::SetPosition(uint16_t position) {
	if (!m_client) return false;
	return m_client->ModifyChannel(m_channel.id, "", "", position);
}

bool DiscordChannel::SetNSFW(bool nsfw) {
	if (!m_client) return false;
	return m_client->ModifyChannel(m_channel.id, "", "", 0, nsfw);
}

bool DiscordChannel::SetRateLimitPerUser(uint16_t seconds) {
	if (!m_client) return false;
	return m_client->ModifyChannel(m_channel.id, "", "", 0, false, seconds);
}

bool DiscordChannel::SetBitrate(uint16_t bitrate) {
	if (!m_client) return false;
	return m_client->ModifyChannel(m_channel.id, "", "", 0, false, 0, bitrate);
}

bool DiscordChannel::SetUserLimit(uint8_t limit) {
	if (!m_client) return false;
	return m_client->ModifyChannel(m_channel.id, "", "", 0, false, 0, 0, limit);
}

bool DiscordChannel::Delete() {
	if (!m_client) return false;
	return m_client->DeleteChannel(m_channel.id);
}

bool DiscordChannel::SetParent(dpp::snowflake parent_id) {
	if (!m_client) return false;
	return m_client->ModifyChannel(m_channel.id, "", "", 0, false, 0, 0, 0, parent_id);
}

bool DiscordChannel::AddPermissionOverwrite(dpp::snowflake target_id, uint8_t type, uint64_t allowed, uint64_t denied) {
	if (!m_client) return false;
	
	try {
		dpp::permission_overwrite overwrite;
		overwrite.id = target_id;
		overwrite.type = static_cast<dpp::overwrite_type>(type);
		overwrite.allow = allowed;
		overwrite.deny = denied;
		
		m_channel.permission_overwrites.push_back(overwrite);
		
		m_client->GetCluster()->channel_edit(m_channel);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to add permission overwrite: %s", e.what());
		return false;
	}
}

bool DiscordChannel::SetPermissionOverwrite(dpp::snowflake target_id, uint8_t type, uint64_t allowed, uint64_t denied) {
	if (!m_client) return false;
	
	try {
		dpp::permission_overwrite overwrite;
		overwrite.id = target_id;
		overwrite.type = static_cast<dpp::overwrite_type>(type);
		overwrite.allow = allowed;
		overwrite.deny = denied;
		
		auto& overwrites = m_channel.permission_overwrites;
		bool found = false;
		for (auto& existing : overwrites) {
			if (existing.id == target_id && static_cast<uint8_t>(existing.type) == type) {
				existing.allow = allowed;
				existing.deny = denied;
				found = true;
				break;
			}
		}
		
		if (!found) {
			overwrites.push_back(overwrite);
		}
		
		m_client->GetCluster()->channel_edit(m_channel);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to set permission overwrite: %s", e.what());
		return false;
	}
}

bool DiscordChannel::RemovePermissionOverwrite(dpp::snowflake target_id, uint8_t type) {
	if (!m_client) return false;
	
	try {
		auto& overwrites = m_channel.permission_overwrites;
		for (auto it = overwrites.begin(); it != overwrites.end(); ++it) {
			if (it->id == target_id && static_cast<uint8_t>(it->type) == type) {
				overwrites.erase(it);
				break;
			}
		}
		
		m_client->GetCluster()->channel_edit(m_channel);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to remove permission overwrite: %s", e.what());
		return false;
	}
}

std::string DiscordChannel::GetUserPermissions(dpp::snowflake user_id) const {
	try {
		dpp::guild* g = dpp::find_guild(m_channel.guild_id);
		dpp::user* u = dpp::find_user(user_id);
		if (!g || !u) return "0";
		
		uint64_t base_perms = g->base_permissions(u);
		dpp::permission perms = g->permission_overwrites(base_perms, u, &m_channel);
		
		return std::to_string(static_cast<uint64_t>(perms));
	}
	catch (const std::exception& e) {
		return "0";
	}
}

bool DiscordChannel::CreateInvite(int max_age, int max_uses, bool temporary, bool unique) {
	if (!m_client) return false;
	
	try {
		dpp::invite invite;
		invite.max_age = max_age;
		invite.max_uses = max_uses;
		invite.temporary = temporary;
		invite.unique = unique;
		
		m_client->GetCluster()->channel_invite_create(m_channel, invite);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to create invite: %s", e.what());
		return false;
	}
}

bool DiscordChannel::SendMessage(const char* content) {
	if (!m_client || !content) return false;
	
	try {
		dpp::message msg(m_channel.id, content);
		m_client->GetCluster()->message_create(msg);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to send message: %s", e.what());
		return false;
	}
}

bool DiscordChannel::SendMessageEmbed(const char* content, const DiscordEmbed* embed) {
	if (!m_client || !embed) return false;
	
	try {
		dpp::message msg(m_channel.id, content ? content : "");
		msg.add_embed(embed->GetEmbed());
		m_client->GetCluster()->message_create(msg);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to send message with embed: %s", e.what());
		return false;
	}
}

bool DiscordChannel::SetRTCRegion(const char* region) {
	if (!m_client) return false;
	
	try {
		dpp::channel ch = m_channel;
		if (region && strlen(region) > 0) {
			ch.rtc_region = region;
		} else {
			ch.rtc_region = "";
		}
		
		m_client->GetCluster()->channel_edit(ch);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to set RTC region: %s", e.what());
		return false;
	}
}

bool DiscordChannel::CreateForumTag(const char* name, const char* emoji, bool moderated) {
	if (!m_client || !name) return false;
	
	try {
		dpp::forum_tag tag;
		tag.set_name(name);
		tag.moderated = moderated;
		
		if (emoji && strlen(emoji) > 0) {
			// Check if emoji is a snowflake ID (numeric) or unicode
			if (std::all_of(emoji, emoji + strlen(emoji), ::isdigit)) {
				tag.emoji = dpp::snowflake(emoji);
			} else {
				tag.emoji = std::string(emoji);
			}
		}
		
		m_channel.available_tags.push_back(tag);
		
		m_client->GetCluster()->channel_edit(m_channel);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to create forum tag: %s", e.what());
		return false;
	}
}

bool DiscordChannel::EditForumTag(dpp::snowflake tag_id, const char* name, const char* emoji, bool moderated) {
	if (!m_client || !name) return false;
	
	try {
		for (auto& tag : m_channel.available_tags) {
			if (tag.id == tag_id) {
				tag.set_name(name);
				tag.moderated = moderated;
				
				if (emoji && strlen(emoji) > 0) {
					if (std::all_of(emoji, emoji + strlen(emoji), ::isdigit)) {
						tag.emoji = dpp::snowflake(emoji);
					} else {
						tag.emoji = std::string(emoji);
					}
				} else {
					tag.emoji = std::monostate{};
				}
				
				m_client->GetCluster()->channel_edit(m_channel);
				return true;
			}
		}
		
		smutils->LogError(myself, "Forum tag with ID %llu not found", tag_id);
		return false;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to edit forum tag: %s", e.what());
		return false;
	}
}

bool DiscordChannel::DeleteForumTag(dpp::snowflake tag_id) {
	if (!m_client) return false;
	
	try {
		auto& tags = m_channel.available_tags;
		for (auto it = tags.begin(); it != tags.end(); ++it) {
			if (it->id == tag_id) {
				tags.erase(it);
				
				m_client->GetCluster()->channel_edit(m_channel);
				return true;
			}
		}
		
		smutils->LogError(myself, "Forum tag with ID %llu not found", tag_id);
		return false;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to delete forum tag: %s", e.what());
		return false;
	}
}

bool DiscordChannel::CreateForumThread(const char* name, const char* message, const std::vector<dpp::snowflake>& tag_ids, int auto_archive, int rate_limit) {
	if (!m_client || !name || !message) return false;
	
	try {
		dpp::message starter_message(m_channel.id, message);
		
		dpp::auto_archive_duration_t archive_duration;
		switch (auto_archive) {
			case 60: archive_duration = dpp::arc_1_hour; break;
			case 1440: archive_duration = dpp::arc_1_day; break;
			case 4320: archive_duration = dpp::arc_3_days; break;
			case 10080: archive_duration = dpp::arc_1_week; break;
			default: archive_duration = dpp::arc_1_day; break;
		}
		
		m_client->GetCluster()->thread_create_in_forum(name, m_channel.id, starter_message, archive_duration, rate_limit, tag_ids, [this](const dpp::confirmation_callback_t& callback) {
			if (callback.is_error()) {
				smutils->LogError(myself, "Failed to create forum thread: %s", callback.get_error().message.c_str());
			}
		});
		
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to create forum thread: %s", e.what());
		return false;
	}
}

bool DiscordChannel::CreateForumThreadEmbed(const char* name, const char* message, const DiscordEmbed* embed, const std::vector<dpp::snowflake>& tag_ids, int auto_archive, int rate_limit) {
	if (!m_client || !name || !message || !embed) return false;
	
	try {
		dpp::message starter_message(m_channel.id, message);
		starter_message.add_embed(embed->GetEmbed());
		
		dpp::auto_archive_duration_t archive_duration;
		switch (auto_archive) {
			case 60: archive_duration = dpp::arc_1_hour; break;
			case 1440: archive_duration = dpp::arc_1_day; break;
			case 4320: archive_duration = dpp::arc_3_days; break;
			case 10080: archive_duration = dpp::arc_1_week; break;
			default: archive_duration = dpp::arc_1_day; break;
		}
		
		m_client->GetCluster()->thread_create_in_forum(name, m_channel.id, starter_message, archive_duration, rate_limit, tag_ids, [this](const dpp::confirmation_callback_t& callback) {
			if (callback.is_error()) {
				smutils->LogError(myself, "Failed to create forum thread with embed: %s", callback.get_error().message.c_str());
			}
		});
		
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to create forum thread with embed: %s", e.what());
		return false;
	}
}

DiscordClient::DiscordClient(const char* token, uint32_t intents) : m_discord_handle(0)
{
	m_cluster = std::make_unique<dpp::cluster>(token, intents);
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
		m_cluster->start();
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to run Discord bot: %s", e.what());
	}
}

void DiscordClient::Start()
{
	if (!m_cluster || IsRunning()) {
		return;
	}
	m_thread = std::make_unique<std::thread>(&DiscordClient::RunBot, this);
}

void DiscordClient::Stop()
{
	if (!m_cluster || !IsRunning()) {
		return;
	}

	try {
		if (m_cluster) {
			m_cluster->shutdown();
		}

		if (m_thread && m_thread->joinable()) {
			m_thread->join();
		}

		m_thread.reset();

		m_cluster.reset();
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Error during Discord bot shutdown: %s", e.what());
	}
}

bool DiscordClient::SetPresence(dpp::presence presence)
{
	if (!IsRunning()) {
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
	if (!IsRunning()) {
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

bool DiscordClient::ExecuteWebhookEmbed(dpp::webhook wh, const char* message, const DiscordEmbed* embed, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles)
{
	if (!IsRunning()) {
		return false;
	}

	dpp::message message_obj(message ? message : "");
	AddAllowedMentionsToMessage(&message_obj, allowed_mentions_mask, users, roles);

	try {
		message_obj.add_embed(embed->GetEmbed());
		m_cluster->execute_webhook(wh, message_obj);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to execute webhook with embed: %s", e.what());
		return false;
	}
}

bool DiscordClient::SendMessage(dpp::snowflake channel_id, const char* message, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles)
{
	if (!IsRunning()) {
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
	if (!IsRunning()) {
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
				DiscordMessage* message = new DiscordMessage(msg, this);
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
				} else {
					delete message;
				}
			}
			});
		});

	m_cluster->on_log([this](const dpp::log_t& event) {
		//std::cout << "[Discord]: " << event.message << std::endl;
		if (event.severity >= dpp::ll_error) {
			g_TaskQueue.Push([this, message = event.message]() {
			if (g_pForwardError && g_pForwardError->GetFunctionCount()) {
				g_pForwardError->PushCell(m_discord_handle);
				g_pForwardError->PushString(message.c_str());
				g_pForwardError->Execute(nullptr);
			}
			});
		}});

	m_cluster->on_slashcommand([this](const dpp::slashcommand_t& event) {
		g_TaskQueue.Push([this, event]() {
			if (g_pForwardSlashCommand && g_pForwardSlashCommand->GetFunctionCount()) {
				DiscordInteraction* interaction = new DiscordInteraction(event, this);

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
				} else {
					delete interaction;
				}
			}
			});
		});

	m_cluster->on_autocomplete([this](const dpp::autocomplete_t& event) {
		g_TaskQueue.Push([this, event]() {
			if (g_pForwardAutocomplete && g_pForwardAutocomplete->GetFunctionCount()) {
				DiscordAutocompleteInteraction* interaction = new DiscordAutocompleteInteraction(event, this);

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
						g_pForwardAutocomplete->PushCell(opt.focused);
						g_pForwardAutocomplete->PushCell(type);
						g_pForwardAutocomplete->PushString(opt.name.c_str());
						g_pForwardAutocomplete->Execute(nullptr);
					}

					handlesys->FreeHandle(interactionHandle, &sec);
				} else {
					delete interaction;
				}
			}
		});
	});
}

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

	DiscordClient* pDiscordClient = new DiscordClient(token, static_cast<uint32_t>(params[2]));

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
		pContext->ReportError("Could not create Discord handle (error %d)", err);
		delete pDiscordClient;
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
		if (!forward || !forward->AddFunction(callback))
		{
			return pContext->ThrowNativeError("Could not create forward.");
		}

		cell_t data = params[4];
		return DiscordWebhook::GetChannelWebhooks(discord, channelFlake, forward, data);
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
		if (!forward || !forward->AddFunction(callback))
		{
			return pContext->ThrowNativeError("Could not create forward.");
		}

		cell_t data = params[4];
		return DiscordWebhook::CreateWebhook(discord, webhook, forward, data);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t discord_GetWebhook(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* webhookId;
	pContext->LocalToString(params[2], &webhookId);

	try {
		dpp::snowflake webhookFlake = std::stoull(webhookId);

		IPluginFunction *callback = pContext->GetFunctionById(params[3]);

		IChangeableForward *forward = forwards->CreateForwardEx(nullptr, ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Any);
		if (!forward || !forward->AddFunction(callback))
		{
			return pContext->ThrowNativeError("Could not create forward.");
		}

		cell_t data = params[4];
		return DiscordWebhook::GetWebhook(discord, webhookFlake, forward, data);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid webhook ID format: %s", webhookId);
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

	for (size_t i = 0; i < users.size(); i++) {
		char* str;

		pContext->LocalToString(users_array[i], &str);
		try {
			users[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue;
		}
	}

	for (size_t i = 0; i < roles.size(); i++) {
		char* str;

		pContext->LocalToString(roles_array[i], &str);
		try {
			roles[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue;
		}
	}

	try {
		dpp::snowflake channel = std::stoull(channelId);
		return discord->SendMessage(channel, message, params[4], users, roles);
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

	for (size_t i = 0; i < users.size(); i++) {
		char* str;

		pContext->LocalToString(users_array[i], &str);
		try {
			users[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue;
		}
	}

	for (size_t i = 0; i < roles.size(); i++) {
		char* str;

		pContext->LocalToString(roles_array[i], &str);
		try {
			roles[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue;
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
		return discord->SendMessageEmbed(channel, message, embed, params[5], users, roles);
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

	return discord->IsRunning();
}

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

	char* url;
	pContext->LocalToString(params[3], &url);

	char* icon_url;
	pContext->LocalToString(params[4], &icon_url);

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

	char* icon_url;
	pContext->LocalToString(params[3], &icon_url);

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

static cell_t embed_GetTitle(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string title = embed->GetTitle();
	pContext->StringToLocal(params[2], params[3], title.c_str());
	return 1;
}

static cell_t embed_GetDescription(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string description = embed->GetDescription();
	pContext->StringToLocal(params[2], params[3], description.c_str());
	return 1;
}

static cell_t embed_GetColor(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	return static_cast<cell_t>(embed->GetColor());
}

static cell_t embed_GetUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string url = embed->GetUrl();
	pContext->StringToLocal(params[2], params[3], url.c_str());
	return 1;
}

static cell_t embed_GetAuthorName(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string author_name = embed->GetAuthorName();
	pContext->StringToLocal(params[2], params[3], author_name.c_str());
	return 1;
}

static cell_t embed_GetAuthorUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string author_url = embed->GetAuthorUrl();
	pContext->StringToLocal(params[2], params[3], author_url.c_str());
	return 1;
}

static cell_t embed_GetAuthorIconUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string author_icon_url = embed->GetAuthorIconUrl();
	pContext->StringToLocal(params[2], params[3], author_icon_url.c_str());
	return 1;
}

static cell_t embed_GetFooterText(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string footer_text = embed->GetFooterText();
	pContext->StringToLocal(params[2], params[3], footer_text.c_str());
	return 1;
}

static cell_t embed_GetFooterIconUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string footer_icon_url = embed->GetFooterIconUrl();
	pContext->StringToLocal(params[2], params[3], footer_icon_url.c_str());
	return 1;
}

static cell_t embed_GetThumbnailUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string thumbnail_url = embed->GetThumbnailUrl();
	pContext->StringToLocal(params[2], params[3], thumbnail_url.c_str());
	return 1;
}

static cell_t embed_GetImageUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	std::string image_url = embed->GetImageUrl();
	pContext->StringToLocal(params[2], params[3], image_url.c_str());
	return 1;
}

static cell_t embed_GetFieldCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	return static_cast<cell_t>(embed->GetFieldCount());
}

static cell_t embed_GetFieldName(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	size_t index = static_cast<size_t>(params[2]);
	std::string field_name = embed->GetFieldName(index);
	pContext->StringToLocal(params[3], params[4], field_name.c_str());
	return field_name.empty();
}

static cell_t embed_GetFieldValue(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	size_t index = static_cast<size_t>(params[2]);
	std::string field_value = embed->GetFieldValue(index);
	pContext->StringToLocal(params[3], params[4], field_value.c_str());
	return field_value.empty();
}

static cell_t embed_GetFieldInline(IPluginContext* pContext, const cell_t* params)
{
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[1]);
	if (!embed) {
		return 0;
	}

	size_t index = static_cast<size_t>(params[2]);
	return embed->GetFieldInline(index);
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

static DiscordSlashCommand* GetSlashCommandPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordSlashCommand* command;
	if ((err = handlesys->ReadHandle(handle, g_DiscordSlashCommandHandle, &sec, (void**)&command)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord slash command handle %x (error %d)", handle, err);
		return nullptr;
	}

	return command;
}

static cell_t user_CreateFromId(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return BAD_HANDLE;
	}

	char* userId;
	pContext->LocalToString(params[2], &userId);

	char* guildId = nullptr;
	if (params[0] >= 3 && params[3] != 0) {
		pContext->LocalToString(params[3], &guildId);
	}

	try {
		dpp::snowflake userFlake = std::stoull(userId);
		dpp::snowflake guildFlake = 0;
		if (guildId && strlen(guildId) > 0) {
			guildFlake = std::stoull(guildId);
		}

		dpp::user* user_ptr = dpp::find_user(userFlake);
		
		if (!user_ptr) {
			dpp::user user_obj;
			user_obj.id = userFlake;

			DiscordUser* pDiscordUser;
			if (guildFlake != 0) {
				dpp::guild* guild_ptr = dpp::find_guild(guildFlake);
				if (guild_ptr) {
					auto member_it = guild_ptr->members.find(userFlake);
					if (member_it != guild_ptr->members.end()) {
						pDiscordUser = new DiscordUser(user_obj, member_it->second, discord);
					} else {
						dpp::guild_member member_obj;
						member_obj.user_id = userFlake;
						member_obj.guild_id = guildFlake;
						pDiscordUser = new DiscordUser(user_obj, member_obj, discord);
					}
				} else {
					dpp::guild_member member_obj;
					member_obj.user_id = userFlake;
					member_obj.guild_id = guildFlake;
					pDiscordUser = new DiscordUser(user_obj, member_obj, discord);
				}
			} else {
				pDiscordUser = new DiscordUser(user_obj, discord);
			}

			HandleError err;
			HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
			Handle_t handle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);

			if (handle == BAD_HANDLE) {
				delete pDiscordUser;
				pContext->ReportError("Could not create user handle (error %d)", err);
				return BAD_HANDLE;
			}

			return handle;
		}

		DiscordUser* pDiscordUser;
		if (guildFlake != 0) {
			dpp::guild* guild_ptr = dpp::find_guild(guildFlake);
			if (guild_ptr) {
				auto member_it = guild_ptr->members.find(userFlake);
				if (member_it != guild_ptr->members.end()) {
					pDiscordUser = new DiscordUser(*user_ptr, member_it->second, discord);
				} else {
					dpp::guild_member member_obj;
					member_obj.user_id = userFlake;
					member_obj.guild_id = guildFlake;
					pDiscordUser = new DiscordUser(*user_ptr, member_obj, discord);
				}
			} else {
				dpp::guild_member member_obj;
				member_obj.user_id = userFlake;
				member_obj.guild_id = guildFlake;
				pDiscordUser = new DiscordUser(*user_ptr, member_obj, discord);
			}
		} else {
			pDiscordUser = new DiscordUser(*user_ptr, discord);
		}

		HandleError err;
		HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
		Handle_t handle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);

		if (handle == BAD_HANDLE) {
			delete pDiscordUser;
			pContext->ReportError("Could not create user handle (error %d)", err);
			return BAD_HANDLE;
		}

		return handle;
	}
	catch (const std::invalid_argument& e) {
		pContext->ReportError("Invalid Discord client handle: %s", e.what());
		return BAD_HANDLE;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid user ID format: %s", userId);
		return BAD_HANDLE;
	}
}

static cell_t user_FetchUser(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* userId;
	pContext->LocalToString(params[2], &userId);

	IPluginFunction* callback = pContext->GetFunctionById(params[3]);
	if (!callback) {
		pContext->ReportError("Invalid callback function");
		return 0;
	}

	cell_t data = params[4];

	try {
		dpp::snowflake userFlake = std::stoull(userId);

		discord->GetCluster()->user_get(userFlake, [discord, callback, data](const dpp::confirmation_callback_t& confirmation) {
			g_TaskQueue.Push([discord, callback, data, confirmation]() {
				if (confirmation.is_error()) {
					return;
				}

				try {
					DiscordUser* pDiscordUser = nullptr;
					
					if (std::holds_alternative<dpp::user>(confirmation.value)) {
						dpp::user user_obj = std::get<dpp::user>(confirmation.value);
						pDiscordUser = new DiscordUser(user_obj, discord);
					} else if (std::holds_alternative<dpp::user_identified>(confirmation.value)) {
						dpp::user_identified user_identified = std::get<dpp::user_identified>(confirmation.value);
						pDiscordUser = new DiscordUser(user_identified, discord);
					} else {
						dpp::user user_obj = confirmation.get<dpp::user>();
						pDiscordUser = new DiscordUser(user_obj, discord);
					}
					
					if (!pDiscordUser) {
						return;
					}

					HandleError err;
					HandleSecurity sec;
					sec.pOwner = myself->GetIdentity();
					sec.pIdentity = myself->GetIdentity();
					Handle_t handle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);

					if (handle == BAD_HANDLE) {
						delete pDiscordUser;
						return;
					}

					callback->PushCell(discord->GetHandle());
					callback->PushCell(handle);
					callback->PushCell(data);
					callback->Execute(nullptr);
					
					handlesys->FreeHandle(handle, &sec);
				}
				catch (const std::exception& e) {
					return;
				}
			});
		});

		return 1;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid user ID format: %s", userId);
		return 0;
	}
}

static cell_t message_FetchMessage(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* messageId;
	pContext->LocalToString(params[2], &messageId);
	
	char* channelId;
	pContext->LocalToString(params[3], &channelId);

	IPluginFunction* callback = pContext->GetFunctionById(params[4]);
	if (!callback) {
		pContext->ReportError("Invalid callback function");
		return 0;
	}

	cell_t data = params[5];

	try {
		dpp::snowflake messageFlake = std::stoull(messageId);
		dpp::snowflake channelFlake = std::stoull(channelId);

		discord->GetCluster()->message_get(messageFlake, channelFlake, [discord, callback, data](const dpp::confirmation_callback_t& confirmation) {
			g_TaskQueue.Push([discord, callback, data, confirmation]() {
				if (confirmation.is_error()) {
					return;
				}

				try {
					dpp::message message_obj = confirmation.get<dpp::message>();
					DiscordMessage* pDiscordMessage = new DiscordMessage(message_obj, discord);

					HandleError err;
					HandleSecurity sec;
					sec.pOwner = myself->GetIdentity();
					sec.pIdentity = myself->GetIdentity();
					Handle_t handle = handlesys->CreateHandleEx(g_DiscordMessageHandle, pDiscordMessage, &sec, nullptr, &err);

					if (handle == BAD_HANDLE) {
						delete pDiscordMessage;
						return;
					}

					callback->PushCell(discord->GetHandle());
					callback->PushCell(handle);
					callback->PushCell(data);
					callback->Execute(nullptr);
					
					handlesys->FreeHandle(handle, &sec);
				}
				catch (const std::exception& e) {
					return;
				}
			});
		});

		return 1;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid message or channel ID format: %s, %s", messageId, channelId);
		return 0;
	}
}

static cell_t channel_FetchChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	IPluginFunction* callback = pContext->GetFunctionById(params[3]);
	if (!callback) {
		pContext->ReportError("Invalid callback function");
		return 0;
	}

	cell_t data = params[4];

	try {
		dpp::snowflake channelFlake = std::stoull(channelId);

		discord->GetCluster()->channel_get(channelFlake, [discord, callback, data](const dpp::confirmation_callback_t& confirmation) {
			g_TaskQueue.Push([discord, callback, data, confirmation]() {
				if (confirmation.is_error()) {
					return;
				}

				try {
					dpp::channel channel_obj = confirmation.get<dpp::channel>();
					DiscordChannel* pDiscordChannel = new DiscordChannel(channel_obj, discord);

					HandleError err;
					HandleSecurity sec;
					sec.pOwner = myself->GetIdentity();
					sec.pIdentity = myself->GetIdentity();
					Handle_t handle = handlesys->CreateHandleEx(g_DiscordChannelHandle, pDiscordChannel, &sec, nullptr, &err);

					if (handle == BAD_HANDLE) {
						delete pDiscordChannel;
						return;
					}

					callback->PushCell(discord->GetHandle());
					callback->PushCell(handle);
					callback->PushCell(data);
					callback->Execute(nullptr);
					
					handlesys->FreeHandle(handle, &sec);
				}
				catch (const std::exception& e) {
					return;
				}
			});
		});

		return 1;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t message_CreateFromId(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return BAD_HANDLE;
	}

	char* messageId;
	pContext->LocalToString(params[2], &messageId);

	char* channelId;
	pContext->LocalToString(params[3], &channelId);

	try {
		dpp::snowflake messageFlake = std::stoull(messageId);
		dpp::snowflake channelFlake = std::stoull(channelId);

		// Try to get the full message information from cache
		// Note: DPP doesn't have a find_message function, so we create a stub message
		// The message content and other properties will only be available if the bot
		// has cached this message from events or API calls
		
		// Create a minimal message object with the IDs set
		dpp::message message_obj;
		message_obj.id = messageFlake;
		message_obj.channel_id = channelFlake;

		DiscordMessage* pDiscordMessage = new DiscordMessage(message_obj, discord);

		HandleError err;
		HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
		Handle_t handle = handlesys->CreateHandleEx(g_DiscordMessageHandle, pDiscordMessage, &sec, nullptr, &err);

		if (handle == BAD_HANDLE) {
			delete pDiscordMessage;
			pContext->ReportError("Could not create message handle (error %d)", err);
			return BAD_HANDLE;
		}

		return handle;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid message or channel ID format: %s, %s", messageId, channelId);
		return BAD_HANDLE;
	}
}

static cell_t user_GetId(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetId().c_str());
	return 1;
}

static cell_t user_GetUserName(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetUserName());
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

	pContext->StringToLocal(params[3], params[4], user->GetAvatarUrl(params[2]).c_str());
	return 1;
}

static cell_t user_GetDefaultAvatarUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetDefaultAvatarUrl().c_str());
	return 1;
}

static cell_t user_GetAvatarDecorationUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[3], params[4], user->GetAvatarDecorationUrl(params[2]).c_str());
	return 1;
}

static cell_t user_GetMention(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetMention().c_str());
	return 1;
}

static cell_t user_GetUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetUrl().c_str());
	return 1;
}

static cell_t user_GetFormattedUsername(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->FormatUsername().c_str());
	return 1;
}

static cell_t user_GetFlags(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->GetFlags();
}

static cell_t user_IsSystem(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsSystem();
}

static cell_t user_IsMfaEnabled(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsMfaEnabled();
}

static cell_t user_IsVerified(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsVerified();
}

static cell_t user_HasNitroFull(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->HasNitroFull();
}

static cell_t user_HasNitroClassic(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->HasNitroClassic();
}

static cell_t user_HasNitroBasic(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->HasNitroBasic();
}

static cell_t user_IsDiscordEmployee(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsDiscordEmployee();
}

static cell_t user_IsPartneredOwner(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsPartneredOwner();
}

static cell_t user_HasHypesquadEvents(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->HasHypesquadEvents();
}

static cell_t user_IsBughunter1(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsBughunter1();
}

static cell_t user_IsHouseBravery(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsHouseBravery();
}

static cell_t user_IsHouseBrilliance(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsHouseBrilliance();
}

static cell_t user_IsHouseBalance(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsHouseBalance();
}

static cell_t user_IsEarlySupporter(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsEarlySupporter();
}

static cell_t user_IsTeamUser(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsTeamUser();
}

static cell_t user_IsBughunter2(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsBughunter2();
}

static cell_t user_IsVerifiedBot(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsVerifiedBot();
}

static cell_t user_IsVerifiedBotDev(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsVerifiedBotDev();
}

static cell_t user_IsCertifiedModerator(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsCertifiedModerator();
}

static cell_t user_IsBotHttpInteractions(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsBotHttpInteractions();
}

static cell_t user_HasAnimatedIcon(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->HasAnimatedIcon();
}

static cell_t user_IsActiveDeveloper(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsActiveDeveloper();
}

static cell_t user_IsBot(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsBot();
}

static cell_t user_HasGuildMember(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->HasGuildMember();
}

static cell_t user_GetNickName(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	std::string nickname = user->GetNickName();
	pContext->StringToLocal(params[2], params[3], nickname.c_str());
	return 1;
}

static cell_t user_GetJoinedAt(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], user->GetJoinedAt().c_str());
	return 1;
}

static cell_t user_IsPending(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->IsPending();
}

static cell_t user_HasPermission(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* permission;
	pContext->LocalToString(params[2], &permission);
	return user->HasPermission(permission);
}

static cell_t user_GetPermissions(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char permStr[32];
	snprintf(permStr, sizeof(permStr), "%" PRIu64, user->GetPermissions());
	pContext->StringToLocal(params[2], params[3], permStr);
	return 1;
}

static cell_t user_HasPermissionInChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);
	
	char* permission;
	pContext->LocalToString(params[3], &permission);

	try {
		dpp::snowflake channel = std::stoull(channelId);
		return user->HasPermissionInChannel(channel, permission);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t user_GetPermissionsInChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	try {
		dpp::snowflake channel = std::stoull(channelId);
		char permStr[32];
		snprintf(permStr, sizeof(permStr), "%" PRIu64, user->GetPermissionsInChannel(channel));
		pContext->StringToLocal(params[3], params[4], permStr);
		return 1;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return 0;
	}
}

static cell_t user_GetRoles(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	std::vector<dpp::snowflake> roles = user->GetRoles();
	cell_t maxSize = params[3];
	cell_t actualSize = (static_cast<cell_t>(roles.size()) < maxSize) ? static_cast<cell_t>(roles.size()) : maxSize;
	
	cell_t* rolesArray;
	pContext->LocalToPhysAddr(params[2], &rolesArray);
	
	for (cell_t i = 0; i < actualSize; i++) {
		char roleStr[32];
		snprintf(roleStr, sizeof(roleStr), "%llu", static_cast<unsigned long long>(roles[i]));
		pContext->StringToLocalUTF8(rolesArray[i], 32, roleStr, nullptr);
	}
	
	return actualSize;
}

static cell_t user_HasRole(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* roleId;
	pContext->LocalToString(params[2], &roleId);

	try {
		dpp::snowflake role = std::stoull(roleId);
		return user->HasRole(role);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid role ID format: %s", roleId);
		return 0;
	}
}

static cell_t user_GetHighestRole(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	dpp::snowflake highest = user->GetHighestRole();
	char roleStr[32];
	snprintf(roleStr, sizeof(roleStr), "%llu", static_cast<unsigned long long>(highest));
	pContext->StringToLocal(params[2], params[3], roleStr);
	return 1;
}

static cell_t user_GetRoleName(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* roleId;
	pContext->LocalToString(params[2], &roleId);

	try {
		dpp::snowflake role = std::stoull(roleId);
		std::string roleName = user->GetRoleName(role);
		pContext->StringToLocal(params[3], params[4], roleName.c_str());
		return roleName.empty();
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid role ID format: %s", roleId);
		return 0;
	}
}

static cell_t user_GetRoleNames(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	std::vector<std::string> roleNames = user->GetRoleNames();
	cell_t maxSize = params[3];
	cell_t actualSize = (static_cast<cell_t>(roleNames.size()) < maxSize) ? static_cast<cell_t>(roleNames.size()) : maxSize;
	
	cell_t* namesArray;
	pContext->LocalToPhysAddr(params[2], &namesArray);
	
	for (cell_t i = 0; i < actualSize; i++) {
		pContext->StringToLocalUTF8(namesArray[i], 64, roleNames[i].c_str(), nullptr);
	}
	
	return actualSize;
}

static cell_t user_SetNickName(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* nickname;
	pContext->LocalToString(params[2], &nickname);
	return user->SetNickName(nickname);
}

static cell_t user_AddRole(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* roleId;
	pContext->LocalToString(params[2], &roleId);

	try {
		dpp::snowflake role = std::stoull(roleId);
		return user->AddRole(role);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid role ID format: %s", roleId);
		return 0;
	}
}

static cell_t user_RemoveRole(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* roleId;
	pContext->LocalToString(params[2], &roleId);

	try {
		dpp::snowflake role = std::stoull(roleId);
		return user->RemoveRole(role);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid role ID format: %s", roleId);
		return 0;
	}
}

static cell_t user_KickFromGuild(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->KickFromGuild();
}

static cell_t user_BanFromGuild(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	char* reason;
	pContext->LocalToString(params[2], &reason);

	int delete_days = params[3];
	return user->BanFromGuild(reason, delete_days);
}

static cell_t user_UnbanFromGuild(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->UnbanFromGuild();
}

static cell_t user_SetTimeout(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	time_t timeout_until = static_cast<time_t>(params[2]);
	return user->SetTimeout(timeout_until);
}

static cell_t user_RemoveTimeout(IPluginContext* pContext, const cell_t* params)
{
	DiscordUser* user = GetUserPointer(pContext, params[1]);
	if (!user) {
		return 0;
	}

	return user->RemoveTimeout();
}

uint64_t DiscordGuild::GetBasePermissions(dpp::snowflake user_id) const
{
	if (!m_client || !m_client->GetCluster()) {
		return 0;
	}
	
	try {
		return m_guild.base_permissions(dpp::find_user(user_id));
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed get base permissions: %s", e.what());
		return 0;
	}
}

uint64_t DiscordGuild::GetPermissionsInChannel(dpp::snowflake user_id, dpp::snowflake channel_id) const
{
	if (!m_client || !m_client->GetCluster()) {
		return 0;
	}
	
	try {
		dpp::channel* ch = dpp::find_channel(channel_id);
		if (!ch) return 0;
		
		dpp::user* user = dpp::find_user(user_id);
		if (!user) return 0;
		
		return m_guild.base_permissions(user);
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed get permissions in channel: %s", e.what());
		return 0;
	}
}

bool DiscordGuild::HasPermission(dpp::snowflake user_id, const char* permission) const
{
	if (!permission) return false;
	
	try {
		uint64_t perm = std::stoull(permission);
		uint64_t base_perms = GetBasePermissions(user_id);
		return (base_perms & perm) != 0;
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed call has permission: %s", e.what());
		return false;
	}
}

bool DiscordGuild::HasPermissionInChannel(dpp::snowflake user_id, dpp::snowflake channel_id, const char* permission) const
{
	if (!permission) return false;
	
	try {
		uint64_t perm = std::stoull(permission);
		uint64_t channel_perms = GetPermissionsInChannel(user_id, channel_id);
		return (channel_perms & perm) != 0;
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed call has permission in channel: %s", e.what());
		return false;
	}
}

bool DiscordGuild::Modify(const char* name, const char* description)
{
	if (!m_client || !m_client->GetCluster()) {
		return false;
	}
	
	try {
		dpp::guild modified_guild = m_guild;
		
		if (name && strlen(name) > 0) {
			modified_guild.name = name;
		}
		
		if (description && strlen(description) > 0) {
			modified_guild.description = description;
		}
		
		m_client->GetCluster()->guild_edit(modified_guild);
		return true;
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to modify guild: %s", e.what());
		return false;
	}
}

static DiscordGuild* GetGuildPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordGuild* guild;
	if ((err = handlesys->ReadHandle(handle, g_DiscordGuildHandle, &sec, (void**)&guild)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord guild handle %x (error %d)", handle, err);
		return nullptr;
	}

	return guild;
}

static cell_t guild_FetchGuild(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* guildId;
	pContext->LocalToString(params[2], &guildId);

	IPluginFunction* callback = pContext->GetFunctionById(params[3]);
	if (!callback) {
		pContext->ReportError("Invalid callback function");
		return 0;
	}

	cell_t data = params[4];

	try {
		dpp::snowflake guildFlake = std::stoull(guildId);

		discord->GetCluster()->guild_get(guildFlake, [discord, callback, data](const dpp::confirmation_callback_t& confirmation) {
			g_TaskQueue.Push([discord, callback, data, confirmation]() {
				if (confirmation.is_error()) {
					return;
				}

				try {
					dpp::guild guild = confirmation.get<dpp::guild>();
					DiscordGuild* pDiscordGuild = new DiscordGuild(guild, discord);

					if (!pDiscordGuild) {
						return;
					}

					HandleError err;
					HandleSecurity sec;
					sec.pOwner = myself->GetIdentity();
					sec.pIdentity = myself->GetIdentity();
					Handle_t handle = handlesys->CreateHandleEx(g_DiscordGuildHandle, pDiscordGuild, &sec, nullptr, &err);

					if (handle == BAD_HANDLE) {
						delete pDiscordGuild;
						return;
					}

					callback->PushCell(discord->GetHandle());
					callback->PushCell(handle);
					callback->PushCell(data);
					callback->Execute(nullptr);

					handlesys->FreeHandle(handle, &sec);
				} catch (const std::exception& e) {
				}
			});
		});
	} catch (const std::exception& e) {
		pContext->ReportError("Invalid guild ID: %s", guildId);
		return 0;
	}

	return 1;
}

static cell_t guild_GetId(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], guild->GetId().c_str());
	return 1;
}

static cell_t guild_GetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], guild->GetName());
	return 1;
}

static cell_t guild_GetDescription(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], guild->GetDescription());
	return 1;
}

static cell_t guild_GetOwnerId(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], guild->GetOwnerId().c_str());
	return 1;
}

static cell_t guild_GetMemberCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return static_cast<cell_t>(guild->GetMemberCount());
}

static cell_t guild_GetVerificationLevel(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetVerificationLevel();
}

static cell_t guild_GetPremiumTier(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetPremiumTier();
}

static cell_t guild_IsLarge(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsLarge();
}

static cell_t guild_IsVerified(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsVerified();
}

static cell_t guild_IsPartnered(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsPartnered();
}

static cell_t guild_GetIconUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	uint16_t size = static_cast<uint16_t>(params[4]);
	bool prefer_animated = params[5] != 0;
	std::string iconUrl = guild->GetIconUrl(size, prefer_animated);

	pContext->StringToLocal(params[2], params[3], iconUrl.c_str());
	return 1;
}

static cell_t guild_GetBannerUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	uint16_t size = static_cast<uint16_t>(params[4]);
	bool prefer_animated = params[5] != 0;
	std::string bannerUrl = guild->GetBannerUrl(size, prefer_animated);

	pContext->StringToLocal(params[2], params[3], bannerUrl.c_str());
	return 1;
}

static cell_t guild_GetSplashUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	uint16_t size = static_cast<uint16_t>(params[4]);
	std::string splashUrl = guild->GetSplashUrl(size);

	pContext->StringToLocal(params[2], params[3], splashUrl.c_str());
	return 1;
}

static cell_t guild_GetDiscoverySplashUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	uint16_t size = static_cast<uint16_t>(params[4]);
	std::string splashUrl = guild->GetDiscoverySplashUrl(size);

	pContext->StringToLocal(params[2], params[3], splashUrl.c_str());
	return 1;
}

static cell_t guild_GetVanityUrlCode(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	const char* vanityUrl = guild->GetVanityUrlCode();
	pContext->StringToLocal(params[2], params[3], vanityUrl);
	return 1;
}

static cell_t guild_GetMaxMembers(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetMaxMembers();
}

static cell_t guild_GetMaxPresences(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetMaxPresences();
}

static cell_t guild_GetPremiumSubscriptionCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetPremiumSubscriptionCount();
}

static cell_t guild_GetExplicitContentFilter(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetExplicitContentFilter();
}

static cell_t guild_GetMfaLevel(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetMfaLevel();
}

static cell_t guild_GetNsfwLevel(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetNsfwLevel();
}

static cell_t guild_GetAfkTimeout(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetAfkTimeout();
}

static cell_t guild_GetDefaultMessageNotifications(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetDefaultMessageNotifications();
}

static cell_t guild_GetShardId(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetShardId();
}

static cell_t guild_GetFlags(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetFlags();
}

static cell_t guild_GetFlagsExtra(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->GetFlagsExtra();
}

static cell_t guild_GetAfkChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	std::string channelId = guild->GetAfkChannelId();
	pContext->StringToLocal(params[2], params[3], channelId.c_str());
	return 1;
}

static cell_t guild_GetSystemChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	std::string channelId = guild->GetSystemChannelId();
	pContext->StringToLocal(params[2], params[3], channelId.c_str());
	return 1;
}

static cell_t guild_GetRulesChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	std::string channelId = guild->GetRulesChannelId();
	pContext->StringToLocal(params[2], params[3], channelId.c_str());
	return 1;
}

static cell_t guild_GetPublicUpdatesChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	std::string channelId = guild->GetPublicUpdatesChannelId();
	pContext->StringToLocal(params[2], params[3], channelId.c_str());
	return 1;
}

static cell_t guild_GetSafetyAlertsChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	std::string channelId = guild->GetSafetyAlertsChannelId();
	pContext->StringToLocal(params[2], params[3], channelId.c_str());
	return 1;
}

static cell_t guild_IsUnavailable(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsUnavailable();
}

static cell_t guild_WidgetEnabled(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->WidgetEnabled();
}

static cell_t guild_HasInviteSplash(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasInviteSplash();
}

static cell_t guild_HasVipRegions(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasVipRegions();
}

static cell_t guild_HasVanityUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasVanityUrl();
}

static cell_t guild_IsCommunity(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsCommunity();
}

static cell_t guild_HasRoleSubscriptions(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasRoleSubscriptions();
}

static cell_t guild_HasNews(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasNews();
}

static cell_t guild_IsDiscoverable(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsDiscoverable();
}

static cell_t guild_IsFeatureable(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsFeatureable();
}

static cell_t guild_HasAnimatedIcon(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasAnimatedIcon();
}

static cell_t guild_HasBanner(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasBanner();
}

static cell_t guild_IsWelcomeScreenEnabled(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsWelcomeScreenEnabled();
}

static cell_t guild_HasMemberVerificationGate(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasMemberVerificationGate();
}

static cell_t guild_IsPreviewEnabled(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->IsPreviewEnabled();
}

static cell_t guild_HasAnimatedIconHash(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasAnimatedIconHash();
}

static cell_t guild_HasAnimatedBannerHash(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasAnimatedBannerHash();
}

static cell_t guild_HasMonetizationEnabled(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasMonetizationEnabled();
}

static cell_t guild_HasMoreStickers(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasMoreStickers();
}

static cell_t guild_HasCreatorStorePage(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasCreatorStorePage();
}

static cell_t guild_HasRoleIcons(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasRoleIcons();
}

static cell_t guild_HasSevenDayThreadArchive(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasSevenDayThreadArchive();
}

static cell_t guild_HasThreeDayThreadArchive(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasThreeDayThreadArchive();
}

static cell_t guild_HasTicketedEvents(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasTicketedEvents();
}

static cell_t guild_HasChannelBanners(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasChannelBanners();
}

static cell_t guild_HasPremiumProgressBarEnabled(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasPremiumProgressBarEnabled();
}

static cell_t guild_HasAnimatedBanner(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasAnimatedBanner();
}

static cell_t guild_HasAutoModeration(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasAutoModeration();
}

static cell_t guild_HasInvitesDisabled(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasInvitesDisabled();
}

static cell_t guild_HasSupportServer(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasSupportServer();
}

static cell_t guild_HasRoleSubscriptionsAvailableForPurchase(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasRoleSubscriptionsAvailableForPurchase();
}

static cell_t guild_HasRaidAlertsDisabled(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->HasRaidAlertsDisabled();
}

static cell_t guild_NoJoinNotifications(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->NoJoinNotifications();
}

static cell_t guild_NoBoostNotifications(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->NoBoostNotifications();
}

static cell_t guild_NoSetupTips(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->NoSetupTips();
}

static cell_t guild_NoStickerGreeting(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->NoStickerGreeting();
}

static cell_t guild_NoRoleSubscriptionNotifications(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->NoRoleSubscriptionNotifications();
}

static cell_t guild_NoRoleSubscriptionNotificationReplies(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}

	return guild->NoRoleSubscriptionNotificationReplies();
}

static cell_t guild_GetBasePermissions(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	char* user_id_str;
	pContext->LocalToString(params[2], &user_id_str);
	
	try {
		dpp::snowflake user_id = std::stoull(user_id_str);
		return static_cast<cell_t>(guild->GetBasePermissions(user_id));
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed get base permissions: %s", e.what());
		return 0;
	}
}

static cell_t guild_GetPermissionsInChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	char* user_id_str;
	char* channel_id_str;
	pContext->LocalToString(params[2], &user_id_str);
	pContext->LocalToString(params[3], &channel_id_str);
	
	try {
		dpp::snowflake user_id = std::stoull(user_id_str);
		dpp::snowflake channel_id = std::stoull(channel_id_str);
		return static_cast<cell_t>(guild->GetPermissionsInChannel(user_id, channel_id));
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed get base permissions in channel: %s", e.what());
		return 0;
	}
}

static cell_t guild_HasPermission(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	char* user_id_str;
	char* permission;
	pContext->LocalToString(params[2], &user_id_str);
	pContext->LocalToString(params[3], &permission);
	
	try {
		dpp::snowflake user_id = std::stoull(user_id_str);
		return guild->HasPermission(user_id, permission);
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed call has permissions: %s", e.what());
		return 0;
	}
}

static cell_t guild_HasPermissionInChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	char* user_id_str;
	char* channel_id_str;
	char* permission;
	pContext->LocalToString(params[2], &user_id_str);
	pContext->LocalToString(params[3], &channel_id_str);
	pContext->LocalToString(params[4], &permission);
	
	try {
		dpp::snowflake user_id = std::stoull(user_id_str);
		dpp::snowflake channel_id = std::stoull(channel_id_str);
		return guild->HasPermissionInChannel(user_id, channel_id, permission);
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed call has permissions in channel: %s", e.what());
		return 0;
	}
}

static cell_t guild_Modify(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	char* name;
	char* description;
	
	pContext->LocalToString(params[2], &name);
	pContext->LocalToString(params[3], &description);
	
	return guild->Modify(name, description);
}

static cell_t guild_GetRoleCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	return static_cast<cell_t>(guild->GetRoleCount());
}

static cell_t guild_GetChannelCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	return static_cast<cell_t>(guild->GetChannelCount());
}

static cell_t guild_GetThreadCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	return static_cast<cell_t>(guild->GetThreadCount());
}

static cell_t guild_GetEmojiCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	return static_cast<cell_t>(guild->GetEmojiCount());
}

static cell_t guild_GetVoiceMemberCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordGuild* guild = GetGuildPointer(pContext, params[1]);
	if (!guild) {
		return 0;
	}
	
	return static_cast<cell_t>(guild->GetVoiceMemberCount());
}

static cell_t discord_GetUptime(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}
	
	return static_cast<cell_t>(discord->GetUptime());
}

static cell_t slashcommand_CreateSlashCommand(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	DiscordSlashCommand* command = new DiscordSlashCommand(discord);

	if (params[0] >= 2 && params[2] != 0) {
		char* commandId;
		pContext->LocalToString(params[2], &commandId);
		
		try {
			dpp::snowflake cmd_id = std::stoull(commandId);
			command->SetCommandId(cmd_id);
			
			if (params[0] >= 3 && params[3] != 0) {
				char* guildId;
				pContext->LocalToString(params[3], &guildId);
				dpp::snowflake guild_id = std::stoull(guildId);
				command->SetGuildId(guild_id);
			}
		}
		catch (const std::exception& e) {
			delete command;
			return pContext->ThrowNativeError("Invalid command ID format: %s", commandId);
		}
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordSlashCommandHandle, command, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete command;
		return pContext->ThrowNativeError("Could not create Discord slash command handle (error %d)", err);
	}

	return handle;
}

static cell_t slashcommand_FromGlobalCommand(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return BAD_HANDLE;
	}

	char* commandId;
	pContext->LocalToString(params[2], &commandId);

	DiscordSlashCommand* command = new DiscordSlashCommand(discord);
	
	try {
		dpp::snowflake cmd_id = std::stoull(commandId);
		command->SetCommandId(cmd_id);
	}
	catch (const std::exception& e) {
		delete command;
		return pContext->ThrowNativeError("Invalid command ID format: %s", commandId);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordSlashCommandHandle, command, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete command;
		return pContext->ThrowNativeError("Could not create Discord slash command handle (error %d)", err);
	}

	return handle;
}

static cell_t slashcommand_FromGuildCommand(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return BAD_HANDLE;
	}

	char* commandId;
	pContext->LocalToString(params[2], &commandId);
	char* guildId;
	pContext->LocalToString(params[3], &guildId);

	DiscordSlashCommand* command = new DiscordSlashCommand(discord);
	
	try {
		dpp::snowflake cmd_id = std::stoull(commandId);
		dpp::snowflake guild_id = std::stoull(guildId);
		command->SetCommandId(cmd_id);
		command->SetGuildId(guild_id);
	}
	catch (const std::exception& e) {
		delete command;
		return pContext->ThrowNativeError("Invalid ID format: command=%s, guild=%s", commandId, guildId);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordSlashCommandHandle, command, &sec, nullptr, &err);

	if (handle == BAD_HANDLE)
	{
		delete command;
		return pContext->ThrowNativeError("Could not create Discord slash command handle (error %d)", err);
	}

	return handle;
}

static cell_t slashcommand_SetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);
	command->SetName(name);
	return 1;
}

static cell_t slashcommand_SetDescription(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* description;
	pContext->LocalToString(params[2], &description);
	command->SetDescription(description);
	return 1;
}

static cell_t slashcommand_SetDefaultPermissions(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* permissions;
	pContext->LocalToString(params[2], &permissions);
	command->SetDefaultPermissions(permissions);
	return 1;
}

static cell_t slashcommand_GetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], command->GetName());
	return 1;
}

static cell_t slashcommand_GetDescription(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], command->GetDescription());
	return 1;
}

static cell_t slashcommand_GetDefaultPermissions(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	std::string permissions = command->GetDefaultPermissions();
	pContext->StringToLocal(params[2], params[3], permissions.c_str());
	return 1;
}

static cell_t slashcommand_AddOption(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);
	
	char* description;
	pContext->LocalToString(params[3], &description);
	
	dpp::command_option_type type = static_cast<dpp::command_option_type>(params[4]);
	bool required = params[5];
	bool autocomplete = params[6];

	command->AddOption(name, description, type, required, autocomplete);
	return 1;
}

static cell_t slashcommand_AddChoiceOption(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);
	
	char* description;
	pContext->LocalToString(params[3], &description);
	
	dpp::command_option_type type = static_cast<dpp::command_option_type>(params[4]);
	bool required = params[5];

	command->AddChoiceOption(name, description, type, required);
	return 1;
}

static cell_t slashcommand_AddStringChoice(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* choice_name;
	pContext->LocalToString(params[2], &choice_name);
	
	char* choice_value;
	pContext->LocalToString(params[3], &choice_value);

	command->AddStringChoice(choice_name, choice_value);
	return 1;
}

static cell_t slashcommand_AddIntChoice(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* choice_name;
	pContext->LocalToString(params[2], &choice_name);
	
	int64_t choice_value = static_cast<int64_t>(params[3]);

	command->AddIntChoice(choice_name, choice_value);
	return 1;
}

static cell_t slashcommand_AddFloatChoice(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* choice_name;
	pContext->LocalToString(params[2], &choice_name);
	
	double choice_value = sp_ctof(params[3]);

	command->AddFloatChoice(choice_name, choice_value);
	return 1;
}

static cell_t slashcommand_RegisterToGuild(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* guildId;
	pContext->LocalToString(params[2], &guildId);

	try {
		dpp::snowflake guild = std::stoull(guildId);
		return command->RegisterToGuild(guild);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid guild ID format: %s", guildId);
		return 0;
	}
}

static cell_t slashcommand_RegisterGlobally(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	return command->RegisterGlobally();
}

static cell_t slashcommand_Update(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	dpp::snowflake guild_id = 0;
	if (params[2] != 0) {
		char* guildId;
		pContext->LocalToString(params[2], &guildId);
		try {
			guild_id = std::stoull(guildId);
		}
		catch (const std::exception& e) {
			pContext->ReportError("Invalid guild ID format: %s", guildId);
			return 0;
		}
	}

	return command->Update(guild_id);
}

static cell_t slashcommand_Delete(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	dpp::snowflake guild_id = 0;
	if (params[2] != 0) {
		char* guildId;
		pContext->LocalToString(params[2], &guildId);
		try {
			guild_id = std::stoull(guildId);
		}
		catch (const std::exception& e) {
			pContext->ReportError("Invalid guild ID format: %s", guildId);
			return 0;
		}
	}

	return command->Delete(guild_id);
}

static cell_t slashcommand_AddPermissionOverride(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}
	
	char* targetId;
	pContext->LocalToString(params[2], &targetId);
	
	int type = params[3];
	bool permission = params[4] != 0;
	
	try {
		dpp::snowflake target = std::stoull(targetId);
		dpp::command_permission_type cmd_type = static_cast<dpp::command_permission_type>(type);
		command->AddPermissionOverride(target, cmd_type, permission);
		return 1;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid target ID format: %s", targetId);
		return 0;
	}
}

static cell_t slashcommand_RemovePermissionOverride(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}
	
	char* targetId;
	pContext->LocalToString(params[2], &targetId);
	
	int type = params[3];
	
	try {
		dpp::snowflake target = std::stoull(targetId);
		dpp::command_permission_type cmd_type = static_cast<dpp::command_permission_type>(type);
		command->RemovePermissionOverride(target, cmd_type);
		return 1;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid target ID format: %s", targetId);
		return 0;
	}
}

static cell_t slashcommand_ClearPermissionOverrides(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}
	
	command->ClearPermissionOverrides();
	return 1;
}

static cell_t slashcommand_GetPermissionOverrideCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}
	
	return static_cast<cell_t>(command->GetPermissionOverrideCount());
}

static cell_t slashcommand_GetPermissionOverride(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}
	
	int index = params[2];
	
	dpp::snowflake target_id;
	dpp::command_permission_type type;
	bool permission;
	
	if (!command->GetPermissionOverride(static_cast<size_t>(index), target_id, type, permission)) {
		return 0;
	}
	
	char targetStr[32];
	snprintf(targetStr, sizeof(targetStr), "%llu", static_cast<unsigned long long>(target_id));
	pContext->StringToLocal(params[3], params[4], targetStr);
	
	cell_t* typePtr;
	pContext->LocalToPhysAddr(params[5], &typePtr);
	*typePtr = static_cast<cell_t>(type);
	
	cell_t* permPtr;
	pContext->LocalToPhysAddr(params[6], &permPtr);
	*permPtr = permission;
	
	return 1;
}

static cell_t slashcommand_ApplyPermissionOverrides(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* guildId;
	pContext->LocalToString(params[2], &guildId);

	try {
		dpp::snowflake guild_id = std::stoull(guildId);
		return command->ApplyPermissionOverrides(guild_id);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid guild ID format: %s", guildId);
		return 0;
	}
}

static cell_t slashcommand_SetContextMenuType(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	int type = params[2];
	dpp::slashcommand_contextmenu_type contextType = static_cast<dpp::slashcommand_contextmenu_type>(type);
	command->SetContextMenuType(contextType);
	return 1;
}

static cell_t slashcommand_GetContextMenuType(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	return static_cast<cell_t>(command->GetContextMenuType());
}

static cell_t slashcommand_SetNSFW(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	bool nsfw = params[2] != 0;
	command->SetNSFW(nsfw);
	return 1;
}

static cell_t slashcommand_GetNSFW(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	return command->GetNSFW();
}

static cell_t slashcommand_SetDMPermission(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	bool dm_permission = params[2] != 0;
	command->SetDMPermission(dm_permission);
	return 1;
}

static cell_t slashcommand_GetDMPermission(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	return command->GetDMPermission();
}

static cell_t slashcommand_AddLocalization(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	char* language;
	char* name;
	char* description = nullptr;
	
	pContext->LocalToString(params[2], &language);
	pContext->LocalToString(params[3], &name);
	
	if (params[0] >= 4) {
		pContext->LocalToString(params[4], &description);
	}

	command->AddLocalization(language, name, description);
	return 1;
}

static cell_t slashcommand_SetInteractionContexts(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	cell_t* contexts_array;
	pContext->LocalToPhysAddr(params[2], &contexts_array);
	int numContexts = params[3];

	std::vector<dpp::interaction_context_type> contexts;
	for (int i = 0; i < numContexts; i++) {
		contexts.push_back(static_cast<dpp::interaction_context_type>(contexts_array[i]));
	}

	command->SetInteractionContexts(contexts);
	return 1;
}

static cell_t slashcommand_SetIntegrationTypes(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	cell_t* types_array;
	pContext->LocalToPhysAddr(params[2], &types_array);
	int numTypes = params[3];

	std::vector<dpp::application_integration_types> types;
	for (int i = 0; i < numTypes; i++) {
		types.push_back(static_cast<dpp::application_integration_types>(types_array[i]));
	}

	command->SetIntegrationTypes(types);
	return 1;
}

static cell_t slashcommand_SetOptionMinValue(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	float min_value = sp_ctof(params[2]);
	command->SetLastOptionMinValue(static_cast<double>(min_value));
	return 1;
}

static cell_t slashcommand_SetOptionMaxValue(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	float max_value = sp_ctof(params[2]);
	command->SetLastOptionMaxValue(static_cast<double>(max_value));
	return 1;
}

static cell_t slashcommand_SetOptionMinLength(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	int min_length = params[2];
	command->SetLastOptionMinLength(static_cast<int64_t>(min_length));
	return 1;
}

static cell_t slashcommand_SetOptionMaxLength(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	int max_length = params[2];
	command->SetLastOptionMaxLength(static_cast<int64_t>(max_length));
	return 1;
}

static cell_t slashcommand_AddOptionChannelType(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	int channel_type = params[2];
	command->AddLastOptionChannelType(static_cast<dpp::channel_type>(channel_type));
	return 1;
}

static cell_t slashcommand_GetMention(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	std::string mention = command->GetMention();
	pContext->StringToLocal(params[2], params[3], mention.c_str());
	return 1;
}

static cell_t slashcommand_GetCommandId(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	std::string commandId = std::to_string(command->GetCommandId());
	pContext->StringToLocal(params[2], params[3], commandId.c_str());
	return 1;
}

static cell_t slashcommand_GetGuildId(IPluginContext* pContext, const cell_t* params)
{
	DiscordSlashCommand* command = GetSlashCommandPointer(pContext, params[1]);
	if (!command) {
		return 0;
	}

	std::string guildId = std::to_string(command->GetGuildId());
	pContext->StringToLocal(params[2], params[3], guildId.c_str());
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

static DiscordForumTag* GetForumTagPointer(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	DiscordForumTag* tag;
	if ((err = handlesys->ReadHandle(handle, g_DiscordForumTagHandle, &sec, (void**)&tag)) != HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Discord forum tag handle %x (error %d)", handle, err);
		return nullptr;
	}

	return tag;
}

static cell_t message_GetContent(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], message->GetContent());
	return 1;
}

static cell_t message_GetContentLength(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return static_cast<cell_t>(message->GetContentLength());
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

	Handle_t handle = message->GetAuthorHandle();
	
	if (handle == BAD_HANDLE) {
		pContext->ReportError("Could not create author handle");
		return BAD_HANDLE;
	}

	return handle;
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

static cell_t message_GetType(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return static_cast<cell_t>(message->GetType());
}

static cell_t message_IsPinned(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->IsPinned();
}

static cell_t message_IsTTS(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->IsTTS();
}

static cell_t message_IsMentionEveryone(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->IsMentionEveryone();
}

static cell_t message_IsBot(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->IsBot();
}

static cell_t message_Edit(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	char* content;
	pContext->LocalToString(params[2], &content);
	return message->Edit(content);
}

static cell_t message_EditEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
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

	return message->EditEmbed(content, embed);
}

static cell_t message_Delete(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->Delete();
}

static cell_t message_Pin(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->Pin();
}

static cell_t message_Unpin(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->Unpin();
}

static cell_t message_AddReaction(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	char* emoji;
	pContext->LocalToString(params[2], &emoji);
	return message->AddReaction(emoji);
}

static cell_t message_RemoveReaction(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	char* emoji;
	pContext->LocalToString(params[2], &emoji);
	return message->RemoveReaction(emoji);
}

static cell_t message_RemoveAllReactions(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->RemoveAllReactions();
}

static cell_t message_Reply(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	char* content;
	pContext->LocalToString(params[2], &content);
	return message->Reply(content);
}

static cell_t message_ReplyEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
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

	return message->ReplyEmbed(content, embed);
}

static cell_t message_Crosspost(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	return message->Crosspost();
}

static cell_t message_CreateThread(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);
	
	int auto_archive_duration = (params[0] >= 3) ? params[3] : 60;
	
	return message->CreateThread(name, auto_archive_duration);
}

static cell_t message_GetFlags(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->GetFlags();
}

static cell_t message_IsCrossposted(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsCrossposted();
}

static cell_t message_IsCrosspost(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsCrosspost();
}

static cell_t message_EmbedsSuppressed(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->EmbedsSuppressed();
}

static cell_t message_IsSourceMessageDeleted(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsSourceMessageDeleted();
}

static cell_t message_IsUrgent(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsUrgent();
}

static cell_t message_HasThread(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->HasThread();
}

static cell_t message_IsEphemeral(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsEphemeral();
}

static cell_t message_IsLoading(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsLoading();
}

static cell_t message_IsThreadMentionFailed(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsThreadMentionFailed();
}

static cell_t message_NotificationsSuppressed(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->NotificationsSuppressed();
}

static cell_t message_IsVoiceMessage(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsVoiceMessage();
}

static cell_t message_HasSnapshot(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->HasSnapshot();
}

static cell_t message_IsUsingComponentsV2(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsUsingComponentsV2();
}

static cell_t message_GetTimestamp(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetTimestamp());
}

static cell_t message_GetEditedTimestamp(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetEditedTimestamp());
}

static cell_t message_IsDM(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->IsDM();
}

static cell_t message_HasRemixAttachment(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return message->HasRemixAttachment();
}

static cell_t message_GetAttachmentCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetAttachmentCount());
}

static cell_t message_GetEmbedCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetEmbedCount());
}

static cell_t message_GetReactionCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetReactionCount());
}

static cell_t message_GetStickerCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetStickerCount());
}

static cell_t message_GetMentionedUserCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetMentionedUserCount());
}

static cell_t message_GetMentionedRoleCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetMentionedRoleCount());
}

static cell_t message_GetMentionedChannelCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordMessage* message = GetMessagePointer(pContext, params[1]);
	if (!message) return 0;
	return static_cast<cell_t>(message->GetMentionedChannelCount());
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

static cell_t channel_CreateFromId(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return BAD_HANDLE;
	}

	char* channelId;
	pContext->LocalToString(params[2], &channelId);

	try {
		dpp::snowflake channelFlake = std::stoull(channelId);

		dpp::channel* channel_ptr = dpp::find_channel(channelFlake);
		
		if (!channel_ptr) {
			dpp::channel channel_obj;
			channel_obj.id = channelFlake;
			
			DiscordChannel* pDiscordChannel = new DiscordChannel(channel_obj, discord);
			
			HandleError err;
			HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
			Handle_t handle = handlesys->CreateHandleEx(g_DiscordChannelHandle, pDiscordChannel, &sec, nullptr, &err);

			if (handle == BAD_HANDLE) {
				delete pDiscordChannel;
				pContext->ReportError("Could not create channel handle (error %d)", err);
				return BAD_HANDLE;
			}

			return handle;
		}

		DiscordChannel* pDiscordChannel = new DiscordChannel(*channel_ptr, discord);

		HandleError err;
		HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
		Handle_t handle = handlesys->CreateHandleEx(g_DiscordChannelHandle, pDiscordChannel, &sec, nullptr, &err);

		if (handle == BAD_HANDLE) {
			delete pDiscordChannel;
			pContext->ReportError("Could not create channel handle (error %d)", err);
			return BAD_HANDLE;
		}

		return handle;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid channel ID format: %s", channelId);
		return BAD_HANDLE;
	}
}

static cell_t channel_GetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], channel->GetName());
	return 1;
}

static cell_t channel_GetId(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], channel->GetId().c_str());
	return 1;
}

static cell_t channel_GetGuildId(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], channel->GetGuildId().c_str());
	return 1;
}

static cell_t channel_GetParentId(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], channel->GetParentId().c_str());
	return 1;
}

static cell_t channel_GetTopic(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], channel->GetTopic());
	return 1;
}

static cell_t channel_GetType(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->GetType();
}

static cell_t channel_GetPosition(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->GetPosition();
}

static cell_t channel_IsNSFW(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->IsNSFW();
}

static cell_t channel_IsTextChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->IsTextChannel();
}

static cell_t channel_IsVoiceChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->IsVoiceChannel();
}

static cell_t channel_IsCategory(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->IsCategory();
}

static cell_t channel_IsThread(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->IsThread();
}

static cell_t channel_IsForum(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->IsForum();
}

static cell_t channel_IsNewsChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->IsNewsChannel();
}

static cell_t channel_IsStageChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->IsStageChannel();
}

static cell_t channel_GetBitrate(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->GetBitrate();
}

static cell_t channel_GetUserLimit(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->GetUserLimit();
}

static cell_t channel_GetRateLimitPerUser(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->GetRateLimitPerUser();
}

static cell_t channel_GetMention(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], channel->GetMention().c_str());
	return 1;
}

static cell_t channel_GetUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], channel->GetUrl().c_str());
	return 1;
}

static cell_t channel_SetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* name;
	pContext->LocalToString(params[2], &name);
	return channel->SetName(name);
}

static cell_t channel_SetTopic(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* topic;
	pContext->LocalToString(params[2], &topic);
	return channel->SetTopic(topic);
}

static cell_t channel_SetNSFW(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	bool nsfw = params[2] ? true : false;
	return channel->SetNSFW(nsfw);
}

static cell_t channel_Delete(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	return channel->Delete();
}

static cell_t channel_SetPosition(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	uint16_t position = static_cast<uint16_t>(params[2]);
	return channel->SetPosition(position);
}

static cell_t channel_SetRateLimitPerUser(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	uint16_t seconds = static_cast<uint16_t>(params[2]);
	return channel->SetRateLimitPerUser(seconds);
}

static cell_t channel_SetBitrate(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	uint16_t bitrate = static_cast<uint16_t>(params[2]);
	return channel->SetBitrate(bitrate);
}

static cell_t channel_SetUserLimit(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	uint8_t limit = static_cast<uint8_t>(params[2]);
	return channel->SetUserLimit(limit);
}

static cell_t channel_SetParent(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* parentId;
	pContext->LocalToString(params[2], &parentId);

	try {
		dpp::snowflake parent = std::stoull(parentId);
		return channel->SetParent(parent);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid parent ID format: %s", parentId);
		return 0;
	}
}

static cell_t channel_AddPermissionOverwrite(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* targetId;
	pContext->LocalToString(params[2], &targetId);
	uint8_t type = static_cast<uint8_t>(params[3]);
	
	char* allowed_str;
	char* denied_str;
	pContext->LocalToString(params[4], &allowed_str);
	pContext->LocalToString(params[5], &denied_str);

	try {
		dpp::snowflake target = std::stoull(targetId);
		uint64_t allowed = std::stoull(allowed_str);
		uint64_t denied = std::stoull(denied_str);
		return channel->AddPermissionOverwrite(target, type, allowed, denied);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid ID or permission format");
		return 0;
	}
}

static cell_t channel_SetPermissionOverwrite(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* targetId;
	pContext->LocalToString(params[2], &targetId);
	uint8_t type = static_cast<uint8_t>(params[3]);
	
	char* allowed_str;
	char* denied_str;
	pContext->LocalToString(params[4], &allowed_str);
	pContext->LocalToString(params[5], &denied_str);

	try {
		dpp::snowflake target = std::stoull(targetId);
		uint64_t allowed = std::stoull(allowed_str);
		uint64_t denied = std::stoull(denied_str);
		return channel->SetPermissionOverwrite(target, type, allowed, denied);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid ID or permission format");
		return 0;
	}
}

static cell_t channel_RemovePermissionOverwrite(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* targetId;
	pContext->LocalToString(params[2], &targetId);
	uint8_t type = static_cast<uint8_t>(params[3]);

	try {
		dpp::snowflake target = std::stoull(targetId);
		return channel->RemovePermissionOverwrite(target, type);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid target ID format: %s", targetId);
		return 0;
	}
}

static cell_t channel_GetUserPermissions(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* userId;
	pContext->LocalToString(params[2], &userId);

	try {
		dpp::snowflake user = std::stoull(userId);
		std::string permissions = channel->GetUserPermissions(user);
		pContext->StringToLocal(params[3], params[4], permissions.c_str());
		return 1;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid user ID format: %s", userId);
		return 0;
	}
}

static cell_t channel_CreateInvite(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	int max_age = (params[0] >= 2) ? params[2] : 86400;
	int max_uses = (params[0] >= 3) ? params[3] : 0;
	bool temporary = (params[0] >= 4) ? (params[4] != 0) : false;
	bool unique = (params[0] >= 5) ? (params[5] != 0) : false;

	return channel->CreateInvite(max_age, max_uses, temporary, unique);
}

static cell_t channel_SendMessage(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* content;
	pContext->LocalToString(params[2], &content);
	return channel->SendMessage(content);
}

static cell_t channel_SendMessageEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
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

	return channel->SendMessageEmbed(content, embed);
}

static cell_t channel_SetRTCRegion(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) {
		return 0;
	}

	char* region;
	pContext->LocalToString(params[2], &region);
	
	return channel->SetRTCRegion(region);
}

static cell_t channel_GetFlags(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->GetFlags();
}

static cell_t channel_GetOwnerId(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	std::string ownerId = channel->GetOwnerId();
	pContext->StringToLocal(params[2], params[3], ownerId.c_str());
	return 1;
}

static cell_t channel_GetLastMessageId(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	std::string lastMessageId = channel->GetLastMessageId();
	pContext->StringToLocal(params[2], params[3], lastMessageId.c_str());
	return 1;
}

static cell_t channel_GetLastPinTimestamp(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return static_cast<cell_t>(channel->GetLastPinTimestamp());
}

static cell_t channel_GetDefaultThreadRateLimitPerUser(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->GetDefaultThreadRateLimitPerUser();
}

static cell_t channel_GetDefaultAutoArchiveDuration(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->GetDefaultAutoArchiveDuration();
}

static cell_t channel_GetDefaultSortOrder(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->GetDefaultSortOrder();
}

static cell_t channel_GetForumLayout(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->GetForumLayout();
}

static cell_t channel_GetRTCRegion(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	std::string region = channel->GetRTCRegion();
	pContext->StringToLocal(params[2], params[3], region.c_str());
	return 1;
}

static cell_t channel_IsDM(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsDM();
}

static cell_t channel_IsGroupDM(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsGroupDM();
}

static cell_t channel_IsMediaChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsMediaChannel();
}

static cell_t channel_IsVideo720p(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsVideo720p();
}

static cell_t channel_IsVideoAuto(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsVideoAuto();
}

static cell_t channel_IsPinnedThread(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsPinnedThread();
}

static cell_t channel_IsTagRequired(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsTagRequired();
}

static cell_t channel_IsDownloadOptionsHidden(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsDownloadOptionsHidden();
}

static cell_t channel_IsLockedPermissions(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return channel->IsLockedPermissions();
}

static cell_t channel_GetPermissionOverwriteCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return static_cast<cell_t>(channel->GetPermissionOverwriteCount());
}

static cell_t channel_GetAvailableTagCount(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	return static_cast<cell_t>(channel->GetAvailableTagCount());
}

static cell_t channel_GetAvailableTagName(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	int index = params[2];
	std::string tagName = channel->GetAvailableTagName(index);
	pContext->StringToLocal(params[3], params[4], tagName.c_str());
	return 1;
}

static cell_t channel_GetAvailableTagId(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	int index = params[2];
	std::string tagId = channel->GetAvailableTagId(index);
	pContext->StringToLocal(params[3], params[4], tagId.c_str());
	return 1;
}

static cell_t channel_GetAvailableTagEmoji(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	int index = params[2];
	std::string tagEmoji = channel->GetAvailableTagEmoji(index);
	pContext->StringToLocal(params[3], params[4], tagEmoji.c_str());
	return 1;
}

static cell_t channel_GetAvailableTagModerated(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	int index = params[2];
	return channel->GetAvailableTagModerated(index);
}

static cell_t channel_GetAvailableTagEmojiIsCustom(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	int index = params[2];
	return channel->GetAvailableTagEmojiIsCustom(index);
}

static cell_t channel_CreateForumTag(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	
	char* name;
	char* emoji = nullptr;
	pContext->LocalToString(params[2], &name);
	
	if (params[0] >= 3) {
		pContext->LocalToString(params[3], &emoji);
	}
	
	bool moderated = (params[0] >= 4) ? (params[4] != 0) : false;
	
	return channel->CreateForumTag(name, emoji ? emoji : "", moderated);
}

static cell_t channel_EditForumTag(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	
	char* tag_id;
	char* name;
	char* emoji = nullptr;
	pContext->LocalToString(params[2], &tag_id);
	pContext->LocalToString(params[3], &name);
	
	if (params[0] >= 4) {
		pContext->LocalToString(params[4], &emoji);
	}
	
	bool moderated = (params[0] >= 5) ? (params[5] != 0) : false;
	
	dpp::snowflake tagId = 0;
	try {
		tagId = std::stoull(tag_id);
	} catch (const std::exception&) {
		pContext->ReportError("Invalid tag ID format");
		return 0;
	}
	
	return channel->EditForumTag(tagId, name, emoji ? emoji : "", moderated);
}

static cell_t channel_DeleteForumTag(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	
	char* tag_id;
	pContext->LocalToString(params[2], &tag_id);
	
	dpp::snowflake tagId = 0;
	try {
		tagId = std::stoull(tag_id);
	} catch (const std::exception&) {
		pContext->ReportError("Invalid tag ID format");
		return 0;
	}
	
	return channel->DeleteForumTag(tagId);
}

static cell_t channel_CreateForumThread(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	
	char* name;
	char* message;
	pContext->LocalToString(params[2], &name);
	pContext->LocalToString(params[3], &message);
	
	std::vector<dpp::snowflake> tag_ids;
	
	if (params[0] >= 4 && params[4] != 0) {
		cell_t* tag_array;
		pContext->LocalToPhysAddr(params[4], &tag_array);
		int tag_count = params[5];
		
		for (int i = 0; i < tag_count; i++) {
			char* tag_id;
			pContext->LocalToString(tag_array[i], &tag_id);
			try {
				tag_ids.push_back(std::stoull(tag_id));
			} catch (const std::exception&) {
				continue;
			}
		}
	}
	
	int auto_archive = (params[0] >= 6) ? params[6] : 1440;
	int rate_limit = (params[0] >= 7) ? params[7] : 0;
	
	return channel->CreateForumThread(name, message, tag_ids, auto_archive, rate_limit);
}

static cell_t channel_CreateForumThreadEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	
	char* name;
	char* message;
	pContext->LocalToString(params[2], &name);
	pContext->LocalToString(params[3], &message);
	
	DiscordEmbed* embed = GetEmbedPointer(pContext, params[4]);
	if (!embed) return 0;
	
	std::vector<dpp::snowflake> tag_ids;
	
	if (params[0] >= 5 && params[5] != 0) {
		cell_t* tag_array;
		pContext->LocalToPhysAddr(params[5], &tag_array);
		int tag_count = params[6];
		
		for (int i = 0; i < tag_count; i++) {
			char* tag_id;
			pContext->LocalToString(tag_array[i], &tag_id);
			try {
				tag_ids.push_back(std::stoull(tag_id));
			} catch (const std::exception&) {
				continue;
			}
		}
	}
	
	int auto_archive = (params[0] >= 7) ? params[7] : 1440;
	int rate_limit = (params[0] >= 8) ? params[8] : 0;
	
	return channel->CreateForumThreadEmbed(name, message, embed, tag_ids, auto_archive, rate_limit);
}

static cell_t forumtag_Create(IPluginContext* pContext, const cell_t* params)
{
	char* name;
	char* emoji = nullptr;
	pContext->LocalToString(params[1], &name);
	
	if (params[0] >= 2) {
		pContext->LocalToString(params[2], &emoji);
	}
	
	bool moderated = (params[0] >= 3) ? (params[3] != 0) : false;
	
	DiscordForumTag* pForumTag = new DiscordForumTag(name, emoji ? emoji : "", moderated);
	
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_DiscordForumTagHandle, pForumTag, &sec, nullptr, &err);
	
	if (handle == BAD_HANDLE)
	{
		delete pForumTag;
		pContext->ReportError("Could not create forum tag handle (error %d)", err);
		return BAD_HANDLE;
	}
	
	return handle;
}

static cell_t forumtag_GetId(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	std::string tagId = tag->GetId();
	pContext->StringToLocal(params[2], params[3], tagId.c_str());
	return 1;
}

static cell_t forumtag_GetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	std::string tagName = tag->GetName();
	pContext->StringToLocal(params[2], params[3], tagName.c_str());
	return 1;
}

static cell_t forumtag_SetName(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	char* name;
	pContext->LocalToString(params[2], &name);
	tag->SetName(name);
	return 1;
}

static cell_t forumtag_GetEmoji(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	std::string emoji = tag->GetEmoji();
	pContext->StringToLocal(params[2], params[3], emoji.c_str());
	return 1;
}

static cell_t forumtag_SetEmoji(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	char* emoji;
	pContext->LocalToString(params[2], &emoji);
	tag->SetEmoji(emoji);
	return 1;
}

static cell_t forumtag_GetIsModerated(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	return tag->IsModerated();
}

static cell_t forumtag_SetIsModerated(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	bool moderated = params[2] != 0;
	tag->SetModerated(moderated);
	return 1;
}

static cell_t forumtag_GetEmojiIsCustom(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	return tag->EmojiIsCustom();
}

static cell_t forumtag_ApplyToChannel(IPluginContext* pContext, const cell_t* params)
{
	DiscordForumTag* tag = GetForumTagPointer(pContext, params[1]);
	if (!tag) return 0;
	
	DiscordChannel* channel = GetChannelPointer(pContext, params[2]);
	if (!channel) return 0;
	
	// Add the tag to the channel's available_tags
	// This would need to be implemented in the DiscordChannel class
	// For now, we'll just return true as this is a complex operation
	// that would require modifying the channel and syncing with Discord
	return 1;
}

static cell_t channel_GetIconUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordChannel* channel = GetChannelPointer(pContext, params[1]);
	if (!channel) return 0;
	int size = (params[0] >= 3) ? params[3] : 0;
	std::string iconUrl = channel->GetIconUrl(size);
	pContext->StringToLocal(params[2], params[3], iconUrl.c_str());
	return 1;
}

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

	DiscordWebhook* pDiscordWebhook = new DiscordWebhook(webhook, nullptr);

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

static cell_t webhook_CreateWebhookFromIdToken(IPluginContext* pContext, const cell_t* params)
{
	char* webhook_id;
	pContext->LocalToString(params[1], &webhook_id);
	
	char* webhook_token;
	pContext->LocalToString(params[2], &webhook_token);

	try {
		dpp::snowflake id = std::stoull(webhook_id);
		DiscordWebhook* pDiscordWebhook = new DiscordWebhook(id, std::string(webhook_token), nullptr);

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
	catch (const std::exception& e) {
		pContext->ReportError("Invalid webhook ID format: %s", webhook_id);
		return BAD_HANDLE;
	}
}

static cell_t webhook_FetchWebhook(IPluginContext* pContext, const cell_t* params)
{
	DiscordClient* discord = GetDiscordPointer(pContext, params[1]);
	if (!discord) {
		return 0;
	}

	char* webhookId;
	pContext->LocalToString(params[2], &webhookId);

	IPluginFunction* callback = pContext->GetFunctionById(params[3]);
	if (!callback) {
		pContext->ReportError("Invalid callback function");
		return 0;
	}

	cell_t data = params[4];

	try {
		dpp::snowflake webhookFlake = std::stoull(webhookId);

		discord->GetCluster()->get_webhook(webhookFlake, [discord, callback, data](const dpp::confirmation_callback_t& confirmation) {
			g_TaskQueue.Push([discord, callback, data, confirmation]() {
				if (confirmation.is_error()) {
					return;
				}

				dpp::webhook webhook_obj = confirmation.get<dpp::webhook>();
				DiscordWebhook* pDiscordWebhook = new DiscordWebhook(webhook_obj, discord);

				HandleError err;
				HandleSecurity sec(nullptr, myself->GetIdentity());
				Handle_t handle = handlesys->CreateHandleEx(g_DiscordWebhookHandle, pDiscordWebhook, &sec, nullptr, &err);

				if (handle == BAD_HANDLE) {
					delete pDiscordWebhook;
					return;
				}

				callback->PushCell(discord->GetHandle());
				callback->PushCell(handle);
				callback->PushCell(data);
				callback->Execute(nullptr);
			});
		});

		return 1;
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid webhook ID format: %s", webhookId);
		return 0;
	}
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

	Handle_t handle = webhook->GetUserHandle();
	
	if (handle == BAD_HANDLE) {
		pContext->ReportError("Could not create user handle");
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

static cell_t webhook_GetType(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	return static_cast<cell_t>(webhook->GetType());
}

static cell_t webhook_GetGuildId(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetGuildId().c_str());
	return 1;
}

static cell_t webhook_GetChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetChannelId().c_str());
	return 1;
}

static cell_t webhook_GetToken(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetToken());
	return 1;
}

static cell_t webhook_GetApplicationId(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetApplicationId().c_str());
	return 1;
}

static cell_t webhook_GetSourceGuildId(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetSourceGuildId().c_str());
	return 1;
}

static cell_t webhook_GetSourceChannelId(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetSourceChannelId().c_str());
	return 1;
}

static cell_t webhook_GetUrl(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetUrl());
	return 1;
}

static cell_t webhook_GetImageData(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], webhook->GetImageData().c_str());
	return 1;
}

static cell_t webhook_Modify(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	return webhook->Modify();
}

static cell_t webhook_Delete(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	return webhook->Delete();
}

static cell_t webhook_Execute(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	char* message;
	pContext->LocalToString(params[2], &message);

	int allowed_mentions_mask = params[3];

	cell_t* users_array;
	cell_t* roles_array;
	pContext->LocalToPhysAddr(params[4], &users_array);
	pContext->LocalToPhysAddr(params[6], &roles_array);

	std::vector<dpp::snowflake> users(params[5]);
	std::vector<dpp::snowflake> roles(params[7]);

	for (size_t i = 0; i < users.size(); i++) {
		char* str;
		pContext->LocalToString(users_array[i], &str);
		try {
			users[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue;
		}
	}

	for (size_t i = 0; i < roles.size(); i++) {
		char* str;
		pContext->LocalToString(roles_array[i], &str);
		try {
			roles[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue;
		}
	}

	return webhook->Execute(message, allowed_mentions_mask, users, roles);
}

static cell_t webhook_ExecuteEmbed(IPluginContext* pContext, const cell_t* params)
{
	DiscordWebhook* webhook = GetWebhookPointer(pContext, params[1]);
	if (!webhook) {
		return 0;
	}

	char* message;
	pContext->LocalToString(params[2], &message);

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	DiscordEmbed* embed;
	if ((err = handlesys->ReadHandle(params[3], g_DiscordEmbedHandle, &sec, (void**)&embed)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Discord embed handle %x (error %d)", params[3], err);
	}

	int allowed_mentions_mask = params[4];

	cell_t* users_array;
	cell_t* roles_array;
	pContext->LocalToPhysAddr(params[5], &users_array);
	pContext->LocalToPhysAddr(params[7], &roles_array);

	std::vector<dpp::snowflake> users(params[6]);
	std::vector<dpp::snowflake> roles(params[8]);

	for (size_t i = 0; i < users.size(); i++) {
		char* str;
		pContext->LocalToString(users_array[i], &str);
		try {
			users[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue;
		}
	}

	for (size_t i = 0; i < roles.size(); i++) {
		char* str;
		pContext->LocalToString(roles_array[i], &str);
		try {
			roles[i] = std::stoull(str);
		}
		catch (const std::exception& e) {
			continue;
		}
	}

	return webhook->ExecuteEmbed(message, embed, allowed_mentions_mask, users, roles);
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

	return static_cast<cell_t>(value);
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

	return value;
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

	Handle_t handle = interaction->GetUserHandle();
	
	if (handle == BAD_HANDLE) {
		pContext->ReportError("Could not create user handle");
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

	Handle_t handle = interaction->GetUserHandle();
	
	if (handle == BAD_HANDLE) {
		pContext->ReportError("Could not create user handle");
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
	return static_cast<cell_t>(value);
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

	if (!interaction->m_client) {
		return 0;
	}

	interaction->m_client->CreateAutocompleteResponse(interaction->m_command.id, interaction->m_command.token, interaction->m_response);
	return 1;
}

void DiscordClient::CreateAutocompleteResponse(dpp::snowflake id, const std::string &token, const dpp::interaction_response &response)
{
	m_cluster->interaction_response_create(id, token, response);
}

bool DiscordClient::EditMessage(dpp::snowflake channel_id, dpp::snowflake message_id, const char* content)
{
	if (!IsRunning()) {
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
	if (!IsRunning()) {
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
	if (!IsRunning()) {
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
		return discord->EditMessage(channel, message, content);
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
		return discord->EditMessageEmbed(channel, message, content, embed);
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
		return discord->DeleteMessage(channel, message);
	}
	catch (const std::exception& e) {
		pContext->ReportError("Invalid ID format");
		return 0;
	}
}

bool DiscordClient::RegisterSlashCommandObject(dpp::snowflake guild_id, const dpp::slashcommand& command)
{
	if (!IsRunning()) {
		return false;
	}

	try {
		m_cluster->guild_command_create(command, guild_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to register slash command object: %s", e.what());
		return false;
	}
}

bool DiscordClient::RegisterGlobalSlashCommandObject(const dpp::slashcommand& command)
{
	if (!IsRunning()) {
		return false;
	}

	try {
		m_cluster->global_command_create(command);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to register global slash command object: %s", e.what());
		return false;
	}
}

bool DiscordClient::DeleteGuildCommand(dpp::snowflake guild_id, dpp::snowflake command_id)
{
	if (!IsRunning()) {
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
	if (!IsRunning()) {
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
	if (!IsRunning()) {
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
	if (!IsRunning()) {
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
		return discord->DeleteGuildCommand(guild, command);
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
		return discord->DeleteGlobalCommand(command);
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
		return discord->BulkDeleteGuildCommands(guild);
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
		return discord->BulkDeleteGlobalCommands();
	}
	catch (const std::exception& e) {
		pContext->ReportError("Unable to bulk delete global commands: %s", e.what());
		return 0;
	}
}

bool DiscordClient::ModifyChannel(dpp::snowflake channel_id, const std::string& name, const std::string& topic, uint16_t position, bool nsfw, uint16_t rate_limit, uint16_t bitrate, uint8_t user_limit, dpp::snowflake parent_id) {
	if (!IsRunning()) return false;
	
	try {
		dpp::channel ch;
		ch.id = channel_id;
		if (!name.empty()) ch.name = name;
		if (!topic.empty()) ch.topic = topic;
		if (position > 0) ch.position = position;
		ch.set_nsfw(nsfw);
		if (rate_limit > 0) ch.rate_limit_per_user = rate_limit;
		if (bitrate > 0) ch.bitrate = bitrate;
		if (user_limit > 0) ch.user_limit = user_limit;
		if (parent_id > 0) ch.parent_id = parent_id;
		m_cluster->channel_edit(ch);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to modify channel: %s", e.what());
		return false;
	}
}

bool DiscordClient::DeleteChannel(dpp::snowflake channel_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->channel_delete(channel_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to delete channel: %s", e.what());
		return false;
	}
}

bool DiscordClient::PinMessage(dpp::snowflake channel_id, dpp::snowflake message_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->message_pin(channel_id, message_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to pin message: %s", e.what());
		return false;
	}
}

bool DiscordClient::UnpinMessage(dpp::snowflake channel_id, dpp::snowflake message_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->message_unpin(channel_id, message_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to unpin message: %s", e.what());
		return false;
	}
}

bool DiscordClient::AddReaction(dpp::snowflake channel_id, dpp::snowflake message_id, const char* emoji) {
	if (!IsRunning() || !emoji) return false;
	
	try {
		m_cluster->message_add_reaction(message_id, channel_id, emoji);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to add reaction: %s", e.what());
		return false;
	}
}

bool DiscordClient::RemoveReaction(dpp::snowflake channel_id, dpp::snowflake message_id, const char* emoji) {
	if (!IsRunning() || !emoji) return false;
	
	try {
		m_cluster->message_delete_reaction(message_id, channel_id, 0, emoji);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to remove reaction: %s", e.what());
		return false;
	}
}

bool DiscordClient::RemoveAllReactions(dpp::snowflake channel_id, dpp::snowflake message_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->message_delete_all_reactions(message_id, channel_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to remove all reactions: %s", e.what());
		return false;
	}
}

bool DiscordClient::ModifyRole(dpp::snowflake guild_id, dpp::snowflake role_id, const std::string& name, uint32_t color, bool hoist, bool mentionable, uint64_t permissions) {
	if (!IsRunning()) return false;
	
	try {
		dpp::role role;
		role.id = role_id;
		role.guild_id = guild_id;
		if (!name.empty()) role.name = name;
		if (color > 0) role.colour = color;
		role.flags = (hoist ? dpp::r_hoist : 0) | (mentionable ? dpp::r_mentionable : 0);
		if (permissions > 0) role.permissions = permissions;
		m_cluster->role_edit(role);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to modify role: %s", e.what());
		return false;
	}
}

bool DiscordClient::DeleteRole(dpp::snowflake guild_id, dpp::snowflake role_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->role_delete(guild_id, role_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to delete role: %s", e.what());
		return false;
	}
}

bool DiscordClient::ModifyMember(dpp::snowflake guild_id, dpp::snowflake user_id, const std::string& nickname) {
	if (!IsRunning()) return false;
	
	try {
		dpp::guild_member member;
		member.guild_id = guild_id;
		member.user_id = user_id;
		if (!nickname.empty()) member.set_nickname(nickname);
		m_cluster->guild_edit_member(member);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to modify member: %s", e.what());
		return false;
	}
}

bool DiscordClient::ModifyWebhook(dpp::snowflake webhook_id, const std::string& name, const std::string& avatar_url) {
	if (!IsRunning()) return false;
	
	try {
		dpp::webhook webhook;
		webhook.id = webhook_id;
		if (!name.empty()) webhook.name = name;
		if (!avatar_url.empty()) webhook.avatar_url = avatar_url;
		m_cluster->edit_webhook(webhook);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to modify webhook: %s", e.what());
		return false;
	}
}

bool DiscordClient::DeleteWebhook(dpp::snowflake webhook_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->delete_webhook(webhook_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to delete webhook: %s", e.what());
		return false;
	}
}

bool DiscordClient::LeaveGuild(dpp::snowflake guild_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->guild_delete(guild_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to leave guild: %s", e.what());
		return false;
	}
}

bool DiscordClient::CreateChannel(dpp::snowflake guild_id, const char* name, dpp::channel_type type, const char* topic, dpp::snowflake parent_id, IForward* callback_forward, cell_t data) {
	if (!IsRunning() || !name) return false;
	
	try {
		dpp::channel channel;
		channel.guild_id = guild_id;
		channel.name = name;
		channel.set_type(type);
		if (topic && strlen(topic) > 0) channel.topic = topic;
		if (parent_id > 0) channel.parent_id = parent_id;
		
		if (callback_forward) {
			m_cluster->channel_create(channel, [this, forward = callback_forward, value = data](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					smutils->LogError(myself, "Failed to create channel: %s", callback.get_error().message.c_str());
					forwards->ReleaseForward(forward);
					return;
				}
				auto ch = callback.get<dpp::channel>();
				
				g_TaskQueue.Push([this, forward, ch = callback.get<dpp::channel>(), value]() {
					DiscordChannel* discord_channel = new DiscordChannel(ch, this);
					if (forward && forward->GetFunctionCount() == 0) {
						delete discord_channel;
						forwards->ReleaseForward(forward);
						return;
					}
					
					HandleError err;
					HandleSecurity sec(myself->GetIdentity(), myself->GetIdentity());
					Handle_t channelHandle = handlesys->CreateHandleEx(g_DiscordChannelHandle, discord_channel, &sec, nullptr, &err);
					if (channelHandle == BAD_HANDLE) {
						smutils->LogError(myself, "Could not create channel handle (error %d)", err);
						delete discord_channel;
						forwards->ReleaseForward(forward);
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
		} else {
			m_cluster->channel_create(channel);
		}
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to create channel: %s", e.what());
		return false;
	}
}

bool DiscordClient::CreateRole(dpp::snowflake guild_id, const char* name, uint32_t color, bool hoist, bool mentionable, uint64_t permissions, IForward* callback_forward, cell_t data) {
	if (!IsRunning() || !name) return false;
	
	try {
		dpp::role role;
		role.guild_id = guild_id;
		role.name = name;
		role.colour = color;
		role.flags = (hoist ? dpp::r_hoist : 0) | (mentionable ? dpp::r_mentionable : 0);
		role.permissions = permissions;
		
		if (callback_forward) {
			m_cluster->role_create(role, [this, forward = callback_forward, value = data](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					smutils->LogError(myself, "Failed to create role: %s", callback.get_error().message.c_str());
					forwards->ReleaseForward(forward);
					return;
				}
				auto r = callback.get<dpp::role>();
				
				g_TaskQueue.Push([this, &forward, role_id = r.id, value = value]() {
					if (forward && forward->GetFunctionCount() == 0) {
						return;
					}
					
					forward->PushCell(m_discord_handle);
					forward->PushString(std::to_string(role_id).c_str());
					forward->PushCell(value);
					forward->Execute(nullptr);
					
					forwards->ReleaseForward(forward);
				});
			});
		} else {
			m_cluster->role_create(role);
		}
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to create role: %s", e.what());
		return false;
	}
}

bool DiscordClient::AddMemberRole(dpp::snowflake guild_id, dpp::snowflake user_id, dpp::snowflake role_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->guild_member_add_role(guild_id, user_id, role_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to add member role: %s", e.what());
		return false;
	}
}

bool DiscordClient::RemoveMemberRole(dpp::snowflake guild_id, dpp::snowflake user_id, dpp::snowflake role_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->guild_member_remove_role(guild_id, user_id, role_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to remove member role: %s", e.what());
		return false;
	}
}

bool DiscordClient::KickMember(dpp::snowflake guild_id, dpp::snowflake user_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->guild_member_kick(guild_id, user_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to kick member: %s", e.what());
		return false;
	}
}

bool DiscordClient::BanMember(dpp::snowflake guild_id, dpp::snowflake user_id, const char* reason, int delete_message_days) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->guild_ban_add(guild_id, user_id, delete_message_days);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to ban member: %s", e.what());
		return false;
	}
}

bool DiscordClient::UnbanMember(dpp::snowflake guild_id, dpp::snowflake user_id) {
	if (!IsRunning()) return false;
	
	try {
		m_cluster->guild_ban_delete(guild_id, user_id);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to unban member: %s", e.what());
		return false;
	}
}

bool DiscordClient::TimeoutMember(dpp::snowflake guild_id, dpp::snowflake user_id, time_t timeout_until) {
	if (!IsRunning()) return false;
	
	try {
		dpp::guild_member member;
		member.guild_id = guild_id;
		member.user_id = user_id;
		member.communication_disabled_until = timeout_until;
		m_cluster->guild_edit_member(member);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to timeout member: %s", e.what());
		return false;
	}
}

bool DiscordClient::RemoveTimeout(dpp::snowflake guild_id, dpp::snowflake user_id) {
	if (!IsRunning()) return false;
	
	try {
		dpp::guild_member member;
		member.guild_id = guild_id;
		member.user_id = user_id;
		member.communication_disabled_until = 0;
		m_cluster->guild_edit_member(member);
		return true;
	}
	catch (const std::exception& e) {
		smutils->LogError(myself, "Failed to remove timeout: %s", e.what());
		return false;
	}
}

bool DiscordClient::IsRunning() const
{
	if (!m_cluster) {
		return false;
	}
	
	try {
		const auto& shards = m_cluster->get_shards();
		for (const auto& shard_pair : shards) {
			if (shard_pair.second && shard_pair.second->is_connected()) {
				return true;
			}
		}
		return false;
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed get running state: %s", e.what());
		return false;
	}
}

uint64_t DiscordClient::GetUptime() const
{
	if (!IsRunning()) {
		return 0;
	}
	
	try {
		return m_cluster->uptime().to_secs();
	} catch (const std::exception& e) {
		smutils->LogError(myself, "Failed get bot uptime: %s", e.what());
		return 0;
	}
}

Handle_t DiscordMessage::GetAuthorHandle() const
{
	if (m_authorHandle != BAD_HANDLE) {
		return m_authorHandle;
	}
	
	DiscordUser* pDiscordUser = this->GetAuthor();
	
	HandleError err;
	HandleSecurity sec(nullptr, myself->GetIdentity());
	m_authorHandle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);
	
	if (m_authorHandle == BAD_HANDLE) {
		delete pDiscordUser;
		return BAD_HANDLE;
	}
	
	return m_authorHandle;
}

Handle_t DiscordInteraction::GetUserHandle() const
{
	if (m_userHandle != BAD_HANDLE) {
		return m_userHandle;
	}
	
	DiscordUser* pDiscordUser = this->GetUser();
	
	HandleError err;
	HandleSecurity sec(nullptr, myself->GetIdentity());
	m_userHandle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);
	
	if (m_userHandle == BAD_HANDLE) {
		delete pDiscordUser;
		return BAD_HANDLE;
	}
	
	return m_userHandle;
}

Handle_t DiscordAutocompleteInteraction::GetUserHandle() const
{
	if (m_userHandle != BAD_HANDLE) {
		return m_userHandle;
	}
	
	DiscordUser* pDiscordUser = this->GetUser();
	
	HandleError err;
	HandleSecurity sec(nullptr, myself->GetIdentity());
	m_userHandle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);
	
	if (m_userHandle == BAD_HANDLE) {
		delete pDiscordUser;
		return BAD_HANDLE;
	}
	
	return m_userHandle;
}

Handle_t DiscordWebhook::GetUserHandle() const
{
	if (m_userHandle != BAD_HANDLE) {
		return m_userHandle;
	}
	
	DiscordUser* pDiscordUser = this->GetUser();
	
	HandleError err;
	HandleSecurity sec(nullptr, myself->GetIdentity());
	m_userHandle = handlesys->CreateHandleEx(g_DiscordUserHandle, pDiscordUser, &sec, nullptr, &err);
	
	if (m_userHandle == BAD_HANDLE) {
		delete pDiscordUser;
		return BAD_HANDLE;
	}
	
	return m_userHandle;
}

const sp_nativeinfo_t discord_natives[] = {
	// Discord
	{"Discord.Discord", discord_CreateClient},
	{"Discord.Start", discord_Start},
	{"Discord.Stop", discord_Stop},
	{"Discord.GetBotId", discord_GetBotId},
	{"Discord.GetBotName", discord_GetBotName},
	{"Discord.GetBotDiscriminator", discord_GetBotDiscriminator},
	{"Discord.GetBotAvatarUrl", discord_GetBotAvatarUrl},
	{"Discord.Uptime.get", discord_GetUptime},
	{"Discord.SetPresence", discord_SetPresence},
	{"Discord.CreateWebhook", discord_CreateWebhook},
	{"Discord.GetWebhook", discord_GetWebhook},
	{"Discord.GetChannelWebhooks", discord_GetChannelWebhooks},
	{"Discord.IsRunning.get", discord_IsRunning},
	{"Discord.EditMessage", discord_EditMessage},
	{"Discord.EditMessageEmbed", discord_EditMessageEmbed},
	{"Discord.DeleteMessage", discord_DeleteMessage},
	{"Discord.DeleteGuildCommand", discord_DeleteGuildCommand},
	{"Discord.DeleteGlobalCommand", discord_DeleteGlobalCommand},
	{"Discord.BulkDeleteGuildCommands", discord_BulkDeleteGuildCommands},
	{"Discord.BulkDeleteGlobalCommands", discord_BulkDeleteGlobalCommands},

	// User
	{"DiscordUser.DiscordUser", user_CreateFromId},
	{"DiscordUser.FetchUser", user_FetchUser},
	{"DiscordUser.GetId", user_GetId},
	{"DiscordUser.GetUserName", user_GetUserName},
	{"DiscordUser.GetDiscriminator.get", user_GetDiscriminator},
	{"DiscordUser.GetGlobalName", user_GetGlobalName},
	{"DiscordUser.GetAvatarUrl", user_GetAvatarUrl},
	{"DiscordUser.GetDefaultAvatarUrl", user_GetDefaultAvatarUrl},
	{"DiscordUser.GetAvatarDecorationUrl", user_GetAvatarDecorationUrl},
	{"DiscordUser.GetMention", user_GetMention},
	{"DiscordUser.GetUrl", user_GetUrl},
	{"DiscordUser.GetFormattedUsername", user_GetFormattedUsername},
	{"DiscordUser.Flags.get", user_GetFlags},
	{"DiscordUser.IsSystem.get", user_IsSystem},
	{"DiscordUser.IsMfaEnabled.get", user_IsMfaEnabled},
	{"DiscordUser.IsVerified.get", user_IsVerified},
	{"DiscordUser.HasNitroFull.get", user_HasNitroFull},
	{"DiscordUser.HasNitroClassic.get", user_HasNitroClassic},
	{"DiscordUser.HasNitroBasic.get", user_HasNitroBasic},
	{"DiscordUser.IsDiscordEmployee.get", user_IsDiscordEmployee},
	{"DiscordUser.IsPartneredOwner.get", user_IsPartneredOwner},
	{"DiscordUser.HasHypesquadEvents.get", user_HasHypesquadEvents},
	{"DiscordUser.IsBughunter1.get", user_IsBughunter1},
	{"DiscordUser.IsHouseBravery.get", user_IsHouseBravery},
	{"DiscordUser.IsHouseBrilliance.get", user_IsHouseBrilliance},
	{"DiscordUser.IsHouseBalance.get", user_IsHouseBalance},
	{"DiscordUser.IsEarlySupporter.get", user_IsEarlySupporter},
	{"DiscordUser.IsTeamUser.get", user_IsTeamUser},
	{"DiscordUser.IsBughunter2.get", user_IsBughunter2},
	{"DiscordUser.IsVerifiedBot.get", user_IsVerifiedBot},
	{"DiscordUser.IsVerifiedBotDev.get", user_IsVerifiedBotDev},
	{"DiscordUser.IsCertifiedModerator.get", user_IsCertifiedModerator},
	{"DiscordUser.IsBotHttpInteractions.get", user_IsBotHttpInteractions},
	{"DiscordUser.HasAnimatedIcon.get", user_HasAnimatedIcon},
	{"DiscordUser.IsActiveDeveloper.get", user_IsActiveDeveloper},
	{"DiscordUser.IsBot.get", user_IsBot},
	{"DiscordUser.HasGuildMember.get", user_HasGuildMember},
	{"DiscordUser.GetNickName", user_GetNickName},
	{"DiscordUser.GetJoinedAt", user_GetJoinedAt},
	{"DiscordUser.IsPending.get", user_IsPending},
	{"DiscordUser.HasPermission", user_HasPermission},
	{"DiscordUser.GetPermissions", user_GetPermissions},
	{"DiscordUser.HasPermissionInChannel", user_HasPermissionInChannel},
	{"DiscordUser.GetPermissionsInChannel", user_GetPermissionsInChannel},
	{"DiscordUser.GetRoles", user_GetRoles},
	{"DiscordUser.HasRole", user_HasRole},
	{"DiscordUser.GetHighestRole", user_GetHighestRole},
	{"DiscordUser.GetRoleName", user_GetRoleName},
	{"DiscordUser.GetRoleNames", user_GetRoleNames},
	{"DiscordUser.SetNickName", user_SetNickName},
	{"DiscordUser.AddRole", user_AddRole},
	{"DiscordUser.RemoveRole", user_RemoveRole},
	{"DiscordUser.KickFromGuild", user_KickFromGuild},
	{"DiscordUser.BanFromGuild", user_BanFromGuild},
	{"DiscordUser.UnbanFromGuild", user_UnbanFromGuild},
	{"DiscordUser.SetTimeout", user_SetTimeout},
	{"DiscordUser.RemoveTimeout", user_RemoveTimeout},

	// Guild
	{"DiscordGuild.FetchGuild", guild_FetchGuild},
	{"DiscordGuild.GetId", guild_GetId},
	{"DiscordGuild.GetName", guild_GetName},
	{"DiscordGuild.GetDescription", guild_GetDescription},
	{"DiscordGuild.GetOwnerId", guild_GetOwnerId},
	{"DiscordGuild.MemberCount.get", guild_GetMemberCount},
	{"DiscordGuild.VerificationLevel.get", guild_GetVerificationLevel},
	{"DiscordGuild.PremiumTier.get", guild_GetPremiumTier},
	{"DiscordGuild.IsLarge.get", guild_IsLarge},
	{"DiscordGuild.IsVerified.get", guild_IsVerified},
	{"DiscordGuild.IsPartnered.get", guild_IsPartnered},
	{"DiscordGuild.GetIconUrl", guild_GetIconUrl},
	{"DiscordGuild.GetBannerUrl", guild_GetBannerUrl},
	
	// Additional DiscordGuild methods
	{"DiscordGuild.GetSplashUrl", guild_GetSplashUrl},
	{"DiscordGuild.GetDiscoverySplashUrl", guild_GetDiscoverySplashUrl},
	{"DiscordGuild.GetVanityUrlCode", guild_GetVanityUrlCode},
	{"DiscordGuild.MaxMembers.get", guild_GetMaxMembers},
	{"DiscordGuild.MaxPresences.get", guild_GetMaxPresences},
	{"DiscordGuild.PremiumSubscriptionCount.get", guild_GetPremiumSubscriptionCount},
	{"DiscordGuild.ExplicitContentFilter.get", guild_GetExplicitContentFilter},
	{"DiscordGuild.MfaLevel.get", guild_GetMfaLevel},
	{"DiscordGuild.NsfwLevel.get", guild_GetNsfwLevel},
	{"DiscordGuild.AfkTimeout.get", guild_GetAfkTimeout},
	{"DiscordGuild.DefaultMessageNotifications.get", guild_GetDefaultMessageNotifications},
	{"DiscordGuild.ShardId.get", guild_GetShardId},
	{"DiscordGuild.Flags.get", guild_GetFlags},
	{"DiscordGuild.FlagsExtra.get", guild_GetFlagsExtra},
	
	// Channel ID getters
	{"DiscordGuild.GetAfkChannelId", guild_GetAfkChannelId},
	{"DiscordGuild.GetSystemChannelId", guild_GetSystemChannelId},
	{"DiscordGuild.GetRulesChannelId", guild_GetRulesChannelId},
	{"DiscordGuild.GetPublicUpdatesChannelId", guild_GetPublicUpdatesChannelId},
	{"DiscordGuild.GetSafetyAlertsChannelId", guild_GetSafetyAlertsChannelId},
	
	// Additional guild flag properties
	{"DiscordGuild.IsUnavailable.get", guild_IsUnavailable},
	{"DiscordGuild.WidgetEnabled.get", guild_WidgetEnabled},
	{"DiscordGuild.HasInviteSplash.get", guild_HasInviteSplash},
	{"DiscordGuild.HasVipRegions.get", guild_HasVipRegions},
	{"DiscordGuild.HasVanityUrl.get", guild_HasVanityUrl},
	{"DiscordGuild.IsCommunity.get", guild_IsCommunity},
	{"DiscordGuild.HasRoleSubscriptions.get", guild_HasRoleSubscriptions},
	{"DiscordGuild.HasNews.get", guild_HasNews},
	{"DiscordGuild.IsDiscoverable.get", guild_IsDiscoverable},
	{"DiscordGuild.IsFeatureable.get", guild_IsFeatureable},
	{"DiscordGuild.HasAnimatedIcon.get", guild_HasAnimatedIcon},
	{"DiscordGuild.HasBanner.get", guild_HasBanner},
	{"DiscordGuild.IsWelcomeScreenEnabled.get", guild_IsWelcomeScreenEnabled},
	{"DiscordGuild.HasMemberVerificationGate.get", guild_HasMemberVerificationGate},
	{"DiscordGuild.IsPreviewEnabled.get", guild_IsPreviewEnabled},
	{"DiscordGuild.HasAnimatedIconHash.get", guild_HasAnimatedIconHash},
	{"DiscordGuild.HasAnimatedBannerHash.get", guild_HasAnimatedBannerHash},
	{"DiscordGuild.HasMonetizationEnabled.get", guild_HasMonetizationEnabled},
	{"DiscordGuild.HasMoreStickers.get", guild_HasMoreStickers},
	{"DiscordGuild.HasCreatorStorePage.get", guild_HasCreatorStorePage},
	{"DiscordGuild.HasRoleIcons.get", guild_HasRoleIcons},
	{"DiscordGuild.HasSevenDayThreadArchive.get", guild_HasSevenDayThreadArchive},
	{"DiscordGuild.HasThreeDayThreadArchive.get", guild_HasThreeDayThreadArchive},
	{"DiscordGuild.HasTicketedEvents.get", guild_HasTicketedEvents},
	{"DiscordGuild.HasChannelBanners.get", guild_HasChannelBanners},
	{"DiscordGuild.HasPremiumProgressBarEnabled.get", guild_HasPremiumProgressBarEnabled},
	{"DiscordGuild.HasAnimatedBanner.get", guild_HasAnimatedBanner},
	{"DiscordGuild.HasAutoModeration.get", guild_HasAutoModeration},
	{"DiscordGuild.HasInvitesDisabled.get", guild_HasInvitesDisabled},
	{"DiscordGuild.HasSupportServer.get", guild_HasSupportServer},
	{"DiscordGuild.HasRoleSubscriptionsAvailableForPurchase.get", guild_HasRoleSubscriptionsAvailableForPurchase},
	{"DiscordGuild.HasRaidAlertsDisabled.get", guild_HasRaidAlertsDisabled},
	
	// Notification settings
	{"DiscordGuild.NoJoinNotifications.get", guild_NoJoinNotifications},
	{"DiscordGuild.NoBoostNotifications.get", guild_NoBoostNotifications},
	{"DiscordGuild.NoSetupTips.get", guild_NoSetupTips},
	{"DiscordGuild.NoStickerGreeting.get", guild_NoStickerGreeting},
	{"DiscordGuild.NoRoleSubscriptionNotifications.get", guild_NoRoleSubscriptionNotifications},
	{"DiscordGuild.NoRoleSubscriptionNotificationReplies.get", guild_NoRoleSubscriptionNotificationReplies},
	{"DiscordGuild.GetBasePermissions", guild_GetBasePermissions},
	{"DiscordGuild.GetPermissionsInChannel", guild_GetPermissionsInChannel},
	{"DiscordGuild.HasPermission", guild_HasPermission},
	{"DiscordGuild.HasPermissionInChannel", guild_HasPermissionInChannel},
	{"DiscordGuild.Modify", guild_Modify},
	
	// Guild collection count properties
	{"DiscordGuild.RoleCount.get", guild_GetRoleCount},
	{"DiscordGuild.ChannelCount.get", guild_GetChannelCount},
	{"DiscordGuild.ThreadCount.get", guild_GetThreadCount},
	{"DiscordGuild.EmojiCount.get", guild_GetEmojiCount},
	{"DiscordGuild.VoiceMemberCount.get", guild_GetVoiceMemberCount},

	// SlashCommand
	{"DiscordSlashCommand.DiscordSlashCommand", slashcommand_CreateSlashCommand},
	{"DiscordSlashCommand.FromGlobalCommand", slashcommand_FromGlobalCommand},
	{"DiscordSlashCommand.FromGuildCommand", slashcommand_FromGuildCommand},
	{"DiscordSlashCommand.SetName", slashcommand_SetName},
	{"DiscordSlashCommand.SetDescription", slashcommand_SetDescription},
	{"DiscordSlashCommand.SetDefaultPermissions", slashcommand_SetDefaultPermissions},
	{"DiscordSlashCommand.GetName", slashcommand_GetName},
	{"DiscordSlashCommand.GetDescription", slashcommand_GetDescription},
	{"DiscordSlashCommand.GetDefaultPermissions", slashcommand_GetDefaultPermissions},
	{"DiscordSlashCommand.AddOption", slashcommand_AddOption},
	{"DiscordSlashCommand.AddChoiceOption", slashcommand_AddChoiceOption},
	{"DiscordSlashCommand.AddStringChoice", slashcommand_AddStringChoice},
	{"DiscordSlashCommand.AddIntChoice", slashcommand_AddIntChoice},
	{"DiscordSlashCommand.AddFloatChoice", slashcommand_AddFloatChoice},
	{"DiscordSlashCommand.RegisterToGuild", slashcommand_RegisterToGuild},
	{"DiscordSlashCommand.RegisterGlobally", slashcommand_RegisterGlobally},
	{"DiscordSlashCommand.Update", slashcommand_Update},
	{"DiscordSlashCommand.Delete", slashcommand_Delete},
	{"DiscordSlashCommand.AddPermissionOverride", slashcommand_AddPermissionOverride},
	{"DiscordSlashCommand.RemovePermissionOverride", slashcommand_RemovePermissionOverride},
	{"DiscordSlashCommand.ClearPermissionOverrides", slashcommand_ClearPermissionOverrides},
	{"DiscordSlashCommand.PermissionOverrideCount.get", slashcommand_GetPermissionOverrideCount},
	{"DiscordSlashCommand.GetPermissionOverride", slashcommand_GetPermissionOverride},
	{"DiscordSlashCommand.ApplyPermissionOverrides", slashcommand_ApplyPermissionOverrides},
	
	// New slash command advanced functionality
	{"DiscordSlashCommand.ContextMenuType.get", slashcommand_GetContextMenuType},
	{"DiscordSlashCommand.ContextMenuType.set", slashcommand_SetContextMenuType},
	{"DiscordSlashCommand.IsNSFW.get", slashcommand_GetNSFW},
	{"DiscordSlashCommand.IsNSFW.set", slashcommand_SetNSFW},
	{"DiscordSlashCommand.DMPermission.get", slashcommand_GetDMPermission},
	{"DiscordSlashCommand.DMPermission.set", slashcommand_SetDMPermission},
	{"DiscordSlashCommand.AddLocalization", slashcommand_AddLocalization},
	{"DiscordSlashCommand.SetInteractionContexts", slashcommand_SetInteractionContexts},
	{"DiscordSlashCommand.SetIntegrationTypes", slashcommand_SetIntegrationTypes},
	{"DiscordSlashCommand.SetOptionMinValue", slashcommand_SetOptionMinValue},
	{"DiscordSlashCommand.SetOptionMaxValue", slashcommand_SetOptionMaxValue},
	{"DiscordSlashCommand.SetOptionMinLength", slashcommand_SetOptionMinLength},
	{"DiscordSlashCommand.SetOptionMaxLength", slashcommand_SetOptionMaxLength},
	{"DiscordSlashCommand.AddOptionChannelType", slashcommand_AddOptionChannelType},
	{"DiscordSlashCommand.GetMention", slashcommand_GetMention},
	{"DiscordSlashCommand.GetCommandId", slashcommand_GetCommandId},
	{"DiscordSlashCommand.GetGuildId", slashcommand_GetGuildId},

	// Message
	{"DiscordMessage.DiscordMessage", message_CreateFromId},
	{"DiscordMessage.FetchMessage", message_FetchMessage},
	{"DiscordMessage.GetContent", message_GetContent},
	{"DiscordMessage.ContentLength.get", message_GetContentLength},
	{"DiscordMessage.GetMessageId", message_GetMessageId},
	{"DiscordMessage.GetChannelId", message_GetChannelId},
	{"DiscordMessage.GetGuildId", message_GetGuildId},
	{"DiscordMessage.Author.get", message_GetAuthor},
	{"DiscordMessage.GetAuthorNickname", message_GetAuthorNickname},
	{"DiscordMessage.Type.get", message_GetType},
	{"DiscordMessage.IsPinned.get", message_IsPinned},
	{"DiscordMessage.IsTTS.get", message_IsTTS},
	{"DiscordMessage.IsMentionEveryone.get", message_IsMentionEveryone},
	{"DiscordMessage.IsBot.get", message_IsBot},
	{"DiscordMessage.Edit", message_Edit},
	{"DiscordMessage.EditEmbed", message_EditEmbed},
	{"DiscordMessage.Delete", message_Delete},
	{"DiscordMessage.Pin", message_Pin},
	{"DiscordMessage.Unpin", message_Unpin},
	{"DiscordMessage.AddReaction", message_AddReaction},
	{"DiscordMessage.RemoveReaction", message_RemoveReaction},
	{"DiscordMessage.RemoveAllReactions", message_RemoveAllReactions},
	{"DiscordMessage.Reply", message_Reply},
	{"DiscordMessage.ReplyEmbed", message_ReplyEmbed},
	{"DiscordMessage.Crosspost", message_Crosspost},
	{"DiscordMessage.CreateThread", message_CreateThread},
	
	// New message properties
	{"DiscordMessage.Flags.get", message_GetFlags},
	{"DiscordMessage.IsCrossposted.get", message_IsCrossposted},
	{"DiscordMessage.IsCrosspost.get", message_IsCrosspost},
	{"DiscordMessage.EmbedsSuppressed.get", message_EmbedsSuppressed},
	{"DiscordMessage.IsSourceMessageDeleted.get", message_IsSourceMessageDeleted},
	{"DiscordMessage.IsUrgent.get", message_IsUrgent},
	{"DiscordMessage.HasThread.get", message_HasThread},
	{"DiscordMessage.IsEphemeral.get", message_IsEphemeral},
	{"DiscordMessage.IsLoading.get", message_IsLoading},
	{"DiscordMessage.IsThreadMentionFailed.get", message_IsThreadMentionFailed},
	{"DiscordMessage.NotificationsSuppressed.get", message_NotificationsSuppressed},
	{"DiscordMessage.IsVoiceMessage.get", message_IsVoiceMessage},
	{"DiscordMessage.HasSnapshot.get", message_HasSnapshot},
	{"DiscordMessage.IsUsingComponentsV2.get", message_IsUsingComponentsV2},
	
	// Additional message properties
	{"DiscordMessage.Timestamp.get", message_GetTimestamp},
	{"DiscordMessage.EditedTimestamp.get", message_GetEditedTimestamp},
	{"DiscordMessage.IsDM.get", message_IsDM},
	{"DiscordMessage.HasRemixAttachment.get", message_HasRemixAttachment},
	{"DiscordMessage.AttachmentCount.get", message_GetAttachmentCount},
	{"DiscordMessage.EmbedCount.get", message_GetEmbedCount},
	{"DiscordMessage.ReactionCount.get", message_GetReactionCount},
	{"DiscordMessage.StickerCount.get", message_GetStickerCount},
	{"DiscordMessage.MentionedUserCount.get", message_GetMentionedUserCount},
	{"DiscordMessage.MentionedRoleCount.get", message_GetMentionedRoleCount},
	{"DiscordMessage.MentionedChannelCount.get", message_GetMentionedChannelCount},

	// Channel
	{"DiscordChannel.DiscordChannel", channel_CreateFromId},
	{"DiscordChannel.FetchChannel", channel_FetchChannel},
	{"DiscordChannel.GetName", channel_GetName},
	{"DiscordChannel.GetId", channel_GetId},
	{"DiscordChannel.GetGuildId", channel_GetGuildId},
	{"DiscordChannel.GetParentId", channel_GetParentId},
	{"DiscordChannel.GetTopic", channel_GetTopic},
	{"DiscordChannel.Type.get", channel_GetType},
	{"DiscordChannel.Position.get", channel_GetPosition},
	{"DiscordChannel.IsNSFW.get", channel_IsNSFW},
	{"DiscordChannel.IsTextChannel.get", channel_IsTextChannel},
	{"DiscordChannel.IsVoiceChannel.get", channel_IsVoiceChannel},
	{"DiscordChannel.IsCategory.get", channel_IsCategory},
	{"DiscordChannel.IsThread.get", channel_IsThread},
	{"DiscordChannel.IsForum.get", channel_IsForum},
	{"DiscordChannel.IsNewsChannel.get", channel_IsNewsChannel},
	{"DiscordChannel.IsStageChannel.get", channel_IsStageChannel},
	{"DiscordChannel.Bitrate.get", channel_GetBitrate},
	{"DiscordChannel.UserLimit.get", channel_GetUserLimit},
	{"DiscordChannel.RateLimitPerUser.get", channel_GetRateLimitPerUser},
	{"DiscordChannel.GetMention", channel_GetMention},
	{"DiscordChannel.GetUrl", channel_GetUrl},
	{"DiscordChannel.SetName", channel_SetName},
	{"DiscordChannel.SetTopic", channel_SetTopic},
	{"DiscordChannel.Position.set", channel_SetPosition},
	{"DiscordChannel.IsNSFW.set", channel_SetNSFW},
	{"DiscordChannel.RateLimitPerUser.set", channel_SetRateLimitPerUser},
	{"DiscordChannel.Bitrate.set", channel_SetBitrate},
	{"DiscordChannel.UserLimit.set", channel_SetUserLimit},
	{"DiscordChannel.Delete", channel_Delete},
	{"DiscordChannel.SetParent", channel_SetParent},
	
	// New channel properties
	{"DiscordChannel.Flags.get", channel_GetFlags},
	{"DiscordChannel.GetOwnerId", channel_GetOwnerId},
	{"DiscordChannel.GetLastMessageId", channel_GetLastMessageId},
	{"DiscordChannel.LastPinTimestamp.get", channel_GetLastPinTimestamp},
	{"DiscordChannel.DefaultThreadRateLimitPerUser.get", channel_GetDefaultThreadRateLimitPerUser},
	{"DiscordChannel.DefaultAutoArchiveDuration.get", channel_GetDefaultAutoArchiveDuration},
	{"DiscordChannel.DefaultSortOrder.get", channel_GetDefaultSortOrder},
	{"DiscordChannel.ForumLayout.get", channel_GetForumLayout},
	{"DiscordChannel.GetRTCRegion", channel_GetRTCRegion},
	{"DiscordChannel.IsDM.get", channel_IsDM},
	{"DiscordChannel.IsGroupDM.get", channel_IsGroupDM},
	{"DiscordChannel.IsMediaChannel.get", channel_IsMediaChannel},
	{"DiscordChannel.IsVideo720p.get", channel_IsVideo720p},
	{"DiscordChannel.IsVideoAuto.get", channel_IsVideoAuto},
	{"DiscordChannel.IsPinnedThread.get", channel_IsPinnedThread},
	{"DiscordChannel.IsTagRequired.get", channel_IsTagRequired},
	{"DiscordChannel.IsDownloadOptionsHidden.get", channel_IsDownloadOptionsHidden},
	{"DiscordChannel.IsLockedPermissions.get", channel_IsLockedPermissions},
	{"DiscordChannel.PermissionOverwriteCount.get", channel_GetPermissionOverwriteCount},
	{"DiscordChannel.AvailableTagCount.get", channel_GetAvailableTagCount},
	{"DiscordChannel.GetAvailableTagName", channel_GetAvailableTagName},
	{"DiscordChannel.GetAvailableTagId", channel_GetAvailableTagId},
	{"DiscordChannel.GetAvailableTagEmoji", channel_GetAvailableTagEmoji},
	{"DiscordChannel.GetAvailableTagModerated", channel_GetAvailableTagModerated},
	{"DiscordChannel.GetAvailableTagEmojiIsCustom", channel_GetAvailableTagEmojiIsCustom},
	{"DiscordChannel.CreateForumTag", channel_CreateForumTag},
	{"DiscordChannel.EditForumTag", channel_EditForumTag},
	{"DiscordChannel.DeleteForumTag", channel_DeleteForumTag},
	{"DiscordChannel.CreateForumThread", channel_CreateForumThread},
	{"DiscordChannel.CreateForumThreadEmbed", channel_CreateForumThreadEmbed},
	{"DiscordChannel.GetIconUrl", channel_GetIconUrl},
	{"DiscordChannel.AddPermissionOverwrite", channel_AddPermissionOverwrite},
	{"DiscordChannel.SetPermissionOverwrite", channel_SetPermissionOverwrite},
	{"DiscordChannel.RemovePermissionOverwrite", channel_RemovePermissionOverwrite},
	{"DiscordChannel.GetUserPermissions", channel_GetUserPermissions},
	{"DiscordChannel.CreateInvite", channel_CreateInvite},
	{"DiscordChannel.SendMessage", channel_SendMessage},
	{"DiscordChannel.SendMessageEmbed", channel_SendMessageEmbed},
	{"DiscordChannel.SetRTCRegion", channel_SetRTCRegion},

	// Webhook
	{"DiscordWebhook.DiscordWebhook", webhook_CreateWebhook},
	{"DiscordWebhook.CreateFromIdToken", webhook_CreateWebhookFromIdToken},
	{"DiscordWebhook.FetchWebhook", webhook_FetchWebhook},
	{"DiscordWebhook.GetId", webhook_GetId},
	{"DiscordWebhook.User.get", webhook_GetUser},
	{"DiscordWebhook.GetName", webhook_GetName},
	{"DiscordWebhook.SetName", webhook_SetName},
	{"DiscordWebhook.GetAvatarUrl", webhook_GetAvatarUrl},
	{"DiscordWebhook.SetAvatarUrl", webhook_SetAvatarUrl},
	{"DiscordWebhook.GetAvatarData", webhook_GetAvatarData},
	{"DiscordWebhook.SetAvatarData", webhook_SetAvatarData},
	{"DiscordWebhook.Type.get", webhook_GetType},
	{"DiscordWebhook.GetGuildId", webhook_GetGuildId},
	{"DiscordWebhook.GetChannelId", webhook_GetChannelId},
	{"DiscordWebhook.GetToken", webhook_GetToken},
	{"DiscordWebhook.GetApplicationId", webhook_GetApplicationId},
	{"DiscordWebhook.GetSourceGuildId", webhook_GetSourceGuildId},
	{"DiscordWebhook.GetSourceChannelId", webhook_GetSourceChannelId},
	{"DiscordWebhook.GetUrl", webhook_GetUrl},
	{"DiscordWebhook.GetImageData", webhook_GetImageData},
	{"DiscordWebhook.Modify", webhook_Modify},
	{"DiscordWebhook.Delete", webhook_Delete},
	{"DiscordWebhook.Execute", webhook_Execute},
	{"DiscordWebhook.ExecuteEmbed", webhook_ExecuteEmbed},
	{"DiscordWebhook.CreateWebhook", discord_CreateWebhook},
	{"DiscordWebhook.GetWebhook", discord_GetWebhook},
	{"DiscordWebhook.GetChannelWebhooks", discord_GetChannelWebhooks},

	// Embed
	{"DiscordEmbed.DiscordEmbed", embed_CreateEmbed},
	{"DiscordEmbed.SetTitle", embed_SetTitle},
	{"DiscordEmbed.SetDescription", embed_SetDescription},
	{"DiscordEmbed.Color.set", embed_SetColor},
	{"DiscordEmbed.SetUrl", embed_SetUrl},
	{"DiscordEmbed.SetAuthor", embed_SetAuthor},
	{"DiscordEmbed.SetFooter", embed_SetFooter},
	{"DiscordEmbed.AddField", embed_AddField},
	{"DiscordEmbed.SetThumbnail", embed_SetThumbnail},
	{"DiscordEmbed.SetImage", embed_SetImage},
	{"DiscordEmbed.GetTitle", embed_GetTitle},
	{"DiscordEmbed.GetDescription", embed_GetDescription},
	{"DiscordEmbed.Color.get", embed_GetColor},
	{"DiscordEmbed.GetUrl", embed_GetUrl},
	{"DiscordEmbed.GetAuthorName", embed_GetAuthorName},
	{"DiscordEmbed.GetAuthorUrl", embed_GetAuthorUrl},
	{"DiscordEmbed.GetAuthorIconUrl", embed_GetAuthorIconUrl},
	{"DiscordEmbed.GetFooterText", embed_GetFooterText},
	{"DiscordEmbed.GetFooterIconUrl", embed_GetFooterIconUrl},
	{"DiscordEmbed.GetThumbnailUrl", embed_GetThumbnailUrl},
	{"DiscordEmbed.GetImageUrl", embed_GetImageUrl},
	{"DiscordEmbed.FieldCount.get", embed_GetFieldCount},
	{"DiscordEmbed.GetFieldName", embed_GetFieldName},
	{"DiscordEmbed.GetFieldValue", embed_GetFieldValue},
	{"DiscordEmbed.GetFieldInline", embed_GetFieldInline},

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
	{"DiscordInteraction.User.get", interaction_GetUser},
	{"DiscordInteraction.GetUserNickname", interaction_GetUserNickname},
	{"DiscordInteraction.GetUserId", interaction_GetUserId},
	{"DiscordInteraction.GetUserName", interaction_GetUserName},

	// Autocomplete
	{"DiscordAutocompleteInteraction.GetCommandName", autocomplete_GetCommandName},
	{"DiscordAutocompleteInteraction.GetGuildId", autocomplete_GetGuildId},
	{"DiscordAutocompleteInteraction.GetChannelId", autocomplete_GetChannelId},
	{"DiscordAutocompleteInteraction.User.get", autocomplete_GetUser},
	{"DiscordAutocompleteInteraction.GetUserNickname", autocomplete_GetUserNickname},
	{"DiscordAutocompleteInteraction.GetOptionValue", autocomplete_GetOptionValue},
	{"DiscordAutocompleteInteraction.GetOptionValueInt", autocomplete_GetOptionValueInt},
	{"DiscordAutocompleteInteraction.GetOptionValueFloat", autocomplete_GetOptionValueFloat},
	{"DiscordAutocompleteInteraction.GetOptionValueBool", autocomplete_GetOptionValueBool},
	{"DiscordAutocompleteInteraction.CreateAutocompleteResponse", autocomplete_CreateAutocompleteResponse},
	{"DiscordAutocompleteInteraction.AddAutocompleteChoice", autocomplete_AddAutocompleteChoice},
	{"DiscordAutocompleteInteraction.AddAutocompleteChoiceString", autocomplete_AddAutocompleteChoiceString},

	// Forum Tag
	{"DiscordForumTag.DiscordForumTag", forumtag_Create},
	{"DiscordForumTag.GetId", forumtag_GetId},
	{"DiscordForumTag.GetName", forumtag_GetName},
	{"DiscordForumTag.SetName", forumtag_SetName},
	{"DiscordForumTag.GetEmoji", forumtag_GetEmoji},
	{"DiscordForumTag.SetEmoji", forumtag_SetEmoji},
	{"DiscordForumTag.IsModerated.get", forumtag_GetIsModerated},
	{"DiscordForumTag.IsModerated.set", forumtag_SetIsModerated},
	{"DiscordForumTag.EmojiIsCustom.get", forumtag_GetEmojiIsCustom},
	{"DiscordForumTag.ApplyToChannel", forumtag_ApplyToChannel},
	{nullptr, nullptr}
};