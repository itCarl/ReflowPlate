//
// Copyright (c) 2016-2017 jackw01
// This code is distrubuted under the MIT License, see LICENSE for details
//

#include "PIDAutotuner.h"

PIDAutotuner::PIDAutotuner() {

}

// Set target input for tuning
void PIDAutotuner::setTargetInputValue(double target) {

    targetInputValue = target;
}

// Set loop interval
void PIDAutotuner::setLoopInterval(long interval) {

    loopInterval = interval;
}

// Set output range
void PIDAutotuner::setOutputRange(double minR, double maxR) {

    minOutput = minR;
    maxOutput = maxR;
}

// Set Ziegler-Nichols tuning mode
void PIDAutotuner::setZNMode(byte zn) {

    znMode = zn;
}

// Set tuning cycles
void PIDAutotuner::setTuningCycles(int tuneCycles) {

    cycles = tuneCycles;
}

// Start loop
void PIDAutotuner::startTuningLoop() {

    // Initialize all variables before loop
    i = 0; // Cycle counter
    output = true; // Current output state
    outputValue = maxOutput;
    t1 = t2 = micros(); // Times used for calculating period
    microseconds = tHigh = tLow = 0; // More time variables
    maxR = -1000000; // maxR input
    minR = 1000000; // minR input
    pAverage = iAverage = dAverage = 0;

    sei();
}

// Run one cycle of the loop
double PIDAutotuner::tunePID(double input) {

    // Useful information on the algorithm used (Ziegler-Nichols method/Relay method)
    // http://www.processcontrolstuff.net/wp-content/uploads/2015/02/relay_autot-2.pdf
    // https://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method
    // https://www.cds.caltech.edu/~murray/courses/cds101/fa04/caltech/am04_ch8-3nov04.pdf

    // Basic explanation of how this works:
    //  * Turn on the output of the PID controller to full power
    //  * Wait for the output of the system being tuned to reach the target input value
    //      and then turn the controller output off
    //  * Wait for the output of the system being tuned to decrease below the target input
    //      value and turn the controller output back on
    //  * Do this a lot
    //  * Calculate the ultimate gain using the amplitude of the controller output and
    //      system output
    //  * Use this and the period of oscillation to calculate PID gains using the
    //      Ziegler-Nichols method

    // Calculate time delta
    long prevMicroseconds = microseconds;
    microseconds = micros();
    double deltaT = microseconds - prevMicroseconds;

    // Calculate maxR and minR
    maxR = max(maxR, input);
    minR = min(minR, input);

    // Output is on and input signal has risen to target
    if (output && abs(input - targetInputValue) > 3.0 && input > targetInputValue) {

        // Turn output off, record current time as t1, calculate tHigh, and reset maxRimum
        output = false;
        outputValue = minOutput;
        t1 = micros();
        tHigh = t1 - t2;
        maxR = targetInputValue;
    }

    // Output is off and input signal has dropped to target
    if (!output && abs(input - targetInputValue) > 1.0 && input < targetInputValue) {

        // Turn output on, record current time as t2, calculate tLow
        output = true;
        outputValue = maxOutput;
        t2 = micros();
        tLow = t2 - t1;

        // Calculate Ku (ultimate gain)
        // Formula given is Ku = 4d / πa
        // d is the amplitude of the output signal
        // a is the amplitude of the input signal
        double ku = (4.0 * ((maxOutput - minOutput) / 2.0)) / (M_PI * (maxR - minR) / 2.0);

        // Calculate Tu (period of output oscillations)
        double tu = tLow + tHigh;

        // How gains are calculated
        // PID control algorithm needs Kp, Ki, and Kd
        // Ziegler-Nichols tuning method gives Kp, Ti, and Td
        //
        // Kp = 0.6Ku = Kc
        // Ti = 0.5Tu = Kc/Ki
        // Td = 0.125Tu = Kd/Kc
        //
        // Solving these equations for Kp, Ki, and Kd gives this:
        //
        // Kp = 0.6Ku
        // Ki = Kp / (0.5Tu)
        // Kd = 0.125 * Kp * Tu

        // Constants
        // https://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method

        double kpConstant, tiConstant, tdConstant;

        if (znMode == znModeBasicPID) {

            kpConstant = 0.6;
            tiConstant = 0.5;
            tdConstant = 0.125;

        } else if (znMode == znModeLessOvershoot) {

            kpConstant = 0.33;
            tiConstant = 0.5;
            tdConstant = 0.33;

        } else if (znMode == znModeNoOvershoot) {

            kpConstant = 0.2;
            tiConstant = 0.5;
            tdConstant = 0.33;
        }

        // Normal PID
        //double pConstant = 0.6, iConstant = 0.5, dConstant = 0.125;

        // Less overshoot
        //double pConstant = 0.33, iConstant = 0.5, dConstant = 0.33;

        // No overshoot
        //double pConstant = 0.2, iConstant = 0.5, dConstant = 0.33;

        // Calculate gains
        kp = kpConstant * ku;
        ki = (kp / (tiConstant * tu)) * loopInterval;
        kd = (tdConstant * kp * tu) / loopInterval;

        // Average all gains after the first two cycles
        if (i > 1) {
            pAverage += kp;
            iAverage += ki;
            dAverage += kd;
        }

        // Reset minRimum
        minR = targetInputValue;

        // Increment cycle count
        i ++;
        Serial.printf("AUTOTUNER %d\n", i);
        Serial.printf("Kp %d\n", kp);
        Serial.printf("Ki %d\n", ki);
        Serial.printf("Kd %d\n\n", kd);
    }

    // If loop is done, disable output and calculate averages
    if (i >= cycles) {

        output = false;
        outputValue = minOutput;

        kp = pAverage / (i - 1);
        ki = iAverage / (i - 1);
        kd = dAverage / (i - 1);
    }

    return outputValue;
}

// Get PID constants after tuning
double PIDAutotuner::getKp() { return kp; };
double PIDAutotuner::getKi() { return ki; };
double PIDAutotuner::getKd() { return kd; };

// Is the tuning loop finished?
bool PIDAutotuner::isFinished() {

    return (i >= cycles);
}
