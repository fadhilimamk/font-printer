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

#define CHAR_WIDTH 15
#define CHAR_HEIGHT 20

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
char *fbp = 0;

void drawPixel(int x, int y, unsigned int color);
void drawChar(int x, int y, char c);
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

    // Draw text 
    printf("Input text: "); scanf("%s", text);
    for (i = 0; i < strlen(text); i++) {
        drawChar(x, y, text[i]);
        x += CHAR_WIDTH;
        if (x > vinfo.xres) {
            x = 0;
            y += CHAR_HEIGHT;
        }
    }
    
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

void drawPixel(int x, int y, unsigned int color) {
    long int location;
    location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
    *(fbp + location) = color;
    *(fbp + location + 1) = color >> 8;
    *(fbp + location + 2) = color >> 16;
    *(fbp + location + 3) = color >> 24;
}

void drawChar(int x, int y, char c) {
    int i = 0, j = 0;
    unsigned int color = rgbaToInt(255, 0, 0, 0);
    unsigned int black = rgbaToInt(0, 0, 0, 0);

    for (i = 1; i < CHAR_WIDTH; i++)
        for (j = 1; j < CHAR_HEIGHT; j++) {
            drawPixel(i+x, j+y, black);
            switch(c) {
                case 'A' :
                    if ((i == 2 || i == CHAR_WIDTH-2) && (j > 1) && (j < CHAR_HEIGHT-1))
                        drawPixel(i+x, j+y, color);
                    if ((j == 2 || j == 7) && (i > 1) && (i < CHAR_WIDTH-1))
                        drawPixel(i+x, j+y, color);
                    break;
                case ' ' :
                    return;
                default :
                    if (i == 1 || i == CHAR_WIDTH-1)
                        drawPixel(i+x, j+y, color);
                    if (j == 1 || j == CHAR_HEIGHT-1)
                        drawPixel(i+x, j+y, color);
                    break;
            }
        }
}

unsigned int rgbaToInt(int r, int g, int b, int a) {
    return a << 24 | r << 16 | g << 8 | b;
}