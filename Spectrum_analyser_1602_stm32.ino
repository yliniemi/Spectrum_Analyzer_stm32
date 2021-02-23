// Include the libraries:
// LiquidCrystal_I2C.h: https://github.com/johnrickman/LiquidCrystal_I2C
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

//    I couldn't for the life of me to get faster fft working on stm32f103c8t6
//    I tried the hardware dsp and arm math

// #include "stm32f1xx_hal.h"  // ADC is too fucking slow
#include <arduinoFFT.h>

#define sampling_frequency 40000  // in Hz
#define number_of_samples 512  // 256 or 1024 is sane

/*
double real_sample[number_of_samples];
double imaginary_sample[number_of_samples];
ArduinoFFT<double> FFT = ArduinoFFT<double>(real_sample, imaginary_sample, number_of_samples, sampling_frequency);
*/

float real_sample[number_of_samples];
float imaginary_sample[number_of_samples];
ArduinoFFT<float> FFT = ArduinoFFT<float>(real_sample, imaginary_sample, number_of_samples, sampling_frequency);

float bin_ratio = 0.31;

// this creates the required characters
void animationSetup() {
  for (int i=0; i < 8; i++) {
    byte charLine[8];
    for (int j=0; j < 8; j++) {
      if (j >= i) charLine[7-j]=B00000;
      else charLine[7-j]=B11111;
    }
    lcd.createChar(i, charLine);
  }
}

void setup() {
  delay(10000);
  //Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  animationSetup();
  analogReadResolution(12); // arduino stm32 library doesn't know this functions
  pinMode(PA0, INPUT_ANALOG);
  Serial.println(666);

  if (number_of_samples == 256) bin_ratio = 0.26;
  if (number_of_samples == 512) bin_ratio = 0.28;
  if (number_of_samples == 1024) bin_ratio = 0.31;
}

void draw_spectrum(float spectrum[])
{
  static char line0[17];
  static char line1[17];
  for (int column = 0; column < 16; column++)
  {
    int int_spectra = spectrum[column];
    if (int_spectra > 16) {
      line0[column]=255;
      line1[column]=255;
    }
    else if (int_spectra > 9) {
      line0[column]=int_spectra-9;
      line1[column]=255;
    }
    else if (int_spectra == 9) {
      line0[column]=32;
      line1[column]=255;
    }
    else if (int_spectra == 8) {
      line0[column]=32;
      line1[column]=255;
    }
    else if (int_spectra < 1) {
      line0[column]=32;
      line1[column]=32;
    }      
    else {
      line0[column]=32;
      line1[column]=int_spectra;
    }
  }
  lcd.setCursor(0,0);
  lcd.print(line0);
  lcd.setCursor(0,1);
  lcd.print(line1);
}

void adc()
{
  uint32_t reading_min = 9000;
  uint32_t reading_max = 0;
  uint32_t previous_millis = millis();

  for (int i=0; i < number_of_samples; i++)
  {
    uint32_t previous_micros = micros();
    uint32_t reading = analogRead(PA0);
    real_sample[i] = analogRead(PA0);
    imaginary_sample[i] = 0;

    if (reading < reading_min) reading_min = reading;
    if (reading > reading_max) reading_max = reading;

    int micro_delay = previous_micros + 21 - micros();
    if (micro_delay > 0 && micro_delay < 22) delayMicroseconds(micro_delay);   // this condition here so we don't accidentally call a negative delay
  }
  Serial.print("Min: ");
  Serial.println(reading_min);
  Serial.print("Max: ");
  Serial.println(reading_max);
}

void loop()
{
  uint32_t previous_millis = 0;
  static float spectrum[16];
  static float envelope_min = 100000;
  static float envelope_max = -100000;
  uint32_t generic_timer = 0;
  
  uint32_t previous_time = millis();

  generic_timer = millis();
  adc();
  Serial.print("Sampling time: ");  // should be between 22 and 30 milliseconds
  Serial.println(millis() -  generic_timer);
  
  generic_timer = millis();
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  Serial.print("Windowing time: ");
  Serial.println(millis() -  generic_timer);

  generic_timer = millis();
  FFT.compute(FFTDirection::Forward);
  Serial.print("Compute time: ");
  Serial.println(millis() -  generic_timer);

  generic_timer = millis();
  FFT.complexToMagnitude();
  Serial.print("Magnitude time: ");
  Serial.println(millis() -  generic_timer);
  
  float spectrum_min=100000;
  float spectrum_max=-100000;


  long starting_point = number_of_samples/2;
  for (int i = 15; i > -1; i--)
  {
    double average = 0;
    // for 256 samples 0.28 shows all frequencies, 0.26 shows the sane ones
    // for 512 samples 0.29 shows all frequencies, 0.28 shows the sane ones
    // for 1024 samples 0.35 shows all frequencies, 0.31 shows the sane ones
    long delta = starting_point * bin_ratio;
    if (delta < 1)
    {
      delta = 1;
      if (starting_point < 1) starting_point = 1;  // 3 is good for 512 samples, 1 is good for 256 sambles. this line is here just to prevent charshing in case the numbers go too low
    }
    starting_point = starting_point - delta;
    for (long pointer = starting_point; pointer < starting_point + delta; pointer++)
    {
      average += real_sample[pointer]/delta;
    }
    // Serial.println(starting_point);  // useless debug information
    double log_average = log(average);
    if (log_average < spectrum_min) spectrum_min = log_average;
    if (log_average > spectrum_max) spectrum_max = log_average;
    spectrum[i] = log_average;
  }

  envelope_min += 0.01;
  if (envelope_min > spectrum_min) envelope_min = spectrum_min;

  envelope_max -= 0.01;
  if (envelope_max < spectrum_max) envelope_max = spectrum_max;

  Serial.print("Envelope min: ");
  Serial.println(envelope_min);
  Serial.print("Envelope max: ");
  Serial.println(envelope_max);

  for (int i=0; i < 16; i++)
  {
    spectrum[i] = (spectrum[i] - envelope_min) * (17 - 0) / (envelope_max - envelope_min) + 0;
  }

  generic_timer = millis();
  draw_spectrum(spectrum);
  Serial.print("Drawing time: ");
  Serial.println(millis() -  generic_timer);


  Serial.print("Frame time: ");
  Serial.println(millis()-previous_time);

  Serial.println();

}
