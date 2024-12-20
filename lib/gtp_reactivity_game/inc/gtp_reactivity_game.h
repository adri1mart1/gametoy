#ifndef GTP_REACTIVITY_GAME_H__
#define GTP_REACTIVITY_GAME_H__

void gtp_reactivity_game_init();
const char *gtp_reactivity_game_get_menu_title();
void gtp_reactivity_game_start();
int gtp_reactivity_game_play();

void gtp_revert_reactivity_game_init();
const char *gtp_revert_reactivity_game_get_menu_title();
void gtp_revert_reactivity_game_start();
int gtp_revert_reactivity_game_play();

void gtp_reactivity_phrase_game_init();
const char *gtp_reactivity_phrase_game_get_menu_title();
void gtp_reactivity_phrase_game_start();
int gtp_reactivity_phrase_game_play();

#endif // GTP_REACTIVITY_GAME_H__