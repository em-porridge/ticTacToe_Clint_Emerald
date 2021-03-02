#ifndef SHARED_H
#define SHARED_H

// https://www.sciencedirect.com/topics/computer-science/registered-port#:~:text=Ports%200%20through%201023%20are,be%20used%20dynamically%20by%20applications.
// /etc/services
#define PORT 65432
#define BACKLOG 5

#define BLANK_SPACE 45

// response codes
#define WELCOME 1
#define INVITE 4
#define INVALID 5
#define ACCEPTED 6
#define WIN 7
#define LOSE 8
#define TIE 9

// randoms to deal with ASCII conversions
#define OFFSET 48
#define LOWER_0 47
#define UPPER_8 57

#define X 88
#define O 79
#endif
