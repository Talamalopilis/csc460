#include <stdint.h>
#include "os.h"

#define LASER_PERIOD 10
#define ESCAPE 'e'
#define USER 'u'
#define CRUISE 'c'

typedef struct system_state {
	uint16_t rjs_x;
	uint16_t rjs_y;
	uint8_t rjs_z;
	uint16_t sjs_x;
	uint16_t sjs_y;
	uint8_t sjs_z;
	char current_action;
	char action_source;
	TICK laser_time;
};

typedef union system_data {
	struct system_state state;
	char data[sizeof(struct system_state)];
};