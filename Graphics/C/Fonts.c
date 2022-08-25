#include <fcntl.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DRAW_OUTLINE    1

typedef struct _CHARACTER_ENTRY {
    struct _CHARACTER_ENTRY * next;
    int character;
    uint8_t data[];
} CHARACTER_ENTRY;

int bytes_high;
CHARACTER_ENTRY * character_list;
int data_bytes;
char * font_text;
uint8_t * font_data;
int height;
int map_width;
int nchar;
int start;
int width;

int
read_value (
    const char * string,
    const char * text_end
    )
{
    const char * text;
    int length;
    int value;

    // Find the map width
    text = font_text;
    length = strlen(string);
    while (text < text_end) {
        if (strncmp (text, string, length) == 0) {
            text += length;
            break;
        }
        text++;
    }

    // Skip over the white space
    while (text < text_end) {
        if ((*text != ' ') && (*text != '\t'))
            break;
        text++;
    }

    // Get the map width
    if (sscanf (text, "%d", &value) != 1) {
        fprintf (stderr, "ERROR - Invalid map width!\n");
        return -1;
    }
    return value;
}

const char *
find_next_character (
    const char * text,
    const char * text_end
    )
{
    // Walk through the font file
    while (text < (text_end - 2)) {
        // Determine if this is an icon name
        if ((text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X')))
            return text;
        text++;
    }

    // No more data
    return NULL;
}

int
read_data (
    const char * text_end
    )
{
    int character_count;
    const char * text;
    int data_offset;
    int index;
    unsigned int value;

    // Read the data array
    character_count = 0;
    text = font_text;
    while (text < text_end) {
        for (index = 0; index < data_bytes; index++) {

            // Find the next data value
            text = find_next_character (text, text_end);
            if ((text == NULL) && (index == 0))
                return character_count;
            else if (text == NULL) {
                fprintf (stderr, "ERROR - Missing data!\n");
                return 0;
            }

            // Get the value
            if (sscanf (text, "0x%02x", &value) != 1) {
                fprintf (stderr, "ERROR - Invalid data value!\n");
                return 0;
            }
            text += 4;

            // Save the data value
            data_offset = (data_bytes * character_count) + index;
            font_data[data_offset] = value;
        }
        character_count++;
    }
    return 0;
}

void
add_entry (
    CHARACTER_ENTRY * new_entry
    )
{
    CHARACTER_ENTRY * previous_entry;

    // Walk to the end of the list
    previous_entry = character_list;
    while (previous_entry && previous_entry->next)
        previous_entry = previous_entry->next;

    // Add this entry to the list
    new_entry->next = NULL;
    if (previous_entry)
        previous_entry->next = new_entry;
    else
        character_list = new_entry;
}

int
process_data (
    int character_count
    )
{
    int font_index;
    int index;
    int map_characters;
    int map_offset;
    int next_character;
    CHARACTER_ENTRY * new_entry;
    ssize_t offset;
    unsigned int value;
    int x;
    int y;

    next_character = start;
    map_characters = map_width / width;
    while ((next_character - start) < character_count) {
        // Allocate the data structure
        new_entry = malloc (sizeof(*new_entry) + data_bytes);

        if (!new_entry) {
            fprintf (stderr, "ERROR - Failed to allocate next_entry!\n");
            return -1;
        }
        new_entry->character = next_character++;
        map_offset = new_entry->character - start;
        map_offset = ((map_offset / map_characters) * map_characters * data_bytes)
                   + ((map_offset % map_characters) * width);
        for (y = 0; y < bytes_high; y++) {
            for (x = 0; x < width; x++) {

                // Save the data value
                font_index = map_offset + (y * map_width) + x;
                value = font_data[font_index];
                index = (y * width) + x;
                new_entry->data[index] = value;
            }
        }

        // Add the entry to the list
        add_entry (new_entry);
    }
    return 0;
}

void
display_character (
    CHARACTER_ENTRY * character
    )
{
    int bit;
    const char * indent = "        ";
    int x;
    int y;

    printf ("/*\n");
    printf ("    0x%02x, %d [%d, %d]\n", character->character, character->character, width, height);
    printf ("\n");
#ifdef  DRAW_OUTLINE
    printf ("%s.", indent);
    for (x = 0; x < width; x++)
        printf ("-");
    printf (".\n");
#endif  // DRAW_OUTLINE
    for (y = 0; y < height; y++) {
        printf ("%s", indent);
#ifdef  DRAW_OUTLINE
        printf ("|");
#endif  // DRAW_OUTLINE
        for (x = 0; x < width; x++) {
            bit = character->data[((y >> 3) * width) + x];
            bit >>= y & 7;
            printf ("%c", (bit & 1) ? '*' : ' ');
        }
#ifdef  DRAW_OUTLINE
        printf ("|");
#endif  // DRAW_OUTLINE
        printf ("\n");
    }
#ifdef  DRAW_OUTLINE
    printf ("%s'", indent);
    for (x = 0; x < width; x++)
        printf ("-");
    printf ("'\n");
#endif  // DRAW_OUTLINE
    printf ("*/\n");
    printf ("\n");
}

int
main (
    int argc,
    char ** argv
    )
{
    CHARACTER_ENTRY * character;
    int character_count;
    const char * text_end;
    char * filename;
    int font_file;
    off_t file_length;
    int status;
    ssize_t valid_data;

    do {
        // Assume failure
        font_file = -1;
        status = -1;

        // Verify the argument count
        if (argc != 2) {
            fprintf (stderr, "%s  font_fliename\n", argv[0]);
            break;
        }

        // Get the font file name
        filename = argv[1];

        // Open the font file
        font_file = open (filename, O_RDONLY);
        if (font_file < 0) {
            perror("ERROR - File open failed!");
            break;
        }

        // Determine the length of the file
        file_length = lseek (font_file, 0, SEEK_END);

        // Go the the beginning of the file
        lseek (font_file, 0, SEEK_SET);

        // Allocate the text buffer
        font_text = malloc (file_length);
        if (!font_text) {
            fprintf (stderr, "ERROR - Failed to allocate font text buffer!\n");
            break;
        }

        // Fill the text buffer
        valid_data = read (font_file, font_text, file_length);
        if (valid_data < 0) {
            fprintf (stderr, "ERROR - File read failed!\n");
            break;
        }
        text_end = &font_text[valid_data];

        // Find the font values
        width = read_value ("WIDTH", text_end);
        if (width < 0) {
            fprintf (stderr, "ERROR - Failed to read font width!\n");
        }
        height = read_value ("HEIGHT", text_end);
        if (height < 0) {
            fprintf (stderr, "ERROR - Failed to read font height!\n");
            break;
        }
        start = read_value ("START", text_end);
        if (start < 0) {
            fprintf (stderr, "ERROR - Failed to read font start!\n");
            break;
        }
        nchar = read_value ("NCHAR", text_end);
        if (nchar < 0) {
            fprintf (stderr, "ERROR - Failed to read font nchar!\n");
            break;
        }
        map_width = read_value ("MAP_WIDTH", text_end);
        if (map_width < 0) {
            fprintf (stderr, "ERROR - Failed to read map width!\n");
            break;
        }
        bytes_high = (height + 7) / 8;
        data_bytes = width * bytes_high;

        // Allocate the data buffer
        font_data = malloc (sizeof(*font_data) * 256 * data_bytes);
        if (!font_data) {
            fprintf (stderr, "ERROR - Failed to allocate font data buffer!\n");
            break;
        }

        // Fill the data buffer
        character_count = read_data (text_end);

        // Process the data
        if (process_data(character_count))
            break;

        // Display the characters in the font file
        character = character_list;
        while (character) {
            display_character (character);
            character = character->next;
        }

        // Indicate success
        status = 0;
    } while (0);

    // Close the file
    if (font_file >= 0)
        close(font_file);

    return status;
}
