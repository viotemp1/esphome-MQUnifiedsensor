#pragma once

#include "esphome.h"
#include <MQUnifiedsensor.h>

/************************Hardware Related Macros************************************/
#define         board                   ("ESP-32") // Wemos ESP-32 or other board, whatever have ESP32 core.
/***********************Software Related Macros************************************/
#define         Voltage_Resolution      (3.3) // 3V3 <- IMPORTANT. Source: https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/
#define         ADC_Bit_Resolution      (12) // ESP-32 bit resolution. Source: https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/
#define         RatioMQ135CleanAir        (3.6) // Ratio of your sensor, for this example an MQ-3


class MQxx : public PollingComponent, public Sensor {
    public:
        
        Sensor *value_sensor = new Sensor();
        
        MQxx(int pin, char* stype) : PollingComponent(1000) {
            _stype = stype;
            _pin = pin;
            _setup_done = false;
            _count = 0;
        }

        
        float get_setup_priority() const override { return esphome::setup_priority::LATE; }
        
        void setup() override {
            
            if (strcmp(_stype, "MQ-135") == 0) {
                MQxxS = new MQUnifiedsensor(board, Voltage_Resolution, ADC_Bit_Resolution, _pin, _stype);
                ESP_LOGI("MQ135", "setup");
                MQxxS->setRegressionMethod(1); //_PPM =  a*ratio^b
                MQxxS->init();
                ESP_LOGI("MQ135", "Calibrating please wait.");
                float _calcR0 = 0;
                for(int i = 1; i<=10; i ++) {
                    MQxxS->update(); // Update data, the arduino will be read the voltage on the analog pin
                    _calcR0 += MQxxS->calibrate(RatioMQ135CleanAir);
                    ESP_LOGI("MQ135", ".");
                }
                MQxxS->setR0(_calcR0/10);
                ESP_LOGI("MQ135", "Calibration  done!");
                if(isinf(_calcR0)) {
                    _setup_warnings = (char*)"Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply";
                }
                if(_calcR0 == 0) {
                    if (_setup_warnings != NULL && strlen(_setup_warnings)>0) {
                        sprintf(_setup_warnings,"%s\n%s",_setup_warnings,(char*)"Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply");
                    }
                    else {
                        _setup_warnings =  (char*)"Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply";
                    }
                }
            }
            _setup_done = true;
        }

        void update() override {
            // print startup warning with delay after ESP-32 is connected to WiFi
            if (_setup_warnings != NULL && strlen(_setup_warnings)>0 and _count == 10) {
                ESP_LOGW("MQxx", "setup_warnings %s", _setup_warnings);
                _setup_warnings = (char*)"";
            }
            
            MQxxS->update(); // Update data, the arduino will be read the voltage on the analog pin
            
            float val_sensor = 0;
            
            if (strcmp(_stype, "MQ-135") == 0) {
                // CO2
                MQxxS->setA(110.47); MQxxS->setB(-2.862); // Configurate the ecuation values to get CO2 concentration
                val_sensor = MQxxS->readSensor() + 400; // Sensor will read PPM concentration using the model and a and b values setted before or in the setup
                value_sensor->publish_state(val_sensor);
                ESP_LOGI("MQxx", "update %d %d %d %s %.2f", _setup_done, _count, _pin, _stype, val_sensor);
            }
            
            if (_count < 10) {
                _count += 1;
            }
        }

    protected:
        int _pin;
        char* _stype; // MQ-135
        bool _setup_done;
        char* _setup_warnings;
        int _count;
        //MQUnifiedsensor MQxxS;
        MQUnifiedsensor *MQxxS;
};
