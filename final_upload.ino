#include <FlowMeter.h>  // https://github.com/sekdiy/FlowMeter
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// enter your own sensor properties here, including calibration points
FlowSensorProperties MySensor = {1.67f, 8.3944f, {1, 1, 1, 0.92, 1.08, 1, 1.08, 1, 1, 1.04}};
FlowMeter Meter = FlowMeter(2, MySensor);


// wifi details
const char* ssid = "jayasanka";
const char* password = "jayseanjaysean";

//Height by cm
long height = 13;


//Automated motor
int relaySwitch = 16;// D0

//ultrasonic sensor
#define trigPin 14 //D5
#define echoPin 12 //D6

// timekeeping variables
long period = 10000;   // fifteen seconds (in milliseconds)
long lastTime = 0;
double lastVolume = 0.0f;

long mPeriod = 2000;
long mLastTime = 0;

// define an 'interrupt service routine' (ISR)
void MeterISR() {
  
  // let our flow meter count the pulses
  Meter.count();
}

void setup() {
  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(relaySwitch, OUTPUT); // Sets the relaySwitch as an Output
  //digitalWrite(relaySwitch, HIGH);
  // prepare serial communication
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.print("Connecting..");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // enable a call to the 'interrupt service handler' (ISR) on every rising edge at the interrupt pin
  attachInterrupt(digitalPinToInterrupt(D2), MeterISR, RISING);//D2 

  // sometimes initializing the gear generates some pulses that we should ignore
  Meter.reset();
}

void loop() {
      
  // do some timekeeping
  long currentTime = millis();
  long duration = currentTime - lastTime;

  long mDuration = currentTime - mLastTime;

//  if(mDuration >= mPeriod){
//      checkMotor();
//    }
  
  if (duration >= period) {
    
    Meter.tick(duration);
   
    Serial.println("FlowMeter - current flow rate: " + String(String(Meter.getTotalVolume()) + " l"));
    int cm = getHeight();
    
    sendData(cm);
    
    lastTime = currentTime;
  }
}

void sendData(int cm) {
  
  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("Wifi is connected.");
    
    double currentVolume = Meter.getTotalVolume() - lastVolume;
    HTTPClient http;
    
    String data = "http://139.59.81.23/apis/droplet/api/v1/update/1?usage=";
        
    data += currentVolume;
    data += "&rHeight=";
    data += cm;
    
    http.begin(data);
    
    int httpCode = http.GET();
    
    //Serial.println(httpCode);
    Serial.println(data+"\n");
    
    if (httpCode > 0) {
      String payload = http.getString();
      //Serial.println(payload);
      lastVolume = Meter.getTotalVolume();
    }
    
    http.end();
  }else{
    Serial.println("Wifi isn't connected.");
    
  }
}

int getHeight(){
  
    long duration, cm;
      
    digitalWrite(trigPin, LOW);  
    delayMicroseconds(2); 
    
  
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH);
    
  
    cm = (duration/2) / 29.1;
   
    return (int) cm;  
 }



void checkMotor(){

  double percentage = (height-getHeight()-5)/height*100;
  if(percentage < 20){    
    digitalWrite(relaySwitch, HIGH);
  }else if(percentage>90){
    digitalWrite(relaySwitch, LOW);
  }
}
