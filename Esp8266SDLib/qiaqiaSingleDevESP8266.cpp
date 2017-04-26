#include "qiaqiaSingleDevESP8266.h"

qiaqiaSingDevESP8266::qiaqiaSingDevESP8266(){

	apConfigStart = 0;
	apFirstTimeState = 1;
	
	RemoteFirstTimeState = 1;
    RemoteConfigStart = 0;
}

void qiaqiaSingDevESP8266::sdDataMemoryInit(){
	memset(&m_topicParseInfo, 0, sizeof(topicParseInfo));
	memset(&m_sdReadCallbackParameters, 0, sizeof(sdReadCallbackParameters));
	memset(&m_sdWriteCallbackParameters, 0, sizeof(sdWriteCallbackParameters));
	
	memset(&m_sdAckInfo, 0, sizeof(sdAckInfo));
	memset(_cloudip, 0, 20);
	memset(_cloudport, 0, 6);
	memset(_acksdpayload, 0, 4);
}

static void sdMQTTRecv(char* topic, byte* payload, unsigned int length, void* user){  //user的作用？？？？？
	qiaqiaSingDevESP8266* qiaqiaMQTTSD = (qiaqiaSingDevESP8266 *)user;
	SD_DBG("Message arrived [");
  	SD_DBG(topic);
  	SD_DBG_ln("] ");
  	for (int i = 0; i < length; i++) {
    	SD_DBG((char)payload[i]);
  	}
  	SD_DBG_ln();

  	qiaqiaMQTTSD->sdMQTTDataParse(topic, payload, length);
}

void qiaqiaSingDevESP8266::sdInit(){
	SD_DBG_ln("enter sdInit...");

	WiFi.mode(WIFI_STA);
    if(WiFi.getMode() == WIFI_STA){
    	Serial.println("STA mode");
    	WiFi.persistent(false);
  	}

	sdDataMemoryInit();
	memset(&m_SdInfo, 0, sizeof(singleDeviceInfo));

	Serial.println("read singledevice parameters from flash file");
	SPIFFS.begin();
	if(SPIFFS.exists("/devcfg/parameters") != true){
		Serial.println("the file does not exist");
		delay(1000);
		Serial.println("please use APMode to config parameters");
	}else {
		char sdParameters[200] = {"\0"};
		File temp = SPIFFS.open("/devcfg/parameters", "r");
		temp.read((uint8_t*)sdParameters, sizeof(sdParameters));
		//Serial.println(sdParameters);
		temp.close();
	
		//char sdParameters[] = "58eeff5035a91424a4700647/57fb3f58d8207e1934bb2e88/101.201.78.22/1883/qiaqiacloud/zhanggaoyuan200/";

		char* pEnd_sdindex = strstr(sdParameters, "/");
		strncpy(m_SdInfo.sdindex, sdParameters, (uint8_t)(pEnd_sdindex - sdParameters));
		Serial.println(m_SdInfo.sdindex);

		char* pEnd_apikey = strstr(pEnd_sdindex +1, "/");
		strncpy(m_SdInfo.apikey, pEnd_sdindex+1, (uint8_t)(pEnd_apikey - (pEnd_sdindex+1)));
		Serial.println(m_SdInfo.apikey);

		char* pEnd_cloudip = strstr(pEnd_apikey +1, "/");
		strncpy(m_SdInfo.cloudip, pEnd_apikey+1, (uint8_t)(pEnd_cloudip - (pEnd_apikey+1)));
		Serial.println(m_SdInfo.cloudip);

		char* pEnd_cloudport = strstr(pEnd_cloudip +1, "/");
		strncpy(m_SdInfo.cloudport, pEnd_cloudip+1, (uint8_t)(pEnd_cloudport - (pEnd_cloudip+1)));
		Serial.println(m_SdInfo.cloudport);

		char* pEnd_ssid = strstr(pEnd_cloudport +1, "/");
		strncpy(m_SdInfo.ssid, pEnd_cloudport+1, (uint8_t)(pEnd_ssid - (pEnd_cloudport+1)));
		Serial.println(m_SdInfo.ssid);

		char* pEnd_password = strstr(pEnd_ssid +1, "/");
		strncpy(m_SdInfo.password, pEnd_ssid+1, (uint8_t)(pEnd_password - (pEnd_ssid+1)));
		Serial.println(m_SdInfo.password);

		m_SdInfo.epindex = EPINDEX;
	}
		//change char port to uint8_t port
    	uint16_t port = atoi(m_SdInfo.cloudport);

    	apName = AP_NAME;
    	apPassword = AP_PASSWORD;
		pClient = new PubSubClient(wifiClient);
		pClient->setServer(m_SdInfo.cloudip, port);
		pClient->setCallback(sdMQTTRecv, this);
}

bool qiaqiaSingDevESP8266::sdWifiConfig(){
	Serial.println("if not connecting to wifi in 20 seconds, then start wificonfig");
	
	if(m_SdInfo.password[0] == '\0'){
        WiFi.begin(m_SdInfo.ssid);
    }else {
        WiFi.begin(m_SdInfo.ssid, m_SdInfo.password);
    }
	
	for(int i = 0; i<20; i++){
		if(WiFi.status() == WL_CONNECTED){
			Serial.println("auto connect wifi succeed");
			return true;
		}
		Serial.print(".");
		delay(1000);
	}
	Serial.println("auto connect wifi failed");

	Serial.println("enter APMode to config wifi parameters");
    sdAPModeConfig();

	return false; //两种方法都失败，则返回false
}

bool qiaqiaSingDevESP8266::sdRun() {
	if(WiFi.status() != WL_CONNECTED){
		Serial.println("wifi disconnected, start wifi Connection");
		sdWifiConfig();
		return false;
	}
	SD_DBG_ln("WiFi Connection success");

	if(WiFi.status() == WL_CONNECTED){
		if(pClient && !pClient->connected()){
			SD_DBG_ln("remote server disconnted, reconnecting...");

			randomSeed((long)millis());  //设置随机数种子
			char clientID[20] = {0};
			sprintf(clientID, "ESP8266%d", random(1,1000));
			SD_DBG_ln(clientID);

			m_SdInfo.sd_sub_successed = false;
			sdDataMemoryInit();

			if(!pClient->connect(clientID, m_SdInfo.sdindex, m_SdInfo.apikey)){
				SD_DBG_ln("connect remote server failed !!");

				if(RemoteFirstTimeState == 1){
                    RemoteConfigStart = millis();
                    RemoteFirstTimeState = 0;
                }
      
                if((millis() - RemoteConfigStart) < 0){
                    Serial.println("millis() runoff");  
                    RemoteFirstTimeState = 1;  
                }

                if((millis() - RemoteConfigStart) > 60000){  //连接云端服务器超时时间60s，之后进人AP配置
                    Serial.println("enter APMode to config wifi parameters");
                    sdAPModeConfig();
                }

			}else {
				RemoteFirstTimeState = 1;
                RemoteConfigStart = 0;
				sdAckuseridSub();
				sdAckuseridRegist();
				Serial.println("connect remote server success");
			}
		}
		pClient->loop();
	}
	return true;
}

int qiaqiaSingDevESP8266::sdAckuseridSub() {
	SD_DBG_ln("enter sdAckuseridSub");
	return pClient->subscribe(pMsgHandler->topicAckuseridEncode(m_SdInfo.epindex, m_SdInfo.sdindex));
}

int qiaqiaSingDevESP8266::sdAckuseridRegist() {
	SD_DBG_ln("enter sdAckuseridRegist");
	return pClient->publish(pMsgHandler->topicRegisterEncode(m_SdInfo.epindex, m_SdInfo.sdindex), "");
}

void qiaqiaSingDevESP8266::sdTopicParse(char *topic) {
	SD_DBG_ln("enter sdTopicParse");

	uint16_t topic_len = strlen(topic);
	memset(&m_topicParseInfo, 0, sizeof(topicParseInfo));

	int len = 0;
	char *pfrom = strstr(topic, "/");
	if(pfrom != NULL)
		strncpy(m_topicParseInfo.str_from, (char *)topic, (uint8_t)(pfrom - topic));
	if(strlen(m_topicParseInfo.str_from) == 0) return;
	len += (strlen(m_topicParseInfo.str_from) + 1);

	char *pver = strstr(pfrom + 1, "/");
  	if(pver != NULL)
    	strncpy(m_topicParseInfo.str_ver, (char *)pfrom + 1, (uint8_t)(pver - pfrom - 1));
  	if(strlen(m_topicParseInfo.str_ver) == 0) return;
  	len += (strlen(m_topicParseInfo.str_ver) + 1);

  	char *psrcuserid = strstr(pver + 1, "/");
  	if(psrcuserid != NULL)
    	strncpy(m_topicParseInfo.str_srcuserid, (char *)pver + 1, (uint8_t)(psrcuserid - pver - 1));
  	if(strlen(m_topicParseInfo.str_srcuserid) == 0) return;
  	len += (strlen(m_topicParseInfo.str_srcuserid) + 1);

  	char *psflag = strstr(psrcuserid + 1, "/");
  	if(psflag != NULL)
    	strncpy(m_topicParseInfo.str_sflag, (char *)psrcuserid + 1, (uint8_t)(psflag - psrcuserid - 1));
  	if(strlen(m_topicParseInfo.str_sflag) == 0) return;
  	len += (strlen(m_topicParseInfo.str_sflag) + 1);

  	char *pepindex = strstr(psflag + 1, "/");
  	if(pepindex != NULL)
    	strncpy(m_topicParseInfo.str_epindex, (char *)psflag + 1, (uint8_t)(pepindex - psflag - 1));
  	if(strlen(m_topicParseInfo.str_epindex) == 0) return;
  	len += (strlen(m_topicParseInfo.str_epindex) + 1);

  	char *pdevindex = strstr(pepindex + 1, "/");
  	if(pdevindex != NULL)
    	strncpy(m_topicParseInfo.str_devindex, (char *)pepindex + 1, (uint8_t)(pdevindex - pepindex - 1));
  	if(strlen(m_topicParseInfo.str_devindex) == 0) return;
  	len += (strlen(m_topicParseInfo.str_devindex) + 1);

  	char *psubindex = strstr(pdevindex + 1, "/");
  	if(psubindex != NULL)
    	strncpy(m_topicParseInfo.str_subindex, (char *)pdevindex + 1, (uint8_t)(psubindex - pdevindex - 1));
  	if(strlen(m_topicParseInfo.str_subindex) == 0) return;
  	len += (strlen(m_topicParseInfo.str_subindex) + 1);

  	char *puserid = strstr(psubindex + 1, "/");
  	if(puserid != NULL)
    	strncpy(m_topicParseInfo.str_userid, (char *)psubindex + 1, (uint8_t)(puserid - psubindex - 1));
  	if(strlen(m_topicParseInfo.str_userid) == 0) return;
  	len += (strlen(m_topicParseInfo.str_userid) + 1);

  	char *poffsetindex = strstr(puserid + 1, "/");
  	if(poffsetindex != NULL)
    	strncpy(m_topicParseInfo.str_offsetindex, (char *)puserid + 1, (uint8_t)(poffsetindex - puserid - 1));
  	if(strlen(m_topicParseInfo.str_offsetindex) == 0) return;
  	len += (strlen(m_topicParseInfo.str_offsetindex) + 1);

  	char *pcmd = poffsetindex + 1;
  	if((pcmd != NULL) && (len < topic_len))
    	strncpy(m_topicParseInfo.str_cmd, (char *)poffsetindex + 1, (size_t)(topic_len -len));
  	if(strlen(m_topicParseInfo.str_cmd) == 0) return;
}

void qiaqiaSingDevESP8266::sdRWSub() {
	SD_DBG_ln("sdRWSub");
	pClient->subscribe(pMsgHandler->topicReadEncode(m_SdInfo.sdindex, m_SdInfo.userid));
	pClient->subscribe(pMsgHandler->topicWriteEncode(m_SdInfo.sdindex, m_SdInfo.userid));
}

void qiaqiaSingDevESP8266::sdPayloadAckuseridParse(byte *payload, unsigned int length) {
	SD_DBG_ln("enter sdPayloadAckuseridParse");

  	uint8_t ackuserid_msg_type;
  	DynamicJsonBuffer jsonBufferAckuserid;
  	JsonObject& root = jsonBufferAckuserid.parseObject((char *)payload);
  	if(!root.success()) {
		Serial.println("Json format wrong");
		return;
	}

	int count = 0;
	_ackuseridcmd = root["cmd"];
	const char *userid = root["userid"];
	const char *ip = root["server"]["ip"];  
	const char *port = root["server"]["port"];  

	ackuserid_msg_type = pMsgHandler->mqtt_get_ackuserid_type(_ackuseridcmd);

	switch (ackuserid_msg_type) {
	case CMD_MSG_TYPE_OK:
		strncpy(m_SdInfo.userid, (char *)userid, strlen(userid));
		m_SdInfo.sd_sub_successed = true;
		sdRWSub();
		break;
	case CMD_MSG_TYPE_REDIRECT:
	{
		strncpy(_cloudip, (char *)ip, strlen(ip));
		strncpy(_cloudport, (char *)port, strlen(port));

		Serial.println("enter server change");

        char gwParameters[200] = {"\0"};
        sprintf(gwParameters, "%s/%s/%s/%s/%s/%s/", m_GwInfo.gwindex, m_GwInfo.apikey, _cloudip, _cloudport, m_GwInfo.ssid, m_GwInfo.password);
                        
        SPIFFS.remove("/devcfg/parameters");  //删掉原有的文件
        File temp = SPIFFS.open("/devcfg/parameters", "w");
		temp.write((uint8_t*)gwParameters, sizeof(gwParameters));
		temp.close();

		delay(500);
		ESP.restart();
	}
		break;
	default:
		break;
	}
}

void qiaqiaSingDevESP8266::sdMQTTDataParse(char *topic, byte *payload, unsigned int length) {
	SD_DBG_ln("enter sdMQTTDataParse");

	uint8_t msg_type;
	sdTopicParse(topic);
	msg_type = pMsgHandler->mqtt_get_type(m_topicParseInfo.str_cmd);
	
	switch (msg_type) {
	case MQTT_MSG_TYPE_ACKUSERID:
		sdPayloadAckuseridParse(payload, length);
		break;

	case MQTT_MSG_TYPE_READ:
		if (m_SdInfo.sd_sub_successed == true) {
			if (atol(m_topicParseInfo.str_offsetindex) <= DATA_FUN_POINT) {
				m_sdReadCallbackParameters = sdReadCallbackParametersDecode(payload, length);
				if (m_ReadCallback) {
					m_ReadCallback(atol(m_sdReadCallbackParameters.offsetindex), m_sdReadCallbackParameters.devack);
				}
			} else if (atol(m_topicParseInfo.str_offsetindex) > DATA_FUN_POINT) {
				switch (atol(m_topicParseInfo.str_offsetindex)) {
				case FUN_MSG_TYPE_READ_DEV_ID:
					break;
				case FUN_MSG_TYPE_MODEL_ID:
					break;
				case FUN_MSG_TYPE_READ_TIME:  
					break;
				case FUN_MSG_TYPE_UPDATE_STATUS_F:  
					break;
				case FUN_MSG_TYPE_UPDATE_STATUS_S:  
					break;
				case FUN_MSG_TYPE_DEBUG_SWITCHER_R:
					break;
				case FUN_MSG_TYPE_DEBUG_INFO:
					break;
				default:
					break;
				}
			}
		}
		break;

	case MQTT_MSG_TYPE_WRITE:
		if (m_SdInfo.sd_sub_successed == true) {
			if (atol(m_topicParseInfo.str_offsetindex) <= DATA_FUN_POINT) {
				m_sdWriteCallbackParameters = sdWriteCallbackParametersDecode(payload, length);
				if (m_WriteCallback) {
					m_WriteCallback(atol(m_sdWriteCallbackParameters.offsetindex), atof(m_sdWriteCallbackParameters.value));
				}
			} else if (atol(m_topicParseInfo.str_offsetindex) > DATA_FUN_POINT) {
				switch (atol(m_topicParseInfo.str_offsetindex)) {
				case FUN_MSG_TYPE_DEV_RESTART:
						Serial.println("dev restart!!");
						ESP.restart();
					break;

				case FUN_MSG_TYPE_ADJUST_TIME:
					break;

				case FUN_MSG_TYPE_UPDATE_F:
					{
						Serial.println("enter update!!");
					 	sdUpdateParametersDecode(payload, length);
					 	t_httpUpdate_return ret = ESPhttpUpdate.update(m_sdUpdateParameters.updateURL);  //升级成功会自动重启

        				switch(ret) {
            				case HTTP_UPDATE_FAILED:
                				Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                				break;

            				case HTTP_UPDATE_NO_UPDATES:
                				Serial.println("HTTP_UPDATE_NO_UPDATES");
                				break;

            				case HTTP_UPDATE_OK:
                				Serial.println("HTTP_UPDATE_OK");
                				break;
        				}
					}
					break;

				case FUN_MSG_TYPE_UPDATE_S:
					break;
				case FUN_MSG_TYPE_DEBUG_SWITCHER_W:
					break;

				case FUN_MSG_TYPE_WIFI_CLEAR:
					{
						Serial.println("enter wifi clear");
						memset(m_SdInfo.ssid, 0, sizeof(m_SdInfo.ssid));
                    	memset(m_SdInfo.password, 0 ,sizeof(m_SdInfo.password));

                    	char sdParameters[200] = {"\0"};
                        sprintf(sdParameters, "%s/%s/%s/%s/%s/%s/", m_SdInfo.sdindex, m_SdInfo.apikey, m_SdInfo.cloudip, m_SdInfo.cloudport, m_SdInfo.ssid, m_SdInfo.password);
                        
                        SPIFFS.remove("/devcfg/parameters");  //删掉原有的文件
                        File temp = SPIFFS.open("/devcfg/parameters", "w");
						temp.write((uint8_t*)sdParameters, sizeof(sdParameters));
						temp.close();

						delay(500);
						ESP.restart();

					}
					break;
					
				default:
					break; 
				}
			}
		}
		break;

	default:
		break;
	}
}

sdWriteCallbackParameters qiaqiaSingDevESP8266::sdWriteCallbackParametersDecode(byte *payload, unsigned int length) {
	SD_DBG_ln("enter sdWriteCallbackParametersDecode");

	sdWriteCallbackParameters gwwcp;
	DynamicJsonBuffer jsonBufferWrite;
	JsonObject& root = jsonBufferWrite.parseObject((char *)payload);
	if(!root.success()) {
		Serial.println("Json format wrong");
	}

	const char *sflag_write = root["data"]["devid"]["sflag"];
	const char *epindex_write = root["data"]["devid"]["epindex"];
	const char *devindex_write = root["data"]["devid"]["devindex"];
	const char *subindex_write = root["data"]["devid"]["subindex"];
	const char *offsetindex_write = root["data"]["offsetindex"];
	const char *value_write = root["data"]["value"];

	strncpy(gwwcp.offsetindex, m_topicParseInfo.str_offsetindex, strlen(m_topicParseInfo.str_offsetindex));
	strncpy(gwwcp.value, value_write, strlen(value_write));
			
	return gwwcp;
}

sdReadCallbackParameters qiaqiaSingDevESP8266::sdReadCallbackParametersDecode(byte *payload, unsigned int length) {
  	SD_DBG_ln("enter sdReadCallbackParametersDecode");

	sdReadCallbackParameters gwrcp;
	DynamicJsonBuffer jsonBufferRead;
	JsonObject& root = jsonBufferRead.parseObject((char *)payload);
	if(!root.success()) {
		SD_DBG("Json format wrong");
	}

	const char *sflag_read = root["devid"]["sflag"];   
	const char *epindex_read = root["devid"]["epindex"];
	const char *devindex_read = root["devid"]["devindex"];
	const char *subindex_read = root["devid"]["subindex"];
	const char *offsetindex_read = root["offsetindex"];
	const char *value_read = root["data"]["value"];

	strncpy(gwrcp.offsetindex, m_topicParseInfo.str_offsetindex, strlen(m_topicParseInfo.str_offsetindex));
	char *devacktemp = pMsgHandler->gwAckReadEncode(m_topicParseInfo.str_srcuserid, (char *)devindex_read,
									(char *)offsetindex_read, m_topicParseInfo.str_offsetindex, (char *)epindex_read); 
	
	strncpy(gwrcp.devack, devacktemp, strlen(devacktemp));
		
	return gwrcp;
}

sdAckInfo qiaqiaSingDevESP8266::sdAckDecode(char *ackdev) {
	SD_DBG_ln("enter sdAckDecode");

	sdAckInfo gai;
	memset(&gai, 0, sizeof(sdAckInfo));

	char *pdesuserid = strstr(ackdev, "/");
	if (pdesuserid != NULL)
		strncpy(gai.userid_des, (char *)ackdev, (uint8_t)(pdesuserid - ackdev));
	if (strlen(gai.userid_des) == 0) return gai;

	char *pdevindex = strstr(pdesuserid + 1, "/");
	if (pdevindex != NULL)
		strncpy(gai.devindex_des, (char *)pdesuserid + 1, (uint8_t)(pdevindex- pdesuserid - 1));
	if (strlen(gai.devindex_des) == 0) return gai;

	char *pdesoffsetindex = strstr(pdevindex + 1, "/");
	if (pdesoffsetindex != NULL)
		strncpy(gai.offsetindex_des, (char *)pdevindex + 1, (uint8_t)(pdesoffsetindex - pdevindex - 1));
	if (strlen(gai.offsetindex_des) == 0) return gai;

	char *psrcoffsetindex = strstr(pdesoffsetindex + 1, "/");
	if (psrcoffsetindex != NULL)
		strncpy(gai.offsetindex_src, (char *)pdesoffsetindex + 1, (uint8_t)(psrcoffsetindex - pdesoffsetindex - 1));
	if (gai.offsetindex_src == 0) return gai;

	char *pepindex = strstr(psrcoffsetindex + 1, "/");
	if (pepindex != NULL)
		strncpy(gai.epindex_des, (char *)psrcoffsetindex + 1, (uint8_t)(pepindex - psrcoffsetindex - 1));
	if (gai.epindex_des == 0) return gai;

	return gai;
}

int qiaqiaSingDevESP8266::sdAck(char *ackdev, String value) {
	SD_DBG_ln("enter sdAck");

	if (m_SdInfo.sd_sub_successed == true) {
		m_sdAckInfo = sdAckDecode(ackdev);

		_acksdpayload[0] = m_SdInfo.epindex;
		_acksdpayload[1] = m_SdInfo.sdindex;  
		_acksdpayload[2] = m_sdAckInfo.offsetindex_src;
		_acksdpayload[3] = value;

		return pClient->publish(pMsgHandler->topicAckEncode(m_SdInfo.userid, m_sdAckInfo.epindex_des, m_sdAckInfo.devindex_des, m_sdAckInfo.userid_des, m_sdAckInfo.offsetindex_des), 
					pMsgHandler->payloadJsonEncode(pMsgHandler->payload_encode_type("devack"), _acksdpayload));
	}
}

int qiaqiaSingDevESP8266::sdValueChange(int offsetindex, String value) {
	static char offset[24] = {"\0"};  
	memset(offset, 0, 24);
	if (m_SdInfo.sd_sub_successed == true) {
		SD_DBG_ln("enter valuechange publish!!");
		_valuechangepayload[0] = value;
		sprintf(offset, "%d", offsetindex);
		return pClient->publish(pMsgHandler->topicValuechangeEncode(m_SdInfo.userid, m_SdInfo.epindex, m_SdInfo.sdindex, m_SdInfo.userid, offset), 
							pMsgHandler->payloadJsonEncode(pMsgHandler->payload_encode_type("valuechange"), _valuechangepayload));
			
		}
}

void qiaqiaSingDevESP8266::setReadCallback(readcallback outerreadcall) {
	m_ReadCallback = outerreadcall;
}

void qiaqiaSingDevESP8266::setWriteCallback(writecallback outerwritecall) {
	m_WriteCallback = outerwritecall;
}

void qiaqiaSingDevESP8266::sdAPModeConfig(void){
	Serial.println("Starting AP... ");
	WiFi.disconnect();
    delay(500);

    WiFiServer MyServer(80);
      
    WiFi.softAP(apName, apPassword);
    delay(500);
    if(WiFi.getMode() == WIFI_AP){
    	Serial.println("set AP mode ok");
  	}
  	IPAddress AP_IP = WiFi.softAPIP();
  	Serial.print("AP IP address: ");
  	Serial.println(AP_IP);
  	MyServer.begin();
    uint8_t IsLoop = 0;  

    while(1)  
    {
        if(apFirstTimeState == 1){
          apConfigStart = millis();
          apFirstTimeState = 0;
        }
      
        if((millis() - apConfigStart) < 0){
            Serial.println("millis() runoff");  
            apFirstTimeState = 1;  
        }

        if(IsLoop == 0){
            if((millis() - apConfigStart) > 120000){  //访问AP模式的时间窗口是120s
                Serial.println("restart the device");
                delay(1000);
                ESP.restart();
            }
        }   

        if((millis() - apConfigStart) > 600000){  //进入AP config 超过10分钟就退出重启
            Serial.println("restart the device");
            delay(1000);
            ESP.restart();
        }

        //判断是否有数据传输
        WiFiClient myClient = MyServer.available();
        uint8_t resetflag = 0;  //0-默认 1-设置成功重启 2-不设置退出 3-设置失败

        if (myClient){   // if you get a client,
            Serial.println(". Client connected to server");  // print a message out the serial port
            char buffer[200] = {0};  // make a buffer to hold incoming data
            int8_t i = 0;
            resetflag = 0;           

            while (myClient.connected())
            {   // loop while the client's connected
                if (myClient.available())
                {    // if there's bytes to read from the client,
                    char c = myClient.read();   // read a byte, then
                    //Serial.write(c);    // print it out the serial monitor
                    if (c == '\n'){    // if the byte is a newline character
                    
                        // if the current line is blank, you got two newline characters in a row.
                        // that's the end of the client HTTP request, so send a response:
                        if (strlen(buffer) == 0)
                        {
                            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                            // and a content-type so the client knows what's coming, then a blank line:
                            myClient.println("HTTP/1.1 200 OK");
                            myClient.println("Content-type:text/html");
                            myClient.println();

                            //user's code  ---start---    author:Wei Yongshu & Yang Lili  @20170301
                            myClient.println("<!DOCTYPE html><html><head><meta charset='UTF-8'/><meta name='viewport', content='width=device-width, initial-scale=1, user-scalable=no'/><title>QIAQIA CLOUGATE</title>");
                            myClient.println("<style>body{margin:0;padding:0;background:rgb(19,67,131);font-size:16px;font-family:'微软雅黑'} p.row2,.row2{text-align:right;width:93%;padding-right:20px;}.row2 button{margin-bottom:16px;}.row2 a,.row p,#modify{color:#fff;display:inline-block;}#modify{margin-top:0;}.box{width:400px;margin:180px auto;}.box h2{text-align: center;color:#fff;} .row{width:100%;height:40px;margin-bottom:10px;box-sizing: border-box;}.row span{color:#fff;display: inline-block;width:120px;text-align: right;margin-right:12px} .row input{width: 240px;height:36px;font-size:18px;border:none;} .btn{display:inline-block;padding:6px 12px;background:#c7c7c7;color:#333;margin-left:20px;margin-top:20px;} .btn:visited,btn:link{background:blue;cursor:pointer;} .btn2{margin-left:220px} .msg{text-align:right;color:red;margin-right:20px;font-size:12px;}");
                            myClient.println("@media screen and (max-width:440px) {body {font-size: 14px;}.box {width: 320px;margin: 60px auto 0;}.row span {color: #fff;display: inline-block;width: 100px;text-align: right;margin-right: 12px}.row input {width: 188px;height: 36px;font-size: 15px}.btn2 {margin-left: 115px}} .info{color:#fff;padding:30px 10px;font-size:20px;text-align:center;}</style></head>");
                            myClient.println("<body>");

                            if (resetflag == 1) {
                                myClient.println("<div class='info'><p>配置成功，设备正在重启</p></div>");
                            } else if (resetflag == 2) {
                                myClient.println("<div class='info'><p>退出配置，设备正在重启</p></div>");
                            } else if (resetflag == 3) {
                                myClient.println("<div class='info'><p>配置失败，设备正在重启</p></div>");
                            } else{
                                myClient.println("<div class='box'><h2>设备参数配置</h2><div class='row'><span>Devindex:</span><input type='text' id='indextxt'/></div><p id='msg1' class='msg'></p><div class='row'><span>Apikey:</span><input type='text' id='keytxt'/></div><p id='msg2' class='msg'></p>");
                                myClient.println(" <div class='row'><span>Cloudip:</span><input type='text' id='iptxt'/></div><div class='row'><span>Port:</span><input type='text' id='porttxt'/></div><p id='msg3' class='msg'></p>");
                                myClient.println("<div class='row2'><p id='modify'>点击修改上述信息</p></div>");
                                myClient.println("<div id='accesswrap' class=' row2'><input type='text' id='access' />");
                                myClient.println("<p id='readmsg'></p>");
                                myClient.println("<button id='accessbtn'>确定</button>");
                                myClient.println("<button id='cancelmod'>取消修改</button></div>");
                                myClient.println("<div class='row'><span>wifi ssid:</span><input type='text' id='ssidtxt'/></div><p id='msg4' class='msg'></p>");
                                myClient.println("<div class='row'><span> wifi password: </span><input type='password' id='pwdtxt'/></div><p id='msg5' class='msg'></p>");
                                myClient.println("<div class=' row2'><a id='show'>点击显示密码</a>");
                                myClient.println("<a id='show2'>点击隐藏密码</a></div>");
                                myClient.println("<p id='msg' class='msg'></p><button class='btn btn2' id='sendcmd'>OK</button><button class='btn' onclick=\"location.href='/abandon'\">cancel</button></div>");
                                myClient.println("<script type='text/javascript'>");
                                myClient.println("window.onload = function() {");
                                myClient.println("var A,B,C,D,E,F;");
                                myClient.print("A='");
                                myClient.print(m_SdInfo.sdindex);
                                myClient.println("'");
                                myClient.print("B='");
                                myClient.print(m_SdInfo.apikey);
                                myClient.println("'");
                                myClient.print("C='");
                                myClient.print(m_SdInfo.cloudip);
                                myClient.println("'");
                                myClient.print("D='");
                                myClient.print(m_SdInfo.cloudport);
                                myClient.println("'");
                                myClient.print("E='");
                                myClient.print(m_SdInfo.ssid);
                                myClient.println("'");
                                myClient.print("F='");
                                myClient.print(m_SdInfo.password);
                                myClient.println("'");       

                                myClient.println("function id(d){return document.getElementById(d);};");
                                myClient.println("var indextxt = id('indextxt'), keytxt = id('keytxt'), iptxt = id('iptxt'), porttxt = id('porttxt'), sendcmd = id('sendcmd'),");
                                myClient.println(" msg = id('msg'), ssidtxt = id('ssidtxt'), pwdtxt = id('pwdtxt');");
                                myClient.println("var msg1 = id('msg1');");
                                myClient.println("var msg2 = id('msg2');");
                                myClient.println("var msg3 = id('msg3');");
                                myClient.println("var msg4 = id('msg4');");
                                myClient.println("var msg5 = id('msg5');");

                                myClient.println("indextxt.value = A;");
                                myClient.println("keytxt.value = B;");
                                myClient.println("iptxt.value = C;");
                                myClient.println("porttxt.value = D;");
                                myClient.println("ssidtxt.value = E;");
                                myClient.println("pwdtxt.value = F;");
                                myClient.println("var show = id('show');var show2 = id('show2');var modify = id('modify');show2.style.display='none';");
                                myClient.println("var access = id('access');var accesswrap = id('accesswrap');var accessbtn = id('accessbtn');");
                                myClient.println("var readmsg = id('readmsg');var cancelmod = id('cancelmod');accesswrap.style.display = 'none';");
                                myClient.println("if(indextxt.value&&keytxt.value&&iptxt.value&&porttxt.value){console.log('有值');indextxt.disabled = true;console.log(indextxt);");
                                myClient.println("keytxt.disabled = true;iptxt.disabled = true;porttxt.disabled = true;modify.style.display = 'inline-block';}else{");
                                myClient.println("console.log('有值');indextxt.disabled=false;keytxt.disabled=false;iptxt.disabled=false;porttxt.disabled=false;modify.style.display = 'none';console.log(indextxt);}");

                                myClient.println("modify.onclick = function (){accesswrap.style.display = 'block';access.value = '';modify.style.display = 'none';}");

                                myClient.println("accessbtn.onclick = function (){if(access.value&&access.value=='jiantongkeji'){readmsg.innerHTML = '';accesswrap.style.display = 'none';");
                                myClient.println("indextxt.disabled=false;keytxt.disabled=false;iptxt.disabled=false;porttxt.disabled=false;");
                                myClient.println("}else if(access.value&&access.value!='jiantongkeji'){readmsg.innerHTML = '密码错误，重新输入';}else if(!access.value){readmsg.innerHTML = '';}}");
                                myClient.println("cancelmod.onclick = function (){accesswrap.style.display = 'none';readmsg.innerHTML = '';");
                                myClient.println("modify.style.display = 'inline-block';access.value = '';}");
                                myClient.println("show.onclick = function () {pwdtxt.type = 'text';show2.style.display='inline-block';show.style.display='none';}");
                                myClient.println("show2.onclick = function () {pwdtxt.type = 'password';show.style.display='inline-block';show2.style.display='none';}");
                                
                                myClient.println("indextxt.onblur= function(){var devindex = indextxt.value;var reg =  /^[0-9a-zA-Z]*$/g;var re = new RegExp(reg);");
                                myClient.println("if(!re.test(devindex)||devindex.length!=24){");
                                myClient.println("msg1.innerHTML = '* devindex 输入等于24位的小写字母和数字的组合';}else{msg1.innerHTML = '';};}");

                                myClient.println("keytxt.onblur= function(){");
                                myClient.println("var apikey = keytxt.value;var reg =  /^[0-9a-zA-Z]*$/g;var re = new RegExp(reg);");
                                myClient.println("if(apikey.length!=24||!re.test(apikey)){msg2.innerHTML = '* apiKey 输入等于24位的小写字母和数字的组合';");
                                myClient.println("}else{msg2.innerHTML = '';}}");

                                myClient.println("porttxt.onblur= function(){");
                                myClient.println("var port = porttxt.value;var reg = /[0-9]/g;var re = new RegExp(reg);");
                                myClient.println("if(port.length>5||!re.test(port)){msg3.innerHTML = '* Port 输入范围 0~65535';");
                                myClient.println("}else{msg3.innerHTML = ''; }}");
                                
                                myClient.println("ssidtxt.onblur= function(){");
                                myClient.println("var wifissid = ssidtxt.value;var reg = /^[0-9a-zA-Z]*$/g;  var re = new RegExp(reg);");
                                myClient.println("if(wifissid.length>15||!re.test(wifissid)){msg4.innerHTML = '* wifissid 输入长度不超过15位的字母、数字或下划线的组合';");
                                myClient.println("}else{msg4.innerHTML = '';}}");
                                
                                myClient.println("pwdtxt.onblur= function(){");
                                myClient.println("var wifipassword = pwdtxt.value;var reg = /^[0-9a-zA-Z]*$/g;var re = new RegExp(reg);");
                                myClient.println("if(wifipassword.length>15||!re.test(wifipassword)){msg5.innerHTML = '* wifipassword 输入长度不超过15位的字母、数字或下划线的组合';");
                                myClient.println("}else{msg5.innerHTML = '';}}");
                                myClient.println("sendcmd.onclick = function() {");
                                myClient.println("var devindex = indextxt.value, apikey = keytxt.value, cloudip = iptxt.value, port = porttxt.value, wifissid = ssidtxt.value, wifipassword = pwdtxt.value;");
                               
                                myClient.println(" url = '/A=' + devindex + '&B=' + apikey + '&C=' + cloudip + '&D=' + port + '&E=' + wifissid + '&F=' + wifipassword+'&\';console.log(url);");
                                myClient.println("if (!devindex || !apikey || !cloudip || !port||!wifissid) {msg.innerHTML='请把信息补充完整'} else {msg.innerHTML='';location.href=url}};};");
                                myClient.println("</script>");
                            }

                            myClient.println("</body>");
                            //user's code  ---end---    author:Wei Yongshu & Yang Lili  @20170301


                            // The HTTP response ends with another blank line:
                            myClient.println();
                            // break out of the while loop:
                            IsLoop = 1;
                            break;
                        }
                        else
                        {  
                            // if you got a newline, then clear the buffer:
                            memset(buffer, 0, 200);
                            i = 0;
                        }
                    }
                    else if (c != '\r')
                    {    // if you got anything else but a carriage return character,
                        buffer[i++] = c;      // add it to the end of the currentLine
                    }
                 
                    //Serial.println(buffer);  //调试用
                    String text = buffer;

                    //解析收到的数据
                    if(text.endsWith("HTTP/1.")){
                        if(text.startsWith("GET /A")){ 

                            //摘取devindex的参数
                            memset(m_SdInfo.sdindex, 0 ,sizeof(m_SdInfo.sdindex));
                            char *pFrom_sdindex = strstr(buffer, "=");
                            char *pEnd_sdindex = strstr(buffer, "&");
                            strncpy(m_SdInfo.sdindex, pFrom_sdindex + 1, (uint8_t)(pEnd_sdindex - (pFrom_sdindex + 1)));
                            Serial.println(m_SdInfo.sdindex);

                            //摘取apikey的参数
                            memset(m_SdInfo.apikey, 0 ,sizeof(m_SdInfo.apikey));
                            char *pFrom_apikey = strstr(pFrom_sdindex + 1, "=");
                            char *pEnd_apikey = strstr(pEnd_sdindex + 1, "&");
                            strncpy(m_SdInfo.apikey, pFrom_apikey + 1, (uint8_t)(pEnd_apikey - (pFrom_apikey + 1)));
                            Serial.println(m_SdInfo.apikey);

                            //摘取cloudip的参数
                            memset(m_SdInfo.cloudip, 0 ,sizeof(m_SdInfo.cloudip));
                            char *pFrom_cloudip = strstr(pFrom_apikey + 1, "=");
                            char *pEnd_cloudip = strstr(pEnd_apikey + 1, "&");
                            strncpy(m_SdInfo.cloudip, pFrom_cloudip + 1, (uint8_t)(pEnd_cloudip - (pFrom_cloudip + 1)));
                            Serial.println(m_SdInfo.cloudip);

                            //摘取port的参数
                            memset(m_SdInfo.cloudport, 0 ,sizeof(m_SdInfo.cloudport));
                            char *pFrom_cloudport = strstr(pFrom_cloudip + 1, "=");
                            char *pEnd_cloudport = strstr(pEnd_cloudip + 1, "&");
                            strncpy(m_SdInfo.cloudport, pFrom_cloudport + 1, (uint8_t)(pEnd_cloudport - (pFrom_cloudport + 1)));
                            Serial.println(m_SdInfo.cloudport);

                            //摘取wifi_ssid的参数
                            memset(m_SdInfo.ssid, 0 ,sizeof(m_SdInfo.ssid));
                            char *pFrom_ssid = strstr(pFrom_cloudport + 1, "=");
                            char *pEnd_ssid = strstr(pEnd_cloudport + 1, "&");
                            strncpy(m_SdInfo.ssid, pFrom_ssid + 1, (uint8_t)(pEnd_ssid - (pFrom_ssid + 1)));
                            Serial.println(m_SdInfo.ssid);

                            //摘取wifi_password的参数
                            memset(m_SdInfo.password, 0 ,sizeof(m_SdInfo.password));
                            char *pFrom_password = strstr(pFrom_ssid + 1, "=");
                            char *pEnd_password = strstr(pEnd_ssid + 1, "&");
                            strncpy(m_SdInfo.password, pFrom_password + 1, (uint8_t)(pEnd_password - (pFrom_password + 1)));
                            Serial.println(m_SdInfo.password);

                            char sdParameters[200] = {"\0"};
                            sprintf(sdParameters, "%s/%s/%s/%s/%s/%s/", m_SdInfo.sdindex, m_SdInfo.apikey, m_SdInfo.cloudip, m_SdInfo.cloudport, m_SdInfo.ssid, m_SdInfo.password);
                        
                            SPIFFS.remove("/devcfg/parameters");  //删掉原有的文件
                            File temp = SPIFFS.open("/devcfg/parameters", "w");
							temp.write((uint8_t*)sdParameters, sizeof(sdParameters));
							temp.close();

                            if(SPIFFS.exists("/devcfg/parameters") == true){
    							Serial.println("the file has been created");
    							resetflag = 1;
  							}else {
  								Serial.println("the file does not created");
  								resetflag = 3;
  							}
                        }
                        if(text.startsWith("GET /abandon")){
                            Serial.println("restart the device");
                            resetflag = 2;
                        }
                    }
                }
            }
        

            
            // close the connection:
            myClient.stop();
            Serial.println(". Client disconnected from server");
            Serial.println();            
        } 

        if(resetflag != 0){
            delay(1000);
            ESP.restart();
        }          
    } 
}

void qiaqiaSingDevESP8266::sdUpdateParametersDecode(byte *payload, unsigned int length){
	SD_DBG_ln("enter sdUpdateParametersDecode");


	DynamicJsonBuffer jsonBufferRead;
	JsonObject& root = jsonBufferRead.parseObject((char *)payload);

	if(!root.success()) {
		SD_DBG("Json format wrong");
	}

	const char *date_value = root["data"]["value"];  //提取完整的升级路径
	const char *date_host = root["data"]["host"];
	const char *date_port = root["data"]["port"];
	const char *date_path = root["data"]["path"]; 
	
	memset(&m_sdUpdateParameters, 0, sizeof(m_sdUpdateParameters));

	SD_DBG("update path [");
  	SD_DBG(date_value);
  	SD_DBG_ln("] ");

  	strncpy(m_sdUpdateParameters.updateURL, (char*)date_value, strlen(date_value));
	strncpy(m_sdUpdateParameters.updateIP, (char*)date_host, strlen(date_host));
	strncpy(m_sdUpdateParameters.updatePort, (char*)date_port, strlen(date_port));
	strncpy(m_sdUpdateParameters.updatePath, (char*)date_path, strlen(date_path));
}