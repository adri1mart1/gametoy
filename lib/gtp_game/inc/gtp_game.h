/**
 * @file gtp_game.h
 * @author Adrien MARTIN
 * @brief Game API for the gametoy project
 * @version 0.1
 * @date 2024-11-11
 *
 * @copyright Copyright (c) 2024
 */

#ifndef GTP_GAME_H__
#define GTP_GAME_H__

#include <zephyr/types.h>
#include <stdbool.h>

typedef struct {
	void (*start)(void);
	void (*stop)(void);
} gtp_game_api_t;

uint8_t *gtp_game_get_random_suite_ptr();
void gtp_game_init();
void gtp_game_countdown_to_play();
void gtp_game_init_random_button_suite();
void gtp_game_display_score_int64(const int64_t score);
void gtp_game_display_score_int32(const int score);
void gtp_game_display_score_int64_millisec(const int64_t score);
void gtp_game_wait_for_any_input(bool *boolean);

#define GAME_WELL_FINISHED 1
#define SONG_WELL_FINISHED GAME_WELL_FINISHED

#endif /* GTP_GAME_H__ */