#ifndef _INCLUDE_DISCORD_H_
#define _INCLUDE_DISCORD_H_

#include "extension.h"

class DiscordEmbed
{
private:
	dpp::embed m_embed;

public:
	DiscordEmbed() {}

	void SetTitle(const char* title) { m_embed.set_title(title); }
	void SetDescription(const char* desc) { m_embed.set_description(desc); }
	void SetColor(int color) { m_embed.set_color(color); }
	void SetUrl(const char* url) { m_embed.set_url(url); }
	void SetAuthor(const char* name, const char* url = nullptr, const char* icon_url = nullptr) {
		m_embed.set_author(name, url ? url : "", icon_url ? icon_url : "");
	}
	void SetFooter(const char* text, const char* icon_url = nullptr) {
		m_embed.set_footer(text, icon_url ? icon_url : "");
	}
	void AddField(const char* name, const char* value, bool inLine = false) {
		m_embed.add_field(name, value, inLine);
	}
	void SetThumbnail(const char* url) { m_embed.set_thumbnail(url); }
	void SetImage(const char* url) { m_embed.set_image(url); }

	const dpp::embed& GetEmbed() const { return m_embed; }
};

class DiscordMessage
{
private:
	dpp::message m_message;

public:
	DiscordMessage(const dpp::message& msg) : m_message(msg) {}

	const char* GetContent() const { return m_message.content.c_str(); }
	std::string GetMessageId() const { return std::to_string(m_message.id); }
	std::string GetChannelId() const { return std::to_string(m_message.channel_id); }
	std::string GetGuildId() const { return std::to_string(m_message.guild_id); }
	std::string GetAuthorId() const { return std::to_string(m_message.author.id); }
	const char* GetAuthorName() const { return m_message.author.username.c_str(); }
	const char* GetAuthorDisplayName() const { return m_message.author.global_name.c_str(); }
	const char* GetAuthorNickname() const { return m_message.member.get_nickname().c_str(); }
	const uint16_t GetAuthorDiscriminator() const { return m_message.author.discriminator; }
	bool IsPinned() const { return m_message.pinned; }
	bool IsTTS() const { return m_message.tts; }
	bool IsMentionEveryone() const { return m_message.mention_everyone; }
	bool IsBot() const { return m_message.author.is_bot(); }
};

class DiscordChannel
{
private:
	dpp::channel m_channel;

public:
	DiscordChannel(const dpp::channel& chnl) : m_channel(chnl) {}

	const char* GetName() const { return m_channel.name.c_str(); }
};

class DiscordWebhook
{
public:
	dpp::webhook m_webhook;
	DiscordWebhook(const dpp::webhook& wbhk) : m_webhook(wbhk) {}

	std::string GetId() const { return std::to_string(m_webhook.id); }

	const char* GetName() const { return m_webhook.name.c_str(); }

	void SetName(const char* value) { m_webhook.name = value; }

	const char* GetAvatarUrl() const { return m_webhook.avatar_url.c_str(); }

	void SetAvatarUrl(const char* value) { m_webhook.avatar_url = value; }
};

class DiscordClient
{
private:
	std::unique_ptr<dpp::cluster> m_cluster;
	bool m_isRunning;
	Handle_t m_discord_handle;
	std::unique_ptr<std::thread> m_thread;

	std::string m_botId;
	std::string m_botName;
	std::string m_botDiscriminator;
	std::string m_botAvatarUrl;

	void RunBot();
	void SetupEventHandlers();

public:
	DiscordClient(const char* token);
	~DiscordClient();

	bool Initialize();
	void Start();
	void Stop();
	bool IsRunning() const { return m_isRunning; }
	void SetHandle(Handle_t handle) { m_discord_handle = handle; }
	bool SetPresence(dpp::presence presence);
	bool ExecuteWebhook(dpp::webhook wh, const char* message);
	bool SendMessage(dpp::snowflake channel_id, const char* message);
	bool SendMessageEmbed(dpp::snowflake channel_id, const char* message, const DiscordEmbed* embed);
	bool GetChannel(dpp::snowflake channel_id, IForward *callback_forward, cell_t data);
	bool GetChannelWebhooks(dpp::snowflake channel_id, IForward *callback_forward, cell_t data);
    bool RegisterSlashCommand(dpp::snowflake guild_id, const char* name, const char* description);
	bool RegisterGlobalSlashCommand(const char* name, const char* description);
	bool RegisterSlashCommandWithOptions(dpp::snowflake guild_id, const char* name, const char* description, const std::vector<dpp::command_option>& options);
	bool RegisterGlobalSlashCommandWithOptions(const char* name, const char* description, const std::vector<dpp::command_option>& options);
	bool EditMessage(dpp::snowflake channel_id, dpp::snowflake message_id, const char* content);
	bool EditMessageEmbed(dpp::snowflake channel_id, dpp::snowflake message_id, const char* content, const DiscordEmbed* embed);
	bool DeleteMessage(dpp::snowflake channel_id, dpp::snowflake message_id);
	bool DeleteGuildCommand(dpp::snowflake guild_id, dpp::snowflake command_id);
	bool DeleteGlobalCommand(dpp::snowflake command_id);

	const char* GetBotId() const { return m_botId.c_str(); }
	const char* GetBotName() const { return m_botName.c_str(); }
	const char* GetBotDiscriminator() const { return m_botDiscriminator.c_str(); }
	const char* GetBotAvatarUrl() const { return m_botAvatarUrl.c_str(); }

	void UpdateBotInfo() {
		if (m_cluster) {
			m_botId = std::to_string(m_cluster->me.id);
			m_botName = m_cluster->me.username;
			m_botDiscriminator = std::to_string(m_cluster->me.discriminator);
			m_botAvatarUrl = m_cluster->me.get_avatar_url();
		}
	}
};

class DiscordInteraction
{
private:
	dpp::slashcommand_t m_interaction;
	std::string m_commandName;

public:
	DiscordInteraction(const dpp::slashcommand_t& interaction) :
		m_interaction(interaction),
		m_commandName(interaction.command.get_command_name())
	{
	}

	const char* GetCommandName() const { return m_commandName.c_str(); }
	std::string GetGuildId() const { return std::to_string(m_interaction.command.guild_id); }
	std::string GetChannelId() const { return std::to_string(m_interaction.command.channel_id); }
	std::string GetUserId() const { return std::to_string(m_interaction.command.usr.id); }
	const char* GetUserName() const { return m_interaction.command.usr.username.c_str(); }

	bool GetOptionValue(const char* name, std::string& value) const {
		auto param = m_interaction.get_parameter(name);
		if (param.index() == 0) return false;
		value = std::get<std::string>(param);
		return true;
	}

	bool GetOptionValueInt(const char* name, int64_t& value) const {
		auto param = m_interaction.get_parameter(name);
		if (param.index() == 0) return false;
		value = std::get<int64_t>(param);
		return true;
	}

	bool GetOptionValueDouble(const char* name, double& value) const {
		auto param = m_interaction.get_parameter(name);
		if (param.index() == 0) return false;
		value = std::get<double>(param);
		return true;
	}

	bool GetOptionValueBool(const char* name, bool& value) const {
		auto param = m_interaction.get_parameter(name);
		if (param.index() == 0) return false;
		value = std::get<bool>(param);
		return true;
	}

	void CreateResponse(const char* content) const {
		m_interaction.reply(dpp::message(content));
	}

	void CreateResponseEmbed(const char* content, const DiscordEmbed* embed) const {
		dpp::message msg(content);
		msg.add_embed(embed->GetEmbed());
		m_interaction.reply(msg);
	}

	void DeferReply(bool ephemeral = false) const {
		m_interaction.thinking(ephemeral);
	}

	void EditResponse(const char* content) const {
		m_interaction.edit_response(dpp::message(content));
	}

	void EditResponseEmbed(const char* content, const DiscordEmbed* embed) const {
		dpp::message msg(content);
		msg.add_embed(embed->GetEmbed());
		m_interaction.edit_response(msg);
	}

	void CreateEphemeralResponse(const char* content) const {
		dpp::message msg(content);
		msg.set_flags(dpp::m_ephemeral);
		m_interaction.reply(msg);
	}

	void CreateEphemeralResponseEmbed(const char* content, const DiscordEmbed* embed) const {
		dpp::message msg(content);
		msg.set_flags(dpp::m_ephemeral);
		msg.add_embed(embed->GetEmbed());
		m_interaction.reply(msg);
	}
};

#endif // _INCLUDE_DISCORD_H_ 