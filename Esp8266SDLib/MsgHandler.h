/*
 MsgHandler.h - The code is used for handling the message from QiaqiaMQTT.
The process including message type analysis, topic and payload parse (encode and deconde).
  Qiaqia Cloud
*/

#ifndef MsgHandler_h
#define MsgHandler_h

#include <Arduino.h>
#include <ArduinoJson.h>

enum mqtt_message_type {
	MQTT_MSG_TYPE_ACKUSERID = 1,
	MQTT_MSG_TYPE_READ = 2,
	MQTT_MSG_TYPE_WRITE = 3
};

enum ackuserid_message_type {
	CMD_MSG_TYPE_OK = 2,
	CMD_MSG_TYPE_REDIRECT = 3
};

enum read_funmsg_type {
	FUN_MSG_TYPE_READ_DEV_ID = 1001,
	FUN_MSG_TYPE_MODEL_ID = 1002,
	FUN_MSG_TYPE_READ_TIME = 1004,
	FUN_MSG_TYPE_UPDATE_STATUS_F = 1005,
	FUN_MSG_TYPE_UPDATE_STATUS_S = 1006,
	FUN_MSG_TYPE_DEBUG_SWITCHER_R = 1007,
	FUN_MSG_TYPE_DEBUG_INFO = 1008
};

enum write_funmsg_type {
	FUN_MSG_TYPE_DEV_RESTART = 1003,
	FUN_MSG_TYPE_ADJUST_TIME = 1004,
	FUN_MSG_TYPE_UPDATE_F = 1005,
	FUN_MSG_TYPE_UPDATE_S = 1006,
	FUN_MSG_TYPE_DEBUG_SWITCHER_W = 1007,
	FUN_MSG_TYPE_WIFI_CLEAR = 1009
};

enum payload_encode_type {
	PYL_MSG_TYPE_DEVACK = 2,
	PYL_MSG_TYPE_VALUECHANGE = 3,
	PYL_MSG_TYPE_UPDATESTATUS = 4
};

class MsgHandler {
private:
	String jsonAckdevEncode(String* payload);
	String jsonValuechangeEncode(String* payload);
	String jsonUpdateStatusEncode(String* payload);  //zdf
public:
	MsgHandler();
	static inline int mqtt_get_type(char *buffer) { return (buffer[0] & 0x03); }
	static inline int mqtt_get_ackuserid_type(const char *buffer) { return (buffer[0] & 0x30) >> 4; }
	static inline int payload_encode_type(char *buffer) { return (buffer[0] & 0x30) >> 4; }
	
	void gwTopicParse(char *topic);

	char *topicAckuseridEncode(char *epindex, char *devindex);
	char *topicRegisterEncode(char *epindex, char *devindex);
	char *topicReadEncode(char *sdindex, char *userid); 
	char *topicWriteEncode(char *sdindex, char *userid);
	char *topicSubdevsReadEncode(char *devindex, char *userid);
	char *topicSubdevsWriteEncode(char *devindex, char *userid);
	char *topicValuechangeEncode(char *srcuserid, char *epindex, char *devindex,
		char *userid, char *offsetindex);
	char *topicAckEncode(char *srcuserid, char *epindex, char *devindex,
		char *userid, char *offsetindex);
	char *gwAckReadEncode(char *topic_srcuserid, char *devindex_read, char *offsetindex_read,
		char *topic_offsetindex, char *epindex_read);
	char *payloadJsonEncode(int json_type, String *payload);
	char *topicOnlineEncode(char *srcuserid, char *devindex, char *userid);
	char *payloadOnlineJsonEncode(bool onlinestate);

	char *topicUpdateStatusEncode(char *srcuserid, char *epindex, char *devindex, char *userid, char *offsetindex); //zdf
};

#endif