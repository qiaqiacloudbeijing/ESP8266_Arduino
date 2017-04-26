#ifndef qiaqiaSingleDevESP8266_h
#define qiaqiaSingleDevESP8266_h

#include <Arduino.h>
#include <Client.h>
#include <string.h>

#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include <FS.h>
#include <FSImpl.h>
#include <Esp.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "MsgHandler.h"
#include "PubSubClient.h"   //a third-party MQTT library


#define DATA_FUN_POINT 1000
#define EPINDEX "8266"

#define AP_NAME  "ESP8266_Dev"   
#define AP_PASSWORD  "esp8266zdf"

//This enables lots of debug output.
#define DEVELOP_VERSION
#ifdef DEVELOP_VERSION
#define SINGLEDEVICE_DEBUG
#endif

#ifdef SINGLEDEVICE_DEBUG
#define SD_DBG_ln Serial.println
#define SD_DBG Serial.print
#else 
#define SD_DBG_ln
#define SD_DBG
#endif

typedef struct {
	char *epindex;
	char userid[15];
	bool sd_sub_successed;

	char sdindex[25];
	char apikey[30];
    char cloudip[20];
    char cloudport[6];
    char ssid[30];
    char password[30];
} singleDeviceInfo;

typedef struct 
{
	char str_from[15];
	char str_ver[15];
	char str_srcuserid[25];
	char str_sflag[25];
	char str_epindex[25];
	char str_devindex[25];
	char str_subindex[25];
	char str_userid[25];
	char str_offsetindex[15];
	char str_cmd[15];
} topicParseInfo;

/*****存储server读本地时的payload信息*****/
typedef struct 
{
	char offsetindex[15];
	char devack[80];
} sdReadCallbackParameters;

/*****存储server写本地时的payload信息*****/
typedef struct 
{
	char offsetindex[15];
	char value[25];
} sdWriteCallbackParameters;

/*****本地设备响应server读命令回传时所需要的信息*****/
typedef struct 
{
	char offsetindex_src[15];
	char userid_des[25];
	char epindex_des[25];
	char devindex_des[25];
	char offsetindex_des[15];
} sdAckInfo;

/*****OTA升级从server收到的升级路径信息*****/
typedef struct   
{
	char updateURL[150];  
	char updateIP[15];
	char updatePort[15];
	char updatePath[100];
} sdUpdateParameters;

typedef void(*readcallback)(int offsetindex, char *ackdev);   //定义读回调函数类型
typedef void(*writecallback)(int offsetindex, float value);   //定义写回调函数类型

class qiaqiaSingDevESP8266 {
public:
	qiaqiaSingDevESP8266();

	singleDeviceInfo m_SdInfo;
	topicParseInfo m_topicParseInfo;
	sdReadCallbackParameters m_sdReadCallbackParameters;
	sdWriteCallbackParameters m_sdWriteCallbackParameters;
	sdAckInfo m_sdAckInfo;
	sdUpdateParameters m_sdUpdateParameters;

	WiFiClient wifiClient;
	MsgHandler *pMsgHandler;
	PubSubClient *pClient;

	void sdInit();
	bool sdWifiConfig();  //连接本地WIFI
	bool sdRun();  //连接远程服务器，接收数据
	void sdMQTTDataParse(char *topic, byte *payload, unsigned int length);

	int sdValueChange(int offsetindex, String value);
	int sdAck(char *ackdev, String value);
	void setReadCallback(readcallback outerreadcall);
	readcallback m_ReadCallback;
	void setWriteCallback(writecallback outerwritecall);
	writecallback m_WriteCallback;

	void sdUpdateParametersDecode(byte *payload, unsigned int length);

private:
	const char *_ackuseridcmd;   
	char _cloudip[20];
	char _cloudport[6];
	String _acksdpayload[4];    
	String _valuechangepayload[1];

	char *apName;
	char *apPassword;

	unsigned long apConfigStart;
	uint8_t apFirstTimeState;
	
	uint8_t RemoteFirstTimeState;
	unsigned long RemoteConfigStart;

	void sdDataMemoryInit();
	int sdAckuseridSub();   //设备向云端订阅
	int sdAckuseridRegist();   //设备注册
	void sdTopicParse(char *topic);
	void sdPayloadAckuseridParse(byte *payload, unsigned int length);    //解析注册时云端传下来的payload
	void sdRWSub();  //设备订阅读写
	sdReadCallbackParameters sdReadCallbackParametersDecode(byte *payload, unsigned int length);
	sdWriteCallbackParameters sdWriteCallbackParametersDecode(byte *payload, unsigned int length);
	sdAckInfo sdAckDecode(char *ackdev);

	void sdAPModeConfig();

};

#endif