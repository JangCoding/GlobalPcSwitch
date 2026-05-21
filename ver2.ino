#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

const char* ap_ssid = "Universal_Switch_Setup";
const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);
const int RELAY_PIN = D2; // 청정 핀 D2 할당

// 고유 UUID 생성 (기기 식별자)
String getDeviceUUID() {
  return "orbital-" + String(ESP.getChipId(), HEX) + String(ESP.getFlashChipId(), HEX);
}

// 캡티브 포털 UI (아이폰 최적화 적용)
void handleSetupPage() {
  int n = WiFi.scanNetworks();
  String currentUUID = getDeviceUUID();
  
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>";
  html += "body{background:#121212; color:#fff; font-family:sans-serif; text-align:center; padding:20px;}";
  html += ".container{max-width:90%; width:400px; margin:0 auto; background:#1e1e1e; padding:25px; border-radius:20px;}";
  html += ".btn{display:block; width:100%; padding:15px; margin:10px 0; font-size:16px; border:none; border-radius:12px; cursor:pointer; color:white; background:linear-gradient(135deg, #00e676, #00b0ff);}";
  html += ".hidden{display:none;}";
  html += "</style></head><body>";

  // 1. 초기화면: [등록하기]
  html += "<div id='home-ui' class='container'><h2>Universal Switch</h2>";
  html += "<button class='btn' onclick='showSetup()'>등록하기</button></div>";

  // 2. Wi-Fi 설정화면
  html += "<div id='setup-ui' class='container hidden'><h3>Wi-Fi 설정</h3><select id='ssid'>";
  for (int i = 0; i < n; ++i) html += "<option>" + WiFi.SSID(i) + "</option>";
  html += "</select><br><input type='password' id='pass' placeholder='비번' style='width:100%; padding:10px; margin:10px 0;'><br>";
  html += "<button class='btn' onclick='verify()'>연결 및 복사</button></div>";

  // 3. 연결 성공 화면
  html += "<div id='success-ui' class='container hidden'><h3>🟢 연결 성공!</h3>";
  html += "<p>UUID가 복사되었습니다. [연결하기]를 누르세요.</p>";
  html += "<button class='btn' onclick='finalRedirect()'>연결하기</button></div>";

  html += "<script>var device_uuid='" + currentUUID + "';";
  html += "function showSetup(){ document.getElementById('home-ui').classList.add('hidden'); document.getElementById('setup-ui').classList.remove('hidden'); }";
  html += "function verify(){ var s=document.getElementById('ssid').value; var p=document.getElementById('pass').value;";
  html += "fetch('/check_wifi',{method:'POST',body:'ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)}).then(res=>{";
  html += "if(res.ok){ navigator.clipboard.writeText(device_uuid); document.getElementById('setup-ui').classList.add('hidden'); document.getElementById('success-ui').classList.remove('hidden'); }else{ alert('실패!'); }});}";
  html += "function finalRedirect(){ window.location.href='https://jangcoding.github.io/GlobalPcSwitch/'; }</script></body></html>";
  
  server.send(200, "text/html", html);
}

void handleCheckWifi() {
  WiFi.begin(server.arg("ssid").c_str(), server.arg("pass").c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 25) { delay(500); attempts++; }
  if (WiFi.status() == WL_CONNECTED) { server.send(200, "text/plain", "SUCCESS"); delay(1000); ESP.restart(); }
  else { WiFi.disconnect(); server.send(400, "text/plain", "FAIL"); }
}

void handleNotFound() { server.sendHeader("Location", "http://192.168.4.1/", true); server.send(302, "text/plain", ""); }

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // 평상시 LOW
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid);
  dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
  server.on("/", handleSetupPage);
  server.on("/check_wifi", handleCheckWifi);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() { dnsServer.processNextRequest(); server.handleClient(); }