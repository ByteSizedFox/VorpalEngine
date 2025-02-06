#ifdef _WIN32

#include <Windows.h>
void alert(const char *message) {
    MessageBox(0, message, "Title", MB_OK);
}

#else

#include <cstdlib>  // For std::system
#include <iostream>
#include <string>

// Error display
bool isZenityAvailable() {
    // Execute 'command -v zenity > /dev/null 2>&1' to check for zenity
    int result = std::system("command -v zenity > /dev/null 2>&1");
    return result == 0;
}
void showZenityError(const std::string& message) {
    // Construct the command to invoke zenity with the --error option
    std::string command = "zenity --error --text=\"" + message + "\"";

    // Execute the command
    int result = std::system(command.c_str());

    // Check the result
    if (result != 0) {
        std::cerr << "Failed to invoke zenity or zenity is not installed." << std::endl;
    }
}
void alert(const char* message) {
    if (isZenityAvailable()) {
        showZenityError(std::string(message));
    } else {
        printf("%s\n",message);
    }
}

#endif