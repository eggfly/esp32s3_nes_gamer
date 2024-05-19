#include "controller.h"
#include "main.h"

// USB Host Shield
#ifdef USB_A_PORT_MODE_UHS
#include "usbhostshield/usbhostshield.h"
#endif

#ifdef USB_A_PORT_MODE_CDC
#include "keyboard/usbkeyboard.h"
#include "joystick/joystick.h"
#endif

// 自带GPIO手柄
#ifdef GPIO_PAD_ENABLED
#include "gpiopad/gpiopad.h"
#endif

#include "blegamepad/blegamepad.h"

#include <Adafruit_AW9523.h>

Adafruit_AW9523 aw;

nes_pad_key_s gamepad_p1 = {0};
nes_pad_key_s gamepad_p2 = {0};

uint8_t get_pad0_value()
{
    uint8_t value = 0;

    // 多个手柄是并集模式
    //  TODO  映射1p 2p
#ifdef GPIO_PAD_ENABLED
    value = gpio_get_key_value();
#endif

    if (cfg.controller == CONTROLLER_WECHAT_BLEPAD)
        value |= wxpad_get_key_value();

#ifdef USB_A_PORT_MODE_UHS
    value |= uhs_get_key_value();
#endif

#ifdef USB_A_PORT_MODE_CDC
    if (cfg.controller == CONTROLLER_USB_HID_KBD)
        value |= keyboard_get_key_value();
    else if (cfg.controller == CONTROLLER_USB_HID_JOYSTICK)
        value |= joystick_get_key_value();
#endif

    return value;
}

uint8_t get_pad1_value()
{
    uint8_t value = 0;
#ifdef USB_A_PORT_MODE_CDC
    if (cfg.controller == CONTROLLER_USB_HID_KBD)
        value |= keyboard_p2_get_key_value();
#endif
    return value;
}

void setup_aw9523()
{
    printf("Adafruit AW9523 test!\n");

    if (!aw.begin(0x5B))
    {
        printf("AW9523 not found? Check wiring!\n");
        while (1)
            delay(10); // halt forever
    }

    printf("AW9523 found!\n");
    aw.pinMode(0, INPUT);
    aw.pinMode(1, INPUT);
    aw.pinMode(2, INPUT);
    aw.pinMode(3, INPUT);
    aw.pinMode(4, INPUT);
    // aw.enableInterrupt(ButtonPin, true);
}
unsigned long last_update_time = 0;

void loop_aw9523()
{
    if (millis() - last_update_time < 20)
    {
        return;
    }
    last_update_time = millis();

    bool keypad_states[5];
    for (uint8_t i = 0; i < 5; i++)
    {
        keypad_states[i] = !aw.digitalRead(i);
    }
    bool hasKey = true;
    if (keypad_states[0])
    {
        gamepad_p1.JOY_UP = 1;
        printf("up");
    }
    else if (keypad_states[1])
    {
        gamepad_p1.JOY_LEFT = 1;
        printf("left");
    }
    else if (keypad_states[2])
    {
        gamepad_p1.JOY_RIGHT = 1;
        printf("right");
    }
    else if (keypad_states[3])
    {
        gamepad_p1.JOY_DOWN = 1;
        printf("down");
    }
    else if (keypad_states[4])
    {
        gamepad_p1.JOY_START = 1;
        printf("start");
    }
    else
    {
        hasKey = false;
    }
    if (hasKey)
    {
        printf("\n");
    }
}

void input_init()
{
    setup_aw9523();
    setup_keypad();
    // #ifdef GPIO_PAD_ENABLED
    //     gpio_pad_init();
    // #endif

    if (cfg.controller == CONTROLLER_WECHAT_BLEPAD)
        ble_gamepad_init();
#ifdef USB_A_PORT_MODE_UHS
    xTaskCreatePinnedToCore(task_host_shield, "task_usb_host", 8196, NULL, 2, &TASK_HOST_SHIELD_HANDLE, 1);
#endif

#ifdef USB_A_PORT_MODE_CDC
    if (cfg.controller == CONTROLLER_USB_HID_KBD)
        keyboard_setup();
    else if (cfg.controller == CONTROLLER_USB_HID_JOYSTICK)
        joystick_init();
#endif
}

void input_clear()
{
    gamepad_p1 = {0};
    gamepad_p2 = {0};
}

void input_refresh()
{
    // gamepad_p1.KEY_VALUE = get_pad0_value();
    gamepad_p1.KEY_VALUE = 0;
    loop_aw9523();
    gamepad_p2.KEY_VALUE = get_pad1_value();
    keypadEvent event;
    bool hasEvent = loop_keypad(&event);
    if (hasEvent)
    {
        if (event.bit.EVENT == KEY_JUST_PRESSED)
        {
            if (event.bit.KEY == '$')
            {
                gamepad_p1.JOY_A = 1;
            }
            if (event.bit.KEY == 'n')
            {
                gamepad_p1.JOY_B = 1;
            }
            if (event.bit.KEY == 's')
            {
                gamepad_p1.JOY_SELECT = 1;
            }
            if (event.bit.KEY == 'r')
            {
                gamepad_p1.JOY_START = 1;
            }
        }
    }
    // printf("Pad: %x,%x\n", gamepad_p1, gamepad_p2);
}
