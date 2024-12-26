#ifndef GTP_SOUND_H__
#define GTP_SOUND_H__

#include <zephyr/types.h>

/* 740 is to be the lowest frequency that can be used
 * for this hardware. */
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST     0

#define BIP_GOOD NOTE_E6
#define BIP_BAD  NOTE_FS5

void gtp_sound_init();
void gtp_sound_good_short_bip();
void gtp_sound_good_long_bip();
void gtp_sound_error_long_bip();

const char *gtp_sound_tell_it_on_the_mountain_get_menu_title();
int gtp_sound_play_tell_it_on_the_mountain();
void gtp_sound_tell_it_on_the_mountain_start();

const char *gtp_sound_merry_christmas_get_menu_title();
int gtp_sound_play_merry_christmas();
void gtp_sound_a_merry_christmas_start();

void gtp_game_sound_play_note(const uint16_t note, const int duration_ms);

#endif /* GTP_SOUND_H__ */
