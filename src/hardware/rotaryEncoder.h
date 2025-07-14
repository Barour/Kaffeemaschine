#pragma once

ESP32Encoder encoder;

unsigned long startMillisEncoderSw = 0;
unsigned long EncoderSwitchBackflushInterval = 2000;
unsigned long EncoderSwitchControlInterval = 800;
bool encoderSwPressed = false;

void initEncoder() {
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachHalfQuad(PIN_ROTARY_DT, PIN_ROTARY_CLK);
    encoder.setCount(0);
}

int getEncoderDelta(void) {
    static long lastencodervalue = 0;
    long value = encoder.getCount() / 2;
    int delta = value - lastencodervalue;

    if (lastencodervalue != value) {
        LOGF(INFO, "Rotary Encoder Value: %i", value);
    }

    lastencodervalue = value;

    return delta;
}

void encoderHandler() {
    if (!config.get<bool>("hardware.switches.encoder.enabled")) {
        return;
    }

    int delta = getEncoderDelta(); // +1 or -1 or 0

    if (machineState != kBackflush) {
        if (delta != 0) {
            if (menuLevel == 1) {
                dimmerMode = constrain(dimmerMode + delta, 0, 3);
                config.set<int>("dimmer.mode", dimmerMode);
            }
            else if (menuLevel == 2) {
                switch (dimmerMode) {
                    case POWER:
                        pumpPowerSetpoint = constrain(pumpPowerSetpoint + delta, PUMP_POWER_SETPOINT_MIN, PUMP_POWER_SETPOINT_MAX);
                        config.set<double>("dimmer.setpoint.power", pumpPowerSetpoint);
                        break;

                    case PRESSURE:
                        pumpPressureSetpoint = constrain(pumpPressureSetpoint + ((float)delta * 0.1), PUMP_PRESSURE_SETPOINT_MIN, PUMP_PRESSURE_SETPOINT_MAX);
                        config.set<double>("dimmer.setpoint.pressure", pumpPressureSetpoint);
                        break;

                    case FLOW:
                        pumpFlowSetpoint = constrain(pumpFlowSetpoint + ((float)delta * 0.1), PUMP_FLOW_SETPOINT_MIN, PUMP_FLOW_SETPOINT_MAX);
                        config.set<double>("dimmer.setpoint.flow", pumpFlowSetpoint);
                        break;

                    case PROFILE:
                        selectedProfile = constrain(selectedProfile + delta, 0, 11);
                        config.set<int>("dimmer.profile", selectedProfile);
                        break;

                    default:
                        break;
                }
            }
        }
    }

    if (encoderSw->isPressed()) {
        if (encoderSwPressed == false) {
            startMillisEncoderSw = millis();
            encoderSwPressed = true;
        }
    }
    else {
        if (encoderSwPressed == true) {
            unsigned long duration = millis() - startMillisEncoderSw;
            if (duration > EncoderSwitchBackflushInterval) { // toggle every interval
                if (machineState == kBackflush) {
                    backflushOn = false;
                    startMillisEncoderSw = millis();
                }

                if (machineState == kPidNormal) {
                    backflushOn = true;
                    startMillisEncoderSw = millis();
                }
            }
            else if (duration > EncoderSwitchControlInterval) { // toggle every interval
                menuLevel = 0;
                if (!config.save()) {
                    LOG(ERROR, "Failed to save config to filesystem!");
                }
            }
            else {
                menuLevel = (menuLevel == 1) ? 2 : 1;
            }

            LOGF(INFO, "Rotary Encoder Button down for: %lu ms", duration);
        }

        encoderSwPressed = false;
    }
}