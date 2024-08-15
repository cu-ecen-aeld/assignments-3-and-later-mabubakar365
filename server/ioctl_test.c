#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<stdint.h>

int main() {
    // Dummy input string
    char buffer[1024] = "Some text before AESDCHAR_IOCSEEKTO:123,456 some text after.";
    const char *pattern = "AESDCHAR_IOCSEEKTO:";
    uint32_t X = 0, Y = 0;
    size_t num_bytes = strlen(buffer);

    // Search for the pattern in the buffer
    char *match = strstr(buffer, pattern);
    if (match) {
        printf("Buffer before removing pattern: %s\n", buffer);
        // Try to extract X and Y values from the pattern
        if (sscanf(match, "AESDCHAR_IOCSEEKTO:%u,%u", &X, &Y) == 2) {
            printf("Pattern found with X = %u and Y = %u\n", X, Y);

            // Remove the pattern from the buffer
            size_t pattern_len = strlen(pattern);
            char *after_pattern = match + pattern_len + snprintf(NULL, 0, "%u,%u", X, Y);
            memmove(match, after_pattern, num_bytes - (after_pattern - buffer));
            num_bytes -= (after_pattern - match);

            // Optionally null-terminate the buffer
            buffer[num_bytes] = '\0';

            printf("Buffer after removing pattern: %s\n", buffer);
        } else {
            printf("Pattern not found or X, Y extraction failed\n");
        }
    } else {
        printf("Pattern not found in the buffer\n");
    }

    return 0;
}
