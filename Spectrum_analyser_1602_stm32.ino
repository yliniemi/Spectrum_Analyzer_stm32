    // Include the libraries:
    // LiquidCrystal_I2C.h: https://github.com/johnrickman/LiquidCrystal_I2C
    #include <Wire.h> // Library for I2C communication
    #include <LiquidCrystal_I2C.h> // Library for LCD
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

//    #define ARM_MATH_CM3
//    #include <arm_math.h>
    
    #include <arduinoFFTfloat.h>
//    #include <cr4_fft_stm32.h>

    const long number_of_samples = 1024;
    float real_sample[number_of_samples];
    float imaginary_sample[number_of_samples];
    long offset = 3000;
    
    arduinoFFT FFT = arduinoFFT();
    
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
      //Serial.begin(115200);
      lcd.init();
      lcd.backlight();
      animationSetup();
      //analogReadResolution(12); // arduino stm32 library doesn't know this function
      pinMode(PA0, INPUT_ANALOG);
      delay(10000);
      Serial.println(666);
    }
    
    /*
    void animation3() {
      byte character[2];
      static int animationStep = 0;
      for (int column = 0; column < numberOfColumns; column++) {
        int actualValue=cos(animationStep*(0.1+column*0.01))*8.5+9;
        if (actualValue == 17) {
          character[0]=byte(255);
          character[1]=byte(255);
        }
        else if (actualValue > 9) {
          character[0]=byte(actualValue-9);
          character[1]=byte(255);
        }
        else if (actualValue == 9) {
          character[0]=byte(32);
          character[1]=byte(255);
        }
        else if (actualValue == 8) {
          character[0]=byte(32);
          character[1]=byte(255);
        }
        else if (actualValue == 0) {
          character[0]=byte(32);
          character[1]=byte(32);
        }      
        else {
          character[0]=byte(32);
          character[1]=byte(actualValue-1);
        }
        lcd.setCursor(15-column,0);
        lcd.write(character[0]);
        lcd.setCursor(15-column,1);
        lcd.write(character[1]);
        }
      // lcd.write(byte(animationStep % 8));
      // lcd.write(7);
      animationStep++;
    }
    */
    
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
    
    void loop() {
      
      static long test_step = -10000;
      static long previous_millis = 0;
      static float spectrum[16];
      static long counter=0;
      static long offset = 0;
      static long new_offset = 2000*100000;
      static float envelope_min = 100000;
      static float envelope_max = -100000;
      long reading_min = 9000;
      long reading_max = -9000;
      

      offset = new_offset / 100000;
      long previous_time = millis();
      long useless_variable = analogRead(PA0);  // i'll get rid of the 

      for (int i=0; i < number_of_samples; i++)
      {
        long previous_micros = micros();
        long reading = analogRead(PA0);
        real_sample[i] = reading - offset;
        imaginary_sample[i] = 0;
        new_offset = new_offset*0.99999 + reading;
        
        
        if (reading < reading_min) reading_min = reading;
        if (reading > reading_max) reading_max = reading;
        
        int micro_delay = previous_micros + 21 - micros();
        if (micro_delay > 0 && micro_delay < 22) delayMicroseconds(micro_delay);
      }
      
      double sampling_time = millis()-previous_time;

      Serial.print("sampling time: ");
      Serial.println(sampling_time);
      Serial.print("Min: ");
      Serial.println(reading_min);
      Serial.print("Max: ");
      Serial.println(reading_max);
      
      FFT.Windowing(real_sample, number_of_samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
      FFT.Compute(real_sample, imaginary_sample, number_of_samples, FFT_FORWARD);
      FFT.ComplexToMagnitude(real_sample, imaginary_sample, number_of_samples);
      
      double spectrum_min=100000;
      double spectrum_max=-100000;
      
      /*
      real_sample[0] = 100;
      real_sample[1] = 100;
      real_sample[2] = 100;
      real_sample[3] = 100;
      for (int i = 0; i < 16; i++)
      {
        double average = 0;
        for (int j = 0; j < number_of_samples/2/16; j++)
        {
          average = average + real_sample[i*number_of_samples/2/16 + j];
        }
        double log_average = log(average);
        if (log_average < spectrum_min) spectrum_min = log_average;
        if (log_average > spectrum_max) spectrum_max = log_average;
        spectrum[i] = log_average;
        Serial.println(spectrum[i]);
      }
      
      */
      long starting_point = number_of_samples/2;
      for (int i = 15; i > -1; i--)
      {
        double average = 0;
        // for 256 samples 0.28 shows all frequencies, 0.26 shows the sane ones
        // for 512 samples 0.29 shows all frequencies, 0.28 shows the sane ones
        // for 512 samples 0.35 shows all frequencies, 0.31 shows the sane ones
        long delta = starting_point * 0.31;
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
        Serial.println(starting_point);
        double log_average = log(average);
        if (log_average < spectrum_min) spectrum_min = log_average;
        if (log_average > spectrum_max) spectrum_max = log_average;
        spectrum[i] = log_average;
      }
      
      envelope_min += 0.1;
      if (envelope_min > spectrum_min) envelope_min = spectrum_min;
      
      envelope_max -= 0.1;
      if (envelope_max < spectrum_max) envelope_max = spectrum_max;
      
      Serial.print("Envelope min: ");
      Serial.println(envelope_min);
      Serial.print("Envelope max: ");
      Serial.println(envelope_max);
      
      for (int i=0; i < 16; i++)
      {
        spectrum[i] = (spectrum[i] - envelope_min) * (17 - 0) / (envelope_max - envelope_min) + 0;
      }
      
      Serial.print("Offset: ");
      Serial.println(offset);
      
      draw_spectrum(spectrum);
      
      
      Serial.print("frame time: ");
      Serial.println(millis()-previous_time);
      
      /*
      if (millis() < previous_millis) previous_millis = millis();
      if (millis() > previous_millis + 100)
      {
        for (int i=0; i < 16; i++)
        {
          spectrum[i] = cos(test_step*(0.1+i*0.01))*8.5+9;
        }
        draw_spectrum(spectrum);
        test_step++;
        //Serial.println(counter);
        counter=0;
        previous_millis = millis();
      }
      */
      
      counter++;
    }
