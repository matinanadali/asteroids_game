#include "welcome_screen.h"

void draw_welcome_screen(State state) {
    // Σχεδίαση κινούμενου background
    scrolling_back += 0.2f;
    if (scrolling_back >= SCREEN_HEIGHT) scrolling_back = 0;
    DrawTextureEx(background, (Vector2){ 0, scrolling_back }, 0.0f, 2.0f, (Color){255,255,255, 70});
    DrawTextureEx(background, (Vector2){ 0, -SCREEN_HEIGHT + scrolling_back}, 0.0f, 2.0f, (Color){255,255,255, 70});

    // Σχεδίαση επικεφαλίδας
    DrawText("ASTEROIDS GAME", 40, MID_HEIGHT-80, 50, WHITE);

    // Σχεδίαση κουμπίων για μετάβαση σε άλλη οθόνη
    DrawTextureEx(play_button, (Vector2){40, MID_HEIGHT}, 0.0, 1.3, WHITE);
    DrawTextureEx(rules_button, (Vector2){100 + BUTTON_WIDTH, MID_HEIGHT}, 0.0, 1.3, WHITE);  
    DrawTextureEx(credits_button, (Vector2){40, MID_HEIGHT + BUTTON_HEIGHT + 30}, 0.0, 1.3, WHITE);
    DrawTextureEx(spaceship, (Vector2){3*MID_WIDTH/2-SPACESHIP_WIDTH/2, SCREEN_HEIGHT-200}, 0.0, 1.5, WHITE);
}