#include <FlowMeter.h>  // https://github.com/sekdiy/FlowMeter
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// enter your own sensor properties here, including calibration points
FlowSensorProperties MySensor = {1.67f, 8.3944f, {1, 1, 1, 0.92, 1.08, 1, 1.08, 1, 1, 1.04}};
FlowMeter Meter = FlowMeter(4, MySensor);


// wifi details
const char* ssid = "Dialog WIFI";
const char* password = "YT43LNAH7NR";

//Height by cm
long height = 18;

//Tank volume by litters
double tankVolume = 6;

//Automated motor
int relaySwitch = 13;// D7

//ultrasonic sensor
#define trigPin 4 //D1
#define echoPin 5 //D2

// timekeeping variables
long period = 10000;   // fifteen seconds (in milliseconds)
long lastTime = 0;
double lastVolume = 0.0f;

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
  attachInterrupt(digitalPinToInterrupt(D4), MeterISR, RISING);//D2 toD4

  // sometimes initializing the gear generates some pulses that we should ignore
  Meter.reset();
}

void loop() {
      
  // do some timekeeping
  long currentTime = millis();
//  Serial.print("Current time: ");
//  Serial.println(currentTime);
  long duration = currentTime - lastTime;
  //Serial.println(duration);
  // wait between display updates
  if (duration >= period) {

    // process the counted ticks
    Meter.tick(duration);

    // output some measurement result
    Serial.println("FlowMeter - current flow rate: " + String(Meter.getCurrentFlowrate()) + " l/min, nominal volume: " + String(Meter.getTotalVolume()) + " l");
    double cm = getHeight();

    double currentVolume = Meter.getTotalVolume() - lastVolume;
    double percentageByVolume = getPercentageByVolume(currentVolume,tankVolume);
    double percentageByHeight = getPercentageByHeight(cm,height);
    checkMotorByHeight(percentageByHeight);
    //checkMotorByVolume(percentageByVolume);
    
    // prepare for next cycle
    sendData(cm);
    
    lastTime = currentTime;
  }
}

void sendData(double cm) {
  
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

double getHeight(){
  
  long duration, cm, inches;
     
  // Clears the trigPin
    digitalWrite(trigPin, LOW);  
    delayMicroseconds(2); 
    
  // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
  // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    
  // Calculating the distance
    cm = (duration/2) / 29.1;
    inches = (duration/2) / 74; 
    Serial.print(inches);
    Serial.print(" inches\n");
    Serial.print(cm);
    Serial.print(" cm ");
    Serial.println();
  // Prints the distance on the Serial Monitor
//    Serial.print(inches + "in, ");
//    Serial.print(cm + "cm\n");
    
    return (double) cm;  
 }

//Get the volume by percentage
double getPercentageByVolume(double currentVolume,double tankVolume){
  //Serial.println("Price ");
  //Serial.println(currentVolume);
  //Serial.println(tankVolume);
  double percentageByVolume = ((tankVolume-currentVolume)/tankVolume)*100;
  
  return percentageByVolume;
} 

//Get the height by percentage
double getPercentageByHeight(double currentHeight,double tankHeight){
  
  //Serial.println(currentHeight);
  //Serial.println(tankHeight);
  double percentageByHeight = ((tankHeight-currentHeight)/tankHeight)*100;
  Serial.print("Percentage ");
  Serial.println(percentageByHeight);
  return percentageByHeight;
}

void checkMotorByHeight(double percentageByHeight){
  Serial.println(percentageByHeight);
  
  if(percentageByHeight < 30){
    
    digitalWrite(relaySwitch, LOW);
    //Serial.print(percentageByHeight);
    Serial.println("Motor is ON Height!");
  }else{
    
    digitalWrite(relaySwitch, HIGH);
       // Serial.print(percentageByHeight);
    Serial.println("Motor is OFF Height!");
  }
}

void checkMotorByVolume(double percentageByVolume){
    Serial.println(percentageByVolume);
    if(percentageByVolume < 30){
      
    digitalWrite(relaySwitch, LOW);
        //Serial.print(percentageByVolume);
    Serial.println("Motor is ON!");    
  }else{
    
    digitalWrite(relaySwitch, HIGH);
            //Serial.print(percentageByVolume);
    Serial.println("Motor is OFF!");
  }
}

