#ifdef _WIN32

#include <windows.h>
void alert(const char *message) {
    MessageBox(0, message, "Title", MB_OK);
}

#else

#include <stdlib.h>  // For system()
#include <stdio.h>   // For printf()

void alert(const char* message) {
    printf("%s\n", message); // always log the message to console
    // display zenity error popup, will do nothing if zenity does not exist
    char command[1024];
    snprintf(command, sizeof(command), "zenity --error --text=\"%s\"", message);
    system(command);
}

#endif