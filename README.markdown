# Description

This is a set of tools for interfacing with USB connected LCD screens using the ArtistaSDK. You can buy screens and obtain the SDK from http://www.datadisplay-group.com/, and possibly others. The main components are:

- artistactrl.cpp : A command line utility for querying and sending images to the screens.
- libartista.rb : A ruby wrapper for the artistactrl command line tool.
- screen_manager.rb : An example ruby program that uses libartista.rb to control a bank of USB connected screens.

# Requirements

- g++
- ArtistaSDK (c++)
- ImageMagick++ library and headers

# Building

- Edit build.sh and change ARTISTA_LIB and  ARTISTA_INC to point to the location of the ArtistaSDK.
- Comment in either the static or shared library build command in build.sh
- Run build.sh

# Running

- Run with `./artistactrl` . You will be presented with a list of options to use. 
- Screens are given an integer number (USB numbering) based on the order that USB subsystem uses to detect them. You use this number to send commands to screens. Screens can also be given ID's that are permanently stored by the screen. This can be useful for precisely identifying individual screens.