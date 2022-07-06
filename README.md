# Sun_clock

![GitHub Workflow Status](https://img.shields.io/github/workflow/status/InzynierDomu/sun_clock/CI?logo=github&style=flat-square)
![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/InzynierDomu/sun_clock?style=flat-square)
<a href="https://inzynierdomu.github.io/sun_clock/">![GitHub docs deployments](https://img.shields.io/github/deployments/InzynierDomu/sun_clock/github-pages?label=docs&logo=BookStack&logoColor=white&style=flat-square)</a>
<a href="https://discord.gg/KmW6mHdg">![Discord](https://img.shields.io/discord/815929748882587688?logo=discord&logoColor=green&style=flat-square)</a>
![GitHub](https://img.shields.io/github/license/InzynierDomu/sun_clock?style=flat-square)
<a href="https://tipo.live/p/inzynierdomu">![support](https://img.shields.io/badge/support-tipo.live-yellow?style=flat-square)</a>

  - [About](#about)
  - [Scheme](#scheme)
  - [IDE](#ide)

## About

Clock tracking the position and color of the sun in the sky regarding date and location. Sun (RGB LED) changes position during the day using a servo. Sun and sky (WS2812) change color during the day.

To set clock, clean projet and upload. Actual date and time uploaded from PC on building and uploading. 

To set location, change latitude nad longitude in Config.h.

## Scheme

![schem](https://github.com/InzynierDomu/sun_clock/blob/main/schem.png)

Part list:
- Arduino nano
- Servo min. 180' 
- RTC DS1307
- LED RGB common anode
- WS2812 10 pcs
- Oscillator 32,788 KHz
- Battery CR2025 3,3V with batbox
- R 220 ohm 3 pcs
- R 470 ohm 1 pcs
- R 4,7k ohm 2 pcs

## IDE

The project is prepared for the Platform IO environment. A video on how to install such an environment can be watched on this [video](https://youtu.be/Em9NuebT2Kc).
<br><br>
Formatting is done using clang-format. The description of the tool configuration is in the [video](https://youtu.be/xxuaOG0WjIE).
<br><br>
The code contains a comment prepared for doxygen, their use is described in the [video](https://youtu.be/1YKJtrCsPD4).
