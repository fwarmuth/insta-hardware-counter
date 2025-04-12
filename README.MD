# Instagram Follower Counter

A real-time Instagram follower counter project that displays your follower count on a physical LED display powered by an ESP32 microcontroller.

## Overview

This project consists of two main components:

1. **ESP32 Hardware Controller**: An ESP32 microcontroller that drives LED displays to show your current Instagram follower count. It periodically fetches updated counts from the API service.

2. **Instagram API Bridge Service**: A Python-based intermediate service that securely handles Instagram API authentication and provides a simple endpoint for the ESP32 to query follower counts without exposing API keys.

The system updates in real-time and provides a physical display of your Instagram followers, ideal for content creators, social media enthusiasts, or anyone wanting to visualize their social media growth.

## Features

- Real-time follower count display on LED hardware
- Secure API handling through a dedicated bridge service
- Containerized API service for easy deployment
- Low-power ESP32 implementation
- Customizable display options

## Technologies

- ESP32 microcontroller with FastLED support
- PlatformIO for embedded development
- Python Flask for API bridge service
- Docker for containerization
- Instagram Graph API integration