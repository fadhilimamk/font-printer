/*
    Program to print font direct to frame buffer.
    Assumption: use 32bpp screen
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define SCALE 5
#define WHITE (rgbaToInt(255,255,255,0))
#define BLACK (rgbaToInt(0,0,0,0))
#define RED rgbaToInt(255, 0, 0, 0)

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
char *fbp = 0;

int char_height = 0;    // Character height in pixel, same for all char
int num_of_char = 0;    // Number of available char
char *char_index = 0;   // Array of char
int *char_width = 0;    // Array of char size, can be accessed with index
char **font = 0;        // Map of pixel for every character

void initFont(char *filename);
int getCharIndex(char c);
void drawPixel(int x, int y, unsigned int color);
int drawChar(int x, int y, char c);
int drawUnknownChar(int x, int y);
unsigned int rgbaToInt(int r, int g, int b, int a);

int main() {
    int fbfd = 0;
    long int screensize = 0;
    int x = 0, y = 0, i = 0;
    char text[1000];

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }
    printf("Detected display: %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }

    initFont("template");

    // Draw text 
    printf("Input text: "); scanf("%[^\n]s", text);
    for (i = 0; i < strlen(text); i++) {
        x += drawChar(x, y, text[i]);
        if (x > vinfo.xres/SCALE) {
            x = 0;
            y += char_height+2;
        }
    }
    
    free(char_index);
    free(char_width);
    for (i = 0; i < num_of_char; i++) {
        free(font[i]);    
    }
    free(font);
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

void initFont(char *filename) {
    FILE *fptr;
    size_t buffer_size = 80;
    char *buffer = malloc(buffer_size * sizeof(char));
    int i = 0, ret = 0;

    // Read from external file: filename
    fptr = fopen(filename, "r");
    if (fptr == NULL){
        printf("Cannot open file \n");
        exit(0);
    }

    ret = fscanf(fptr, "%d %d\n", &num_of_char, &char_height);
    if (ret != 2) {
        printf ("Failed to read font specification!\n");
    } else {
        printf("num_of_char: %d\n", num_of_char);
        printf("char_height: %d\n", char_height);
    }
    char_index = (char*) malloc(num_of_char * sizeof(char));
    font = (char**) malloc(num_of_char * sizeof(char*));
    char_width = (int*) malloc(num_of_char * sizeof(int));

    while (-1 != getline(&buffer, &buffer_size, fptr) && 1 < num_of_char) {
        font[i] = (char*) malloc(buffer_size * sizeof(char));
        ret = 0; ret = sscanf(buffer, "%c|%d|%[^\n]s", &char_index[i], &char_width[i], font[i]);
        if (ret != 3) {
            printf("Error reading line %d, only read %d\n", i+2, ret);
        }
        i++;
    }

    free(buffer);
    fclose(fptr);
}

int getCharIndex(char c) {
    int i = 0;
    for (i = 0; i < num_of_char; i++) {
        if (char_index[i] == c)
            return i;
    }
    return -1;
}

void drawPixel(int x, int y, unsigned int color) {
    long int location;
    int i = 0, j = 0;
    x = x*SCALE; y = y*SCALE;
    for (i = 0; i < SCALE; i++)
        for (j = 0; j < SCALE; j++) {
            location = (x+i+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+j+vinfo.yoffset) * finfo.line_length;
            *(fbp + location) = color;
            *(fbp + location + 1) = color >> 8;
            *(fbp + location + 2) = color >> 16;
            *(fbp + location + 3) = color >> 24;
        }
}

int drawChar(int x, int y, char c) {
    int i = 0, j = 0, k = 0;
    int idx = getCharIndex(c);
    int width = 4, pixel_length = 0;
    char* pixel = 0;

    if (idx == -1) {
        return drawUnknownChar(x,y);
    }

    if (x+width+2 > vinfo.xres/SCALE) {
        x = 0;
        y += char_height+2;
    }
    
    width = char_width[idx];
    pixel = font[idx];
    pixel_length = strlen(pixel);

    for (j = 0; j < char_height+2; j++)
        for (i = 0; i < width+2; i++) {
            if ((i == 0) || (i == width+1) || (j == 0) || (j == char_height+1))
                drawPixel(i+x, j+y, WHITE);
            else {
                if (k >= pixel_length) {
                    drawPixel(i+x, j+y, WHITE);
                    continue;
                }
                if (pixel[k] == '0')
                    drawPixel(i+x, j+y, WHITE);
                else
                    drawPixel(i+x, j+y, RED);
                k++;
            }
        }

    return width+2;
}

int drawUnknownChar(int x, int y) {
    int i = 0, j = 0, width = 4;

    if (x+width+2 > vinfo.xres/SCALE) {
        x = 0;
        y += char_height+2;
    }

    for (i = 0; i < width+2; i++)
        for (j = 0; j < char_height+2; j++)
            if (i>0 && i < width+1 && j>0 && j < char_height+1) {
                drawPixel(i+x, j+y, RED);
            } else {
                drawPixel(i+x, j+y, WHITE);
            }
    
    return width+2;
}

unsigned int rgbaToInt(int r, int g, int b, int a) {
    return a << 24 | r << 16 | g << 8 | b;
}
