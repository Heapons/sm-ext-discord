# SourceMod Discord Extension

## Overview
A [SourceMod](http://www.sourcemod.net/) extension that provides Discord bot integration capabilities, allowing you to create and manage Discord bots directly from your SourceMod plugins.

## Features
* Built with [D++](https://github.com/brainboxdotcc/DPP), a modern C++ Discord API library
* Support for basic bot operations (sending messages, managing commands)
* Slash command support with options
* Message embed support
* Support x64

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
  char botName[32];
  discord.GetBotName(botName, sizeof(botName));
  PrintToServer("Bot %s is ready!", botName);
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

## Slash Command with Options Example
```cpp
public void OnPluginStart()
{
  // Create option arrays
  char option_names[2][64];
  char option_descriptions[2][256];
  DiscordCommandOptionType option_types[2];
  bool option_required[2];
  
  // Setup first option
  strcopy(option_names[0], sizeof(option_names[]), "user");
  strcopy(option_descriptions[0], sizeof(option_descriptions[]), "Target user");
  option_types[0] = Option_User;
  option_required[0] = true;
  
  // Setup second option
  strcopy(option_names[1], sizeof(option_names[]), "reason");
  strcopy(option_descriptions[1], sizeof(option_descriptions[]), "Ban reason");
  option_types[1] = Option_String;
  option_required[1] = false;
  
  // Register command with options
  g_Discord.RegisterGlobalSlashCommandWithOptions("ban", "Ban a user", option_names, option_descriptions, option_types, option_required, 2);
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
* Slash command registration and handling
* Rich embed support
* Event handling (ready, messages, commands, errors)
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