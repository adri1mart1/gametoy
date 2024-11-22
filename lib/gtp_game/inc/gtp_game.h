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

typedef struct {
    void (*start)(void);
    void (*stop)(void);
} gtp_game_api_t;

#endif /* GTP_GAME_H__ */