#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

#define SMEXT_CONF_NAME			"SourceMod Discord Extension"
#define SMEXT_CONF_DESCRIPTION	"Provide Discord Native"
#define SMEXT_CONF_VERSION		"1.0.2"
#define SMEXT_CONF_AUTHOR		"ProjectSky"
#define SMEXT_CONF_URL			"https://github.com/ProjectSky/sm-ext-discord"
#define SMEXT_CONF_LOGTAG		"discord"
#define SMEXT_CONF_LICENSE		"GPL"
#define SMEXT_CONF_DATESTRING	__DATE__

#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

#define SMEXT_ENABLE_HANDLESYS
#define SMEXT_ENABLE_FORWARDSYS

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
