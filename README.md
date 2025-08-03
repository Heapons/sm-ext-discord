# SourceMod Discord Extension

## Version 2.0 - Breaking Changes Notice ⚠️
**Version 2.0 is a major release with extensive new features and improvements. Due to the significant scope of changes, backward compatibility with older API versions has not been maintained. Many existing APIs have been modified or replaced, requiring updates to existing scripts.**

**Key Breaking Changes:**
- Constructor parameters updated for most classes (now require Discord client parameter)
- Method signatures changed for many operations
- New property-style accessors replace many Get/Set methods
- Enhanced object lifecycle management
- Comprehensive forum functionality added
- Extended embed support with full DPP feature parity
- Permission System
- Role System
- Webhook Enhancement

**Migration Required:** Existing plugins using previous versions will need to be updated to work with v2.0. Please review the examples below for the new API patterns.

**Not Full Test:** This major update contains many untested APIs that could potentially crash or behave unexpectedly.

## Overview
A [SourceMod](http://www.sourcemod.net/) extension that provides Discord bot integration capabilities, allowing you to create and manage Discord bots directly from your SourceMod plugins.

## Available Features
* **Bot Management**: Start, stop, status checking, bot information retrieval, presence setting
* **Message System**: Send, edit, delete messages with rich formatting and allowed mentions
* **Embed Support**: Complete Discord embed functionality with all fields, colors, and media
* **Webhook Integration**: Create, modify, delete, and execute webhooks with full embed support
* **Slash Commands**: Full slash command support with all option types, choices, and autocomplete
* **Permission System**: Complete Discord permission checking with enum support (47 permissions)
* **User Management**: Comprehensive user information, role management, moderation actions
* **Role System**: Role checking, permission validation, role name resolution
* **Event Handling**: Ready, message, command, autocomplete, and error events
* **Channel Management**: Channel information retrieval, modification, and permission checking
* **Forum Support**: Complete forum channel functionality with tags, thread creation, and management
* **Message Components**: Reaction support (add, remove, remove all)
* **Moderation Tools**: Ban, kick, timeout, role management with reason support

## Platform Support
* ⚠️ **Windows Users**: D++ library does not support static compilation on Windows. You need to manually copy the following DLL files to your server's directory:
  - `dpp.dll`
  - `libcrypto-1_1.dll`
  - `libssl-1_1.dll`
  - `opus.dll`

These files can be found in the `bin` folder of the [release package](https://github.com/brainboxdotcc/DPP/releases/tag/v10.1.3). Please download the appropriate version for your platform (x86 or x64).

Steps for Windows users:
1. Download the Windows release package from the [D++ releases page](https://github.com/brainboxdotcc/DPP/releases/tag/v10.1.3)
2. Extract the package
3. Copy all required DLL files from the `bin` folder to your servers directory
4. Make sure the DLL files match your server's architecture (x86 or x64)

## Building
```sh
# Install dependencies
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install cbuild-essential cmake libssl-dev libssl-dev:i386

# Clone and build
git clone https://github.com/ProjectSky/sm-ext-discord
cd sm-ext-discord
mkdir build && cd build
python ../configure.py --enable-optimize --symbol-files --sm-path=YOUR_SOURCEMOD_PATH --targets=x86,x64
ambuild
```

# Binary files
* [GitHub Releases](https://github.com/ProjectSky/sm-ext-discord/releases)

## Basic Usage Example
```cpp
#include <sourcemod>
#include <discord>

Discord g_Discord;

public void OnPluginStart()
{
  // Create Discord bot instance
  g_Discord = new Discord("YOUR_BOT_TOKEN");
  
  // Start the bot
  g_Discord.Start();
}

public void Discord_OnReady(Discord discord)
{
  char botName[32], botId[32];
  discord.GetBotName(botName, sizeof(botName));
  discord.GetBotId(botId, sizeof(botId));
  
  PrintToServer("Bot %s (ID: %s) is ready!", botName, botId);
  
  // Create a simple slash command
  DiscordSlashCommand pingCmd = new DiscordSlashCommand(discord);
  pingCmd.SetName("ping");
  pingCmd.SetDescription("Check bot latency");
  pingCmd.RegisterGlobally();
  delete pingCmd;
}

public void Discord_OnSlashCommand(Discord discord, DiscordInteraction interaction)
{
  char commandName[32];
  interaction.GetCommandName(commandName, sizeof(commandName));

  if (strcmp(commandName, "ping") == 0)
  {
    interaction.CreateResponse("Pong!");
  }
}
```

## Advanced Slash Command with Options Example
```cpp
public void Discord_OnReady(Discord discord)
{
  // Create a ban command with multiple options
  DiscordSlashCommand banCmd = new DiscordSlashCommand(discord);
  banCmd.SetName("ban");
  banCmd.SetDescription("Ban a player from the server");
  
  // Set default permissions - only administrators can use this command
  banCmd.SetDefaultPermissionsEnum(Permission_Administrator);
  
  // Add player option (required, with autocomplete)
  banCmd.AddOption("player", "Select a player to ban", Option_String, true, true);
  
  // Add duration option (required)
  banCmd.AddOption("duration", "Ban duration in minutes", Option_Integer, true, false);
  
  // Add reason option (optional)
  banCmd.AddOption("reason", "Reason for the ban", Option_String, false, false);
  
  banCmd.RegisterGlobally();
  delete banCmd;
}

public void Discord_OnSlashCommand(Discord discord, DiscordInteraction interaction)
{
  char commandName[32];
  interaction.GetCommandName(commandName, sizeof(commandName));
  
  if (strcmp(commandName, "ban") == 0)
  {
    // Check if user has permission to use this command
    DiscordUser user = interaction.User;
    if (!user.HasPermissionEnum(Permission_Administrator))
    {
      interaction.CreateEphemeralResponse("You don't have permission to use this command!");
      return;
    }
    
    char playerName[64], reason[256];
    int duration;
    
    interaction.GetOptionValue("player", playerName, sizeof(playerName));
    duration = interaction.GetOptionValueInt("duration");
    
    if (!interaction.GetOptionValue("reason", reason, sizeof(reason)))
    {
      strcopy(reason, sizeof(reason), "No reason provided");
    }
    
    interaction.DeferReply();
    
    // Perform the ban logic here
    
    char response[512];
    FormatEx(response, sizeof(response), "Banned %s for %d minutes.\nReason: %s", playerName, duration, reason);
    interaction.EditResponse(response);
  }
}
```

## Permission System Example
```cpp
public void Discord_OnSlashCommand(Discord discord, DiscordInteraction interaction)
{
  DiscordUser user = interaction.User;
  
  // Check if user has guild member information
  if (!user.HasGuildMember)
  {
    interaction.CreateEphemeralResponse("This command can only be used in a server!");
    return;
  }
  
  // Check specific permissions using enum
  if (user.HasPermissionEnum(Permission_Administrator))
  {
    PrintToServer("User is an administrator");
  }
  
  // Check permission in specific channel
  char channelId[32];
  interaction.GetChannelId(channelId, sizeof(channelId));
  if (user.HasPermissionInChannelEnum(channelId, Permission_SendMessages))
  {
    PrintToServer("User can send messages in this channel");
  }
  
  // Get all user roles
  char roles[32][32];
  int roleCount = user.GetRoles(roles, sizeof(roles));
  
  for (int i = 0; i < roleCount; i++)
  {
    char roleName[64];
    if (user.GetRoleName(roles[i], roleName, sizeof(roleName)))
    {
      PrintToServer("User has role: %s (ID: %s)", roleName, roles[i]);
    }
  }
}
```

## Rich Embed Example
```cpp
void SendServerStatus()
{
  DiscordEmbed embed = new DiscordEmbed();
  embed.SetTitle("Server Status");
  embed.SetDescription("Current server information");
  embed.SetColor(0x00FF00);  // Green color
  embed.SetAuthor("Game Server", "", "https://example.com/icon.png");
  embed.SetFooter("Last updated", "https://example.com/footer.png");
  embed.SetThumbnail("https://example.com/thumbnail.png");
  
  // Add fields
  embed.AddField("Players", "24/32", true);
  embed.AddField("Map", "de_dust2", true);
  embed.AddField("Status", "Online", true);
  
  g_Discord.SendMessageEmbed("CHANNEL_ID", "", embed);
  delete embed;
}
```

## Webhook Example
```cpp
public void OnPluginStart()
{
  // Create a webhook and execute it with embed
  DiscordWebhook webhook = new DiscordWebhook("https://discord.com/api/webhooks/YOUR_WEBHOOK_URL");
  
  DiscordEmbed embed = new DiscordEmbed();
  embed.SetTitle("Plugin Started");
  embed.SetDescription("The Discord plugin has been loaded successfully");
  embed.SetColor(0x00AA00);
  
  webhook.ExecuteEmbed("Plugin notification:", embed);
  
  delete embed;
  delete webhook;
}
```

## User Information Example
```cpp
public void Discord_OnMessage(Discord discord, DiscordMessage message)
{
  DiscordUser author = message.Author;
  
  char userId[32], username[64], nickname[64];
  author.GetId(userId, sizeof(userId));
  author.GetUserName(username, sizeof(username));
  
  PrintToServer("Message from user %s (ID: %s)", username, userId);
  
  // If in a guild, get additional information
  if (author.HasGuildMember)
  {
    author.GetNickName(nickname, sizeof(nickname));
    if (strlen(nickname) > 0)
    {
      PrintToServer("User nickname: %s", nickname);
    }
    
    if (author.IsPending)
    {
      PrintToServer("User is pending verification");
    }
    
    char joinedAt[32];
    author.GetJoinedAt(joinedAt, sizeof(joinedAt));
    PrintToServer("User joined at: %s", joinedAt);
  }
}
```

### Channel Management Example
```cpp
void ManageChannels()
{
  // Create a channel from ID
  DiscordChannel channel = new DiscordChannel(g_Discord, "123456789012345678");
  
  char channelName[64];
  channel.GetName(channelName, sizeof(channelName));
  PrintToServer("Channel name: %s", channelName);
  
  // Check channel properties
  if (channel.IsTextChannel)
  {
    PrintToServer("This is a text channel");
    PrintToServer("Rate limit: %d seconds", channel.RateLimitPerUser);
  }
  else if (channel.IsVoiceChannel)
  {
    PrintToServer("This is a voice channel");
    PrintToServer("Bitrate: %d", channel.Bitrate);
    PrintToServer("User limit: %d", channel.UserLimit);
  }
  
  delete channel;
}
```

### Message Management Example
```cpp
void ManageMessage()
{
  // Create a message object from IDs
  DiscordMessage message = new DiscordMessage(g_Discord, "MESSAGE_ID", "CHANNEL_ID");
  
  // Add reactions to the message
  message.AddReaction("👍");
  message.AddReaction("👎");
  
  // Pin the message
  message.Pin();
  
  CreateTimer(5.0, Timer_EditMessage, message);
}

Action Timer_EditMessage(Handle timer, any messageHandle)
{
  DiscordMessage message = view_as<DiscordMessage>(messageHandle);
  message.Edit("This message has been edited!");
  delete message;
  return Plugin_Stop;
}
```

### Webhook Management with Static Methods Example
```cpp
void WebhookManagement()
{
  // Method 1: Create webhook from ID and token (limited info)
  DiscordWebhook webhook = DiscordWebhook.CreateFromIdToken("860851945437790209", "RGgZYVTi2TZUQA6t-KRiHcbhldWx_YNg9ZEV_4yyMof5J4EwqaRWo46kaCglIP0Q4C9F");
  
  char webhookId[64];
  webhook.GetId(webhookId, sizeof(webhookId));
  PrintToServer("Webhook ID: %s", webhookId); // This works
  
  char webhookName[64];
  webhook.GetName(webhookName, sizeof(webhookName));
  PrintToServer("Webhook Name: %s (may be empty)", webhookName); // This may be empty
  
  delete webhook;
  
  // Method 2: Get complete webhook info asynchronously (recommended)
  DiscordWebhook.GetWebhook(g_Discord, "860851945437790209", OnWebhookFetched, 0);
  
  // Method 3: Create a webhook using the API
  DiscordWebhook.CreateWebhook(g_Discord, "CHANNEL_ID", "My Bot Webhook", OnWebhookCreated, 0);
}

void OnWebhookFetched(Discord discord, DiscordWebhook webhook, any data)
{
  if (webhook == INVALID_HANDLE)
  {
    PrintToServer("Failed to get webhook information");
    return;
  }
  
  char webhookId[64], webhookName[64];
  webhook.GetId(webhookId, sizeof(webhookId));
  webhook.GetName(webhookName, sizeof(webhookName));
  
  PrintToServer("Retrieved webhook: %s (ID: %s)", webhookName, webhookId);
  
  // Get webhook creator information
  DiscordUser creator = webhook.User;
  if (creator != INVALID_HANDLE)
  {
    char creatorName[64], creatorId[64];
    creator.GetUserName(creatorName, sizeof(creatorName));  
    creator.GetId(creatorId, sizeof(creatorId));
    PrintToServer("Webhook created by: %s (ID: %s)", creatorName, creatorId);
  }
  
  // Execute the webhook with a message
  webhook.Execute("Hello from the retrieved webhook!");
  
  delete webhook;
}

void OnWebhookCreated(Discord discord, DiscordWebhook webhook, any data)
{
  PrintToServer("Webhook created successfully!");
  
  char webhookId[32];
  webhook.GetId(webhookId, sizeof(webhookId));
  PrintToServer("Webhook ID: %s", webhookId);
  
  // Execute the webhook with a message
  webhook.Execute("Hello from the newly created webhook!");
  
  // Get all webhooks in a channel
  DiscordWebhook.GetChannelWebhooks(discord, "CHANNEL_ID", OnChannelWebhooksReceived, 0);
  
  delete webhook;
}

void OnChannelWebhooksReceived(Discord discord, DiscordWebhook[] webhooks, int count, any data)
{
  PrintToServer("Found %d webhooks in channel", count);
  
  for (int i = 0; i < count; i++)
  {
    char webhookName[64];
    webhooks[i].GetName(webhookName, sizeof(webhookName));
    PrintToServer("Webhook %d: %s", i + 1, webhookName);
  }
}
```

### Advanced User Management Example
```cpp
void AdvancedUserManagement()
{
  // Method 1: Create user object directly (may have limited info)
  DiscordUser user = new DiscordUser(g_Discord, "USER_ID", "GUILD_ID");
  
  char username[64], nickname[64];
  user.GetUserName(username, sizeof(username));
  PrintToServer("User: %s (may be empty if not cached)", username);
  
  // Method 2: Fetch complete user info asynchronously (recommended)
  DiscordUser.FetchUser(g_Discord, "192591975612940288", OnUserFetched, 0);
}

void OnUserFetched(Discord discord, DiscordUser user, any data)
{
  if (user == INVALID_HANDLE)
  {
    PrintToServer("Failed to fetch user information");
    return;
  }
  
  char username[64], userId[32];
  user.GetUserName(username, sizeof(username));
  user.GetId(userId, sizeof(userId));
  
  PrintToServer("Fetched user: %s (ID: %s)", username, userId);
  
  // Check if guild member info is available
  if (user.HasGuildMember)
  {
    char nickname[64];
    user.GetNickName(nickname, sizeof(nickname));
    if (strlen(nickname) > 0)
    {
      PrintToServer("User nickname: %s", nickname);
    }
    
    // Check if user has specific roles
    if (user.HasRole("MODERATOR_ROLE_ID"))
    {
      PrintToServer("User is a moderator");
      
      // Set a timeout for the user (5 minutes)
      user.SetTimeout(GetTime() + 300);
    }
    
    // Get all user roles
    char roles[10][32];
    int roleCount = user.GetRoles(roles, sizeof(roles));
    
    for (int i = 0; i < roleCount; i++)
    {
      char roleName[64];
      if (user.GetRoleName(roles[i], roleName, sizeof(roleName)))
      {
        PrintToServer("Role: %s", roleName);
      }
    }
  }
  else
  {
    PrintToServer("No guild member info available - user may not be in any mutual servers");
  }
}

void AdvancedUserManagement_Old()
{
  // Create a user object from ID and guild ID
  DiscordUser user = new DiscordUser(g_Discord, "USER_ID", "GUILD_ID");
  
  char username[64], nickname[64];
  user.GetUserName(username, sizeof(username));
  
  // Check if guild member info is available before accessing member-specific data
  if (user.HasGuildMember)
  {
    user.GetNickName(nickname, sizeof(nickname));
    PrintToServer("User: %s (Nickname: %s)", username, nickname);
    
    // Check if user has specific roles
    if (user.HasRole("MODERATOR_ROLE_ID"))
    {
      PrintToServer("User is a moderator");
      
      // Set a timeout for the user (5 minutes)
      user.SetTimeout(GetTime() + 300);
    }
    
    // Get all user roles
    char roles[10][32];
    int roleCount = user.GetRoles(roles, sizeof(roles));
    
    for (int i = 0; i < roleCount; i++)
    {
      char roleName[64];
      if (user.GetRoleName(roles[i], roleName, sizeof(roleName)))
      {
        PrintToServer("Role: %s", roleName);
      }
    }
  }
  else
  {
    PrintToServer("User: %s (No guild member info available)", username);
  }
}
```

### Event Handling Example
```cpp
public void Discord_OnMessage(Discord discord, DiscordMessage message)
{
  char content[512];
  message.GetContent(content, sizeof(content));
  
  // Ignore bot messages
  if (message.IsBot)
    return;
  
  DiscordUser author = message.Author;
  char username[64];
  author.GetUserName(username, sizeof(username));
  
  PrintToServer("[Discord] %s: %s", username, content);
  
  // Auto-react to messages containing specific keywords
  if (StrContains(content, "good job", false) != -1)
  {
    message.AddReaction("👏");
  }
  
  // Moderate messages
  if (StrContains(content, "spam", false) != -1)
  {
    message.Delete();
    
    char channelId[32];
    message.GetChannelId(channelId, sizeof(channelId));
    
    discord.SendMessage(channelId, "Please don't spam in this channel!");
  }
}

public void Discord_OnSlashCommand(Discord discord, DiscordInteraction interaction)
{
  char commandName[32];
  interaction.GetCommandName(commandName, sizeof(commandName));
  
  if (strcmp(commandName, "server") == 0)
  {
    // Create detailed server info embed
    DiscordEmbed embed = new DiscordEmbed();
    embed.SetTitle("Server Information");
    embed.SetColor(0x007ACC);
    
    char playerCount[16], mapName[64];
    FormatEx(playerCount, sizeof(playerCount), "%d/%d", GetClientCount(false), MaxClients);
    GetCurrentMap(mapName, sizeof(mapName));
    
    embed.AddField("Players", playerCount, true);
    embed.AddField("Map", mapName, true);
    embed.AddField("Server IP", "connect your.server.ip:27015", false);
    
    // Add server uptime
    char uptime[32];
    FormatEx(uptime, sizeof(uptime), "%.1f hours", GetEngineTime() / 3600.0);
    embed.AddField("Uptime", uptime, true);
    
    interaction.CreateResponseEmbed("", embed);
    delete embed;
  }
  else if (strcmp(commandName, "players") == 0)
  {
    char playerList[1024] = "";
    bool hasPlayers = false;
    
    for (int i = 1; i <= MaxClients; i++)
    {
      if (IsClientConnected(i) && !IsFakeClient(i))
      {
        char playerName[64];
        GetClientName(i, playerName, sizeof(playerName));
        
        if (hasPlayers)
          StrCat(playerList, sizeof(playerList), "\n");
        
        StrCat(playerList, sizeof(playerList), playerName);
        hasPlayers = true;
      }
    }
    
    if (!hasPlayers)
    {
      interaction.CreateResponse("No players currently online.");
    }
    else
    {
      DiscordEmbed embed = new DiscordEmbed();
      embed.SetTitle("Online Players");
      embed.SetDescription(playerList);
      embed.SetColor(0x00FF00);
      
      interaction.CreateResponseEmbed("", embed);
      delete embed;
    }
  }
}

public void Discord_OnAutocomplete(Discord discord, DiscordAutocompleteInteraction interaction)
{
  char commandName[32];
  interaction.GetCommandName(commandName, sizeof(commandName));
  
  if (strcmp(commandName, "ban") == 0)
  {
    char playerInput[64];
    interaction.GetOptionValue("player", playerInput, sizeof(playerInput));
    
    // Add online players as autocomplete options
    for (int i = 1; i <= MaxClients; i++)
    {
      if (IsClientConnected(i) && !IsFakeClient(i))
      {
        char playerName[64];
        GetClientName(i, playerName, sizeof(playerName));
        
        // Filter by input
        if (strlen(playerInput) == 0 || StrContains(playerName, playerInput, false) != -1)
        {
          interaction.AddAutocompleteChoiceString(playerName, playerName);
        }
      }
    }
  }
}
```

## Advanced Slash Command Management Example
```cpp
void ManageExistingCommands()
{
  // Load an existing global command by ID
  DiscordSlashCommand globalCmd = DiscordSlashCommand.FromGlobalCommand(g_Discord, "123456789012345678");
  if (globalCmd != null)
  {
    char cmdName[64];
    globalCmd.GetName(cmdName, sizeof(cmdName));
    PrintToServer("Loaded global command: %s", cmdName);
    
    // Update the command description
    globalCmd.SetDescription("Updated description for global command");
    globalCmd.Update(); // Update globally
    
    delete globalCmd;
  }
  
  // Load an existing guild-specific command by ID
  DiscordSlashCommand guildCmd = DiscordSlashCommand.FromGuildCommand(g_Discord, "987654321098765432", "GUILD_ID");
  if (guildCmd != null)
  {
    char cmdName[64];
    guildCmd.GetName(cmdName, sizeof(cmdName));
    PrintToServer("Loaded guild command: %s", cmdName);
    
    // Delete the guild command
    guildCmd.Delete("GUILD_ID");
    
    delete guildCmd;
  }
}
```

## Forum Management Example
```cpp
void ManageForumChannel()
{
  // Create a forum channel
  DiscordChannel forum = new DiscordChannel(g_Discord, "FORUM_CHANNEL_ID");
  
  // Check if it's a forum channel
  if (!forum.IsForum)
  {
    PrintToServer("This is not a forum channel");
    delete forum;
    return;
  }
  
  // Get existing forum tags
  int tagCount = forum.AvailableTagCount;
  PrintToServer("Forum has %d available tags", tagCount);
  
  for (int i = 0; i < tagCount; i++)
  {
    char tagName[64], tagEmoji[32];
    forum.GetAvailableTagName(i, tagName, sizeof(tagName));
    forum.GetAvailableTagEmoji(i, tagEmoji, sizeof(tagEmoji));
    bool isModerated = forum.GetAvailableTagModerated(i);
    bool isCustomEmoji = forum.GetAvailableTagEmojiIsCustom(i);
    
    PrintToServer("Tag %d: %s %s (Moderated: %s, Custom Emoji: %s)", 
                  i + 1, tagName, tagEmoji, 
                  isModerated ? "Yes" : "No",
                  isCustomEmoji ? "Yes" : "No");
  }
  
  // Create a new forum tag with emoji
  if (forum.CreateForumTag("Bug Report", "🐛", true))
  {
    PrintToServer("Created moderated bug report tag");
  }
  
  // Create a forum tag with custom emoji (using emoji ID)
  if (forum.CreateForumTag("Feature Request", "123456789012345678", false))
  {
    PrintToServer("Created feature request tag with custom emoji");
  }
  
  delete forum;
}

void CreateForumThread()
{
  DiscordChannel forum = new DiscordChannel(g_Discord, "FORUM_CHANNEL_ID");
  
  // Create a simple forum thread
  if (forum.CreateForumThread("Server Bug Report", "There's an issue with the spawn system.", {}, {}, 1440, 0))
  {
    PrintToServer("Created forum thread successfully");
  }
  
  // Create a forum thread with specific tags
  char tagIds[2][32];
  strcopy(tagIds[0], sizeof(tagIds[]), "TAG_ID_1");
  strcopy(tagIds[1], sizeof(tagIds[]), "TAG_ID_2");
  
  if (forum.CreateForumThread("Feature Discussion", "Let's discuss new features for the server.", tagIds, 2, 2880, 5))
  {
    PrintToServer("Created tagged forum thread with auto-archive in 2 days and 5 second rate limit");
  }
  
  delete forum;
}
```

## Forum Tag Management Example
```cpp
void WorkWithForumTags()
{
  // Create individual forum tags
  DiscordForumTag bugTag = new DiscordForumTag("Bug", "🐛", true);
  DiscordForumTag featureTag = new DiscordForumTag("Feature", "✨", false);
  DiscordForumTag helpTag = new DiscordForumTag("Help", "", false); // No emoji
  
  // Get tag information
  char tagName[64], tagEmoji[32];
  bugTag.GetName(tagName, sizeof(tagName));
  bugTag.GetEmoji(tagEmoji, sizeof(tagEmoji));
  bool isModerated = bugTag.IsModerated;
  bool hasCustomEmoji = bugTag.EmojiIsCustom;
  
  PrintToServer("Bug tag: %s %s (Moderated: %s, Custom: %s)", 
                tagName, tagEmoji,
                isModerated ? "Yes" : "No",
                hasCustomEmoji ? "Yes" : "No");
  
  // Modify tag properties
  featureTag.SetName("New Feature");
  featureTag.SetEmoji("⭐");
  featureTag.IsModerated = true;
  
  // Use custom emoji by ID
  helpTag.SetEmoji("987654321098765432"); // Custom emoji snowflake ID
  
  delete bugTag;
  delete featureTag;
  delete helpTag;
}
```

## TODO
- [x] Reaction support (AddReaction, RemoveReaction, RemoveAllReactions)
- [x] Forum support (forum channels, tags, thread creation)
- [ ] Message component support (buttons, select menus)
- [ ] Thread support (general thread management)