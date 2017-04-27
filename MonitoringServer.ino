/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* AJAX web page code mostly copied from Peter Katz 
  *  http://www.esp8266.com/viewtopic.php?f=8&t=4307
  *  Thanks a lot, Peter!
  */
 
/* Create a WiFi access point and provide a web server on it. */

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <Adafruit_INA219.h>

/* Set these to your desired credentials. */
const char *ssid = "WemosData";
const char *password = "no";

uint32_t currentFrequency;

float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float current_A = 0;
float loadvoltage = 0;
float voltage = 0;
  
int sensorValue = 0;

String webSite,javaScript,css,csc,XML,volttext,curtext;

ESP8266WebServer server(80);

Adafruit_INA219 ina219;

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void handleRoot() {
	server.send(200, "text/html", "<h1>You are connected</h1>");
}

void buildWebsite(){
  buildJavascript();
  buildCSS();
  webSite="<!DOCTYPE HTML>\n";
  webSite+="<head><meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  webSite+=javaScript;
  webSite+=css;
  webSite+="</head><body onload='process()'>\n";
  webSite+="<div class='top'><div><a id='volt-all'></a> V</div><div><a id='cur-all'></a> A</div></div>";
  webSite+="<div class='bat'><div class='ok'>1,53</div><div>1,53</div><div>3,53</div><div>4,53</div><div>5,53</div></div>";
  webSite+="<div class='bat'><div class='ok'>1,53</div><div>1,53</div><div>3,53</div><div>4,53</div><div>5,53</div></div>";
  webSite+="</body></html>\n";
}

void buildCSS(){
  css="<style>"; 
  css+="* {box-sizing: content-box;font: bold 100% /1.2 Arial;}";
  css+=".top { display: flex; width: 98vw; text-align: center; }"; 
  css+=".top div { font-size: 11vw; flex: 1; margin: 1vw; padding: 1vw; background: #eee; }";
  css+=".bat { display: flex; width: 97vw; margin: 2vw 0.5vw; position: relative; counter-increment: bat; }";
  css+=".bat:before { content: 'Akku ' counter(bat); position: absolute; top: -1em; left: 0.3em; font-size: 2vw; }"; 
  css+=".bat * { text-align: center; font-size: 6vw; padding: 0.5vw 1vw; margin: 0.5vw; flex: 1; background: #eee; }";
  css+="div.ok { background: #6d2; } div.low { background: #fc0; } div.crit { background: #b00; color: #fff; }";
  css+="</style>\n"; 
}

void buildJavascript(){
  javaScript="<script>\n";
  javaScript+="var xmlHttp=createXmlHttpObject();\n";

  javaScript+="function createXmlHttpObject(){\n";
  javaScript+=" if(window.XMLHttpRequest){\n";
  javaScript+="    xmlHttp=new XMLHttpRequest();\n";
  javaScript+=" }else{\n";
  javaScript+="    xmlHttp=new ActiveXObject('Microsoft.XMLHTTP');\n";
  javaScript+=" }\n";
  javaScript+=" return xmlHttp;\n";
  javaScript+="}\n";

  javaScript+="function process(){\n";
  javaScript+=" if(xmlHttp.readyState==0 || xmlHttp.readyState==4){\n";
  javaScript+="   xmlHttp.open('PUT','xml',true);\n";
  javaScript+="   xmlHttp.onreadystatechange=handleServerResponse;\n"; // no brackets?????
  javaScript+="   xmlHttp.send(null);\n";
  javaScript+=" }\n";
  javaScript+=" setTimeout('process()',1000);\n";
  javaScript+="}\n";
  javaScript+="function handleServerResponse(){\n";
  javaScript+=" if(xmlHttp.readyState==4 && xmlHttp.status==200){\n";
  javaScript+="   xmlResponse=xmlHttp.responseXML;\n";
  javaScript+="   xmldocV = xmlResponse.getElementsByTagName('volt');\n";
  javaScript+="   messageV = xmldocV[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('volt-all').innerHTML=messageV;\n";
  javaScript+="   xmldocA = xmlResponse.getElementsByTagName('current');\n";
  javaScript+="   messageA = xmldocA[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('cur-all').innerHTML=messageA;\n";
  javaScript+=" }\n";
  javaScript+="   volt = parseFloat(document.getElementById('volt-all').innerHTML);\n";
  javaScript+="   if (volt > 4) { document.getElementById('volt-all').parentNode.className = 'ok'; }\n";
  javaScript+="     else if (volt < 3.5) { document.getElementById('volt-all').parentNode.className = 'crit'; }\n";
  javaScript+="     else if (volt < 3.7) { document.getElementById('volt-all').parentNode.className = 'low'; }\n";
  javaScript+="     else { document.getElementById('volt-all').parentNode.className = ''; }\n";
  javaScript+="}\n";
  javaScript+="</script>\n";
}

void buildXML(){
  
  volttext = String(voltage);
  curtext = String(current_A);
  
  XML="<?xml version='1.0'?>";
  XML+="<values>";
  XML+="<volt>";
  XML+=volttext;
  XML+="</volt>\n";
  XML+="<voltstat>";
  XML+=csc;
  XML+="</voltstat>\n";
  XML+="<current>";
  XML+=curtext;
  XML+="</current>";
  XML+="</values>";
}

void handleWebsite(){
  buildWebsite();
  server.send(200,"text/html",webSite);
}
void handleXML(){
  buildXML();
  server.send(200,"text/xml",XML);
}


void setup() {
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print("Configuring access point...");
	/* You can remove the password parameter if you want the AP to be open. */
	WiFi.softAP(ssid);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.on("/", handleRoot);
  server.on("/ajax",handleWebsite);
  server.on("/xml",handleXML);
	server.begin();
	Serial.println("HTTP server started");

  Serial.println("Measuring voltage and current with INA219 ...");
  ina219.begin();
}

void loop() {

  // reads voltage from A0 und builds xml from data
  sensorValue = analogRead(A0);

  // Kalkuliert fuer 1M R als Spannungsteiler
  voltage = sensorValue / 71.32;
  
  if (voltage > 4) {
   csc = "ok";
  }
  else if (voltage < 3.5) {
    csc = "low";
  }
  else if (voltage < 3.3) {
    csc = "crit";
  }
  else {
    csc="";
  }
  
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  current_A = current_mA/1000;
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.print("Current:       "); Serial.print(current_A); Serial.println(" A");
  Serial.print("Voltage:       ");  Serial.print(voltage); Serial.println(" V");
  Serial.print(csc);
  Serial.println("");

  delay(2000);
   
	server.handleClient();
  
}
