# SourceMod Discord Extension

## Overview
A [SourceMod](http://www.sourcemod.net/) extension that provides Discord bot integration capabilities, allowing you to create and manage Discord bots directly from your SourceMod plugins.

## Features
* Built with [D++](https://github.com/brainboxdotcc/DPP), a modern C++ Discord API library
* Support for basic bot operations (sending messages, managing commands)
* Slash command support with options
* Message embed support
* Support x64

## Platform Support
* ⚠️ **Windows Users**: D++ library does not support static compilation on Windows. You need to manually copy the following DLL files to your server's directory:
  - `dpp.dll`
  - `libcrypto-1_1.dll`
  - `libssl-1_1.dll`
  - `opus.dll`
  - `zlib1.dll`

These files can be found in the `bin` folder of the [release package](https://github.com/brainboxdotcc/DPP/releases/tag/v10.0.35). Please download the appropriate version for your platform (x86 or x64).

Steps for Windows users:
1. Download the Windows release package from the [D++ releases page](https://github.com/brainboxdotcc/DPP/releases/tag/v10.0.35)
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
  
  // Register a slash command
  g_Discord.RegisterGlobalSlashCommand("ping", "Check bot latency");
}

public void Discord_OnReady(Discord discord)
{
  char botName[32], botId[32];
  discord.GetBotName(botName, sizeof(botName));
  discord.GetBotId(botId, sizeof(botId));
  
  PrintToServer("Bot %s (ID: %s) is ready!", botName, botId);
}

public void Discord_OnSlashCommand(Discord discord, DiscordInteraction interaction)
{
  char commandName[32];
  interaction.GetCommandName(commandName, sizeof(commandName));

  if (strcmp(commandName, "ping") == 0)
  {
    interaction.CreateResponse("Pong!");
  }
  
  if (strcmp(commandName, "ban") == 0)
  {
    char playerName[64], reason[256];
    int duration;
    
    interaction.GetOptionValue("player", playerName, sizeof(playerName));
    duration = interaction.GetOptionValueInt("duration");
    
    if (!interaction.GetOptionValue("reason", reason, sizeof(reason)))
    {
      strcopy(reason, sizeof(reason), "No reason provided");
    }
    
    interaction.DeferReply();
    
    char response[512];
    FormatEx(response, sizeof(response), "Banned %s for %d minutes.\nReason: %s", playerName, duration, reason);
    interaction.EditResponse(response);
  }
}
```

## Slash Command with Options Example
```cpp
public void OnPluginStart()
{
  // Register command with multiple options
  char option_names[3][64];
  char option_descriptions[3][256];
  DiscordCommandOptionType option_types[3];
  bool option_required[3];
  bool option_autocomplete[3];
  
  // Player option
  strcopy(option_names[0], sizeof(option_names[]), "player");
  strcopy(option_descriptions[0], sizeof(option_descriptions[]), "Select a player");
  option_types[0] = Option_String;
  option_required[0] = true;
  option_autocomplete[0] = true;
  
  // Duration option
  strcopy(option_names[1], sizeof(option_names[]), "duration");
  strcopy(option_descriptions[1], sizeof(option_descriptions[]), "Ban duration in minutes");
  option_types[1] = Option_Integer;
  option_required[1] = true;
  option_autocomplete[1] = false;
  
  // Reason option
  strcopy(option_names[2], sizeof(option_names[]), "reason");
  strcopy(option_descriptions[2], sizeof(option_descriptions[]), "Ban reason");
  option_types[2] = Option_String;
  option_required[2] = false;
  option_autocomplete[2] = false;
  
  g_Discord.RegisterGlobalSlashCommandWithOptions("ban", "Ban a player from the server", "0", option_names, option_descriptions, option_types, option_required, option_autocomplete, 3);
}
```

## Message Embed Example
```cpp
public void SendEmbed()
{
  DiscordEmbed embed = new DiscordEmbed();
  embed.SetTitle("Server Status");
  embed.SetDescription("Current server information");
  embed.SetColor(0x00FF00);  // Green color
  embed.AddField("Players", "24/32", true);
  embed.AddField("Map", "de_dust2", true);
  
  g_Discord.SendMessageEmbed("CHANNEL_ID", "Server status update:", embed);
  delete embed;
}
```

## Available Features
* Basic bot operations (start, stop, status)
* Message sending and management
* Webhook support (Creation and execution)
* Slash command registration and handling
* Slash command autocomplete handling
* Rich embed support
* Event handling (ready, messages, commands, autocomplete, errors)
* Command option support (string, integer, boolean, user, channel, role)

## Notes
* ⚠️ **BETA VERSION**: This extension is currently in beta testing. Some features may not work as expected or could cause server crashes.
* The extension runs the Discord bot in a separate thread to avoid blocking the game server
* All callbacks are executed on the main thread for thread safety
* Make sure to properly handle bot token security

## TODO
- [ ] Message component support (buttons, select menus)
- [ ] Reaction support
- [ ] Thread support
- [ ] Permission system integration