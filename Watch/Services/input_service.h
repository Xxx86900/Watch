#ifndef WATCH_SERVICES_INPUT_SERVICE_H
#define WATCH_SERVICES_INPUT_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "app_event.h"

void input_service_init(uint32_t now_ms);
void input_service_update(uint32_t now_ms);
bool input_service_get_event(app_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_SERVICES_INPUT_SERVICE_H */
