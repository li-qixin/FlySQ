#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tarox_board_control_id {
	TAROX_BOARD_CONTROL_BLDC_ENABLE = 1,
};

int tarox_board_control_set(enum tarox_board_control_id id, bool value);
bool tarox_board_control_get(enum tarox_board_control_id id);

#ifdef __cplusplus
}
#endif

