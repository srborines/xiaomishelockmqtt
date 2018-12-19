#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

//WiFi setup
const char* ssid = "<YOUR SSID>";
const char* password = "<YOUR WiFi PASSWORD>";
//MQTT setup
const char* mqtt_server = "<YOUR HASSIO IP OR YOUR MQTT SERVER IP>";
const char* mqtt_id = "<THE ID THAT YOU PREFER>";
const char* mqtt_user = "<YOUR MQTT USER>";
const char* mqtt_password = "<YOUR MQTT PASSWORD>";
//Pinout setup
const int lock_pin = 2;
const int pushButton = 0;

WiFiClient espClient;
PubSubClient client(espClient);
bool status_opened = false;


void setup() {
  //Start Wifi
  setup_wifi();
  //Start mqtt
  client.setServer(mqtt_server, 1883);
  //Set mqtt callback
  client.setCallback(callback);
  Serial.begin(115200);

  pinMode(pushButton, INPUT);
  pinMode(lock_pin, OUTPUT);
  digitalWrite(lock_pin, HIGH);
}

/**
  Initialize WiFi parameters and try to connect
*/
void setup_wifi() {
  //Wait for boot
  delay(10);
  //Start WiFi connection
  WiFi.begin(ssid, password);

  //Wait for connection success
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  randomSeed(micros());
  
  Serial.println("WiFi connected");
}

/**
  Receive MQTT message
  
  @param topic path from mqtt.
  @param payload Content of the message.
  @param lenght lenght of the payload.
*/
void callback(char* topic, byte* payload, unsigned int length) {
  //If it's the right topic process it
  if (strcmp(topic,"door/action")==0){
    door_action((char)payload[0]);
  }
}

/**
  Connect for the first time or reconnect to the mqtt server
*/
void reconnect() {
  // Loop until MQTT client is reconnected
  while (!client.connected()) {
    // Attempt to connect to MQTT server
    if (client.connect(mqtt_id,mqtt_user,mqtt_password)) {
      //Publish to notify that door is available
      client.publish("door/available", "online");
      //Subscribe to receive the action
      client.subscribe("door/action");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  read_real_switch();
}

/**
  Execute an action over the door lock.
  
  @param action Depending of this param one action or other will be executed. These are the possible values:
    0: Close the door
    1: Open the door
    2: Change the current status
*/
void door_action(char action){
  if(action == '0'){
    close_door();
  }else if(action == '1'){
    open_door();
  }else if(action == '2'){
    status_opened ? close_door() : open_door();
  }
}

/**
  Send a down pulse to 'lock_pin' during 50 miliseconds
*/
void open_door(){
  digitalWrite(lock_pin, LOW);
  delay(50);
  digitalWrite(lock_pin, HIGH);
  delay(3000);
  status_opened = true;
  client.publish("door/status", "1");
}

/**
  Send a down pulse to 'lock_pin' during 15 miliseconds
*/
void close_door(){
  digitalWrite(lock_pin, LOW);
  delay(15);
  digitalWrite(lock_pin, HIGH);
  delay(3000);
  status_opened = false;
  client.publish("door/status", "0");
}

void read_real_switch(){
  int buttonState = digitalRead(pushButton);
  if(buttonState == 0){
    delay(30);
    buttonState = digitalRead(pushButton);
    if(buttonState == 1){
     open_door();
    }else{
      close_door();
    }
    delay(30);
  }
}
