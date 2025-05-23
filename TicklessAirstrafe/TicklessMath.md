To integrate air strafing mathematically correctly, we need to solve a couple of problems. Our currentspeed, that is, the velocity along the axis of acceleration, must never exceed 30, the game will just cut your acceleration short if your acceleration would exceed it.
I am making one assumption: The rotation during the tick is constant and constitutes an interpolation between the previous ticks rotation and the current ticks rotation.


Firstly, if we are already above 30u/s currentspeed, we must find the next point in time at which we go down to 30u/s and check if we have to limit acceleration (see the limited acceleration at the end, we just have to check if the velocity at start is > max accel): https://www.desmos.com/calculator/h0mzyabyg1

If our rotation speed allows us to use full acceleration (or if our currentspeed is < 30), we must get the integral up to the point in time at which we hit 30 (or until the end of the tick): https://www.desmos.com/calculator/mjrgh8vf2w

If our rotation speed is such that currentspeed decreases but not fast enough to allow full acceleration, we must find the function that describes the required acceleration as a function of time and integrate with that: https://www.desmos.com/calculator/ya5hspd8la, https://www.desmos.com/calculator/xneqrjkaud
