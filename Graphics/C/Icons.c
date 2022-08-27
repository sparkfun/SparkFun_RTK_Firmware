#include <fcntl.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ALPHABETICAL_ORDER  0
#define DISPLAY_COMMENT     1
#define DISPLAY_VARIABLES   1
#define DRAW_OUTLINE        1
#define PRINT_FILE_LINES    0
#define PRINT_HEIGHT        0
#define PRINT_SYMBOL        0
#define PRINT_VALUES        0
#define PRINT_WIDTH         0
#define WRAP_AT_16          0
#define USE_UPPERCASE_A     1
#define ADD_TRAILING_COMMA  0

typedef struct _ICON_ENTRY {
    struct _ICON_ENTRY * next;
    const char * name;
    int height;
    const char * height_name;
    int width;
    const char * width_name;
    int bytes_high;
    int bytes_wide;
    uint8_t data[];
} ICON_ENTRY;

int height;
char * icon_text;
ICON_ENTRY * icon_list;
int width;

// Replace CR and LF with zeros
char *
terminate_end_of_line (
    char * text,
    char * text_end
    )
{
    // Locate the end of line
    while ((text < text_end) && (*text != '\r') && (*text != '\n'))
        text++;

    // Skip over the end of line
    while ((text < text_end) && ((*text == '\r') || (*text == '\n')))
        *text++ = 0;

    // Return the start of the next line
    return text;
}

// Skip over white space
char *
skip_white_space (
    char * text,
    char * text_end
    )
{
    // Locate the first non-white space character
    while ((text < text_end) && ((*text == ' ') || (*text == '\t')))
        text++;

    // Return the address of the first non-white space character
    return text;
}

// Find the icon name
char *
find_icon_data (
    char * text,
    char * text_end
    )
{
    // Locate the end of the icon symbol name
    while ((text < text_end) && (*text != ' ') && (*text != '\t') && (*text != '['))
        text++;

    // Zero terminate the symbol name
    if (text < text_end)
        *text++ = 0;

    // Locate the beginning of the icon data
    while ((text < text_end) && (*text != '{'))
        text++;

    // Skip over the left parenthesis
    if (text < text_end)
        text++;

    // Return the beginning of the data
    return skip_white_space (text, text_end);
}

void
add_entry (
    ICON_ENTRY * new_entry
    )
{
    ICON_ENTRY * next_entry;

    if (ALPHABETICAL_ORDER) {
        // Add to the head of the list
        if ((icon_list == NULL) || (strcasecmp(icon_list->name, new_entry->name) > 0)) {
            new_entry->next = icon_list;
            icon_list = new_entry;
        } else {

            // Locate the point in the list to insert the new entry
            next_entry = icon_list;
            while (next_entry->next && (strcasecmp(next_entry->next->name, new_entry->name) < 0))
                next_entry = next_entry->next;

            new_entry->next = next_entry->next;
            next_entry->next = new_entry;
        }
    } else {
        // Add to the head of the list
        if (icon_list == NULL) {
            new_entry->next = icon_list;
            icon_list = new_entry;
        } else {

            // Locate the point in the list to insert the new entry
            next_entry = icon_list;
            while (next_entry->next)
                next_entry = next_entry->next;

            new_entry->next = next_entry->next;
            next_entry->next = new_entry;
        }
    }
}

char *
read_icon_data (
    char * name,
    char * height_name,
    char * width_name,
    char * text,
    char * text_end
    )
{
    int bytes_high;
    int bytes_wide;
    int data_bytes;
    ICON_ENTRY * icon_entry;
    int index;
    char * number;
    unsigned int value;
    int x;
    int y;

    // Allocate the data structure
    bytes_high = (height + 7) / 8;
    bytes_wide = width;
    data_bytes = width * bytes_high;
    icon_entry = malloc (sizeof(*icon_entry) + data_bytes);
    if (icon_entry) {
        icon_entry->name = name;
        icon_entry->height = height;
        icon_entry->height_name = height_name;
        icon_entry->width = width;
        icon_entry->width_name = width_name;
        icon_entry->bytes_high = bytes_high;
        icon_entry->bytes_wide = bytes_wide;
        add_entry (icon_entry);

        // Read the data bytes
        for (y = 0; y < bytes_high; y++) {
            for (x = 0; x < bytes_wide; x++) {

                // Skip any white space
                while ((text < text_end) && (*text != '0')) {
                    // Treat comments as white space
                    if ((*text != '/') || ((text[1] != '/') && (text[1] != '*')))
                        text++;
                    else {
                        // Skip over C++ style comment //
                        if (text[1] == '/')
                            while ((*text != '\r') && (*text != '\n'))
                                text++;
                        // Skip over the C style comment /* .. */
                        else {
                            text += 2;
                            while ((*text != '*') && (text[1] != '/'))
                                text++;
                        }
                    }
                }

                // Save the number location
                number = text;

                // Zero terminate the number
                while (((*text >= '0') && (*text <= '9'))
                    || ((*text >= 'a') && (*text <= 'f'))
                    || ((*text >= 'A') && (*text <= 'F'))
                    || (*text == 'x') || (*text == 'X'))
                    text++;
                *text++ = 0;

                // Get the value
                sscanf (number, "0x%x", &value);

                // Display the value if requested
                if (PRINT_VALUES)
                    printf("0x%02x\r\n", value);

                // Place the value in the array
                index = (y * bytes_wide) + x;
                icon_entry->data[index] = value;
                icon_entry->data[(y * bytes_wide) + x] = value;
            }
        }
    }

    // Skip over the data
    return text;
}

const char *
display_name (
    ICON_ENTRY * icon
    )
{
    return icon ? icon->name : "NULL";
}

int
process_data (
    int icon_file,
    ssize_t valid_data
    )
{
    char * data;
    char * data_end;
    char * height_name;
    const char * height_string = "_Height = ";
    int height_string_length;
    const char * int_string = "const int ";
    int int_string_length;
    char * name;
    char * next_line;
    int status;
    int temp;
    char * text;
    const char * uint8_t_string = "const uint8_t ";
    int uint8_t_string_length;
    char * width_name;
    const char * width_string = "_Width = ";
    int width_string_length;

    data = icon_text;
    data_end = &data[valid_data];
    status = -1;
    height_string_length = strlen (height_string);
    int_string_length = strlen (int_string);
    uint8_t_string_length = strlen (uint8_t_string);
    width_string_length = strlen (width_string);
    width_name = NULL;
    height_name = NULL;
    while (data < data_end) {
        text = data;
        next_line = terminate_end_of_line (text, data_end);

        // Display the line if requested
        if (PRINT_FILE_LINES)
            printf ("%s\r\n", text);

        // Locate the text at the beginning of the line
        text = skip_white_space (text, data_end);

        // Determine if this is a width or height line
        if (strncmp (text, int_string, int_string_length) == 0)
        {
            text = skip_white_space (&text[int_string_length], data_end);
            name = text;
            while (1) {
                // Locate the underscore
                while (*text && (*text != '_'))
                    text++;

                // Not a width or height line when end of line is reached
                if (!*text)
                    break;

                // Determine if this is a height line
                if (strncmp (text, height_string, height_string_length) == 0)
                {
                    // Zero terminate the name
                    name[&text[height_string_length] - 3 - name] = 0;
                    height_name = name;

                    // Skip over any additional white space
                    text = skip_white_space (&text[height_string_length], data_end);

                    // Get the height
                    if (sscanf (text, "%d", &temp) == 1)
                    {
                        height = temp;
                        if (PRINT_HEIGHT)
                            printf ("Height: %d\r\n", height);
                    }
                    break;
                }

                // Determine if this is a width line
                else if (strncmp (text, width_string, width_string_length) == 0)
                {
                    // Zero terminate the name
                    name[&text[width_string_length] - 3 - name] = 0;
                    width_name = name;

                    // Skip over any additional white space
                    text = skip_white_space (&text[width_string_length], data_end);

                    // Get the width
                    if (sscanf (text, "%d", &temp) == 1)
                    {
                        width = temp;
                        if (PRINT_WIDTH)
                            printf ("Width: %d\r\n", width);
                    }
                    break;
                }

                // Continue the underscore search
                text++;
            }
        }

        // Determine if this is an icon line
        else if (strncmp (text, uint8_t_string, uint8_t_string_length) == 0)
        {
            // Skip over any additional white space
            text = skip_white_space (&text[uint8_t_string_length], data_end);
            name = text;

            // Locate the beginning of the data
            text = find_icon_data (text, data_end);

            // Print the symbol name if requested
            if (PRINT_SYMBOL)
                printf ("Symbol: %s\r\n", name);

            // Read the icon data
            text = read_icon_data (name, height_name, width_name, text, data_end);
            height_name = NULL;
            width_name = NULL;
        }

        // Skip to the next line
        data = next_line;
    }
    if (data >= data_end)
        status = 0;
    return status;
}

void
display_icon (
    ICON_ENTRY * icon
    )
{
    int bit;
    int display_width;
    const char * indent = "        ";
    int index;
    int max_index;
    int x;
    int y;

    /*
        0x20, 0x60, 0xC0, 0xFF, 0xFF, 0xC0, 0x60, 0x20, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00

           **
           **
           **
           **
           **
        ** ** **
         ******
          ****
           **
    */

    printf ("/*\r\n");
    printf ("    %s [%d, %d]\r\n", icon->name, icon->width, icon->height);
    printf ("\r\n");
    if (DRAW_OUTLINE) {
        if (icon->width > 9) {
            printf ("%s     ", indent);
            for (x = 1; x <= icon->width; x++)
                printf ("%c", (x % 10) ? ' ' : ('0' + (x / 10)));
            printf ("\r\n");
        }
        printf ("%s     ", indent);
        for (x = 1; x <= icon->width; x++)
            printf ("%d", x % 10);
        printf ("\r\n");
        printf ("%s    .", indent);
        for (x = 0; x < icon->width; x++)
            printf ("-");
        printf (".\r\n");
    }
    for (y = 0; y < icon->height; y++) {
        printf ("%s", indent);
        if (DRAW_OUTLINE)
            printf ("0x%02x|", 1 << (y & 7));
        for (x = 0; x < icon->width; x++) {
            bit = icon->data[((y >> 3) * icon->bytes_wide) + x];
            bit >>= y & 7;
            printf ("%c", (bit & 1) ? '*' : ' ');
        }
        if (DRAW_OUTLINE)
            printf ("|");
        printf ("\r\n");
    }
    if (DRAW_OUTLINE) {
        printf ("%s    '", indent);
        for (x = 0; x < icon->width; x++)
            printf ("-");
        printf ("'\r\n");
    }
    printf ("*/\r\n");
    printf ("\r\n");

    // Display the symbols if requested
    if (DISPLAY_VARIABLES) {
        if (icon->height_name)
            printf ("const int %s = %d;\r\n", icon->height_name, icon->height);
        if (icon->width_name)
            printf ("const int %s = %d;\r\n", icon->width_name, icon->width);
        printf ("const uint8_t %s [] = {", icon->name);
        display_width = icon->bytes_wide;
        if (WRAP_AT_16 || (display_width > 20))
            display_width = 16;
        max_index = icon->bytes_high * icon->bytes_wide;
        for (index = 0; index < max_index; index++) {
            if ((index % display_width) == 0)
            printf ("\r\n ");
            if (USE_UPPERCASE_A)
                printf (" 0x%02X", icon->data[index]);
            else
                printf (" 0x%02x", icon->data[index]);
            if (index != (max_index - 1))
                printf (",");
        }
        if (ADD_TRAILING_COMMA)
            printf (",");
        printf ("\r\n");
        printf ("};\r\n");
        printf ("\r\n");
    }
}

int
main (
    int argc,
    char ** argv
    )
{
    char * filename;
    ICON_ENTRY * icon;
    char * icon_name;
    int icon_file;
    off_t file_length;
    int status;
    ssize_t valid_data;

    do {
        // Assume failure
        icon_file = -1;
        status = -1;

        // Get the icon name
        if (argc != 2) {
            fprintf (stderr, "ERROR - Icon file name not specified!\r\n");
            break;
        }
        filename = argv[1];

        // Open the icon file
        icon_file = open (filename, O_RDONLY);
        if (icon_file < 0) {
            perror("ERROR - File open failed!");
            break;
        }

        // Determine the length of the file
        file_length = lseek (icon_file, 0, SEEK_END);

        // Go the the beginning of the file
        lseek (icon_file, 0, SEEK_SET);

        // Allocate the buffer
        icon_text = malloc (file_length + 1);
        if (!icon_text) {
            fprintf (stderr, "ERROR - Failed to allocate icon buffer!\r\n");
            break;
        }

        // Zero terminate the last line in the file
        icon_text[file_length] = 0;

        // Fill the buffer
        valid_data = read(icon_file, icon_text, file_length);
        if (valid_data < 0) {
            fprintf (stderr, "ERROR - File read failed!\r\n");
            break;
        }

        // Process the data
        if (valid_data)
            if (process_data(icon_file, valid_data))
                break;

        // Display the initial comment
        if (DISPLAY_COMMENT) {
            printf ("//Create a bitmap then use http://en.radzio.dxp.pl/bitmap_converter/ to generate output\r\n");
            printf ("//Make sure the bitmap is n*8 pixels tall (pad white pixels to lower area as needed)\r\n");
            printf ("//Otherwise the bitmap bitmap_converter will compress some of the bytes together\r\n");
            printf ("\r\n");
        }

        // Display the icon names
        icon = icon_list;
        while (icon) {
            display_icon (icon);
            icon = icon->next;
        }

        // Indicate success
        status = 0;
    } while (0);

    // Close the file
    if (icon_file >= 0)
        close(icon_file);

    return status;
}
