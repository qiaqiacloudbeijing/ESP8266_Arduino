/*
 MsgHandler.h - The code is used for handling the message from QiaqiaMQTT.
The process including message type analysis, topic and payload parse (encode and deconde).
  Qiaqia Cloud
*/

#include <MsgHandler.h>

MsgHandler::MsgHandler() {
}

char* MsgHandler::topicAckuseridEncode(char *epindex, char *sdindex) {
	static char temp[50] = {"\0"};
	memset(temp, 0, 50);
	sprintf(temp, "s/1/0/0/%s/%s/+/0/0/ackuserid", epindex, sdindex);
	return temp;
}

char* MsgHandler::topicRegisterEncode(char *epindex, char *sdindex) {
	static char temp[50] = {"\0"};
	memset(temp, 0, 50);
	sprintf(temp, "c/1/0/0/%s/%s/0/0/0/register", epindex, sdindex);
	return temp;
}

char* MsgHandler::topicReadEncode(char *sdindex, char *userid) {
	static char temp[80] = {"\0"};
	memset(temp, 0, 80);
	sprintf(temp, "s/1/+/0/+/%s/+/%s/+/read", sdindex, userid);
	return temp;
}

char* MsgHandler::topicWriteEncode(char *sdindex, char *userid) {
	static char temp[80] = {"\0"};
	memset(temp, 0, 80);
	sprintf(temp, "s/1/+/0/+/%s/+/%s/+/write", sdindex, userid);
	return temp;
}

char* MsgHandler::topicSubdevsReadEncode(char *devindex, char *userid) {
	static char temp[80] = {"\0"};
	memset(temp, 0, 80);
	sprintf(temp, "s/1/+/0/+/%s/+/%s/+/read", devindex, userid);
	return temp;
}

char* MsgHandler::topicSubdevsWriteEncode(char *devindex, char *userid) {
	static char temp[80] = {"\0"};
	memset(temp, 0, 80);
	sprintf(temp, "s/1/+/0/+/%s/+/%s/+/write", devindex, userid);
	return temp;
}

char* MsgHandler::topicValuechangeEncode(char *srcuserid, char *epindex, char *devindex,
		char *userid, char *offsetindex) {
	static char temp[120] = {"\0"};
	memset(temp, 0, 120);
	sprintf(temp, "c/1/%s/0/%s/%s/0/%s/%s/valuechange", srcuserid, epindex, devindex,
		userid, offsetindex);
	Serial.print("the valuechange topic = ");
	Serial.println(temp);//zdf
	return temp;
}

char* MsgHandler::topicAckEncode(char *srcuserid, char *epindex, char *devindex,
		char *userid, char *offsetindex) {
	static char temp[120] = {"\0"};
	memset(temp, 0, 120);
	sprintf(temp, "c/1/%s/1/%s/%s/0/%s/%s/ack", srcuserid, epindex, devindex,
		userid, offsetindex);
	return temp;
}

String MsgHandler::jsonAckdevEncode(String* payload) {
	DynamicJsonBuffer jsonBuffer_devack;
	JsonObject& root = jsonBuffer_devack.createObject();
	JsonObject& data = root.createNestedObject("data");
	JsonObject& devid = data.createNestedObject("devid");
	devid["sflag"] = "0";
	devid["epindex"] = *payload;//_device.epindex;
	devid["devindex"] = *(++payload);//_device.devindex;
	devid["subdevindex"] = "0";
	data["offsetindex"] = *(++payload);//offsetindex_src;
	data["value"] = *(++payload);//value;
	String temp;
	root.prettyPrintTo(temp);
	return temp;
}

String MsgHandler::jsonValuechangeEncode(String *payload) {
	DynamicJsonBuffer jsonBufferValuechange;
	JsonObject& root = jsonBufferValuechange.createObject();
	JsonObject& data = root.createNestedObject("data");
	data["value"] = *payload;
	String temp;
	root.prettyPrintTo(temp);
	
    Serial.print("the valuechange padload = ");
	Serial.println(temp);

	return temp;
}

//zdf
String MsgHandler::jsonUpdateStatusEncode(String *payload) {
	DynamicJsonBuffer jsonBufferValuechange;
	JsonObject& root = jsonBufferValuechange.createObject();
	JsonObject& data = root.createNestedObject("data");
	data["value"] = *payload;
	String temp;
	root.prettyPrintTo(temp);
	return temp;

}

char* MsgHandler::payloadJsonEncode(int json_type, String *payload) {
	switch(json_type) {
		case PYL_MSG_TYPE_DEVACK:
			return (char *)jsonAckdevEncode(payload).c_str();
			break;
		case PYL_MSG_TYPE_VALUECHANGE:
		{
			return (char *)jsonValuechangeEncode(payload).c_str();
			//char* json = (char *)jsonValuechangeEncode(payload).c_str();
			//Serial.print("valuechange payload = ");
			//Serial.println(json);
			//return json;
			break;
		}
		case PYL_MSG_TYPE_UPDATESTATUS:  //zdf
			return (char *)jsonUpdateStatusEncode(payload).c_str();
			break;
		default:
			break;
	}
}

char* MsgHandler::gwAckReadEncode(char *topic_srcuserid, char *devindex_read, char *offsetindex_read,
		char *topic_offsetindex, char *epindex_read) {
	static char temp[120] = {"\0"};
	memset(temp, 0, 120);
	sprintf(temp, "%s/%s/%s/%s/%s/", topic_srcuserid, devindex_read, offsetindex_read, 
		topic_offsetindex, epindex_read);
	return temp;
}

char* MsgHandler::topicOnlineEncode(char *srcuserid, char *devindex, char *userid) {
	static char temp[60] = {"\0"};
	memset(temp, 0, 120);
	sprintf(temp, "c/1/%s/0/0/%s/0/%s/0/online", srcuserid, devindex, userid);
	return temp;
}

char* MsgHandler::payloadOnlineJsonEncode(bool onlinestate) {
	DynamicJsonBuffer jsonBufferValuechange;
	JsonObject& root = jsonBufferValuechange.createObject();
	JsonObject& data = root.createNestedObject("data");
	if (onlinestate == true)
		data["value"] = "true";
	else if (onlinestate == false)
		data["value"] = "false";
	String temp;
	root.prettyPrintTo(temp);
	return (char *)(temp.c_str());
}

//zdf
char* MsgHandler::topicUpdateStatusEncode(char *srcuserid, char *epindex, char *devindex, char *userid, char *offsetindex) {
	static char temp[120] = {"\0"};
	memset(temp, 0, 120);
	sprintf(temp, "c/1/%s/0/%s/%s/0/%s/%s/updatestate", srcuserid, epindex, devindex, userid, offsetindex);
	//Serial.println(temp);
	return temp;
}