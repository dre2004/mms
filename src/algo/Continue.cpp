#include "Continue.h"

#include <cmath> // TODO

#include "../sim/Utilities.h"
#include "../sim/units/Milliseconds.h"

void Continue::solve(sim::MouseInterface* mouse) {

    while (true) {
        mouse->setRightWheelSpeed(2*M_PI);
        mouse->setLeftWheelSpeed(-2.4*M_PI);
        sim::sleep(sim::Milliseconds(1000));
        /*
        mouse->setRightWheelSpeed(2.7*M_PI);
        mouse->setLeftWheelSpeed(-2*M_PI);
        sim::sleep(sim::Milliseconds(1000));
        */
    }
    // ---- REMINDER ---- //
    /* Valid function calls:
     * 1) mouse->wallFront()
     * 2) mouse->wallRight()
     * 3) mouse->wallLeft()
     * 4) mouse->moveForward()
     * 5) mouse->turnRight()
     * 6) mouse->turnLeft()
     */
}
