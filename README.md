# Spectrum_Analyzer_stm32
Here are links to my YouTube-videos of this project:
https://www.youtube.com/watch?v=VyRllcX4HxQ

I made a little spectrum analyzer that takes line in signal and shows you what frequencies is consists of. The screen is 1602 text lcd and the microcontroller is the trusty old Bluepill.

To run this little program you have to use Roger Clarks library. Atleast on the Bluepill. His Arduino_STM32 can do ADC fast enough unlike stm32duino. Here is a link to his library.
https://github.com/rogerclarkmelbourne/Arduino_STM32

You also need the development vesion of ArduinoFFT library. The original library uses doubles instead of floats which doubles the memory and cpu time requirements.
