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

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 20
#define SCALE 5

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
char *fbp = 0;

int char_height = 5;    // Character height in pixel, same for all char
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
    char text[256];

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
    printf("Input text: "); scanf("%s", text);
    for (i = 0; i < strlen(text); i++) {
        x += drawChar(x, y, text[i]);
        if (x > vinfo.xres/SCALE) {
            x = 0;
            y += char_height+2;
        }
    }
    
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

void initFont(char *filename) {
    // Read from external file: filename
    char_height = 5;
    num_of_char = 3;
    
    char_index = (char*) malloc(num_of_char * sizeof(char));
    char_width = (int*) malloc(num_of_char * sizeof(int));
    font = (char**) malloc(num_of_char);

    // Inject dummy data (sementara, karena belum bisa baca file eksternal)
    char_index[0] = 'A';
    char_width[0] = 4;
    font[0] = "01101001111110011001";
    char_index[2] = ' ';
    char_width[2] = 4;
    font[2] = "00000000000000000000";
    char_index[1] = 'T';
    char_width[1] = 3;
    font[1] = "111010010010010";
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
    unsigned int color = rgbaToInt(255, 0, 0, 0);
    unsigned int black = rgbaToInt(0, 0, 0, 0);
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
                drawPixel(i+x, j+y, black);
            else {
                if (k >= pixel_length) {
                    drawPixel(i+x, j+y, black);
                    continue;
                }
                if (pixel[k] == '0')
                    drawPixel(i+x, j+y, black);
                else
                    drawPixel(i+x, j+y, color);
                k++;
            }
        }

    return width+2;
}

int drawUnknownChar(int x, int y) {
    int i = 0, j = 0, width = 4;
    unsigned int color = rgbaToInt(255, 0, 0, 0);
    unsigned int black = rgbaToInt(0, 0, 0, 0);

    if (x+width+2 > vinfo.xres/SCALE) {
        x = 0;
        y += char_height+2;
    }

    for (i = 0; i < width+2; i++)
        for (j = 0; j < char_height+2; j++)
            if (i>0 && i < width+1 && j>0 && j < char_height+1) {
                drawPixel(i+x, j+y, color);
            } else {
                drawPixel(i+x, j+y, black);
            }
    
    return width+2;
}

unsigned int rgbaToInt(int r, int g, int b, int a) {
    return a << 24 | r << 16 | g << 8 | b;
}