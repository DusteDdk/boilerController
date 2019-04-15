#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WIDTH 1280
#define HEIGHT 800

#define DL 1280

#define LH 3
TTF_Font* font;

void txt(SDL_Surface* screen, char* str, int x, int y) {
        SDL_Surface* sur;

        SDL_Color col = { 109, 90, 85, 0x00 };

                sur = TTF_RenderText_Solid(font, str, col);
                SDL_Rect r= {x,y, sur->w, sur->h};
                SDL_BlitSurface( sur, NULL, screen, &r );
                SDL_FreeSurface(sur);
}

int main() {
        SDL_Window* win = NULL;
        SDL_Surface* screen = NULL;
        char buf[256];
        float ct=0;
        int cs = 0;
        float data[DL];

        float acc = 0;
        float cacc = 1;

        int frame=0;

        memset(&data, 0, sizeof(float)*DL );

        fprintf(stderr, "Hi!\n");
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
                printf("Couldnt' init sdl2: %s\n", SDL_GetError());
                return 1;
        }

        win = SDL_CreateWindow("Graph",
                        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                        WIDTH, HEIGHT,
                        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
                        );

        if( win == NULL ) { printf("Couldn't create window: %s\n", SDL_GetError()); }

        TTF_Init();
        font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", 36);
        if(!font) {
                printf("Error loading font.\n");
                return 1;
        }

        screen = SDL_GetWindowSurface(win);
        SDL_Rect rect = { 0,0, 1,LH };
        SDL_Rect lrect = { 0,0, WIDTH, 1 };
        SDL_ShowCursor(SDL_DISABLE);
        while(1) {
                fgets(buf, 128, stdin);
                if(buf[0]==0) {
                        return 0;
                } else
                if(buf[strlen(buf)-1] == '\n' ) {
                        // get it as string
                        float at = atof(buf);

                        acc += at;
                        ct = acc / cacc;
                        cacc++;

                        printf("%03i", (int)ct);
                        fflush(stdout);
                        fprintf(stderr, "Sample: %i Temp: %0.1f (%0.1f)\n", cs, ct, at);
                        cs++;

                        if(cacc == 7) {

                                //Shift the array
                                for(int i=0; i < DL; i++) {
                                        if(i>0) {
                                                data[i-1]=data[i];
                                        }
                                }
                                data[DL-1]=ct;

                                cacc=1;
                                acc=0;

                                // Render and update
                                SDL_FillRect( screen, NULL, SDL_MapRGB(screen->format, 0x0,0,0));

                                lrect.y= HEIGHT-LH-(int)( 140.0 * ((float)HEIGHT/300.0));

                                if(ct < 140.1 && ct > 120 ) {
                                        SDL_FillRect( screen, &lrect, SDL_MapRGB(screen->format, 150,120,55));
                                } else {
                                        SDL_FillRect( screen, &lrect, SDL_MapRGB(screen->format, 55,55,55));
                                }

                                lrect.y= HEIGHT-LH-(int)( 120.0 * ((float)HEIGHT/300.0));

                                if( ct < 120.1 && ct > 90.0 ) {
                                        SDL_FillRect( screen, &lrect, SDL_MapRGB(screen->format, 150,120,55));
                                } else {
                                        SDL_FillRect( screen, &lrect, SDL_MapRGB(screen->format, 55,55,55));
                                }

                                lrect.y= HEIGHT-LH-(int)( 90.0 * ((float)HEIGHT/300.0));
                                SDL_FillRect( screen, &lrect, SDL_MapRGB(screen->format, 55,55,55));


                                for(int i=0; i < DL; i++) {
                                        rect.x = i;
                                        rect.y = HEIGHT-LH - (int)(data[i] * ((float)HEIGHT/300.0));
                                        if( data[i] > 89.9 ) {
                                                SDL_FillRect( screen, &rect, SDL_MapRGB(screen->format, 244,113,66));
                                        } else if(data[i] > 10.0) {
                                                SDL_FillRect( screen, &rect, SDL_MapRGB(screen->format, 66,113,244));
                                        }
                                }
                                frame++;
                                fprintf(stderr, "Rendered frame %i\n", frame);
                                SDL_UpdateWindowSurface(win);

                        }
                        buf[0]=0;
                } else {
                        printf("Invalid input '%s'\n", buf);
                }
        }


        SDL_DestroyWindow(win);
        SDL_Quit();
        return 0;
}

