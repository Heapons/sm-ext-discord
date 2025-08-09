#ifndef _INCLUDE_DISCORD_H_
#define _INCLUDE_DISCORD_H_

#include "extension.h"
#include <algorithm>

// Forward declarations
class DiscordClient;

class DiscordEmbed
{
private:
	dpp::embed m_embed;

public:
	DiscordEmbed() {}

	void SetTitle(const char* title) { m_embed.set_title(title); }
	void SetDescription(const char* desc) { m_embed.set_description(desc); }
	void SetColor(uint32_t color) { m_embed.set_color(color); }
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
	void SetTimestamp(time_t timestamp) { m_embed.set_timestamp(timestamp); }
	void SetVideo(const char* url) { m_embed.set_video(url); }
	void SetProvider(const char* name, const char* url = nullptr) {
		m_embed.set_provider(name, url ? url : "");
	}

	// Get methods
	std::string GetTitle() const { return m_embed.title; }
	std::string GetDescription() const { return m_embed.description; }
	uint32_t GetColor() const { return m_embed.color.value_or(0); }
	time_t GetTimestamp() const { return m_embed.timestamp; }
	std::string GetType() const { return m_embed.type; }
	std::string GetUrl() const { return m_embed.url; }
	std::string GetAuthorName() const { return m_embed.author.has_value() ? m_embed.author->name : ""; }
	std::string GetAuthorUrl() const { return m_embed.author.has_value() ? m_embed.author->url : ""; }
	std::string GetAuthorIconUrl() const { return m_embed.author.has_value() ? m_embed.author->icon_url : ""; }
	std::string GetAuthorProxyIconUrl() const { return m_embed.author.has_value() ? m_embed.author->proxy_icon_url : ""; }
	std::string GetFooterText() const { return m_embed.footer.has_value() ? m_embed.footer->text : ""; }
	std::string GetFooterIconUrl() const { return m_embed.footer.has_value() ? m_embed.footer->icon_url : ""; }
	std::string GetFooterProxyUrl() const { return m_embed.footer.has_value() ? m_embed.footer->proxy_url : ""; }
	std::string GetThumbnailUrl() const { return m_embed.thumbnail.has_value() ? m_embed.thumbnail->url : ""; }
	std::string GetThumbnailProxyUrl() const { return m_embed.thumbnail.has_value() ? m_embed.thumbnail->proxy_url : ""; }
	uint32_t GetThumbnailWidth() const { return m_embed.thumbnail.has_value() ? m_embed.thumbnail->width : 0; }
	uint32_t GetThumbnailHeight() const { return m_embed.thumbnail.has_value() ? m_embed.thumbnail->height : 0; }
	std::string GetImageUrl() const { return m_embed.image.has_value() ? m_embed.image->url : ""; }
	std::string GetImageProxyUrl() const { return m_embed.image.has_value() ? m_embed.image->proxy_url : ""; }
	uint32_t GetImageWidth() const { return m_embed.image.has_value() ? m_embed.image->width : 0; }
	uint32_t GetImageHeight() const { return m_embed.image.has_value() ? m_embed.image->height : 0; }
	std::string GetVideoUrl() const { return m_embed.video.has_value() ? m_embed.video->url : ""; }
	std::string GetVideoProxyUrl() const { return m_embed.video.has_value() ? m_embed.video->proxy_url : ""; }
	uint32_t GetVideoWidth() const { return m_embed.video.has_value() ? m_embed.video->width : 0; }
	uint32_t GetVideoHeight() const { return m_embed.video.has_value() ? m_embed.video->height : 0; }
	std::string GetProviderName() const { return m_embed.provider.has_value() ? m_embed.provider->name : ""; }
	std::string GetProviderUrl() const { return m_embed.provider.has_value() ? m_embed.provider->url : ""; }
	size_t GetFieldCount() const { return m_embed.fields.size(); }
	std::string GetFieldName(size_t index) const { return index < m_embed.fields.size() ? m_embed.fields[index].name : ""; }
	std::string GetFieldValue(size_t index) const { return index < m_embed.fields.size() ? m_embed.fields[index].value : ""; }
	bool GetFieldInline(size_t index) const { return index < m_embed.fields.size() ? m_embed.fields[index].is_inline : false; }
	
	// Field management
	bool RemoveField(size_t index) {
		if (index < m_embed.fields.size()) {
			m_embed.fields.erase(m_embed.fields.begin() + index);
			return true;
		}
		return false;
	}
	void ClearFields() { m_embed.fields.clear(); }
	
	// Utility properties
	bool HasThumbnail() const { return m_embed.thumbnail.has_value(); }
	bool HasImage() const { return m_embed.image.has_value(); }
	bool HasVideo() const { return m_embed.video.has_value(); }
	bool HasProvider() const { return m_embed.provider.has_value(); }
	bool HasAuthor() const { return m_embed.author.has_value(); }
	bool HasFooter() const { return m_embed.footer.has_value(); }
	bool HasTimestamp() const { return m_embed.timestamp != 0; }

	const dpp::embed& GetEmbed() const { return m_embed; }
};

class DiscordForumTag
{
private:
	dpp::forum_tag m_tag;

public:
	DiscordForumTag() {}
	DiscordForumTag(const char* name, const char* emoji = "", bool moderated = false) {
		m_tag.set_name(name);
		if (emoji && strlen(emoji) > 0) {
			// Check if emoji is a snowflake ID (numeric) or unicode
			if (std::all_of(emoji, emoji + strlen(emoji), ::isdigit)) {
				m_tag.emoji = dpp::snowflake(emoji);
			} else {
				m_tag.emoji = std::string(emoji);
			}
		}
		m_tag.moderated = moderated;
	}
	DiscordForumTag(const dpp::forum_tag& tag) : m_tag(tag) {}

	std::string GetId() const { return std::to_string(m_tag.id); }
	std::string GetName() const { return m_tag.name; }
	void SetName(const char* name) { m_tag.set_name(name); }
	
	std::string GetEmoji() const {
		if (std::holds_alternative<dpp::snowflake>(m_tag.emoji)) {
			return std::to_string(std::get<dpp::snowflake>(m_tag.emoji));
		} else if (std::holds_alternative<std::string>(m_tag.emoji)) {
			return std::get<std::string>(m_tag.emoji);
		}
		return "";
	}
	
	void SetEmoji(const char* emoji) {
		if (emoji && strlen(emoji) > 0) {
			// Check if emoji is a snowflake ID (numeric) or unicode
			if (std::all_of(emoji, emoji + strlen(emoji), ::isdigit)) {
				m_tag.emoji = dpp::snowflake(emoji);
			} else {
				m_tag.emoji = std::string(emoji);
			}
		} else {
			m_tag.emoji = std::monostate{};
		}
	}
	
	bool IsModerated() const { return m_tag.moderated; }
	void SetModerated(bool moderated) { m_tag.moderated = moderated; }
	
	bool EmojiIsCustom() const {
		return std::holds_alternative<dpp::snowflake>(m_tag.emoji);
	}
	
	const dpp::forum_tag& GetTag() const { return m_tag; }
};

class DiscordUser
{
private:
	dpp::user m_user;
	dpp::guild_member m_member;
	bool m_has_member;
	DiscordClient* m_client;

public:
	DiscordUser(const dpp::user& user, DiscordClient* client) : m_user(user), m_has_member(false), m_client(client) {}
	DiscordUser(const dpp::user& user, const dpp::guild_member& member, DiscordClient* client) : m_user(user), m_member(member), m_has_member(true), m_client(client) {}
	
	std::string GetId() const { return std::to_string(m_user.id); }
	const char* GetUserName() const { return m_user.username.c_str(); }
	const uint16_t GetDiscriminator() const { return m_user.discriminator; }
	const char* GetGlobalName() const { return m_user.global_name.c_str(); }
	std::string GetAvatarUrl(bool prefer_animated_avatars) const { return m_user.get_avatar_url(0, dpp::i_png, prefer_animated_avatars); }
	std::string GetDefaultAvatarUrl() const { return m_user.get_default_avatar_url(); }
	std::string GetAvatarDecorationUrl(uint16_t size = 0) const { return m_user.get_avatar_decoration_url(size); }
	std::string GetMention() const { return m_user.get_mention(); }
	std::string GetUrl() const { return m_user.get_url(); }
	std::string FormatUsername() const { return m_user.format_username(); }
	bool IsBot() const { return m_user.is_bot(); }
	
	// User flag checking methods
	bool IsSystem() const { return m_user.is_system(); }
	bool IsMfaEnabled() const { return m_user.is_mfa_enabled(); }
	bool IsVerified() const { return m_user.is_verified(); }
	bool HasNitroFull() const { return m_user.has_nitro_full(); }
	bool HasNitroClassic() const { return m_user.has_nitro_classic(); }
	bool HasNitroBasic() const { return m_user.has_nitro_basic(); }
	bool IsDiscordEmployee() const { return m_user.is_discord_employee(); }
	bool IsPartneredOwner() const { return m_user.is_partnered_owner(); }
	bool HasHypesquadEvents() const { return m_user.has_hypesquad_events(); }
	bool IsBughunter1() const { return m_user.is_bughunter_1(); }
	bool IsHouseBravery() const { return m_user.is_house_bravery(); }
	bool IsHouseBrilliance() const { return m_user.is_house_brilliance(); }
	bool IsHouseBalance() const { return m_user.is_house_balance(); }
	bool IsEarlySupporter() const { return m_user.is_early_supporter(); }
	bool IsTeamUser() const { return m_user.is_team_user(); }
	bool IsBughunter2() const { return m_user.is_bughunter_2(); }
	bool IsVerifiedBot() const { return m_user.is_verified_bot(); }
	bool IsVerifiedBotDev() const { return m_user.is_verified_bot_dev(); }
	bool IsCertifiedModerator() const { return m_user.is_certified_moderator(); }
	bool IsBotHttpInteractions() const { return m_user.is_bot_http_interactions(); }
	bool HasAnimatedIcon() const { return m_user.has_animated_icon(); }
	bool IsActiveDeveloper() const { return m_user.is_active_developer(); }
	uint32_t GetFlags() const { return m_user.flags; }
	bool HasFlag(uint32_t flag) const { return (m_user.flags & flag) != 0; }
	
	// Guild member specific methods
	bool HasGuildMember() const { return m_has_member; }
	std::string GetNickName() const { return m_has_member ? m_member.get_nickname() : ""; }
	std::string GetJoinedAt() const { return m_has_member ? std::to_string(m_member.joined_at) : "0"; }
	bool IsPending() const { return m_has_member ? m_member.is_pending() : false; }
	
	// Permission methods
	bool HasPermission(const char* permission) const;
	uint64_t GetPermissions() const;
	bool HasPermissionInChannel(dpp::snowflake channel_id, const char* permission) const;
	uint64_t GetPermissionsInChannel(dpp::snowflake channel_id) const;
	
	// Role methods
	std::vector<dpp::snowflake> GetRoles() const;
	bool HasRole(dpp::snowflake role_id) const;
	bool HasAnyRole(const std::vector<dpp::snowflake>& role_ids) const;
	bool HasAllRoles(const std::vector<dpp::snowflake>& role_ids) const;
	dpp::snowflake GetHighestRole() const;
	std::string GetRoleName(dpp::snowflake role_id) const;
	std::vector<std::string> GetRoleNames() const;
	
	// Member management methods
	bool SetNickName(const char* nickname);
	bool AddRole(dpp::snowflake role_id);
	bool RemoveRole(dpp::snowflake role_id);
	bool KickFromGuild();
	bool BanFromGuild(const char* reason = nullptr, int delete_message_days = 0);
	bool UnbanFromGuild();
	bool SetTimeout(time_t timeout_until);
	bool RemoveTimeout();
	
	// Internal accessors
	const dpp::user& GetDPPUser() const { return m_user; }
	const dpp::guild_member& GetDPPMember() const { return m_member; }
};

class DiscordMessage
{
private:
	dpp::message m_message;
	DiscordClient* m_client;
	mutable Handle_t m_authorHandle;

public:
	DiscordMessage(const dpp::message& msg, DiscordClient* client) : m_message(msg), m_client(client), m_authorHandle(BAD_HANDLE) {}
	DiscordMessage(DiscordClient* client) : m_client(client), m_authorHandle(BAD_HANDLE) {} // Empty message
	DiscordMessage(const char* content, DiscordClient* client) : m_client(client), m_authorHandle(BAD_HANDLE) { 
		m_message.content = content; 
	}
	DiscordMessage(dpp::snowflake channel_id, const char* content, DiscordClient* client) : m_client(client), m_authorHandle(BAD_HANDLE) { 
		m_message.channel_id = channel_id;
		m_message.content = content; 
	}
	DiscordMessage(const DiscordEmbed* embed, DiscordClient* client) : m_client(client), m_authorHandle(BAD_HANDLE) { 
		if (embed) m_message.add_embed(embed->GetEmbed()); 
	}
	DiscordMessage(dpp::snowflake channel_id, const DiscordEmbed* embed, DiscordClient* client) : m_client(client), m_authorHandle(BAD_HANDLE) { 
		m_message.channel_id = channel_id;
		if (embed) m_message.add_embed(embed->GetEmbed()); 
	}
	
	~DiscordMessage() {
		if (m_authorHandle != BAD_HANDLE) {
			handlesys->FreeHandle(m_authorHandle, nullptr);
		}
	}

	Handle_t GetAuthorHandle() const;
	
	DiscordUser* GetAuthor() const { 
		if (m_message.guild_id != 0 && m_message.member.user_id != 0) {
			return new DiscordUser(m_message.author, m_message.member, m_client);
		}
		return new DiscordUser(m_message.author, m_client); 
	}
	const char* GetContent() const { return m_message.content.c_str(); }
	const size_t GetContentLength() const { return m_message.content.length(); }
	std::string GetMessageId() const { return std::to_string(m_message.id); }
	std::string GetChannelId() const { return std::to_string(m_message.channel_id); }
	std::string GetGuildId() const { return std::to_string(m_message.guild_id); }
	std::string GetAuthorNickname() const { return m_message.member.get_nickname(); }
	dpp::message_type GetType() const { return m_message.type; }
	bool IsPinned() const { return m_message.pinned; }
	bool IsTTS() const { return m_message.tts; }
	bool IsMentionEveryone() const { return m_message.mention_everyone; }
	bool IsBot() const { return m_message.author.is_bot(); }
	
	// Message flags and properties
	uint16_t GetFlags() const { return m_message.flags; }
	bool IsCrossposted() const { return (m_message.flags & (1 << 0)) != 0; }
	bool IsCrosspost() const { return (m_message.flags & (1 << 1)) != 0; }
	bool EmbedsSuppressed() const { return (m_message.flags & (1 << 2)) != 0; }
	bool IsSourceMessageDeleted() const { return (m_message.flags & (1 << 3)) != 0; }
	bool IsUrgent() const { return (m_message.flags & (1 << 4)) != 0; }
	bool HasThread() const { return (m_message.flags & (1 << 5)) != 0; }
	bool IsEphemeral() const { return (m_message.flags & (1 << 6)) != 0; }
	bool IsLoading() const { return (m_message.flags & (1 << 7)) != 0; }
	bool IsThreadMentionFailed() const { return (m_message.flags & (1 << 8)) != 0; }
	bool NotificationsSuppressed() const { return (m_message.flags & (1 << 12)) != 0; }
	bool IsVoiceMessage() const { return (m_message.flags & (1 << 13)) != 0; }
	bool HasSnapshot() const { return (m_message.flags & (1 << 14)) != 0; }
	bool IsUsingComponentsV2() const { return (m_message.flags & (1 << 15)) != 0; }
	
	// Additional properties
	std::string GetWebhookId() const { return std::to_string(m_message.webhook_id); }
	time_t GetTimestamp() const { return static_cast<time_t>(m_message.sent); }
	time_t GetEditedTimestamp() const { return static_cast<time_t>(m_message.edited); }
	const char* GetNonce() const { return m_message.nonce.c_str(); }
	bool IsDM() const { return m_message.guild_id == 0; }
	bool HasRemixAttachment() const {
		for (const auto& attachment : m_message.attachments) {
			if (attachment.flags & (1 << 2)) return true; // a_is_remix
		}
		return false;
	}
	std::string GetURL() const {
		if (m_message.guild_id != 0) {
			return "https://discord.com/channels/" + std::to_string(m_message.guild_id) + "/" + 
						 std::to_string(m_message.channel_id) + "/" + std::to_string(m_message.id);
		} else {
			return "https://discord.com/channels/@me/" + std::to_string(m_message.channel_id) + "/" + 
						 std::to_string(m_message.id);
		}
	}
	
	// Collection counts
	size_t GetAttachmentCount() const { return m_message.attachments.size(); }
	size_t GetEmbedCount() const { return m_message.embeds.size(); }
	size_t GetReactionCount() const { return m_message.reactions.size(); }
	size_t GetStickerCount() const { return m_message.stickers.size(); }
	size_t GetMentionedUserCount() const { return m_message.mentions.size(); }
	size_t GetMentionedRoleCount() const { return m_message.mention_roles.size(); }
	size_t GetMentionedChannelCount() const { return m_message.mention_channels.size(); }
	
	// Collection accessors
	std::string GetAttachmentFilename(size_t index) const {
		if (index >= m_message.attachments.size()) return "";
		return m_message.attachments[index].filename;
	}
	std::string GetAttachmentURL(size_t index) const {
		if (index >= m_message.attachments.size()) return "";
		return m_message.attachments[index].url;
	}
	uint32_t GetAttachmentSize(size_t index) const {
		if (index >= m_message.attachments.size()) return 0;
		return m_message.attachments[index].size;
	}
	std::string GetMentionedUserId(size_t index) const {
		if (index >= m_message.mentions.size()) return "";
		return std::to_string(m_message.mentions[index].first.id);
	}
	std::string GetMentionedRoleId(size_t index) const {
		if (index >= m_message.mention_roles.size()) return "";
		return std::to_string(m_message.mention_roles[index]);
	}
	
	// Message actions
	bool Reply(const char* content);
	bool ReplyEmbed(const char* content, const class DiscordEmbed* embed);
	bool Crosspost();
	bool CreateThread(const char* name, int auto_archive_duration = 60);
	
	// Message management methods
	bool Edit(const char* new_content);
	bool EditEmbed(const char* new_content, const class DiscordEmbed* embed);
	bool Delete();
	bool Pin();
	bool Unpin();
	bool AddReaction(const char* emoji);
	bool RemoveReaction(const char* emoji);
	bool RemoveAllReactions();
	
	// Message property setters
	void SetContent(const char* content) { m_message.content = content; }
	void SetChannelId(dpp::snowflake channel_id) { m_message.channel_id = channel_id; }
	void SetType(dpp::message_type type) { m_message.type = type; }
	void SetFlags(uint16_t flags) { m_message.flags = flags; }
	void SetTTS(bool tts) { m_message.tts = tts; }
	void SetNonce(const char* nonce) { m_message.nonce = nonce; }
	void AddEmbed(const class DiscordEmbed* embed) { 
		if (embed) m_message.add_embed(embed->GetEmbed()); 
	}
	void ClearEmbeds() { m_message.embeds.clear(); }
	bool Send(); // Send this message to its channel
	
	// Internal accessor
	const dpp::message& GetDPPMessage() const { return m_message; }
};

class DiscordChannel
{
private:
	dpp::channel m_channel;
	DiscordClient* m_client;

public:
	DiscordChannel(const dpp::channel& chnl, DiscordClient* client) : m_channel(chnl), m_client(client) {}

	// Basic information
	const char* GetName() const { return m_channel.name.c_str(); }
	std::string GetId() const { return std::to_string(m_channel.id); }
	std::string GetGuildId() const { return std::to_string(m_channel.guild_id); }
	std::string GetParentId() const { return std::to_string(m_channel.parent_id); }
	const char* GetTopic() const { return m_channel.topic.c_str(); }
	
	// Channel type and properties
	uint8_t GetType() const { return static_cast<uint8_t>(m_channel.get_type()); }
	uint16_t GetPosition() const { return m_channel.position; }
	bool IsNSFW() const { return m_channel.is_nsfw(); }
	bool IsTextChannel() const { return m_channel.is_text_channel(); }
	bool IsVoiceChannel() const { return m_channel.is_voice_channel(); }
	bool IsCategory() const { return m_channel.is_category(); }
	bool IsThread() const { 
		dpp::channel_type type = m_channel.get_type();
		return type == dpp::CHANNEL_ANNOUNCEMENT_THREAD || 
					 type == dpp::CHANNEL_PUBLIC_THREAD || 
					 type == dpp::CHANNEL_PRIVATE_THREAD;
	}
	bool IsForum() const { return m_channel.is_forum(); }
	bool IsNewsChannel() const { return m_channel.is_news_channel(); }
	bool IsStageChannel() const { return m_channel.is_stage_channel(); }
	
	// Voice channel specific
	uint16_t GetBitrate() const { return m_channel.bitrate; }
	uint8_t GetUserLimit() const { return m_channel.user_limit; }
	
	// Text channel specific
	uint16_t GetRateLimitPerUser() const { return m_channel.rate_limit_per_user; }
	
	// Additional channel properties
	uint16_t GetFlags() const { return m_channel.flags; }
	std::string GetOwnerId() const { return std::to_string(m_channel.owner_id); }
	std::string GetLastMessageId() const { return std::to_string(m_channel.last_message_id); }
	time_t GetLastPinTimestamp() const { return static_cast<time_t>(m_channel.last_pin_timestamp); }
	uint16_t GetDefaultThreadRateLimitPerUser() const { return m_channel.default_thread_rate_limit_per_user; }
	uint8_t GetDefaultAutoArchiveDuration() const { return static_cast<uint8_t>(m_channel.default_auto_archive_duration); }
	uint8_t GetDefaultSortOrder() const { return static_cast<uint8_t>(m_channel.default_sort_order); }
	uint8_t GetForumLayout() const { return static_cast<uint8_t>(m_channel.get_default_forum_layout()); }
	const char* GetRTCRegion() const { return m_channel.rtc_region.c_str(); }
	
	// Additional channel type checks
	bool IsDM() const { return m_channel.is_dm(); }
	bool IsGroupDM() const { return m_channel.is_group_dm(); }
	bool IsMediaChannel() const { return m_channel.is_media_channel(); }
	bool IsVideo720p() const { return m_channel.is_video_720p(); }
	bool IsVideoAuto() const { return m_channel.is_video_auto(); }
	bool IsPinnedThread() const { return m_channel.is_pinned_thread(); }
	bool IsTagRequired() const { return m_channel.is_tag_required(); }
	bool IsDownloadOptionsHidden() const { return m_channel.is_download_options_hidden(); }
	bool IsLockedPermissions() const { return m_channel.is_locked_permissions(); }
	
	// URLs and mentions
	std::string GetMention() const { return m_channel.get_mention(); }
	std::string GetUrl() const { return m_channel.get_url(); }
	std::string GetIconUrl(uint16_t size = 0) const { return m_channel.get_icon_url(size); }
	
	// Permission overwrites
	size_t GetPermissionOverwriteCount() const { return m_channel.permission_overwrites.size(); }
	std::string GetPermissionOverwriteTargetId(size_t index) const {
		if (index >= m_channel.permission_overwrites.size()) return "";
		return std::to_string(m_channel.permission_overwrites[index].id);
	}
	uint8_t GetPermissionOverwriteType(size_t index) const {
		if (index >= m_channel.permission_overwrites.size()) return 0;
		return m_channel.permission_overwrites[index].type;
	}
	
	// Forum tags
	size_t GetAvailableTagCount() const { return m_channel.available_tags.size(); }
	std::string GetAvailableTagName(size_t index) const {
		if (index >= m_channel.available_tags.size()) return "";
		return m_channel.available_tags[index].name;
	}
	std::string GetAvailableTagId(size_t index) const {
		if (index >= m_channel.available_tags.size()) return "";
		return std::to_string(m_channel.available_tags[index].id);
	}
	
	// New Forum tag methods
	std::string GetAvailableTagEmoji(size_t index) const {
		if (index >= m_channel.available_tags.size()) return "";
		const auto& tag = m_channel.available_tags[index];
		if (std::holds_alternative<dpp::snowflake>(tag.emoji)) {
			return std::to_string(std::get<dpp::snowflake>(tag.emoji));
		} else if (std::holds_alternative<std::string>(tag.emoji)) {
			return std::get<std::string>(tag.emoji);
		}
		return "";
	}
	
	bool GetAvailableTagModerated(size_t index) const {
		if (index >= m_channel.available_tags.size()) return false;
		return m_channel.available_tags[index].moderated;
	}
	
	bool GetAvailableTagEmojiIsCustom(size_t index) const {
		if (index >= m_channel.available_tags.size()) return false;
		return std::holds_alternative<dpp::snowflake>(m_channel.available_tags[index].emoji);
	}
	
	// Forum tag management
	bool CreateForumTag(const char* name, const char* emoji = "", bool moderated = false);
	bool EditForumTag(dpp::snowflake tag_id, const char* name, const char* emoji = "", bool moderated = false);
	bool DeleteForumTag(dpp::snowflake tag_id);
	
	// Forum thread creation
	bool CreateForumThread(const char* name, const char* message, const std::vector<dpp::snowflake>& tag_ids = {}, int auto_archive = 1440, int rate_limit = 0);
	bool CreateForumThreadEmbed(const char* name, const char* message, const class DiscordEmbed* embed, const std::vector<dpp::snowflake>& tag_ids = {}, int auto_archive = 1440, int rate_limit = 0);
	
	// Permission management
	bool AddPermissionOverwrite(dpp::snowflake target_id, uint8_t type, uint64_t allowed, uint64_t denied);
	bool SetPermissionOverwrite(dpp::snowflake target_id, uint8_t type, uint64_t allowed, uint64_t denied);
	bool RemovePermissionOverwrite(dpp::snowflake target_id, uint8_t type);
	std::string GetUserPermissions(dpp::snowflake user_id) const;
	
	// Channel actions
	bool CreateInvite(int max_age = 86400, int max_uses = 0, bool temporary = false, bool unique = false);
	bool SendMessage(const char* content);
	bool SendMessageEmbed(const char* content, const class DiscordEmbed* embed);
	bool SendDiscordMessage(const class DiscordMessage* message);
	bool SetRTCRegion(const char* region);
	
	// Permission checking
	bool HasUserPermission(const dpp::user& user, const char* permission) const;
	bool HasMemberPermission(const dpp::guild_member& member, const char* permission) const;
	
	// Channel management methods
	bool SetName(const char* name);
	bool SetTopic(const char* topic);
	bool SetPosition(uint16_t position);
	bool SetNSFW(bool nsfw);
	bool SetRateLimitPerUser(uint16_t seconds);
	bool SetBitrate(uint16_t bitrate);
	bool SetUserLimit(uint8_t limit);
	bool Delete();
	bool SetParent(dpp::snowflake parent_id);
	
	// Internal accessor
	const dpp::channel& GetDPPChannel() const { return m_channel; }
};

class DiscordWebhook
{
private:
	mutable Handle_t m_userHandle;

public:
	dpp::webhook m_webhook;
	DiscordClient* m_client;
	
	// Constructors
	DiscordWebhook(const dpp::webhook& wbhk, DiscordClient* client) : m_userHandle(BAD_HANDLE), m_webhook(wbhk), m_client(client) {}
	DiscordWebhook(const std::string& webhook_url, DiscordClient* client = nullptr) : m_userHandle(BAD_HANDLE), m_webhook(webhook_url), m_client(client) {}
	DiscordWebhook(const char* webhook_url, DiscordClient* client = nullptr) : m_userHandle(BAD_HANDLE), m_webhook(std::string(webhook_url)), m_client(client) {}
	DiscordWebhook(dpp::snowflake webhook_id, const std::string& webhook_token, DiscordClient* client = nullptr) 
		: m_userHandle(BAD_HANDLE), m_client(client) { 
		m_webhook.id = webhook_id; 
		m_webhook.token = webhook_token; 
	}
	
	~DiscordWebhook() {
		if (m_userHandle != BAD_HANDLE) {
			handlesys->FreeHandle(m_userHandle, nullptr);
		}
	}

	// Basic properties
	std::string GetId() const { return std::to_string(m_webhook.id); }
	
	Handle_t GetUserHandle() const;
	
	DiscordUser* GetUser() const { return new DiscordUser(m_webhook.user_obj, m_client); }
	const char* GetName() const { return m_webhook.name.c_str(); }
	void SetName(const char* value) { m_webhook.name = value; }
	const char* GetAvatarUrl() const { return m_webhook.avatar_url.c_str(); }
	void SetAvatarUrl(const char* value) { m_webhook.avatar_url = value; }
	std::string GetAvatarData() const { return m_webhook.avatar.to_string(); }
	void SetAvatarData(const char* value) { m_webhook.avatar = dpp::utility::iconhash(value); }
	
	// Additional webhook properties (read-only)
	uint8_t GetType() const { return static_cast<uint8_t>(m_webhook.type); }
	std::string GetGuildId() const { return std::to_string(m_webhook.guild_id); }
	std::string GetChannelId() const { return std::to_string(m_webhook.channel_id); }
	const char* GetToken() const { return m_webhook.token.c_str(); }
	std::string GetApplicationId() const { return std::to_string(m_webhook.application_id); }
	std::string GetSourceGuildId() const { return std::to_string(m_webhook.source_guild.id); }
	std::string GetSourceChannelId() const { return std::to_string(m_webhook.source_channel.id); }
	const char* GetUrl() const { return m_webhook.url.c_str(); }
	std::string GetImageData() const { return m_webhook.image_data; }
	
	// Webhook management methods
	bool Modify();
	bool Delete();
	bool Execute(const char* message, int allowed_mentions_mask = 0, std::vector<dpp::snowflake> users = {}, std::vector<dpp::snowflake> roles = {});
	bool ExecuteEmbed(const char* message, const class DiscordEmbed* embed, int allowed_mentions_mask = 0, std::vector<dpp::snowflake> users = {}, std::vector<dpp::snowflake> roles = {});
	
	// Static webhook operations
	static bool CreateWebhook(DiscordClient* discord, dpp::webhook wh, IForward *callback_forward, cell_t data);
	static bool GetWebhook(DiscordClient* discord, dpp::snowflake webhook_id, IForward* callback_forward, cell_t data);
	static bool GetChannelWebhooks(DiscordClient* discord, dpp::snowflake channel_id, IForward *callback_forward, cell_t data);
	
	// Internal accessor
	const dpp::webhook& GetDPPWebhook() const { return m_webhook; }
};

class DiscordClient
{
private:
	std::unique_ptr<dpp::cluster> m_cluster;
	Handle_t m_discord_handle;
	std::unique_ptr<std::thread> m_thread;

	std::string m_botId;
	std::string m_botName;
	uint16_t m_botDiscriminator;
	std::string m_botAvatarUrl;

	struct CallbackData {
		IChangeableForward* forward;
		cell_t data;
		CallbackData() : forward(nullptr), data(0) {}
	};

	CallbackData m_readyCallback;
	CallbackData m_messageCallback;
	CallbackData m_errorCallback;
	CallbackData m_slashCommandCallback;
	CallbackData m_autocompleteCallback;

	void RunBot();
	void SetupEventHandlers();

public:
	DiscordClient(const char* token, uint32_t intents = 0);
	~DiscordClient();

	bool Initialize();
	void Start();
	void Stop();
	bool IsRunning() const;
	void SetHandle(Handle_t handle) { 
		m_discord_handle = handle; 
	}
	Handle_t GetHandle() const { return m_discord_handle; }

	// Per-instance forward callback registration methods
	void SetReadyCallback(IChangeableForward* forward, cell_t data = 0) { 
		if (m_readyCallback.forward) forwards->ReleaseForward(m_readyCallback.forward);
		m_readyCallback.forward = forward; 
		m_readyCallback.data = data;
	}
	void SetMessageCallback(IChangeableForward* forward, cell_t data = 0) { 
		if (m_messageCallback.forward) forwards->ReleaseForward(m_messageCallback.forward);
		m_messageCallback.forward = forward; 
		m_messageCallback.data = data;
	}
	void SetErrorCallback(IChangeableForward* forward, cell_t data = 0) { 
		if (m_errorCallback.forward) forwards->ReleaseForward(m_errorCallback.forward);
		m_errorCallback.forward = forward; 
		m_errorCallback.data = data;
	}
	void SetSlashCommandCallback(IChangeableForward* forward, cell_t data = 0) { 
		if (m_slashCommandCallback.forward) forwards->ReleaseForward(m_slashCommandCallback.forward);
		m_slashCommandCallback.forward = forward; 
		m_slashCommandCallback.data = data;
	}
	void SetAutocompleteCallback(IChangeableForward* forward, cell_t data = 0) { 
		if (m_autocompleteCallback.forward) forwards->ReleaseForward(m_autocompleteCallback.forward);
		m_autocompleteCallback.forward = forward; 
		m_autocompleteCallback.data = data;
	}

	bool SetPresence(dpp::presence presence);
	bool ExecuteWebhook(dpp::webhook wh, const char* message, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles);
	bool ExecuteWebhookEmbed(dpp::webhook wh, const char* message, const DiscordEmbed* embed, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles);
	bool SendMessage(dpp::snowflake channel_id, const char* message, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles);
	bool SendMessageEmbed(dpp::snowflake channel_id, const char* message, const DiscordEmbed* embed, int allowed_mentions_mask, std::vector<dpp::snowflake> users, std::vector<dpp::snowflake> roles);
	bool SendDiscordMessage(const DiscordMessage* message);
	bool SendDiscordMessageToChannel(dpp::snowflake channel_id, const DiscordMessage* message);
	bool RegisterSlashCommandObject(dpp::snowflake guild_id, const dpp::slashcommand& command);
	bool RegisterGlobalSlashCommandObject(const dpp::slashcommand& command);
	void CreateAutocompleteResponse(dpp::snowflake id, const std::string &token, const dpp::interaction_response &response);
	bool EditMessage(dpp::snowflake channel_id, dpp::snowflake message_id, const char* content);
	bool EditMessageEmbed(dpp::snowflake channel_id, dpp::snowflake message_id, const char* content, const DiscordEmbed* embed);
	bool DeleteMessage(dpp::snowflake channel_id, dpp::snowflake message_id);
	bool DeleteGuildCommand(dpp::snowflake guild_id, dpp::snowflake command_id);
	bool DeleteGlobalCommand(dpp::snowflake command_id);
	bool BulkDeleteGuildCommands(dpp::snowflake guild_id);
	bool BulkDeleteGlobalCommands();

	// Guild management methods
	bool LeaveGuild(dpp::snowflake guild_id);
	
	// Channel management methods
	bool CreateChannel(dpp::snowflake guild_id, const char* name, dpp::channel_type type, const char* topic = "", dpp::snowflake parent_id = 0, IForward* callback_forward = nullptr, cell_t data = 0);
	bool ModifyChannel(dpp::snowflake channel_id, const std::string& name = "", const std::string& topic = "", uint16_t position = 0, bool nsfw = false, uint16_t rate_limit = 0, uint16_t bitrate = 0, uint8_t user_limit = 0, dpp::snowflake parent_id = 0);
	bool DeleteChannel(dpp::snowflake channel_id);
	
	// Role management methods
	bool CreateRole(dpp::snowflake guild_id, const char* name, uint32_t color = 0, bool hoist = false, bool mentionable = false, uint64_t permissions = 0, IForward* callback_forward = nullptr, cell_t data = 0);
	bool ModifyRole(dpp::snowflake guild_id, dpp::snowflake role_id, const std::string& name = "", uint32_t color = 0, bool hoist = false, bool mentionable = false, uint64_t permissions = 0);
	bool DeleteRole(dpp::snowflake guild_id, dpp::snowflake role_id);
	
	// Member management methods
	bool ModifyMember(dpp::snowflake guild_id, dpp::snowflake user_id, const std::string& nickname = "");
	bool AddMemberRole(dpp::snowflake guild_id, dpp::snowflake user_id, dpp::snowflake role_id);
	bool RemoveMemberRole(dpp::snowflake guild_id, dpp::snowflake user_id, dpp::snowflake role_id);
	bool KickMember(dpp::snowflake guild_id, dpp::snowflake user_id);
	bool BanMember(dpp::snowflake guild_id, dpp::snowflake user_id, const char* reason = nullptr, int delete_message_days = 0);
	bool UnbanMember(dpp::snowflake guild_id, dpp::snowflake user_id);
	bool TimeoutMember(dpp::snowflake guild_id, dpp::snowflake user_id, time_t timeout_until);
	bool RemoveTimeout(dpp::snowflake guild_id, dpp::snowflake user_id);
	
	// Message management methods
	bool PinMessage(dpp::snowflake channel_id, dpp::snowflake message_id);
	bool UnpinMessage(dpp::snowflake channel_id, dpp::snowflake message_id);
	bool AddReaction(dpp::snowflake channel_id, dpp::snowflake message_id, const char* emoji);
	bool RemoveReaction(dpp::snowflake channel_id, dpp::snowflake message_id, const char* emoji);
	bool RemoveAllReactions(dpp::snowflake channel_id, dpp::snowflake message_id);
	
	// Webhook management methods
	bool ModifyWebhook(dpp::snowflake webhook_id, const std::string& name = "", const std::string& avatar_url = "");
	bool DeleteWebhook(dpp::snowflake webhook_id);

	const char* GetBotId() const { return m_botId.c_str(); }
	const char* GetBotName() const { return m_botName.c_str(); }
	uint16_t GetBotDiscriminator() const { return m_botDiscriminator; }
	const char* GetBotAvatarUrl() const { return m_botAvatarUrl.c_str(); }

	uint64_t GetUptime() const;
	
	dpp::cluster* GetCluster() const { return m_cluster.get(); }

	void UpdateBotInfo() {
		if (m_cluster) {
			m_botId = std::to_string(m_cluster->me.id);
			m_botName = m_cluster->me.username;
			m_botDiscriminator = m_cluster->me.discriminator;
			m_botAvatarUrl = m_cluster->me.get_avatar_url();
		}
	}
};

class DiscordInteraction
{
private:
	dpp::slashcommand_t m_interaction;
	std::string m_commandName;
	DiscordClient* m_client;
	mutable Handle_t m_userHandle;

public:
	DiscordInteraction(const dpp::slashcommand_t& interaction, DiscordClient* client) :
		m_interaction(interaction),
		m_commandName(interaction.command.get_command_name()),
		m_client(client),
		m_userHandle(BAD_HANDLE)
	{
	}
	
	~DiscordInteraction() {
		if (m_userHandle != BAD_HANDLE) {
			handlesys->FreeHandle(m_userHandle, nullptr);
		}
	}

	const char* GetCommandName() const { return m_commandName.c_str(); }
	std::string GetGuildId() const { return std::to_string(m_interaction.command.guild_id); }
	std::string GetChannelId() const { return std::to_string(m_interaction.command.channel_id); }
	
	Handle_t GetUserHandle() const;
	
	DiscordUser* GetUser() const { 
		if (m_interaction.command.guild_id != 0 && m_interaction.command.member.user_id != 0) {
			return new DiscordUser(m_interaction.command.usr, m_interaction.command.member, m_client);
		}
		return new DiscordUser(m_interaction.command.usr, m_client); 
	}
	std::string GetUserId() const { return std::to_string(m_interaction.command.usr.id); }
	const char* GetUserName() const { return m_interaction.command.usr.username.c_str(); }
	std::string GetUserNickname() const { return m_interaction.command.member.get_nickname(); }

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

class DiscordAutocompleteInteraction
{
private:
	mutable Handle_t m_userHandle;

public:
	dpp::interaction_response m_response;
	std::string m_commandName;
	dpp::interaction m_command;
	dpp::autocomplete_t m_autocomplete;
	DiscordClient* m_client;

	DiscordAutocompleteInteraction(const dpp::autocomplete_t& autocomplete, DiscordClient* client) :
		m_userHandle(BAD_HANDLE),
		m_response(dpp::ir_autocomplete_reply),
		m_commandName(autocomplete.command.get_command_name()),
		m_command(autocomplete.command),
		m_autocomplete(autocomplete),
		m_client(client)
	{
	}
	
	~DiscordAutocompleteInteraction() {
		if (m_userHandle != BAD_HANDLE) {
			handlesys->FreeHandle(m_userHandle, nullptr);
		}
	}

	const char* GetCommandName() const { return m_commandName.c_str(); }
	std::string GetGuildId() const { return std::to_string(m_command.guild_id); }
	std::string GetChannelId() const { return std::to_string(m_command.channel_id); }
	
	Handle_t GetUserHandle() const;
	
	DiscordUser* GetUser() const { 
		if (m_command.guild_id != 0 && m_command.member.user_id != 0) {
			return new DiscordUser(m_command.usr, m_command.member, m_client);
		}
		return new DiscordUser(m_command.usr, m_client); 
	}
	std::string GetUserNickname() const { return m_command.member.get_nickname(); }

	dpp::command_option GetOption(const char* name) const {
		for (auto & opt : m_autocomplete.options) {
			if (opt.name == name) return opt;
		}

		throw std::runtime_error("Option not found");
	}

	std::string GetOptionValue(const char* name) const {
		return std::get<std::string>(GetOption(name).value);
	}

	int64_t GetOptionValueInt(const char* name) const {
		return std::get<int64_t>(GetOption(name).value);
	}

	double GetOptionValueDouble(const char* name) const {
		return std::get<double>(GetOption(name).value);
	}

	bool GetOptionValueBool(const char* name) const {
		return std::get<bool>(GetOption(name).value);
	}

	void AddAutocompleteOption(dpp::command_option_choice choice) {
		m_response.add_autocomplete_choice(choice);
	}
};

class DiscordGuild
{
private:
	dpp::guild m_guild;
	DiscordClient* m_client;

public:
	DiscordGuild(const dpp::guild& guild, DiscordClient* client) : m_guild(guild), m_client(client) {}

	// Basic guild information
	std::string GetId() const { return std::to_string(m_guild.id); }
	const char* GetName() const { return m_guild.name.c_str(); }
	const char* GetDescription() const { return m_guild.description.c_str(); }
	std::string GetOwnerId() const { return std::to_string(m_guild.owner_id); }
	std::string GetApplicationId() const { return std::to_string(m_guild.application_id); }
	
	// Guild URLs and icons
	std::string GetIconUrl(uint16_t size = 0, bool prefer_animated = true) const { 
		return m_guild.get_icon_url(size, dpp::i_png, prefer_animated); 
	}
	std::string GetBannerUrl(uint16_t size = 0, bool prefer_animated = true) const { 
		return m_guild.get_banner_url(size, dpp::i_png, prefer_animated); 
	}
	std::string GetSplashUrl(uint16_t size = 0) const { 
		return m_guild.get_splash_url(size); 
	}
	std::string GetDiscoverySplashUrl(uint16_t size = 0) const { 
		return m_guild.get_discovery_splash_url(size); 
	}
	
	// Guild features and flags
	bool IsLarge() const { return m_guild.is_large(); }
	bool IsUnavailable() const { return m_guild.is_unavailable(); }
	bool WidgetEnabled() const { return m_guild.widget_enabled(); }
	bool HasInviteSplash() const { return m_guild.has_invite_splash(); }
	bool HasVipRegions() const { return m_guild.has_vip_regions(); }
	bool HasVanityUrl() const { return m_guild.has_vanity_url(); }
	bool IsVerified() const { return m_guild.is_verified(); }
	bool IsPartnered() const { return m_guild.is_partnered(); }
	bool IsCommunity() const { return m_guild.is_community(); }
	bool HasRoleSubscriptions() const { return m_guild.has_role_subscriptions(); }
	bool HasNews() const { return m_guild.has_news(); }
	bool IsDiscoverable() const { return m_guild.is_discoverable(); }
	bool IsFeatureable() const { return m_guild.is_featureable(); }
	bool HasAnimatedIcon() const { return m_guild.has_animated_icon(); }
	bool HasBanner() const { return m_guild.has_banner(); }
	bool IsWelcomeScreenEnabled() const { return m_guild.is_welcome_screen_enabled(); }
	bool HasMemberVerificationGate() const { return m_guild.has_member_verification_gate(); }
	bool IsPreviewEnabled() const { return m_guild.is_preview_enabled(); }
	bool HasAnimatedIconHash() const { return m_guild.has_animated_icon_hash(); }
	bool HasAnimatedBannerHash() const { return m_guild.has_animated_banner_hash(); }
	bool HasMonetizationEnabled() const { return m_guild.has_monetization_enabled(); }
	bool HasMoreStickers() const { return m_guild.has_more_stickers(); }
	bool HasCreatorStorePage() const { return m_guild.has_creator_store_page(); }
	bool HasRoleIcons() const { return m_guild.has_role_icons(); }
	bool HasTicketedEvents() const { return m_guild.has_ticketed_events(); }
	bool HasPremiumProgressBarEnabled() const { return m_guild.has_premium_progress_bar_enabled(); }
	bool HasInvitesDisabled() const { return m_guild.has_invites_disabled(); }
	bool HasAnimatedBanner() const { return m_guild.has_animated_banner(); }
	bool HasAutoModeration() const { return m_guild.has_auto_moderation(); }
	bool HasSupportServer() const { return m_guild.has_support_server(); }
	bool HasRoleSubscriptionsAvailableForPurchase() const { return m_guild.has_role_subscriptions_available_for_purchase(); }
	bool HasRaidAlertsDisabled() const { return m_guild.has_raid_alerts_disabled(); }
	
	// Guild properties
	uint64_t GetMemberCount() const { return dpp::get_user_count(); }
	uint32_t GetMaxPresences() const { return m_guild.max_presences; }
	uint32_t GetMaxMembers() const { return m_guild.max_members; }
	uint16_t GetPremiumSubscriptionCount() const { return m_guild.premium_subscription_count; }
	uint8_t GetMaxVideoChannelUsers() const { return m_guild.max_video_channel_users; }
	uint8_t GetPremiumTier() const { return static_cast<uint8_t>(m_guild.premium_tier); }
	uint8_t GetVerificationLevel() const { return static_cast<uint8_t>(m_guild.verification_level); }
	uint8_t GetExplicitContentFilter() const { return static_cast<uint8_t>(m_guild.explicit_content_filter); }
	uint8_t GetMfaLevel() const { return static_cast<uint8_t>(m_guild.mfa_level); }
	uint8_t GetNsfwLevel() const { return static_cast<uint8_t>(m_guild.nsfw_level); }
	uint8_t GetAfkTimeout() const { return static_cast<uint8_t>(m_guild.afk_timeout); }
	uint8_t GetDefaultMessageNotifications() const { return static_cast<uint8_t>(m_guild.default_message_notifications); }
	
	// Channel IDs
	std::string GetAfkChannelId() const { return std::to_string(m_guild.afk_channel_id); }
	std::string GetSystemChannelId() const { return std::to_string(m_guild.system_channel_id); }
	std::string GetRulesChannelId() const { return std::to_string(m_guild.rules_channel_id); }
	std::string GetPublicUpdatesChannelId() const { return std::to_string(m_guild.public_updates_channel_id); }
	std::string GetWidgetChannelId() const { return std::to_string(m_guild.widget_channel_id); }
	std::string GetSafetyAlertsChannelId() const { return std::to_string(m_guild.safety_alerts_channel_id); }
	
	// Collections
	uint64_t GetRoleCount() const { return dpp::get_role_count(); }
	uint64_t GetEmojiCount() const { return dpp::get_emoji_count(); }
	uint64_t GetChannelCount() const { return dpp::get_channel_count(); }
	size_t GetVoiceMemberCount() const { return m_guild.voice_members.size(); }
	size_t GetThreadCount() const { return m_guild.threads.size(); }
	
	// Collection accessors
	std::string GetRoleId(size_t index) const {
		if (index >= m_guild.roles.size()) return "";
		return std::to_string(m_guild.roles[index]);
	}
	std::string GetChannelId(size_t index) const {
		if (index >= m_guild.channels.size()) return "";
		return std::to_string(m_guild.channels[index]);
	}
	std::string GetThreadId(size_t index) const {
		if (index >= m_guild.threads.size()) return "";
		return std::to_string(m_guild.threads[index]);
	}
	std::string GetEmojiId(size_t index) const {
		if (index >= m_guild.emojis.size()) return "";
		return std::to_string(m_guild.emojis[index]);
	}
	
	// Additional properties
	const char* GetVanityUrlCode() const { return m_guild.vanity_url_code.c_str(); }
	uint32_t GetFlags() const { return m_guild.flags; }
	uint16_t GetFlagsExtra() const { return m_guild.flags_extra; }
	uint16_t GetShardId() const { return m_guild.shard_id; }
	
	// Welcome screen
	bool HasWelcomeScreen() const { return !m_guild.welcome_screen.description.empty(); }
	const char* GetWelcomeScreenDescription() const { return m_guild.welcome_screen.description.c_str(); }
	size_t GetWelcomeChannelCount() const { return m_guild.welcome_screen.welcome_channels.size(); }
	std::string GetWelcomeChannelId(size_t index) const {
		if (index >= m_guild.welcome_screen.welcome_channels.size()) return "";
		return std::to_string(m_guild.welcome_screen.welcome_channels[index].channel_id);
	}
	const char* GetWelcomeChannelDescription(size_t index) const {
		if (index >= m_guild.welcome_screen.welcome_channels.size()) return "";
		return m_guild.welcome_screen.welcome_channels[index].description.c_str();
	}

	// Additional flag checking methods that were missing
	bool HasSevenDayThreadArchive() const { return m_guild.has_seven_day_thread_archive(); }
	bool HasThreeDayThreadArchive() const { return m_guild.has_three_day_thread_archive(); }
	bool HasChannelBanners() const { return m_guild.has_channel_banners(); }
	
	// Additional notification settings - these check for disabled notifications
	bool NoJoinNotifications() const { return (m_guild.flags & dpp::g_no_join_notifications) != 0; }
	bool NoBoostNotifications() const { return (m_guild.flags & dpp::g_no_boost_notifications) != 0; }
	bool NoSetupTips() const { return (m_guild.flags & dpp::g_no_setup_tips) != 0; }
	bool NoStickerGreeting() const { return (m_guild.flags & dpp::g_no_sticker_greeting) != 0; }
	bool NoRoleSubscriptionNotifications() const { return (m_guild.flags_extra & dpp::g_no_role_subscription_notifications) != 0; }
	bool NoRoleSubscriptionNotificationReplies() const { return (m_guild.flags_extra & dpp::g_no_role_subscription_notification_replies) != 0; }
	
	// Permission methods for users
	uint64_t GetBasePermissions(dpp::snowflake user_id) const;
	uint64_t GetPermissionsInChannel(dpp::snowflake user_id, dpp::snowflake channel_id) const;
	bool HasPermission(dpp::snowflake user_id, const char* permission) const;
	bool HasPermissionInChannel(dpp::snowflake user_id, dpp::snowflake channel_id, const char* permission) const;
	
	// Guild management methods
	bool Modify(const char* name = nullptr, const char* description = nullptr);
	
	// Internal accessor
	const dpp::guild& GetDPPGuild() const { return m_guild; }
};

class DiscordSlashCommand
{
private:
	dpp::slashcommand m_command;
	std::vector<dpp::command_option> m_options;
	DiscordClient* m_client;
	dpp::snowflake m_guild_id; // Store guild ID for guild-specific commands
	std::vector<dpp::command_permission> m_permissions; // Store command permissions

public:
	DiscordSlashCommand(DiscordClient* client) : m_client(client), m_guild_id(0) {}

	void SetName(const char* name) { m_command.set_name(name); }
	void SetDescription(const char* description) { m_command.set_description(description); }
	void SetDefaultPermissions(const char* permissions) {
		if (permissions && permissions[0] != '\0') {
			m_command.set_default_permissions(std::stoull(permissions));
		}
	}
	void SetApplicationId(dpp::snowflake app_id) { m_command.set_application_id(app_id); }
	
	// Methods for managing existing commands
	void SetCommandId(dpp::snowflake command_id) { m_command.id = command_id; }
	void SetGuildId(dpp::snowflake guild_id) { m_guild_id = guild_id; }
	dpp::snowflake GetCommandId() const { return m_command.id; }
	dpp::snowflake GetGuildId() const { return m_guild_id; }
	
	const char* GetName() const { return m_command.name.c_str(); }
	const char* GetDescription() const { return m_command.description.c_str(); }
	std::string GetDefaultPermissions() const { 
		char permStr[32];
		snprintf(permStr, sizeof(permStr), "%" PRIu64, static_cast<uint64_t>(m_command.default_member_permissions));
		return std::string(permStr);
	}
	
	void AddOption(const char* name, const char* description, dpp::command_option_type type, bool required = false, bool autocomplete = false) {
		dpp::command_option option(type, name, description, required);
		option.set_auto_complete(autocomplete);
		m_options.push_back(option);
		m_command.options = m_options;
	}
	
	void AddChoiceOption(const char* name, const char* description, dpp::command_option_type type, bool required = false) {
		dpp::command_option option(type, name, description, required);
		m_options.push_back(option);
		m_command.options = m_options;
	}
	
	void AddStringChoice(const char* choice_name, const char* choice_value) {
		if (!m_options.empty()) {
			m_options.back().add_choice(dpp::command_option_choice(choice_name, std::string(choice_value)));
			m_command.options = m_options;
		}
	}
	
	void AddIntChoice(const char* choice_name, int64_t choice_value) {
		if (!m_options.empty()) {
			m_options.back().add_choice(dpp::command_option_choice(choice_name, choice_value));
			m_command.options = m_options;
		}
	}
	
	void AddFloatChoice(const char* choice_name, double choice_value) {
		if (!m_options.empty()) {
			m_options.back().add_choice(dpp::command_option_choice(choice_name, choice_value));
			m_command.options = m_options;
		}
	}
	
	bool RegisterToGuild(dpp::snowflake guild_id) {
		if (!m_client) return false;
		m_command.set_application_id(m_client->GetCluster()->me.id);
		return m_client->RegisterSlashCommandObject(guild_id, m_command);
	}
	
	bool RegisterGlobally() {
		if (!m_client) return false;
		m_command.set_application_id(m_client->GetCluster()->me.id);
		return m_client->RegisterGlobalSlashCommandObject(m_command);
	}
	
	// New advanced functionality methods
	void SetContextMenuType(dpp::slashcommand_contextmenu_type type) { m_command.set_type(type); }
	dpp::slashcommand_contextmenu_type GetContextMenuType() const { return m_command.type; }
	
	void SetNSFW(bool nsfw) { m_command.set_nsfw(nsfw); }
	bool GetNSFW() const { return m_command.nsfw; }
	
	void SetDMPermission(bool dm_permission) { m_command.set_dm_permission(dm_permission); }
	bool GetDMPermission() const { return m_command.dm_permission; }
	
	void AddLocalization(const char* language, const char* name, const char* description = nullptr) {
		if (description && strlen(description) > 0) {
			m_command.add_localization(language, name, description);
		} else {
			m_command.add_localization(language, name);
		}
	}
	
	void SetInteractionContexts(const std::vector<dpp::interaction_context_type>& contexts) {
		m_command.set_interaction_contexts(contexts);
	}
	
	void SetIntegrationTypes(const std::vector<dpp::application_integration_types>& types) {
		m_command.integration_types.clear();
		for (const auto& type : types) {
			m_command.integration_types.push_back(type);
		}
	}
	
	// Option-specific methods for the last added option
	void SetLastOptionMinValue(double min_value) {
		if (!m_options.empty()) {
			m_options.back().set_min_value(min_value);
			m_command.options = m_options;
		}
	}
	
	void SetLastOptionMaxValue(double max_value) {
		if (!m_options.empty()) {
			m_options.back().set_max_value(max_value);
			m_command.options = m_options;
		}
	}
	
	void SetLastOptionMinLength(int64_t min_length) {
		if (!m_options.empty()) {
			m_options.back().set_min_length(min_length);
			m_command.options = m_options;
		}
	}
	
	void SetLastOptionMaxLength(int64_t max_length) {
		if (!m_options.empty()) {
			m_options.back().set_max_length(max_length);
			m_command.options = m_options;
		}
	}
	
	void AddLastOptionChannelType(dpp::channel_type channel_type) {
		if (!m_options.empty()) {
			m_options.back().add_channel_type(channel_type);
			m_command.options = m_options;
		}
	}
	
	std::string GetMention() const { return m_command.get_mention(); }
	
	// Permission override management methods
	void AddPermissionOverride(dpp::snowflake target_id, dpp::command_permission_type type, bool permission) {
		dpp::command_permission perm;
		perm.id = target_id;
		perm.type = type;
		perm.permission = permission;
		
		// Remove existing permission override for the same target and type if it exists
		RemovePermissionOverride(target_id, type);
		
		// Add the new permission override
		m_permissions.push_back(perm);
	}
	
	void RemovePermissionOverride(dpp::snowflake target_id, dpp::command_permission_type type) {
		m_permissions.erase(
			std::remove_if(m_permissions.begin(), m_permissions.end(),
				[target_id, type](const dpp::command_permission& perm) {
					return perm.id == target_id && perm.type == type;
				}),
			m_permissions.end()
		);
	}
	
	void ClearPermissionOverrides() { m_permissions.clear(); }
	
	size_t GetPermissionOverrideCount() const { return m_permissions.size(); }
	
	bool GetPermissionOverride(size_t index, dpp::snowflake& target_id, dpp::command_permission_type& type, bool& permission) const {
		if (index >= m_permissions.size()) return false;
		target_id = m_permissions[index].id;
		type = m_permissions[index].type;
		permission = m_permissions[index].permission;
		return true;
	}
	
	// Command management methods (declared only, implemented in discord.cpp)
	bool Update(dpp::snowflake guild_id = 0);
	bool Delete(dpp::snowflake guild_id = 0);
	bool ApplyPermissionOverrides(dpp::snowflake guild_id);  // Renamed from ModifyPermissions

	const dpp::slashcommand& GetCommand() const { return m_command; }
};

#endif // _INCLUDE_DISCORD_H_ 