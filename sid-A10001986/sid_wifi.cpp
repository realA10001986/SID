/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * WiFi and Config Portal handling
 *
 * -------------------------------------------------------------------
 * License: MIT NON-AI
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * In addition, the following restrictions apply:
 * 
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "sid_global.h"

#include <Arduino.h>

#include "src/WiFiManager/WiFiManager.h"

#ifndef WM_MDNS
#define SID_MDNS
#include <ESPmDNS.h>
#endif

#include "sid_settings.h"
#include "sid_wifi.h"
#include "sid_main.h"
#ifdef SID_HAVEMQTT
#include "mqtt.h"
#endif

#define STRLEN(x) (sizeof(x)-1)

Settings settings;

IPSettings ipsettings;

WiFiManager wm;
bool wifiSetupDone = false;

#ifdef SID_HAVEMQTT
WiFiClient mqttWClient;
PubSubClient mqttClient(mqttWClient);
#endif

static const char *apChannelCustHTMLSrc[14] = {
    "'>WiFi channel",
    "apchnl",
    ">Random%s1'",
    ">1%s2'",
    ">2%s3'",
    ">3%s4'",
    ">4%s5'",
    ">5%s6'",
    ">6%s7'",
    ">7%s8'",
    ">8%s9'",
    ">9%s10'",
    ">10%s11'",
    ">11%s"
};

#ifdef SID_HAVEMQTT
static const char *mqttpCustHTMLSrc[4] = {
    "'>Protocol version",
    "mprot",
    ">3.1.1%s1'",
    ">5.0%s"
};
static const char mqttMsgDisabled[] = "Disabled";
static const char mqttMsgResolvErr[] = "DNS lookup error";
static const char mqttMsgConnecting[] = "Connecting...";
static const char mqttMsgTimeout[] = "Connection time-out";
static const char mqttMsgFailed[] = "Connection failed";
static const char mqttMsgDisconnected[] = "Disconnected";
static const char mqttMsgConnected[] = "Connected";
static const char mqttMsgBadProtocol[] = "Protocol error";
static const char mqttMsgUnavailable[] = "Server unavailable/busy";
static const char mqttMsgBadCred[] = "Login failed";
static const char mqttMsgGenError[] = "Error";
#endif

static const char *wmBuildApChnl(const char *dest, int op);
static const char *wmBuildBestApChnl(const char *dest, int op);

static const char *wmBuildHaveSD(const char *dest, int op);

#ifdef SID_HAVEMQTT
static const char *wmBuildMQTTprot(const char *dest, int op);
static const char *wmBuildMQTTstate(const char *dest, int op);
#endif

static const char custHTMLHdr1[] = "<div class='cmp0";
static const char custHTMLHdrI[] = " ml20";
static const char custHTMLHdr2[] = "'><label class='mp0' for='";
static const char custHTMLSHdr[] = "</label><select class='sel0' value='";
static const char osde[] = "</option></select></div>";
static const char ooe[]  = "</option><option value='";
static const char custHTMLSel[] = " selected";
static const char custHTMLSelFmt[] = "' name='%s' id='%s' autocomplete='off'><option value='0'";
static const char col_g[] = "609b71";
static const char col_r[] = "dc3630";
static const char col_gr[] = "777";

// double-% since this goes through sprintf!
static const char bannerStart[] = "<div class='c' style='background-color:#";
static const char bannerMid[] = ";color:#fff;font-size:80%;border-radius:5px'>";

static const char bestAP[]   = "%s%s%sProposed channel at current location: %d<br>%s(Non-WiFi devices not taken into account)</div>";
static const char badWiFi[]  = "<br><i>Operating in AP mode not recommended</i>";

static const char bannerGen[] = "%s%s%s%s</div>";
static const char haveNoSD[] = "<i>No SD card present</i>";

#ifdef SID_HAVEMQTT
static const char mqttStatus[] = "%s%s%s%s%s (%d)</div>";
#endif

// WiFi Configuration

#if defined(SID_MDNS) || defined(WM_MDNS)
#define HNTEXT "Hostname<br><span>The Config Portal is accessible at http://<i>hostname</i>.local<br>(Valid characters: a-z/0-9/-)</span>"
#else
#define HNTEXT "Hostname<br><span>(Valid characters: a-z/0-9/-)</span>"
#endif
WiFiManagerParameter custom_hostName("hostname", HNTEXT, settings.hostName, 31, "pattern='[A-Za-z0-9\\-]+' placeholder='Example: sid'", WFM_LABEL_BEFORE|WFM_SECTS_HEAD);

WiFiManagerParameter custom_sectstart_wifi("WiFi connection: Other settings", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_wifiConRetries("wifiret", "Connection attempts (1-10)", settings.wifiConRetries, 2, "type='number' min='1' max='10'");

WiFiManagerParameter custom_sectstart_ap("Access point (AP) mode settings", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_sysID("sysID", "Network name (SSID) appendix<br><span>Will be appended to \"SID-AP\" [a-z/0-9/-]</span>", settings.systemID, 7, "pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_appw("appw", "Password<br><span>Password to protect SID-AP. Empty or 8 characters [a-z/0-9/-]</span>", settings.appw, 8, "minlength='8' pattern='[A-Za-z0-9\\-]+'");
WiFiManagerParameter custom_apch(wmBuildApChnl);
WiFiManagerParameter custom_bapch(wmBuildBestApChnl);
WiFiManagerParameter custom_wifiAPOffDelay("wifiAPoff", "Power save timer<br><span>(10-99[minutes]; 0=off)</span>", settings.wifiAPOffDelay, 2, "type='number' min='0' max='99' title='WiFi-AP will be shut down after chosen period. 0 means never.'");
WiFiManagerParameter custom_wifihint("<div style='margin:0;padding:0;font-size:80%'>Enter *77OK to re-enable Wifi when in power save mode</div>", WFM_FOOT);

// Settings

WiFiManagerParameter custom_sStrict("sStrict", "Adhere strictly to movie patterns<br><span>Check to strictly show movie patterns in idle modes 0-3 and with TCD-provided speed; uncheck to allow variations.</span>", settings.strictMode, "class='mt5' style='margin-bottom:0px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS_HEAD);
WiFiManagerParameter custom_sTTANI("sTTANI", "Skip time tunnel animation", settings.skipTTAnim, "title='Check to skip the time tunnel animation'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_SApeaks("sap", "Show peaks in Spectrum Analyzer", settings.SApeaks, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_PIRFB("pir", "Show positive IR feedback on display", settings.PIRFB, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_PIRCFB("pirc", "Show IR command entry feedback on display", settings.PIRCFB, "class='mb10'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ssDelay("ssDel", "Screen Saver timer (1-999[minutes]; 0=off)", settings.ssTimer, 3, "type='number' min='0' max='999'");

WiFiManagerParameter custom_sectstart_nw("Wireless communication (BTTF-Network)", WFM_SECTS|WFM_HL);
WiFiManagerParameter custom_tcdIP("tcdIP", "IP address or hostname of TCD", settings.tcdIP, 31, "pattern='(^((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.?\\b){4}$)|([A-Za-z0-9\\-]+)' placeholder='Example: 192.168.4.1'");
WiFiManagerParameter custom_uGPS("uGPS", "Adapt pattern to TCD-provided speed<br><span>Speed from TCD (GPS, rotary encoder, remote control), if available, will overrule idle pattern</span>", settings.useGPSS, "class='mb0'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_uNM("uNM", "Follow TCD night-mode<br><span>If checked, the Screen Saver will activate when TCD is in night-mode.</span>", settings.useNM, "class='mb0'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_uFPO("uFPO", "Follow TCD fake power", settings.useFPO, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_bttfnTT("bttfnTT", "'0' and button trigger BTTFN-wide TT<br><span>If checked, pressing '0' on the IR remote or pressing the Time Travel button triggers a BTTFN-wide TT</span>", settings.bttfnTT, "class='mb0'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ssClock("ssClk", "Show clock when Screen Saver is active", settings.ssClock, "", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
WiFiManagerParameter custom_ssClockO("ssClkO", "Clock off in Night Mode", settings.ssClockOffNM, "class='mb0 mt5 ml20'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);

WiFiManagerParameter custom_TCDpresent("TCDpres", "TCD connected by wire", settings.TCDpresent, "title='Check if you have a Time Circuits Display connected via wire' class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS);
WiFiManagerParameter custom_noETTOL("uEtNL", "TCD signals Time Travel without 5s lead", settings.noETTOLead, "class='mt5 ml20'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);

WiFiManagerParameter custom_haveSD(wmBuildHaveSD, WFM_SECTS);
WiFiManagerParameter custom_CfgOnSD("CfgOnSD", "Save secondary settings on SD<br><span>Check this to avoid flash wear</span>", settings.CfgOnSD, "class='mt5 mb0'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);
//WiFiManagerParameter custom_sdFrq("sdFrq", "4MHz SD clock speed<br><span>Checking this might help in case of SD card problems</span>", settings.sdFreq, "style='margin-top:12px'", WFM_LABEL_AFTER|WFM_IS_CHKBOX);

WiFiManagerParameter custom_disDIR("dDIR", "Disable supplied IR control", settings.disDIR, "title='Check to disable the supplied IR remote control' class='mt5'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS|WFM_FOOT);

#ifdef SID_HAVEMQTT
WiFiManagerParameter custom_useMQTT("uMQTT", "Home Assistant support (MQTT)", settings.useMQTT, "class='mt5 mb10'", WFM_LABEL_AFTER|WFM_IS_CHKBOX|WFM_SECTS_HEAD);
WiFiManagerParameter custom_state(wmBuildMQTTstate);
WiFiManagerParameter custom_mqttServer("ha_server", "Broker IP[:port] or domain[:port]", settings.mqttServer, 79, "pattern='[a-zA-Z0-9\\.:\\-]+' placeholder='Example: 192.168.1.5'");
WiFiManagerParameter custom_mqttVers(wmBuildMQTTprot);
WiFiManagerParameter custom_mqttUser("ha_usr", "User[:Password]", settings.mqttUser, 63, "placeholder='Example: ronald:mySecret'");
WiFiManagerParameter custom_mqttTopic("MQt", "Topic to display", settings.mqttTopic, 63, "placeholder='Optional. Example: home/alarm/status'", WFM_LABEL_BEFORE|WFM_SECTS|WFM_FOOT);
#endif // HAVEMQTT

static const int8_t wifiMenu[] = { 
    WM_MENU_WIFI,
    WM_MENU_PARAM,
    #ifdef SID_HAVEMQTT
    WM_MENU_PARAM2,
    #endif
    WM_MENU_SEP_F,
    WM_MENU_UPDATE,
    WM_MENU_SEP,
    WM_MENU_CUSTOM,
    WM_MENU_END
};
#define TC_MENUSIZE (sizeof(wifiMenu) / sizeof(wifiMenu[0]))

#define AA_TITLE "SID"
#define AA_ICON "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQBAMAAADt3eJSAAAAD1BMVEWOkJHMy8dDtiT22DHyJT118zqFAAAALUlEQVQI12MQhAIGAQYwYAQzXGAMY0wGswGQYYzBAJIQhhKTAhARxYBbCncGAAUkB7Vkqvl1AAAAAElFTkSuQmCC"
#define UNI_VERSION SID_VERSION 
#define UNI_VERSION_EXTRA SID_VERSION_EXTRA
#define WEBHOME "sid"
#define PARM2TITLE WM_PARAM2_TITLE
#define PARM3TITLE ""
#define CURRVERSION SID_VERSION
static const char r_link[] = "sidr.out-a-ti.me";
static const char apName[]  = "SID-AP";

static const char myTitle[] = AA_TITLE;
static const char myHead[]  = "<link rel='icon' type='image/png' href='data:image/png;base64," AA_ICON "'><script>window.onload=function(){xxx='" AA_TITLE "';yyy='?';wr=ge('wrap');if(wr){aa=ge('h3');if(aa){yyy=aa.innerHTML;aa.remove();dlel('h1')}zz=(Math.random()>0.8);dd=document.createElement('div');dd.classList.add('tpm0');dd.innerHTML='<div class=\"tpm\" onClick=\"shsp(1);window.location=\\'/\\'\"><div class=\"tpm2\"><img id=\"spi\" src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABAAQMAAACQp+OdAAAABlBMVEUAAABKnW0vhlhrAAAAAXRSTlMAQObYZgAAA'+(zz?'GBJREFUKM990aEVgCAABmF9BiIjsIIbsJYNRmMURiASePwSDPD0vPT12347GRejIfaOOIQwigSrRHDKBK9CCKoEqQF2qQMOSQAzEL9hB9ICNyMv8DPKgjCjLtAD+AV4dQM7O4VX9m1RYAAAAABJRU5ErkJggg==':'HtJREFUKM990bENwyAUBuFnuXDpNh0rZIBIrJUqMBqjMAIlBeIihQIF/fZVX39229PscYG32esCzeyjsXUzNHZsI0ocxJ0kcZIOsoQjnxQJT3FUiUD1NAloga6wQQd+4B/7QBQ4BpLAOZAn3IIy4RfUibCgTTDq+peG6AvsL/jPTu1L9wAAAABJRU5ErkJggg==')+'\" class=\"tpm3\"></div><H1 class=\"tpmh1\"'+(zz?' style=\"margin-left:1.4em\"':'')+'>'+xxx+'</H1>'+'<H3 class=\"tpmh3\"'+(zz?' style=\"padding-left:5em\"':'')+'>'+yyy+'</div></div>';wr.insertBefore(dd,wr.firstChild);wr.style.position='relative'}var lc=ge('lc');if(lc){lc.style.transform='rotate('+(358+[0,1,3,4,5][Math.floor(Math.random()*4)])+'deg)'}}</script><style>H1{font-family:Bahnschrift,-apple-system,'Segoe UI Semibold',Roboto,'Helvetica Neue',Arial,Verdana,sans-serif;margin:0;text-align:center;}H3{margin:0 0 5px 0;text-align:center;}input{border:thin inset}em > small{display:inline}form{margin-block-end:0;}.tpm{background-color:#fff;cursor:pointer;border:1px solid black;border-radius:5px;padding:0 0 0 0px;min-width:18em;}.tpm2{position:absolute;top:-0.7em;z-index:130;left:0.7em;}.tpm3{width:4em;height:4em;}.tpmh1{font-variant-caps:all-small-caps;font-weight:normal;margin-left:2.2em;overflow:clip;}.tpmh3{background:#000;font-size:0.6em;color:#ffa;padding-left:7.2em;margin-left:0.5em;margin-right:0.5em;border-radius:5px;overflow:hidden;white-space:nowrap}.tpm0{position:relative;width:20em;padding:5px 0px 5px 0px;margin:0 auto 0 auto;}.cmp0{margin:0;padding:0;}.sel0{font-size:90%;width:auto;margin-left:10px;vertical-align:baseline;}.mt5{margin-top:5px!important}.mb10{margin-bottom:10px!important}.mb0{margin-bottom:0px!important}.mb15{margin-bottom:15px!important}.ml20{margin-left:20px}.ss>label span{font-size:80%}</style>";
static const char* myCustMenu = "<img style='display:block;margin:10px auto 5px auto;' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAQsAAAAsCAMAAABFVW1aAAAAQlBMVEUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACO4fbyAAAAFXRSTlMAgMBAd0Twu98QIDGuY1KekNBwiMxE8vI7AAAG9klEQVRo3uSY3Y7cIAyFA5EQkP9IvP+r1sZn7CFMdrLtZa1qMxBjH744QDq0lke2abjaNMLycGdJHAaz8VwnvZX0MtRLhm+2mOPLkrpmFsMNWG6UvLw7g1c2ZcjgToSFTRDqRFipFlztdUVsuyTwBea0y7zDJoHEPEiuobZqavoxiseCVh27cgyLWV42VtdT7npuWHpTogPi+iYmLtCLWUGZKSpbZl+IZW4Rui3RJgFhR3rKAt4WKEw18XsggXDyeHFMdez2I4vjIQvFBlve9W7GYtTwDYscNAg8EMNZ1scsIAaBAHuIBbZLgwZi+gtdpBF+ZFHyYxZwtYa3WMriKLChYbEVmNSF9+wXRVl0Dq2WRXRsth403l4yOuchZuHrtvOEEw9nJLM4OvwlW68sNseWZfonfDN1QcDYKOF4bg/qGt2Ocb7eidSYXyxyVUSBkNyx0fMP7OTmEuAoOifkFlapYSH9rcGbgUcNtMgUZ4FwSmuvjl4QU2MGi+3KAqiFxSEZNFWnRKojBQRExSOVa5XpErQuknyACa9hcjqFbM8BL/v4lAUiI1ASgSRpQ6a9ehzcRyY6wSLcs2DLj1igCx4zTR85WmWjhu9YnJaVuyEeAcdfsdiKFRgEJsmAgbiH+fkXdUq5/sji/C0LTPOWxfmZxdyw0IBWF9NjFpGiNXWxGMy5VsRETf7juZdvSakQ/nsWPpT5X1ns+pREQ1g/sWDfexYzx7iwCJ5subKIsnatGuhkjGBhWblJfY49bYc4SrywODjJVINFEpE6FqZEWWS8h/07kqJVJXY2n1+qOMguAyjv+JFFlPV3+7inuisLdCOQbkFXFpGaRIlxICFLd4St21PtWEb/ehamBPvIVt6X/YS1s94JHOVyvgh2dGBPPV/sysJ2OlhIv2ARJwTCXHoWnjnRL8o52ilqouY9i1TK/JWFnWHsMZ7qxalsiqeGNxZ2HD37ukCIPDxjEbwvoG/Hzp7FRkPnEqk+/KnvtadmvGfB1fudBX43j9G85qQsyKaj3r+wGPJcG7lnEXyUeE/Xzux1hfKcrGMhl9mTsy9HnTzG7tR9u4/wUWX7tnZGj927O4NHH+GqLFAaI1SZjXJe+7B2Tgj4iAVyTQgUpGBtH9GNiUTvPPmNs75lumeRiPHXfQQ7urKYR3g5ZlmysjAYZ8eiCnHqGHQxxib5OxajBFJlryl6dTm4x2FftUz3LCrI7yxW++D1ptcOOS0L7nM9Cxbi4YhaQMCdGumvWEAZcCK1rgUrO0UuIs30I4vlOws74vYshqNdO9nWri4MERzTYbs+wGCOHQvrhXfajIUq28RrBxrq5g68FDZ2+pFFesICpdizkBciKwtPQtLcrRc7DmVg4T2S9idJYxE82269to/YSVeVrdx5MIFgy3+S+ojNmbU/a/lJvxh7FqYELFCKn1hkcQBYWWj1P098NVbAuwWWPJi9xXhJWhYwf2EBm6/fqcPRbHiMCGUDyZqp31OtyM6ehSkBi+ZTCtZ/pzIy2P4ufMgWz1iclxVg+QWLYJWYcGadAgaYq0eg/ZLpnkV+wAJfDD2L5oOAqYsdqWGx2LEILOI8NikDXR+z8LueaKBMTzAB86wpPerDXTLdsxiOrywQe/nIIr8vie5gQVWrsaDRtYLnPPxn9ocdOxYAAABAANafv28EGWwYO43fBgAAWDNWszMpCATr0E0Dih6g3v9VV76eBXWdwx5MvjqMTVI/Y2WAZH49crVd0SECh0oGsugxHChntq62F1etx1PKwXOSytI91Neirh8jUFar+eY01f5I8mN+kQwOillVvIjIA9n/q4FDqIBSjqGjTvLGjgUaeEBAg7LDIIwHgeJriuvHiLWP4e401Z7U+JO/nSQjofryRZQN0oNTsIcuFFvgJBsXLNILlKxr9i7kI+oDJak2qua/erhVYdtyqXenqfak2hXGs2RwGBO2glfhYbvJYxewcxf+cgtXX1+6sJDQF664dbFywcBwmurRQQW4XiSzi4zXIdygzKcumln7vEuO4cxkrMm/3egimhk6l/XahRt5XaPRm5OrZ1Jo2ChnyeCsZBO8iyUYEAWnLn4gPsQNE6WR7dZFRxfpznzvouOhi+nkjJm0MlXms2RwkkSy4k0soSVIUN25/LNH9v2eno2qrA97RFNYv+4RYwIenFw9kwpLi1eJc3yMxHvwKiDssIfzIjIja/6QD2rtLx0WoNy7gPC5C68VSBscw2mqPQmZK+tFMhK6V3u1C2Mzq32S53uEu180HS3YGkKCMFhjmedFdVH82kUKbBYiHMNpqj2pW3C7Sj6cxLgaDS/Cxg/i6z2iowuJn/NDAmnLPC/MudvXLrAYGdxmOl3V1j8qeZN8OGn3zP/BH6Wx/qV3/+q3AAAAAElFTkSuQmCC'><div style='font-size:0.75em;line-height:1.2em;font-weight:bold;text-align:center;text-transform:uppercase'>" UNI_VERSION " (" UNI_VERSION_EXTRA ")<br>Powered by A10001986 <a href='https://" WEBHOME ".out-a-ti.me' target=_blank>[Home/Updates]</a></div>";

static char newversion[8];
static unsigned long lastUpdateCheck = 0;

#define WLA_IP      1
#define WLA_DEL_IP  2
#define WLA_WIFI    4
#define WLA_SET1    8
#define WLA_SET1_B      3
#define WLA_SET2    16
#define WLA_SET2_B      4
#define WLA_SET3    32
#define WLA_SET3_B      5
#define WLA_SET     (WLA_WIFI|WLA_SET1|WLA_SET2|WLA_SET3)
#define WLA_ANY     (WLA_IP|WLA_DEL_IP|WLA_SET)
static uint32_t     wifiLoopSaveAction = 0;

// Did user configure a WiFi network to connect to?
bool wifiHaveSTAConf = false;
static bool connectedToTCDAP = false;

//static unsigned long lastConnect = 0;
//static unsigned long consecutiveAPmodeFB = 0;

// WiFi power management in AP mode
bool          wifiInAPMode = false;
bool          wifiAPIsOff = false;
unsigned long wifiAPModeNow;
unsigned long wifiAPOffDelay = 0;     // default: never

// WiFi power management in STA mode
bool          wifiIsOff = false;
unsigned long wifiOnNow = 0;
unsigned long wifiOffDelay     = 0;   // default: never
unsigned long origWiFiOffDelay = 0;

#ifdef SID_HAVEMQTT
#define       MQTT_SHORT_INT  (30*1000)
#define       MQTT_LONG_INT   (5*60*1000)
static const char    emptyStr[1] = { 0 };
static bool          useMQTT = false;
static char          *mqttUser = (char *)emptyStr;
static char          *mqttPass = (char *)emptyStr;
static char          *mqttServer = (char *)emptyStr;
static unsigned long mqttReconnectNow = 0;
static unsigned long mqttReconnectInt = MQTT_SHORT_INT;
static uint16_t      mqttReconnFails = 0;
static bool          mqttSubAttempted = false;
static bool          mqttOldState = true;
static bool          mqttDoPing = true;
static bool          mqttRestartPing = false;
static bool          mqttPingDone = false;
static unsigned long mqttPingNow = 0;
static unsigned long mqttPingInt = MQTT_SHORT_INT;
static uint16_t      mqttPingsExpired = 0;
char                 mqttMsg[10] = { 0 };
#endif
uint32_t             mqttDisp = 0;

static unsigned int wmLenBuf = 0;

extern bool doPeaks;

static void wifiConnect(bool deferConfigPortal = false);
static void wifiOff(bool force);

static void checkForUpdate();

static void saveParamsCallback(int);
static void saveWiFiCallback(const char *ssid, const char *pass, const char *bssid);
static void preUpdateCallback();
static void postUpdateCallback(bool);
static bool preWiFiScanCallback();

static void updateConfigPortalValues();

static IPAddress stringToIp(char *str);

static void getParam(String name, char *destBuf, size_t length, int defaultVal);
static bool myisspace(char mychar);
static char* strcpytrim(char* destination, const char* source, bool doFilter = false);
static void mystrcpy(char *sv, WiFiManagerParameter *el);
static void mystrcpyWiFiDelay(char *sv, WiFiManagerParameter *el);
static void evalCB(char *sv, WiFiManagerParameter *el);
static void setCBVal(WiFiManagerParameter *el, char *sv);

#ifdef SID_HAVEMQTT
static void strcpyutf8(char *dst, const char *src, unsigned int len);
static void mqttPing();
static bool mqttReconnect(bool force = false);
static void mqttLooper();
static void mqttCallback(char *topic, byte *payload, unsigned int length);
static void mqttSubscribe();
#endif

/*
 * wifi_setup()
 *
 */
void wifi_setup()
{
    int temp;

    WiFiManagerParameter *wifiParmArray[] = {

      &custom_hostName,

      &custom_sectstart_wifi,
      &custom_wifiConRetries,

      &custom_sectstart_ap,
      &custom_sysID,
      &custom_appw,
      &custom_apch,
      &custom_bapch,
      &custom_wifiAPOffDelay,
      &custom_wifihint,

      NULL      
    };

    WiFiManagerParameter *parmArray[] = {

      &custom_sStrict,        // 5
      &custom_sTTANI,
      &custom_SApeaks,
      &custom_PIRFB,
      &custom_PIRCFB,
      &custom_ssDelay,
      
      &custom_sectstart_nw,   // 8
      &custom_tcdIP,
      &custom_uGPS,
      &custom_uNM,
      &custom_uFPO,
      &custom_bttfnTT,
      &custom_ssClock,
      &custom_ssClockO,
  
      &custom_TCDpresent,     // 2
      &custom_noETTOL,
      
      &custom_haveSD,         // 2(3)
      &custom_CfgOnSD,
      //&custom_sdFrq,

      &custom_disDIR,         // 1
  
      NULL
    };

    #ifdef SID_HAVEMQTT
    WiFiManagerParameter *parm2Array[] = {

      &custom_useMQTT,
      &custom_state,
      &custom_mqttServer,
      &custom_mqttVers,
      &custom_mqttUser,
      &custom_mqttTopic,

      NULL
    };
    #endif

    // Transition from NVS-saved data to own management:
    if(!settings.ssid[0] && settings.ssid[1] == 'X') {
        
        // Read NVS-stored WiFi data
        wm.getStoredCredentials(settings.ssid, sizeof(settings.ssid), settings.pass, sizeof(settings.pass));

        #ifdef SID_DBG
        Serial.printf("WiFi Transition: ssid '%s' pass '%s'\n", settings.ssid, settings.pass);
        #endif

        write_settings();
    }

    wm.setHostname(settings.hostName);

    wm.setSaveWiFiCallback(saveWiFiCallback);
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setPreOtaUpdateCallback(preUpdateCallback);
    wm.setPostOtaUpdateCallback(postUpdateCallback);
    wm.setPreWiFiScanCallback(preWiFiScanCallback);

    // Our style-overrides, the page title
    wm.setCustomHeadElement(myHead);
    wm.setTitle(myTitle);

    // Hack version number into WiFiManager main page
    wm.setCustomMenuHTML(myCustMenu);

    temp = atoi(settings.apChnl);
    if(temp < 0) temp = 0;
    else if(temp > 11) temp = 11;
    if(!temp) temp = random(1, 11);
    wm.setWiFiAPChannel(temp);

    temp = atoi(settings.wifiConRetries);
    if(temp < 1) temp = 1;
    else if(temp > 10) temp = 10;
    wm.setConnectRetries(temp);

    wm.setMenu(wifiMenu, TC_MENUSIZE, false);

    // WiFi Settings
    wm.allocParms(WM_PARM_WIFI, (sizeof(wifiParmArray) / sizeof(WiFiManagerParameter *)) - 1);
    temp = 0;
    while(wifiParmArray[temp]) {
        wm.addParameter(WM_PARM_WIFI, wifiParmArray[temp]);
        temp++;
    }

    // Settings
    wm.allocParms(WM_PARM_SETTINGS, (sizeof(parmArray) / sizeof(WiFiManagerParameter *)) - 1);
    temp = 0;
    while(parmArray[temp]) {
        wm.addParameter(WM_PARM_SETTINGS, parmArray[temp]);
        temp++;
    }

    // HA/MQTT
    #ifdef SID_HAVEMQTT
    wm.allocParms(WM_PARM_SETTINGS2, (sizeof(parm2Array) / sizeof(WiFiManagerParameter *)) - 1);
    temp = 0;
    while(parm2Array[temp]) {
        wm.addParameter(WM_PARM_SETTINGS2, parm2Array[temp]);
        temp++;
    }
    #endif

    updateConfigPortalValues();

    #ifdef SID_HAVEMQTT
    useMQTT = evalBool(settings.useMQTT);
    #endif
    
    wifiHaveSTAConf = (settings.ssid[0] != 0);

    // See if we have a configured WiFi network to connect to.
    // If we detect "TCD-AP" as the SSID, we make sure that we retry
    // at least 2 times so we have a chance to catch the TCD's AP if 
    // both are powered up at the same time.
    if(wifiHaveSTAConf) {
        if(!strncmp("TCD-AP", settings.ssid, 6)) {
            if(wm.getConnectRetries() < 2) {
                wm.setConnectRetries(2);
            }
            // Delay to give the TCD some time
            // (differs accross the props)
            delay(1100);
            #ifdef SID_HAVEMQTT
            useMQTT = false;
            #endif
            connectedToTCDAP = true;
        }      
    } else {
        // No point in retry when we have no WiFi config'd
        wm.setConnectRetries(1);
    }

    // No WiFi powersave features for STA mode here
    wifiOffDelay = origWiFiOffDelay = 0;

    // Eval AP-mode powersave delay
    wifiAPOffDelay = (unsigned long)atoi(settings.wifiAPOffDelay);
    if(wifiAPOffDelay > 0 && wifiAPOffDelay < 10) wifiAPOffDelay = 10;
    wifiAPOffDelay *= (60 * 1000);
    
    // Configure static IP
    if(loadIpSettings()) {
        if(checkIPConfig()) {
            IPAddress ip = stringToIp(ipsettings.ip);
            IPAddress gw = stringToIp(ipsettings.gateway);
            IPAddress sn = stringToIp(ipsettings.netmask);
            IPAddress dns = stringToIp(ipsettings.dns);
            wm.setSTAStaticIPConfig(ip, gw, sn, dns);
        }
    }

    wifi_setup2();
}

void wifi_setup2()
{
    // Connect, but defer starting the CP
    wifiConnect(true);

    #ifdef SID_MDNS
    if(MDNS.begin(settings.hostName)) {
        MDNS.addService("http", "tcp", 80);
    }
    #endif

    checkForUpdate();

#ifdef SID_HAVEMQTT
    if((!settings.mqttServer[0]) || // No server -> no MQTT
       (wifiInAPMode))              // WiFi in AP mode -> no MQTT
        useMQTT = false;  
    
    if(useMQTT) {

        uint16_t mqttPort = 1883;
        char *t;
        int tt;

        if((t = strchr(settings.mqttServer, ':'))) {
            size_t ts = (t - settings.mqttServer) + 1;
            mqttServer = (char *)malloc(ts);
            memset(mqttServer, 0, ts);
            strncpy(mqttServer, settings.mqttServer, t - settings.mqttServer);
            tt = atoi(t + 1);
            if(tt > 0 && tt <= 65535) {
                mqttPort = tt;
            }
        } else {
            mqttServer = settings.mqttServer;
        }

        if(isIp(mqttServer)) {
            mqttClient.setServer(stringToIp(mqttServer), mqttPort);
        } else {
            IPAddress remote_addr;
            if(WiFi.hostByName(mqttServer, remote_addr)) {
                mqttClient.setServer(remote_addr, mqttPort);
            } else {
                /*
                mqttClient.setServer(mqttServer, mqttPort);
                // Disable PING if we can't resolve domain
                mqttDoPing = false;
                */
                useMQTT = false;
                mqttReconnFails = 1; // Abuse for "resolv error"
                Serial.printf("MQTT: Failed to resolve '%s'\n", mqttServer);
            }
        }

        #ifdef SID_DBG
        Serial.printf("MQTT: server '%s' port %d\n", mqttServer, mqttPort);
        #endif
    }

    if(useMQTT) {

        char *t;

        // No WiFi power save if we're using MQTT
        origWiFiOffDelay = wifiOffDelay = 0;

        mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);
        mqttClient.setVersion(atoi(settings.mqttVers) > 0 ? 5 : 3);
        mqttClient.setClientID(settings.hostName);

        mqttClient.setCallback(mqttCallback);
        mqttClient.setLooper(mqttLooper);

        if(settings.mqttUser[0] != 0) {
            if((t = strchr(settings.mqttUser, ':'))) {
                size_t ts = strlen(settings.mqttUser) + 1;
                mqttUser = (char *)malloc(ts);
                strcpy(mqttUser, settings.mqttUser);
                mqttUser[t - settings.mqttUser] = 0;
                mqttPass = mqttUser + (t - settings.mqttUser + 1);
            } else {
                mqttUser = settings.mqttUser;
            }
        }

        #ifdef SID_DBG
        Serial.printf("MQTT: user '%s' pass '%s'\n", mqttUser, mqttPass);
        #endif
            
        mqttReconnect(true);
        // Rest done in loop
            
    } else {

        #ifdef SID_DBG
        Serial.println("MQTT: Disabled");
        #endif

    }
#endif

    // Start the Config Portal
    if(WiFi.status() == WL_CONNECTED) {
        wifiStartCP();
    }

    wifiSetupDone = true;
}

/*
 * wifi_loop()
 *
 */
void wifi_loop()
{
    char oldCfgOnSD = 0;

    if(!wifiSetupDone)
        return;

#ifdef SID_HAVEMQTT
    if(useMQTT) {
        if(mqttClient.state() != MQTT_CONNECTING) {
            if(!mqttClient.connected()) {
                if(mqttOldState || mqttRestartPing) {
                    // Disconnection first detected:
                    mqttPingDone = mqttDoPing ? false : true;
                    mqttPingNow = mqttRestartPing ? millisNonZero() : 0;
                    mqttOldState = false;
                    mqttRestartPing = false;
                    mqttSubAttempted = false;
                }
                if(mqttDoPing && !mqttPingDone) {
                    //audio_loop();
                    mqttPing();
                    //audio_loop();
                }
                if(mqttPingDone) {
                    //audio_loop();
                    mqttReconnect();
                    //audio_loop();
                }
            } else {
                // Only call Subscribe() if connected
                mqttSubscribe();
                mqttOldState = true;
            }
        }
        mqttClient.loop();
    }
#endif

    if(millis() - lastUpdateCheck > 24*60*60*1000) {
        if(!TTrunning && !IRLearning && !sidBusy) {
            checkForUpdate();
        }
    }

    if(wifiLoopSaveAction & WLA_SET) {

        int temp;

        // Save settings and restart esp32

        #ifdef SID_DBG
        Serial.println("Config Portal: Saving config");
        #endif

        if(wifiLoopSaveAction & WLA_WIFI) {

            // Parameters on WiFi Config page
            // Note: Parameters that need to be grabbed from the server directly
            // through getParam() must be handled in saveWiFiCallback().

            if(wifiLoopSaveAction & WLA_IP) {
                #ifdef SID_DBG
                Serial.println("WiFi: Saving IP config");
                #endif
                writeIpSettings();
            } else if(wifiLoopSaveAction & WLA_DEL_IP) {
                #ifdef SID_DBG
                Serial.println("WiFi: Deleting IP config");
                #endif
                deleteIpSettings();
            }

            // ssid, pass copied to settings in saveWiFiCallback()

            strcpytrim(settings.hostName, custom_hostName.getValue(), true);
            if(strlen(settings.hostName) == 0) {
                strcpy(settings.hostName, DEF_HOSTNAME);
            } else {
                char *s = settings.hostName;
                for ( ; *s; ++s) *s = tolower(*s);
            }
            mystrcpy(settings.wifiConRetries, &custom_wifiConRetries);
            
            strcpytrim(settings.systemID, custom_sysID.getValue(), true);
            strcpytrim(settings.appw, custom_appw.getValue(), true);
            if((temp = strlen(settings.appw)) > 0) {
                if(temp < 8) {
                    settings.appw[0] = 0;
                }
            }
            mystrcpyWiFiDelay(settings.wifiAPOffDelay, &custom_wifiAPOffDelay);

        } else if(wifiLoopSaveAction & WLA_SET1) {

            // Parameters on Settings page
            // Note: Parameters that need to be grabbed from the server directly
            // through getParam() must be handled in saveParamsCallback()

            // Extract settings saved as secSettings
            evalCB(settings.strictMode, &custom_sStrict);
            strictMode = evalBool(settings.strictMode);
            evalCB(settings.SApeaks, &custom_SApeaks);
            doPeaks = evalBool(settings.SApeaks);
            evalCB(settings.PIRFB, &custom_PIRFB);
            irShowPosFBDisplay = evalBool(settings.PIRFB);
            evalCB(settings.PIRCFB, &custom_PIRCFB);
            irShowCmdFBDisplay = evalBool(settings.PIRCFB);           
            saveAllSecCP();

            evalCB(settings.skipTTAnim, &custom_sTTANI);
            mystrcpy(settings.ssTimer, &custom_ssDelay);
            
            strcpytrim(settings.tcdIP, custom_tcdIP.getValue());
            if(strlen(settings.tcdIP) > 0) {
                char *s = settings.tcdIP;
                for( ; *s; ++s) *s = tolower(*s);
            }
            evalCB(settings.useGPSS, &custom_uGPS);
            evalCB(settings.useNM, &custom_uNM);
            evalCB(settings.useFPO, &custom_uFPO);
            evalCB(settings.bttfnTT, &custom_bttfnTT);
            evalCB(settings.ssClock, &custom_ssClock);
            evalCB(settings.ssClockOffNM, &custom_ssClockO);

            evalCB(settings.TCDpresent, &custom_TCDpresent);
            evalCB(settings.noETTOLead, &custom_noETTOL);

            oldCfgOnSD = settings.CfgOnSD[0];
            evalCB(settings.CfgOnSD, &custom_CfgOnSD);
            //evalCB(settings.sdFreq, &custom_sdFrq);

            evalCB(settings.disDIR, &custom_disDIR);

            // Copy volume/speed/IR settings to other medium if
            // user changed respective option
            if(oldCfgOnSD != settings.CfgOnSD[0]) {
                moveSettings();
            }

        } else if(wifiLoopSaveAction & WLA_SET2) {

            // Parameters on HA/MQTT Settings page
            // Note: Parameters that need to be grabbed from the server directly
            // through getParam() must be handled in saveParamsCallback()

            #ifdef SID_HAVEMQTT
            evalCB(settings.useMQTT, &custom_useMQTT);
            strcpytrim(settings.mqttServer, custom_mqttServer.getValue());
            strcpyutf8(settings.mqttUser, custom_mqttUser.getValue(), sizeof(settings.mqttUser));
            strcpyutf8(settings.mqttTopic, custom_mqttTopic.getValue(), sizeof(settings.mqttTopic));
            #endif

        }

        write_settings();

        // Reset esp32 to load new settings

        #ifdef SID_DBG
        Serial.println("Config Portal: Restarting ESP....");
        #endif
        Serial.flush();

        prepareReboot();
        delay(1000);
        esp_restart();
    }

    wm.process(true);

    // WiFi power management
    // If a delay > 0 is configured, WiFi is powered-down after timer has
    // run out. The timer starts when the device is powered-up/boots.
    // There are separate delays for AP mode and STA mode.
    // WiFi can be re-enabled for the configured time by *77OK
    
    if(wifiInAPMode) {
        // Disable WiFi in AP mode after a configurable delay (if > 0)
        if(wifiAPOffDelay > 0) {
            if(!wifiAPIsOff && (millis() - wifiAPModeNow >= wifiAPOffDelay)) {
                if(!WiFi.softAPgetStationNum()) {
                    wifiOff(false);
                    wifiAPIsOff = true;
                    wifiIsOff = false;
                    #ifdef SID_DBG
                    Serial.println("WiFi (AP-mode) switched off (power-save)");
                    #endif
                } else {
                    wifiAPModeNow += (wifiAPOffDelay / 10);   // Check again later
                }
            }
        }
    } else {
        // Disable WiFi in STA mode after a configurable delay (if > 0)
        if(origWiFiOffDelay > 0) {
            if(!wifiIsOff && (millis() - wifiOnNow >= wifiOffDelay)) {
                wifiOff(false);
                wifiIsOff = true;
                wifiAPIsOff = false;
                #ifdef SID_DBG
                Serial.println("WiFi (STA-mode) switched off (power-save)");
                #endif
            }
        }
    }

}

static void wifiConnect(bool deferConfigPortal)
{
    char realAPName[16];

    strcpy(realAPName, apName);
    if(settings.systemID[0]) {
        strcat(realAPName, settings.systemID);
    }
    
    // Connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    if(wm.wifiConnect(settings.ssid, settings.pass, settings.bssid, realAPName, settings.appw)) {
        #ifdef SID_DBG
        Serial.println("WiFi connected");
        #endif

        // During boot, we start the CP later
        if(!deferConfigPortal) {
            wm.startWebPortal();
        }

        // Allow modem sleep:
        // WIFI_PS_MIN_MODEM is the default, and activated when calling this
        // with "true". When this is enabled, received WiFi data can be
        // delayed for as long as the DTIM period.
        // Disable modem sleep, don't want delays accessing the CP or
        // with BTTFN/MQTT.
        WiFi.setSleep(false);

        // Set transmit power to max; we might be connecting as STA after
        // a previous period in AP mode.
        #ifdef SID_DBG
        {
            wifi_power_t power = WiFi.getTxPower();
            Serial.printf("WiFi: Max TX power in STA mode %d\n", power);
        }
        #endif
        WiFi.setTxPower(WIFI_POWER_19_5dBm);

        wifiInAPMode = false;
        wifiIsOff = false;
        wifiOnNow = millis();
        wifiAPIsOff = false;  // Sic! Allows checks like if(wifiAPIsOff || wifiIsOff)

        //consecutiveAPmodeFB = 0;  // Reset counter of consecutive AP-mode fall-backs

    } else {

        #ifdef SID_DBG
        Serial.println("Config portal running in AP-mode");
        #endif

        {
            #ifdef SID_DBG
            int8_t power;
            esp_wifi_get_max_tx_power(&power);
            Serial.printf("WiFi: Max TX power %d\n", power);
            #endif

            // Try to avoid "burning" the ESP when the WiFi mode
            // is "AP" by reducing the max. transmit power.
            // The choices are:
            // WIFI_POWER_19_5dBm    = 19.5dBm
            // WIFI_POWER_19dBm      = 19dBm
            // WIFI_POWER_18_5dBm    = 18.5dBm
            // WIFI_POWER_17dBm      = 17dBm
            // WIFI_POWER_15dBm      = 15dBm
            // WIFI_POWER_13dBm      = 13dBm
            // WIFI_POWER_11dBm      = 11dBm
            // WIFI_POWER_8_5dBm     = 8.5dBm
            // WIFI_POWER_7dBm       = 7dBm     <-- proven to avoid any issues
            // WIFI_POWER_5dBm       = 5dBm
            // WIFI_POWER_2dBm       = 2dBm
            // WIFI_POWER_MINUS_1dBm = -1dBm
            WiFi.setTxPower(WIFI_POWER_7dBm);

            #ifdef SID_DBG
            esp_wifi_get_max_tx_power(&power);
            Serial.printf("WiFi: Max TX power set to %d\n", power);
            #endif
        }

        wifiInAPMode = true;
        wifiAPIsOff = false;
        wifiAPModeNow = millis();
        wifiIsOff = false;    // Sic!

        /*
        if(wifiHaveSTAConf) {    
            consecutiveAPmodeFB++;  // increase counter of consecutive AP-mode fall-backs
        }
        */
    }

    //lastConnect = millis();
}

static void wifiOff(bool force)
{
    if(!force) {
        if( (!wifiInAPMode && wifiIsOff) ||
            (wifiInAPMode && wifiAPIsOff) ) {
            return;
        }
    }

    // Parm for disableWiFi() is "waitForOFF"
    // which should be true if we stop in AP
    // mode and immediately re-connect, without
    // process()ing for a while after this call.
    // "force" is true if we want to try to
    // reconnect after disableWiFi(), false if 
    // we disconnect upon timer expiration, 
    // so it matches the purpose.
    // "false" also does not cause any delays,
    // while "true" may take up to 2 seconds.
    wm.disableWiFi(force);
}

void wifiOn(unsigned long newDelay, bool alsoInAPMode, bool deferCP)
{
    unsigned long desiredDelay;
    unsigned long Now = millis();
    
    // wifiON() is called when the user entered *77OK (with alsoInAPMode
    // TRUE) [and - UNUSED - (with alsoInAPMode FALSE)].
    //
    // *77OK serves two purposes: To re-enable WiFi if in power save mode, and 
    // to re-connect to a configured WiFi network if we failed to connect to 
    // that network at the last connection attempt. In both cases, the Config
    // Portal is started.
    //
    // [Unused:
    // The call with alsoInAPMode=FALSE should only re-connect if we are in 
    // power-save  mode after being connected to a user-configured network, 
    // or if we are in AP mode but the user had config'd a network. Should  
    // only be called when a short freeze is feasible.]
    //    
    // "wifiInAPMode" only tells us our latest mode; if the configured WiFi
    // network was - for whatever reason - was not available when we
    // tried to (re)connect, "wifiInAPMode" is true.

    // At this point, wifiInAPMode reflects the state after
    // the last connection attempt.

    if(alsoInAPMode) {    // User entered *77OK
        
        if(wifiInAPMode) {  // We are in AP mode

            if(!wifiAPIsOff) {

                // If ON but no user-config'd WiFi network -> bail
                if(!wifiHaveSTAConf) {
                    // Best we can do is to restart the timer
                    wifiAPModeNow = Now;
                    return;
                }

                // If ON and User has config's a NW, disable WiFi at this point
                // (in hope of successful connection below)
                wifiOff(true);

            }

        } else {            // We are in STA mode

            // If WiFi is not off, check if caller wanted
            // to start the CP, and do so, if not running
            if(!wifiIsOff && (WiFi.status() == WL_CONNECTED)) {
                if(!deferCP) {
                    if(!wm.getWebPortalActive()) {
                        wm.startWebPortal();
                    }
                }
                // Restart timer
                wifiOnNow = Now;
                return;
            }

        }

    } else 
        return;

    // (Re)connect
    wifiConnect(deferCP);

    // Restart timers
    // Note that wifiInAPMode now reflects the
    // result of our above wifiConnect() call

    if(wifiInAPMode) {

        #ifdef SID_DBG
        Serial.println("wifiOn: in AP mode after connect");
        #endif
      
        wifiAPModeNow = Now;
        
        #ifdef SID_DBG
        if(wifiAPOffDelay > 0) {
            Serial.printf("Restarting WiFi-off timer (AP mode); delay %d\n", wifiAPOffDelay);
        }
        #endif
        
    } else {

        #ifdef SID_DBG
        Serial.println("wifiOn: in STA mode after connect");
        #endif

        if(origWiFiOffDelay) {
            desiredDelay = (newDelay > 0) ? newDelay : origWiFiOffDelay;
            if((Now - wifiOnNow >= wifiOffDelay) ||                    // If delay has run out, or
               (wifiOffDelay - (Now - wifiOnNow))  < desiredDelay) {   // new delay exceeds remaining delay:
                wifiOffDelay = desiredDelay;                           // Set new timer delay, and
                wifiOnNow = Now;                                       // restart timer
                #ifdef SID_DBG
                Serial.printf("Restarting WiFi-off timer; delay %d\n", wifiOffDelay);
                #endif
            }
        }

    }
}

// Check if a longer interruption due to a re-connect is to
// be expected when calling wifiOn(true, xxx).
bool wifiOnWillBlock()
{
    if(wifiInAPMode) {  // We are in AP mode
        if(!wifiAPIsOff) {
            if(!wifiHaveSTAConf) {
                return false;
            }
        }
    } else {            // We are in STA mode
        if(!wifiIsOff) return false;
    }

    return true;
}

void wifiStartCP()
{
    if(wifiInAPMode || wifiIsOff)
        return;

    wm.startWebPortal();
}

static void checkForUpdate()
{
    int cver = 0, crev = 0, uver = 0, urev = 0;
    bool haveCVer = false;

    *newversion = 0;

    lastUpdateCheck = millis();

    if(sscanf(CURRVERSION, "V%d.%d", &cver, &crev) != 2)
        return;
    
    if(WiFi.status() == WL_CONNECTED) {
        IPAddress remote_addr;
        if(WiFi.hostByName(WEBHOME "v.out-a-ti.me", remote_addr)) {
            uver = remote_addr[0]; urev = remote_addr[1];
            if(uver) saveUpdVers(uver, urev);
        }
    } else {
        loadUpdVers(uver, urev);
    }

    if(uver) {
        haveCVer = true;
        if(((uver << 8) | urev) > ((cver << 8) | crev)) {
            snprintf(newversion, sizeof(newversion), "%d.%d", uver, urev);
        }
    }

    wm.setDownloadLink(r_link, haveCVer, (*newversion) ? newversion : NULL);
}

bool updateAvailable()
{
    return (*newversion != 0);
}

bool checkIPConfig()
{
    return (*ipsettings.ip            &&
            isIp(ipsettings.ip)       &&
            isIp(ipsettings.gateway)  &&
            isIp(ipsettings.netmask)  &&
            isIp(ipsettings.dns));
}

// This is called when the WiFi config is to be saved. 
// SSID, password and BSSID are copied to settings here.
// Static IP and other parameters are taken from WiFiManager's server.
static void saveWiFiCallback(const char *ssid, const char *pass, const char *bssid)
{
    char ipBuf[20] = "";
    char gwBuf[20] = "";
    char snBuf[20] = "";
    char dnsBuf[20] = "";
    bool invalConf = false;

    #ifdef SID_DBG
    Serial.println("saveConfigCallback");
    #endif

    // clear as strncpy might leave us unterminated
    memset(ipBuf, 0, 20);
    memset(gwBuf, 0, 20);
    memset(snBuf, 0, 20);
    memset(dnsBuf, 0, 20);

    String temp;
    temp.reserve(16);
    if((temp = wm.server->arg(FPSTR(WMS_ip))) != "") {
        strncpy(ipBuf, temp.c_str(), 19);
    } else invalConf |= true;
    if((temp = wm.server->arg(FPSTR(WMS_sn))) != "") {
        strncpy(snBuf, temp.c_str(), 19);
    } else invalConf |= true;
    if((temp = wm.server->arg(FPSTR(WMS_gw))) != "") {
        strncpy(gwBuf, temp.c_str(), 19);
    } else invalConf |= true;
    if((temp = wm.server->arg(FPSTR(WMS_dns))) != "") {
        strncpy(dnsBuf, temp.c_str(), 19);
    } else invalConf |= true;

    #ifdef SID_DBG
    if(strlen(ipBuf) > 0) {
        Serial.printf("IP:%s / SN:%s / GW:%s / DNS:%s\n", ipBuf, snBuf, gwBuf, dnsBuf);
    } else {
        Serial.println("Static IP unset, using DHCP");
    }
    #endif

    if(!invalConf && isIp(ipBuf) && isIp(gwBuf) && isIp(snBuf) && isIp(dnsBuf)) {

        #ifdef SID_DBG
        Serial.println("All IPs valid");
        #endif

        wifiLoopSaveAction |= WLA_IP;
          
        memset((void *)&ipsettings, 0, sizeof(ipsettings));
        strcpy(ipsettings.ip, ipBuf);
        strcpy(ipsettings.gateway, gwBuf);
        strcpy(ipsettings.netmask, snBuf);
        strcpy(ipsettings.dns, dnsBuf);

    } else {

        #ifdef SID_DBG
        if(*ipBuf) {
            Serial.println("Invalid IP");
        }
        #endif

        wifiLoopSaveAction |= WLA_DEL_IP;

    }
    
    // ssid is the (new?) ssid to connect to, pass the password.
    // (We don't need to compare to the old ones, the settings
    // hash will decide on save/not save.)
    // This is also used to "forget" a saved WiFi network, in
    // which case ssid and pass are empty strings.
    memset(settings.ssid, 0, sizeof(settings.ssid));
    memset(settings.pass, 0, sizeof(settings.pass));
    memset(settings.bssid, 0, sizeof(settings.bssid));
    if(*ssid) {
        strncpy(settings.ssid, ssid, sizeof(settings.ssid) - 1);
        strncpy(settings.pass, pass, sizeof(settings.pass) - 1);
        strncpy(settings.bssid, bssid, sizeof(settings.bssid) - 1);
    }

    // Other parameters on WiFi Config page that
    // need grabbing directly from the server

    getParam("apchnl", settings.apChnl, 2, DEF_AP_CHANNEL);
    
    wifiLoopSaveAction |= WLA_WIFI;
}


// This is the callback from the actual Params page. We read out
// the WM "Settings" parameters and save them.
// paramspage is 1 or 2
static void saveParamsCallback(int paramspage)
{
    wifiLoopSaveAction |= (1 << (paramspage - 1 + WLA_SET1_B));

    switch(paramspage) {
    case 1:
        break;
    case 2:
        #ifdef SID_HAVEMQTT
        getParam("mprot", settings.mqttVers, 1, 0);
        #endif
        break;
    }
}

// This is called before a firmware updated is initiated.
static void preUpdateCallback()
{
    wifiAPOffDelay = 0;
    origWiFiOffDelay = 0;

    flushDelayedSave();

    showWaitSequence(true);
}

// This is called after a firmware updated has finished.
// parm = true of ok, false if error. WM reboots only 
// if the update worked, ie when res is true.
static void postUpdateCallback(bool res)
{
    Serial.flush();
    prepareReboot();

    // WM does not reboot on OTA update errors.
    // However, don't bother for that really
    // rare case to put code here to restore
    // under all possible circumstances, like
    // fake-off, time-travel going on, ss, ....
    if(!res) {
        delay(1000);
        esp_restart();
    }
}

static bool preWiFiScanCallback()
{
    // Do not allow a WiFi scan under some circumstances (as
    // it may disrupt sequences)
    
    if(blockScan || TTrunning || IRLearning || networkAlarm)
        return false;

    return true;
}


static void setBoolAndUpdCB(bool myBool, char *sett, WiFiManagerParameter *wmParm)
{
    sett[0] = myBool ? '1' : '0';
    sett[1] = 0;
    setCBVal(wmParm, sett);
}

static void updateConfigPortalValues()
{
    // Make sure the settings form has the correct values

    custom_hostName.setValue(settings.hostName);
    custom_wifiConRetries.setValue(settings.wifiConRetries);
    
    custom_sysID.setValue(settings.systemID);
    custom_appw.setValue(settings.appw);
    // ap channel done on-the-fly
    custom_wifiAPOffDelay.setValue(settings.wifiAPOffDelay);

    setCBVal(&custom_sTTANI, settings.skipTTAnim);
    setCBVal(&custom_SApeaks, settings.SApeaks);
    custom_ssDelay.setValue(settings.ssTimer);
    
    custom_tcdIP.setValue(settings.tcdIP);
    setCBVal(&custom_uGPS, settings.useGPSS);
    setCBVal(&custom_uNM, settings.useNM);
    setCBVal(&custom_uFPO, settings.useFPO);
    setCBVal(&custom_bttfnTT, settings.bttfnTT);
    setCBVal(&custom_ssClock, settings.ssClock);
    setCBVal(&custom_ssClockO, settings.ssClockOffNM);

    setCBVal(&custom_TCDpresent, settings.TCDpresent);
    setCBVal(&custom_noETTOL, settings.noETTOLead);

    setCBVal(&custom_CfgOnSD, settings.CfgOnSD);
    //setCBVal(&custom_sdFrq, settings.sdFreq);

    setCBVal(&custom_disDIR, settings.disDIR);

    #ifdef SID_HAVEMQTT
    setCBVal(&custom_useMQTT, settings.useMQTT);
    custom_mqttServer.setValue(settings.mqttServer);
    custom_mqttUser.setValue(settings.mqttUser);
    custom_mqttTopic.setValue(settings.mqttTopic);
    #endif
}

void updateConfigPortalStrictValue()
{
    setBoolAndUpdCB(strictMode, settings.strictMode, &custom_sStrict);
}

void updateConfigPortalPeaksValue()
{
    setBoolAndUpdCB(doPeaks, settings.SApeaks, &custom_SApeaks);
}

void updateConfigPortalIRFBValues()
{
    setBoolAndUpdCB(irShowPosFBDisplay, settings.PIRFB, &custom_PIRFB);
    setBoolAndUpdCB(irShowCmdFBDisplay, settings.PIRCFB, &custom_PIRCFB);
}

static const char *buildBanner(const char *msg, const char *col, int op) 
{   // "%s%s%s%s</div>";
    unsigned int l = STRLEN(bannerStart) + 7 + STRLEN(bannerMid) + strlen(msg) + 6 + 4;

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);
    sprintf(str, bannerGen, bannerStart, col, bannerMid, msg);        

    return str;
}

static unsigned int calcSelectMenu(const char **theHTML, int cnt, char *setting, bool indent = false)
{
    int sr = atoi(setting);

    unsigned int l = 0;

    l += STRLEN(custHTMLHdr1);
    if(indent) l += STRLEN(custHTMLHdrI);
    l += STRLEN(custHTMLHdr2);
    l += strlen(theHTML[0]);
    l += STRLEN(custHTMLSHdr);
    l += strlen(setting);
    l += (STRLEN(custHTMLSelFmt) - (2*2));
    l += (3*strlen(theHTML[1]));
    for(int i = 0; i < cnt - 2; i++) {
        if(sr == i) l += STRLEN(custHTMLSel);
        l += (strlen(theHTML[i+2]) - 2);
        l += strlen((i == cnt - 3) ? osde : ooe);
    }

    return l + 8;
}

static void buildSelectMenu(char *target, const char **theHTML, int cnt, char *setting, bool indent = false)
{
    int sr = atoi(setting);

    strcpy(target, custHTMLHdr1);
    if(indent) strcat(target, custHTMLHdrI);
    strcat(target, custHTMLHdr2);
    strcat(target, theHTML[1]);
    strcat(target, theHTML[0]);
    strcat(target, custHTMLSHdr);
    strcat(target, setting);
    sprintf(target + strlen(target), custHTMLSelFmt, theHTML[1], theHTML[1]);
    for(int i = 0; i < cnt - 2; i++) {
        if(sr == i) strcat(target, custHTMLSel);
        sprintf(target + strlen(target), 
            theHTML[i+2], (i == cnt - 3) ? osde : ooe);
    }
}

static const char *wmBuildSelect(const char *dest, int op, const char **src, int count, char *setting, bool indent = false)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = calcSelectMenu(src, count, setting, indent);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    buildSelectMenu(str, src, count, setting, indent);
    
    return str;
}

static const char *wmBuildApChnl(const char *dest, int op)
{
    return wmBuildSelect(dest, op, apChannelCustHTMLSrc, 14, settings.apChnl, false);

    /*
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = calcSelectMenu(apChannelCustHTMLSrc, 14, settings.apChnl);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    buildSelectMenu(str, apChannelCustHTMLSrc, 14, settings.apChnl);
    
    return str;
    */
}

static const char *wmBuildBestApChnl(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    int32_t mychan = 0;
    int qual = 0;

    if(wm.getBestAPChannel(mychan, qual)) {
        unsigned int l = STRLEN(bestAP) - (5*2) + STRLEN(bannerStart) + 6 + STRLEN(bannerMid) + 4 + STRLEN(badWiFi) + 1 + 8;
        if(op == WM_CP_LEN) {
            wmLenBuf = l;
            return (const char *)&wmLenBuf;
        }
        char *str = (char *)malloc(l);
        sprintf(str, bestAP, bannerStart, qual < 0 ? col_r : (qual > 0 ? col_g : col_gr), bannerMid, mychan, qual < 0 ? badWiFi : "");
        return str;
    }

    return NULL;
}

static const char *wmBuildHaveSD(const char *dest, int op)
{
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }
    
    if(haveSD)
        return NULL;

    return buildBanner(haveNoSD, col_r, op);
}

#ifdef SID_HAVEMQTT
static const char *wmBuildMQTTprot(const char *dest, int op)
{
    return wmBuildSelect(dest, op, mqttpCustHTMLSrc, 4, settings.mqttVers, false);

    /*
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    unsigned int l = calcSelectMenu(mqttpCustHTMLSrc, 4, settings.mqttVers);

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }
    
    char *str = (char *)malloc(l);

    buildSelectMenu(str, mqttpCustHTMLSrc, 4, settings.mqttVers);
    
    return str;
    */
}

static const char *wmBuildMQTTstate(const char *dest, int op)
{
    // Check original setting, not "useMQTT" which
    // might be overruled.
    if(!evalBool(settings.useMQTT)) {
        return NULL;
    }
    
    if(op == WM_CP_DESTROY) {
        if(dest) free((void *)dest);
        return NULL;
    }

    int s = 0;
    const char *msg = NULL;
    const char *cls = col_r;

    if(!useMQTT) {
        if(mqttReconnFails) {
            msg = mqttMsgResolvErr;
        } else {
            msg = mqttMsgDisabled;
            cls = col_gr;
        }
    } else {
        s = mqttClient.state();
        switch(s) {
        case MQTT_CONNECTED:
            msg = mqttMsgConnected;
            cls = col_g;
            break;
        case MQTT_CONNECTING:
            msg = mqttMsgConnecting;
            cls = col_gr;
            break;
        case MQTT_CONNECTION_TIMEOUT:
            msg = mqttMsgTimeout;
            break;
        case MQTT_CONNECTION_LOST:
        case MQTT_CONNECT_FAILED:
            msg = mqttMsgFailed;
            break;
        case MQTT_DISCONNECTED:
            msg = mqttMsgDisconnected;
            break;
        case MQTT_CONNECT_BAD_PROTOCOL:
        case MQTT_CONNECT_BAD_CLIENT_ID:
        case 129:
        case 130:
        case 132:
        case 133:
            msg = mqttMsgBadProtocol;
            break;
        case MQTT_CONNECT_UNAVAILABLE:
        case 136:
        case 137:
            msg = mqttMsgUnavailable;
            break;
        case MQTT_CONNECT_BAD_CREDENTIALS:
        case MQTT_CONNECT_UNAUTHORIZED:
        case 134:
        case 135:
        case 138:
        case 140:
        case 156:
        case 157:
            msg = mqttMsgBadCred;
            break;
        default:
            msg = mqttMsgGenError;
            break;
        }
    }

    // "%s%s%s%s%s (%d)</div>"
    unsigned int l = STRLEN(mqttStatus) - (6*2) + STRLEN(bannerStart) + strlen(cls) + 20 + STRLEN(bannerMid) + strlen(msg) + 6;

    if(op == WM_CP_LEN) {
        wmLenBuf = l;
        return (const char *)&wmLenBuf;
    }

    char *str = (char *)malloc(l);

    sprintf(str, mqttStatus, bannerStart, cls, ";margin-bottom:10px", bannerMid, msg, s);

    return str;
}
#endif


bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d)
{
    IPAddress myip;

    switch(WiFi.getMode()) {
      case WIFI_MODE_STA:
          myip = WiFi.localIP();
          break;
      case WIFI_MODE_AP:
      case WIFI_MODE_APSTA:
          myip = WiFi.softAPIP();
          break;
      default:
          a = b = c = d = 0;
          return true;
    }

    a = myip[0];
    b = myip[1];
    c = myip[2];
    d = myip[3];

    return true;
}

// Check if String is a valid IP address
bool isIp(char *str)
{
    int segs = 0;
    int digcnt = 0;
    int num = 0;

    while(*str) {

        if(*str == '.') {

            if(!digcnt || (++segs == 4))
                return false;

            num = digcnt = 0;
            str++;
            continue;

        } else if((*str < '0') || (*str > '9')) {

            return false;

        }

        if((num = (num * 10) + (*str - '0')) > 255)
            return false;

        digcnt++;
        str++;
    }

    if(segs == 3) 
        return true;

    return false;
}

// String to IPAddress
static IPAddress stringToIp(char *str)
{
    int ip1, ip2, ip3, ip4;

    sscanf(str, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

    return IPAddress(ip1, ip2, ip3, ip4);
}

/*
 * Read parameter from server, for customhmtl input
 */
static void getParam(String name, char *destBuf, size_t length, int defaultVal)
{
    memset(destBuf, 0, length+1);
    if(wm.server->hasArg(name)) {
        strncpy(destBuf, wm.server->arg(name).c_str(), length);
    }
    if(!*destBuf) {
        sprintf(destBuf, "%d", defaultVal);
    }
}

static bool myisspace(char mychar)
{
    return (mychar == ' ' || mychar == '\n' || mychar == '\t' || mychar == '\v' || mychar == '\f' || mychar == '\r');
}

static bool myisgoodchar(char mychar)
{
    return ((mychar >= '0' && mychar <= '9') || (mychar >= 'a' && mychar <= 'z') || (mychar >= 'A' && mychar <= 'Z') || mychar == '-');
}

static char* strcpytrim(char* destination, const char* source, bool doFilter)
{
    char *ret = destination;
    
    while(*source) {
        if(!myisspace(*source) && (!doFilter || myisgoodchar(*source))) *destination++ = *source;
        source++;
    }
    
    *destination = 0;
    
    return ret;
}

static void mystrcpy(char *sv, WiFiManagerParameter *el)
{
    strcpy(sv, el->getValue());
}

static void mystrcpyWiFiDelay(char *sv, WiFiManagerParameter *el)
{
    int a = atoi(el->getValue());
    if(a > 0 && a < 10) a = 10;
    else if(a > 99)     a = 99;
    else if(a < 0)      a = 0;
    sprintf(sv, "%d", a);
}

static void evalCB(char *sv, WiFiManagerParameter *el)
{
    *sv++ = (*(el->getValue()) == '0') ? '0' : '1';
    *sv = 0;
}

static void setCBVal(WiFiManagerParameter *el, char *sv)
{
    el->setValue((*sv == '0') ? "0" : "1");
}

#ifdef SID_HAVEMQTT
// Filter out UTF8 and non-displayable characters
static int16_t filterOutNonDisp(char *src, char *dst, int srcLen = 0, int maxChars = 99999)
{
    int i, j, slen = srcLen ? srcLen : strlen(src);
    unsigned char c, e;

    for(i = 0, j = 0; i < slen && j < maxChars; i++) {
        c = (unsigned char)src[i];
        if(c >= 32 && c <= 126) {     // skip 127 = DEL
            if(c >= 'a' && c <= 'z') c &= ~0x20;
            if( (c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'Z') ) {
                dst[j++] = c; 
            }
        } else {
            e = 0;
            if     (c >= 192 && c < 224)  e = 1;
            else if(c >= 224 && c < 240)  e = 2;
            else if(c >= 240 && c < 245)  e = 3;    // yes, 245 (otherwise bad UTF8)
            if(e) {
                if((i + e) < slen) {
                    /*
                    for(k = i + 1, d = 0; k <= i + 1 + e; k++) {
                        d |= (unsigned char)src[k];
                    }
                    if(d > 127 && d < 192) i += e;
                    */
                    i += e;   // Why check? Just skip.
                } else {
                    break;
                }
            }
        }
    }
    dst[j] = 0;

    return j;
}

static void truncateUTF8(char *src)
{
    int i, slen = strlen(src);
    unsigned char c, e;

    for(i = 0; i < slen; i++) {
        c = (unsigned char)src[i];
        e = 0;
        if     (c >= 192 && c < 224)  e = 1;
        else if(c >= 224 && c < 240)  e = 2;
        else if(c >= 240 && c < 248)  e = 3;  // Invalid UTF8 >= 245, but consider 4-byte char anyway
        if(e) {
            if((i + e) < slen) {
                i += e;
            } else {
                src[i] = 0;
                return;
            }
        }
    }
}

static void strcpyutf8(char *dst, const char *src, unsigned int len)
{
    char *dest = dst;
    len--;  // leave room for 0
    while(*src && len--) {
        if(*src != '\'') *dst++ = *src;
        src++;
    }
    *dst = 0;
    truncateUTF8(dest);
}

static void mqttLooper()
{
}

static uint16_t a2i(char *p)
{
    unsigned int t = 0;
    t += (*p++ - '0') * 1000;
    t += (*p++ - '0') * 100;
    t += (*p++ - '0') * 10;
    t += (*p - '0');

    return (uint16_t)t;
}

static void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    int i = 0, j, ml = (length <= 255) ? length : 255;
    char tempBuf[256];
    static const char *cmdList[] = {
      "TIMETRAVEL",       // 0
      "IDLE_",            // 1
      "IDLE",             // 2
      "SA",               // 3
      "INJECT_",          // 4
      NULL
    };
    static const char *cmdList2[] = {
      "PREPARE",          // 0
      "TIMETRAVEL",       // 1
      "REENTRY",          // 2
      "ABORT_TT",         // 3
      "ALARM",            // 4
      "WAKEUP",           // 5
      NULL
    };

    // Note: This might be called while we are in a
    // wait-delay-loop. Best to just set flags here
    // that are evaluated synchronously (=later).
    // Do not stuff that messes with display, input,
    // etc.

    if(!length) return;

    memcpy(tempBuf, (const char *)payload, ml);
    tempBuf[ml] = 0;

    if(!strcmp(topic, "bttf/tcd/pub")) {

        for(j = 0; j < ml; j++) {
            if(tempBuf[j] >= 'a' && tempBuf[j] <= 'z') tempBuf[j] &= ~0x20;
        }

        // Commands from TCD or other props

        while(cmdList2[i]) {
            j = strlen(cmdList2[i]);
            if((length >= j) && !strncmp((const char *)tempBuf, cmdList2[i], j)) {
                break;
            }
            i++;          
        }

        if(!cmdList2[i]) return;

        switch(i) {
        case 0:
            // Prepare for TT. Comes at some undefined point,
            // an undefined time before the actual tt, and may
            // now come at all.
            // We disable our Screen Saver.
            // We don't ignore this if TCD is connected by wire,
            // because this signal does not come via wire.
            doPrepareTT = true;
            break;
        case 1:
            // Trigger Time Travel (if not running already)
            // Ignore command if TCD is connected by wire
            if(!TCDconnected && !TTrunning && !IRLearning && !sidBusy) {
                networkTimeTravel = true;
                networkTCDTT = true;
                networkReentry = false;
                networkAbort = false;
                if(strlen(tempBuf) == 20) {
                    networkLead = a2i(&tempBuf[11]);
                    networkP1 = a2i(&tempBuf[16]);
                } else {
                    networkLead = ETTO_LEAD;
                    networkP1 = 6600;
                }
            }
            break;
        case 2:   // Re-entry
            // Start re-entry (if TT currently running)
            // Ignore command if TCD is connected by wire
            if(!TCDconnected && TTrunning && networkTCDTT) {
                networkReentry = true;
            }
            break;
        case 3:   // Abort TT (TCD fake-powered down during TT)
            // Ignore command if TCD is connected by wire
            // (mainly because this is no network-triggered TT)
            if(!TCDconnected && TTrunning && networkTCDTT) {
                networkAbort = true;
            }
            break;
        case 4:
            networkAlarm = true;
            // Eval this at our convenience
            break;
        case 5:
            doWakeup = true;
            break;
        }
       
    } else if(!sidBusy && !strcmp(topic, "bttf/sid/cmd")) {

        for(j = 0; j < ml; j++) {
            if(tempBuf[j] >= 'a' && tempBuf[j] <= 'z') tempBuf[j] &= ~0x20;
        }
        
        // User commands

        while(cmdList[i]) {
            j = strlen(cmdList[i]);
            if((length >= j) && !strncmp((const char *)tempBuf, cmdList[i], j)) {
                break;
            }
            i++;          
        }

        if(!cmdList[i]) return;

        // What needs to be handled here:
        // - complete command parsing
        // - stuff to execute when fake power is off
        // All other stuff translated into command and queued

        switch(i) {
        case 1:
            if(strlen(tempBuf) > j && tempBuf[j] >= '0' && tempBuf[j] <= '5') {
                addCmdQueue(10 + (uint32_t)(tempBuf[j] - '0'));
            }
            break;
        case 2:
            addCmdQueue(20);
            break;
        case 3:
            addCmdQueue(21);
            break;
        case 4:
            if(strlen(tempBuf) > j) {
                addCmdQueue(atoi(tempBuf+j) | 0x80000000);
            }
            break;
        default:
            addCmdQueue(1000 + i);
        }
            
    } else if(*settings.mqttTopic && (!strcmp(topic, settings.mqttTopic))) {

        if(filterOutNonDisp(tempBuf, mqttMsg, 0, 8)) {
            mqttDisp = 1;
            #ifdef SID_DBG
            Serial.printf("MQTT: Message about [%s]: %s\n", topic, mqttMsg);
            #endif
        } else {
            #ifdef SID_DBG
            Serial.printf("MQTT: Message about [%s] contained no displayable characters\n", topic);
            #endif
        }
    }
}

#ifdef SID_DBG
#define MQTT_FAILCOUNT 6
#else
#define MQTT_FAILCOUNT 120
#endif

static void mqttPing()
{
    switch(mqttClient.pstate()) {
    case PING_IDLE:
        if(WiFi.status() == WL_CONNECTED) {
            if(!mqttPingNow || (millis() - mqttPingNow > mqttPingInt)) {
                mqttPingNow = millisNonZero();
                if(!mqttClient.sendPing()) {
                    // Mostly fails for internal reasons;
                    // skip ping test in that case
                    mqttDoPing = false;
                    mqttPingDone = true;  // allow mqtt-connect attempt
                }
            }
        }
        break;
    case PING_PINGING:
        if(mqttClient.pollPing()) {
            mqttPingDone = true;          // allow mqtt-connect attempt
            mqttPingNow = 0;
            mqttPingsExpired = 0;
            mqttPingInt = MQTT_SHORT_INT; // Overwritten on fail in reconnect
            // Delay re-connection for 5 seconds after first ping echo
            if(!(mqttReconnectNow = millis() - (mqttReconnectInt - 5000)))
                mqttReconnectNow--;
        } else if(millis() - mqttPingNow > 5000) {
            mqttClient.cancelPing();
            mqttPingNow = millisNonZero();
            mqttPingsExpired++;
            mqttPingInt = MQTT_SHORT_INT * (1 << (mqttPingsExpired / MQTT_FAILCOUNT));
            mqttReconnFails = 0;
        }
        break;
    } 
}

static bool mqttReconnect(bool force)
{
    bool success = false;

    if(useMQTT && (WiFi.status() == WL_CONNECTED)) {

        if(!mqttClient.connected()) {
    
            if(force || !mqttReconnectNow || (millis() - mqttReconnectNow > mqttReconnectInt)) {

                #ifdef SID_DBG
                Serial.println("MQTT: Attempting to (re)connect");
                #endif
    
                if(strlen(mqttUser)) {
                    success = mqttClient.connect(mqttUser, strlen(mqttPass) ? mqttPass : NULL);
                } else {
                    success = mqttClient.connect();
                }
    
                mqttReconnectNow = millisNonZero();
                
                if(!success) {
                    mqttRestartPing = true;  // Force PING check before reconnection attempt
                    mqttReconnFails++;
                    if(mqttDoPing) {
                        mqttPingInt = MQTT_SHORT_INT * (1 << (mqttReconnFails / MQTT_FAILCOUNT));
                    } else {
                        mqttReconnectInt = MQTT_SHORT_INT * (1 << (mqttReconnFails / MQTT_FAILCOUNT));
                    }
                    #ifdef SID_DBG
                    Serial.printf("MQTT: Failed to reconnect (%d)\n", mqttReconnFails);
                    #endif
                } else {
                    mqttReconnFails = 0;
                    mqttReconnectInt = MQTT_SHORT_INT;
                    #ifdef SID_DBG
                    Serial.println("MQTT: Connected to broker, waiting for CONNACK");
                    #endif
                }
    
                return success;
            } 
        }
    }
      
    return true;
}

static void mqttSubscribe()
{
    // Meant only to be called when connected!
    if(!mqttSubAttempted) {
        if(!mqttClient.subscribe("bttf/sid/cmd", "bttf/tcd/pub")) {
            #ifdef SID_DBG
            Serial.println("MQTT: Failed to subscribe to command topics");
            #endif
        }
        if(*settings.mqttTopic) {
            if(!mqttClient.subscribe(settings.mqttTopic)) {
                #ifdef SID_DBG
                Serial.printf("MQTT: Failed to subscribe to user topic '%s'\n", settings.mqttTopic);
                #endif
            }
        }
        mqttSubAttempted = true;
    }
}

bool mqttState()
{
    return (useMQTT && mqttClient.connected());
}

void mqttPublish(const char *topic, const char *pl, unsigned int len)
{
    if(useMQTT) {
        mqttClient.publish(topic, (uint8_t *)pl, len, false);
    }
}           

#endif
