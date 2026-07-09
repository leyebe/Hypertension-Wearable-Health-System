# Hypertension-Wearable-Health-System

An AI-assisted wearable health management system for hypertension patients.  
This project integrates STM32-based sensor data collection, ESP8266 Wi-Fi communication, Android App visualization, MySQL-based data storage, and AI-assisted health report generation.

## Project Overview

The system is designed to support long-term health monitoring for hypertension patients.  
The STM32 device collects health-related data such as heart rate, SpO2, body temperature, motion status, and abnormal events. The data can be transmitted through the ESP8266 Wi-Fi module and displayed on the Android App. Historical data can be used for trend analysis and AI-generated health reports.

## System Architecture

STM32 → ESP8266 → MQTT / IoT Platform → Android App → MySQL → AI Health Report

## Main Features

- Heart rate and SpO2 detection using MAX30102
- Motion and fall detection using MPU6050
- Temperature measurement using DS18B20
- OLED local data display
- ESP8266 Wi-Fi communication
- Android App real-time data display
- Historical health data management
- AI-assisted health report generation

## Repository Structure

- `BSP/`: Custom module drivers for sensors and external modules
- `Core/`: Main STM32 application logic and peripheral initialization
- `Drivers/`: STM32 HAL and CMSIS drivers
- `MDK-ARM/`: Keil MDK project files
- `Android_App/`: Extracted Android App source code
- `AI_Report/`: AI prompt design and health report generation logic
- `raw_extracted/`: Raw source code extracted from the project PDF
- `project.ioc`: STM32CubeMX configuration file

## Technology Stack

- STM32F103C8T6
- C language
- STM32 HAL Library
- Keil MDK-ARM
- STM32CubeMX
- ESP8266 Wi-Fi module
- MQTT / JSON
- Android Studio
- Java
- MySQL
- Qwen / DashScope AI API

## Note

This repository is a prototype project for learning and system integration purposes.  
The AI-generated health report is only used for health trend summary and lifestyle suggestions. It does not replace professional medical diagnosis.
