#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <string.h>

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwmled0));

#define MIN_PERIOD PWM_SEC(1U) / 16384U
#define MAX_PERIOD PWM_SEC(1U) / 1024U

#define DUTY_CYCLE_PERCENT 50u

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
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

// change this to make the song slower or faster
int tempo = 105;

// change this to whichever pin you want to use
int buzzer = 11;

// notes of the moledy followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
/*
int melody[] = {

	// Pacman
	// Score available at https://musescore.com/user/85429/scores/107109
	NOTE_B4,  16,  NOTE_B5,  16,  NOTE_FS5, 16,  NOTE_DS5, 16, // 1
	NOTE_B5,  32,  NOTE_FS5, -16, NOTE_DS5, 8,   NOTE_C5,  16, NOTE_C6, 16, NOTE_G6, 16,
	NOTE_E6,  16,  NOTE_C6,  32,  NOTE_G6,  -16, NOTE_E6,  8,

	NOTE_B4,  16,  NOTE_B5,  16,  NOTE_FS5, 16,  NOTE_DS5, 16, NOTE_B5, 32, // 2
	NOTE_FS5, -16, NOTE_DS5, 8,   NOTE_DS5, 32,  NOTE_E5,  32, NOTE_F5, 32, NOTE_F5, 32,
	NOTE_FS5, 32,  NOTE_G5,  32,  NOTE_G5,  32,  NOTE_GS5, 32, NOTE_A5, 16, NOTE_B5, 8};
*/

int melody[] = {

	// We Wish You a Merry Christmas
	// Score available at https://musescore.com/user/6208766/scores/1497501

	NOTE_C5,  4, // 1
	NOTE_F5,  4, NOTE_F5, 8, NOTE_G5,  8, NOTE_F5, 8, NOTE_E5,  8, NOTE_D5,  4, NOTE_D5, 4,
	NOTE_D5,  4, NOTE_G5, 4, NOTE_G5,  8, NOTE_A5, 8, NOTE_G5,  8, NOTE_F5,  8, NOTE_E5, 4,
	NOTE_C5,  4, NOTE_C5, 4, NOTE_A5,  4, NOTE_A5, 8, NOTE_AS5, 8, NOTE_A5,  8, NOTE_G5, 8,
	NOTE_F5,  4, NOTE_D5, 4, NOTE_C5,  8, NOTE_C5, 8, NOTE_D5,  4, NOTE_G5,  4, NOTE_E5, 4,

	NOTE_F5,  2, NOTE_C5, 4, // 8
	NOTE_F5,  4, NOTE_F5, 8, NOTE_G5,  8, NOTE_F5, 8, NOTE_E5,  8, NOTE_D5,  4, NOTE_D5, 4,
	NOTE_D5,  4, NOTE_G5, 4, NOTE_G5,  8, NOTE_A5, 8, NOTE_G5,  8, NOTE_F5,  8, NOTE_E5, 4,
	NOTE_C5,  4, NOTE_C5, 4, NOTE_A5,  4, NOTE_A5, 8, NOTE_AS5, 8, NOTE_A5,  8, NOTE_G5, 8,
	NOTE_F5,  4, NOTE_D5, 4, NOTE_C5,  8, NOTE_C5, 8, NOTE_D5,  4, NOTE_G5,  4, NOTE_E5, 4,
	NOTE_F5,  2, NOTE_C5, 4,

	NOTE_F5,  4, NOTE_F5, 4, NOTE_F5,  4, // 17
	NOTE_E5,  2, NOTE_E5, 4, NOTE_F5,  4, NOTE_E5, 4, NOTE_D5,  4, NOTE_C5,  2, NOTE_A5, 4,
	NOTE_AS5, 4, NOTE_A5, 4, NOTE_G5,  4, NOTE_C6, 4, NOTE_C5,  4, NOTE_C5,  8, NOTE_C5, 8,
	NOTE_D5,  4, NOTE_G5, 4, NOTE_E5,  4, NOTE_F5, 2, NOTE_C5,  4, NOTE_F5,  4, NOTE_F5, 8,
	NOTE_G5,  8, NOTE_F5, 8, NOTE_E5,  8, NOTE_D5, 4, NOTE_D5,  4, NOTE_D5,  4,

	NOTE_G5,  4, NOTE_G5, 8, NOTE_A5,  8, NOTE_G5, 8, NOTE_F5,  8, // 27
	NOTE_E5,  4, NOTE_C5, 4, NOTE_C5,  4, NOTE_A5, 4, NOTE_A5,  8, NOTE_AS5, 8, NOTE_A5, 8,
	NOTE_G5,  8, NOTE_F5, 4, NOTE_D5,  4, NOTE_C5, 8, NOTE_C5,  8, NOTE_D5,  4, NOTE_G5, 4,
	NOTE_E5,  4, NOTE_F5, 2, NOTE_C5,  4, NOTE_F5, 4, NOTE_F5,  4, NOTE_F5,  4, NOTE_E5, 2,
	NOTE_E5,  4, NOTE_F5, 4, NOTE_E5,  4, NOTE_D5, 4,

	NOTE_C5,  2, NOTE_A5, 4, // 36
	NOTE_AS5, 4, NOTE_A5, 4, NOTE_G5,  4, NOTE_C6, 4, NOTE_C5,  4, NOTE_C5,  8, NOTE_C5, 8,
	NOTE_D5,  4, NOTE_G5, 4, NOTE_E5,  4, NOTE_F5, 2, NOTE_C5,  4, NOTE_F5,  4, NOTE_F5, 8,
	NOTE_G5,  8, NOTE_F5, 8, NOTE_E5,  8, NOTE_D5, 4, NOTE_D5,  4, NOTE_D5,  4, NOTE_G5, 4,
	NOTE_G5,  8, NOTE_A5, 8, NOTE_G5,  8, NOTE_F5, 8, NOTE_E5,  4, NOTE_C5,  4, NOTE_C5, 4,

	NOTE_A5,  4, NOTE_A5, 8, NOTE_AS5, 8, NOTE_A5, 8, NOTE_G5,  8, // 45
	NOTE_F5,  4, NOTE_D5, 4, NOTE_C5,  8, NOTE_C5, 8, NOTE_D5,  4, NOTE_G5,  4, NOTE_E5, 4,
	NOTE_F5,  2, NOTE_C5, 4, NOTE_F5,  4, NOTE_F5, 8, NOTE_G5,  8, NOTE_F5,  8, NOTE_E5, 8,
	NOTE_D5,  4, NOTE_D5, 4, NOTE_D5,  4, NOTE_G5, 4, NOTE_G5,  8, NOTE_A5,  8, NOTE_G5, 8,
	NOTE_F5,  8, NOTE_E5, 4, NOTE_C5,  4, NOTE_C5, 4,

	NOTE_A5,  4, NOTE_A5, 8, NOTE_AS5, 8, NOTE_A5, 8, NOTE_G5,  8, // 53
	NOTE_F5,  4, NOTE_D5, 4, NOTE_C5,  8, NOTE_C5, 8, NOTE_D5,  4, NOTE_G5,  4, NOTE_E5, 4,
	NOTE_F5,  2, REST,    4};

typedef struct {
	char note[4];
	int frequency;
} note_t;

static const note_t notes[] = {
	{.note = "DO", .frequency = 1046},  {.note = "RE", .frequency = 1175},
	{.note = "MI", .frequency = 1318},  {.note = "FA", .frequency = 1397},
	{.note = "SOL", .frequency = 1568}, {.note = "LA", .frequency = 1760},
	{.note = "SI", .frequency = 1976}};

typedef struct {
	const char *note;
	int duration_ms; // Durée de la note en millisecondes
} mario_note_t;

int find_frequency(const char *note_name)
{
	for (size_t i = 0; i < ARRAY_SIZE(notes); i++) {
		if (strcmp(notes[i].note, note_name) == 0) {
			return notes[i].frequency;
		}
	}
	return 0; // Note non trouvée
}

int main(void)
{
	uint32_t max_period;
	uint32_t period;
	uint8_t dir = 0U;
	int ret;

	printk("PWM-based blinky\n");

	if (!pwm_is_ready_dt(&pwm_led0)) {
		printk("Error: PWM device %s is not ready\n", pwm_led0.dev->name);
		return 0;
	}

	/*
	 * In case the default MAX_PERIOD value cannot be set for
	 * some PWM hardware, decrease its value until it can.
	 *
	 * Keep its value at least MIN_PERIOD * 4 to make sure
	 * the sample changes frequency at least once.
	 */
	printk("Calibrating for channel %d...\n", pwm_led0.channel);
	max_period = MAX_PERIOD;
	while (pwm_set_dt(&pwm_led0, max_period, max_period / 2U)) {
		printk("max period = %d\n", max_period);
		max_period /= 2U;
		if (max_period < (4U * MIN_PERIOD)) {
			printk("Error: PWM device "
			       "does not support a period at least %lu\n",
			       4U * MIN_PERIOD);
			return 0;
		}
	}

	printk("Done calibrating; maximum/minimum periods %u/%lu nsec\n", max_period, MIN_PERIOD);

	// Mélodie de Mario (simplifiée)
	const mario_note_t mario_melody[] = {{"MI", 200}, {"MI", 200},  {"MI", 200},  {"DO", 200},
					     {"MI", 200}, {"SOL", 400}, {"SOL", 400}, // Pause
					     {"DO", 200}, {"SOL", 200}, {"MI", 200},  {"LA", 400},
					     {"SI", 400}, {"LA", 400},  {"SOL", 400}};

	for (int i = 0; i < sizeof(notes) / sizeof(notes[0]); i++) {
		ret = pwm_set_dt(&pwm_led0, PWM_HZ(notes[i].frequency),
				 PWM_HZ(notes[i].frequency) / 128U);
		if (ret) {
			printk("Error %d: failed to set pulse width\n", ret);
			return 0;
		}
		printk("Note %s frequency %d period %d\n", notes[i].note, notes[i].frequency,
		       PWM_HZ(notes[i].frequency));

		k_msleep(500);
	}

	pwm_set_dt(&pwm_led0, PWM_HZ(1024u), 0);

	for (size_t i = 0; i < ARRAY_SIZE(mario_melody); i++) {
		int frequency = find_frequency(mario_melody[i].note);

		if (frequency > 0) {
			// Configurer la PWM avec la fréquence correspondante
			pwm_set_dt(&pwm_led0, PWM_USEC(1000000 / frequency),
				   PWM_USEC(1000000 / frequency) / 128U);
			k_msleep(mario_melody[i].duration_ms); // Maintenir la note pendant sa durée
		}

		// Arrêter la PWM entre les notes
		pwm_set_dt(&pwm_led0, PWM_HZ(1024u), 0);
		k_msleep(50); // Pause entre les notes
	}

	// play pacman

	// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
	// there are two values per note (pitch and duration), so for each note there are four bytes
	int notes = sizeof(melody) / sizeof(melody[0]) / 2;

	// this calculates the duration of a whole note in ms
	int wholenote = (60000 * 4) / tempo;

	int divider = 0, noteDuration = 0;

	for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

		// calculates the duration of each note
		divider = melody[thisNote + 1];
		if (divider > 0) {
			// regular note, just proceed
			noteDuration = (wholenote) / divider;
		} else if (divider < 0) {
			// dotted notes are represented with negative durations!!
			noteDuration = (wholenote) / abs(divider);
			noteDuration *= 1.5; // increases the duration in half for dotted notes
		}

		printk("%d\n", melody[thisNote]);
		// we only play the note for 90% of the duration, leaving 10% as a pause
		pwm_set_dt(&pwm_led0, PWM_HZ(melody[thisNote]), PWM_HZ(melody[thisNote]) / 128U);

		// Wait for the specief duration before playing the next note.
		k_msleep(noteDuration);

		pwm_set_dt(&pwm_led0, PWM_HZ(1024u), 0);
	}
	// Arrêter la PWM entre les notes
	pwm_set_dt(&pwm_led0, PWM_HZ(1024u), 0);

	while (1) {
		k_msleep(1000);
	}
	/*
	    period = max_period;
	    while (1) {
		    ret = pwm_set_dt(&pwm_led0, period, period / 2U);
		    if (ret) {
			    printk("Error %d: failed to set pulse width\n", ret);
			    return 0;
		    }
		    printk("Using period %d\n", period);

		    period = dir ? (period * 2U) : (period / 2U);
		    if (period > max_period) {
			    period = max_period / 2U;
			    dir = 0U;
		    } else if (period < MIN_PERIOD) {
			    period = MIN_PERIOD * 2U;
			    dir = 1U;
		    }

		    k_sleep(K_SECONDS(4U));
	    }
	*/
	return 0;
}
