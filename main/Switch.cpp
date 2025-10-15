#include "Switch.h"
#include "esp32-hal.h"
#include "driver/gpio.h"
#include <algorithm>
#include <esp_log.h>
#include <SetupMenu.h>
#include "esp_task_wdt.h"

std::list<SwitchObserver*> Switch::observers;
std::list<Switch*> Switch::instances;
TaskHandle_t Switch::pid = nullptr;
extern bool inch2dot4;

#define REPEAT_DELAY_MS 500     // Time to first repeat
#define REPEAT_RATE_MS  200     // Repeat intervall

Switch::Switch() {
    _sw = GPIO_NUM_0;
    _mode = B_MODE;
    _state = B_IDLE;
    _holddown = 0;
    p_time = 0;
    repeat_timer = 0;
    repeating = false;

    // Registriere die Instanz automatisch für Task
    instances.push_back(this);
}

Switch::~Switch() {
    // Entferne Instanz aus Liste
    instances.remove(this);
}

void Switch::begin(gpio_num_t sw, t_button mode) {
    _sw = sw;
    _mode = mode;
    gpio_set_direction(_sw, GPIO_MODE_INPUT);
    gpio_set_pull_mode(_sw, GPIO_PULLUP_ONLY);
}

bool Switch::isClosed() {
    return gpio_get_level(_sw) == 0;
}

bool Switch::isOpen() {
    return !isClosed();
}

void Switch::attach(SwitchObserver* obs) {
    observers.push_back(obs);
}

void Switch::detach(SwitchObserver* obs) {
    auto it = std::find(observers.begin(), observers.end(), obs);
    if (it != observers.end()) observers.erase(it);
}

void Switch::notifyObserversPress() {
    for (auto& obs : observers) {
        switch (_mode) {
            case B_MODE: obs->press(); break;
            case B_UP: obs->up(1); break;
            case B_DOWN: obs->down(1); break;
        }
    }
}

void Switch::sendPress(int dur) {
    ESP_LOGI(FNAME, "Press (%d ms)", dur);
    if( inch2dot4 ){
       notifyObserversPress();
    }else{ // 1.4 inch display

       for (auto& obs : observers){
    	   if( SetupMenu::isActive() )
    		   obs->up(1);
    	   else
    		   obs->press();
       	   // obs->press();
       	   ESP_LOGI(FNAME, "send obs %p press, key-nr: %d", obs, _mode );
       }
    }
}

void Switch::sendLongPress(int dur) {
    ESP_LOGI(FNAME, "Long Press (%d ms)", dur);
    if (_mode == B_MODE) for (auto& obs : observers) obs->longPress();
    else if (_mode == B_UP) for (auto& obs : observers) obs->up(1);
    else if (_mode == B_DOWN) for (auto& obs : observers) obs->down(1);
}

void Switch::sendLongLongPress(int dur) {
    ESP_LOGI(FNAME, "Long Long Press (%d ms)", dur);
    if (_mode == B_MODE) for (auto& obs : observers) obs->longLongPress();
    else if (_mode == B_UP) for (auto& obs : observers) obs->up(1);
    else if (_mode == B_DOWN) for (auto& obs : observers) obs->down(1);
}

void Switch::tick() {
    unsigned long currentMillis = millis();

    if (_holddown > 0) {
        _holddown--;
        return;
    }

    switch (_state) {
        case B_IDLE:
            if (isClosed()) {
                _state = B_PRESSED;
                p_time = currentMillis;
                ESP_LOGI(FNAME, "Button PRESSED");
            }
            break;

        case B_PRESSED:
        	if (isOpen()) {
        		int dur = currentMillis - p_time;
        		if (dur < LONG_PRESS_MS) sendPress(dur);
        		else if (dur < LONG_LONG_PRESS_MS) sendLongPress(dur);
        		_holddown = DEBOUNCE_MS / TASK_PERIOD_MS;
        		_state = B_IDLE;
        		repeating = false;        // Stop Repeat when released
        	} else {
        		int dur = currentMillis - p_time;

        		// PC-Key like repeat only for B_UP / B_DOWN buttons
        		if ((_mode == B_UP || _mode == B_DOWN)) {
        			if (!repeating && dur >= REPEAT_DELAY_MS) {
        				repeating = true;
        				repeat_timer = currentMillis + REPEAT_RATE_MS;
        				notifyObserversPress();
        			}
        			if (repeating && currentMillis >= repeat_timer) {
        				notifyObserversPress();
        				repeat_timer = currentMillis + REPEAT_RATE_MS;
        			}
        		}

        		// Long long Press (or hold) für B_MODE
        		if (_mode == B_MODE && dur > LONG_LONG_PRESS_MS) {
        			sendLongLongPress(dur);
        			_holddown = DEBOUNCE_MS / TASK_PERIOD_MS;
        			_state = B_PRESSED_STILL;
        		}
        	}
        	break;


        case B_PRESSED_STILL:
            if (isOpen()) {
                _state = B_IDLE;
                _holddown = DEBOUNCE_MS / TASK_PERIOD_MS;
            }
            break;
    }
}

void Switch::switchTask(void* pvParameters) {
	esp_task_wdt_add(NULL);
	while (1) {
		for (auto& sw : instances) {
			sw->tick();
		}
		esp_task_wdt_reset();
		vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
	}
}

void Switch::startTask() {
	ESP_LOGI(FNAME, "Starting Switch Task");
	if (pid == nullptr)
		xTaskCreatePinnedToCore(&switchTask, "Switch", 6096, NULL, 9, &pid, 0);
}
