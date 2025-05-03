#include "controller.h"
#include "main.h"
#include <Adafruit_TCA8418.h>
#include <string>
#include <map>

Adafruit_TCA8418 keypad;

#define ROWS 8
#define COLS 10
#define DATA_ROWS 5

//  typical Arduino UNO
const int IRQPIN = 8;

volatile bool TCA8418_event = false;

void TCA8418_irq()
{
    TCA8418_event = true;
}

const char *keymap[DATA_ROWS][COLS] = {
    {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
    {"A", "S", "D", "F", "G", "H", "J", "K", "L", "Backspace"},
    {"Z", "X", "C", "V", "B", "N", "M", "PgUp", "Up", "PgDn"},
    {"Help", "Shift", "CapsLock", "Esc", "Sym", ".", "Space", "Left", "Down", "Right"},
    {"GameUp", "GameDown", "GameLeft", "GameRight", "", "", "", "", "", ""},
};

const char *navButtons[9] = {
    "North",
    "NorthEast",
    "East",
    "SouthEast",
    "South",
    "SouthWest",
    "West",
    "NorthWest",
    "Push",
};

std::map<std::string, std::string> func_decorator_map = {
    {"Q", "P1"},
    {"W", "P2"},
    {"E", "P3"},
    {"R", "P4"},
    {"T", "P5"},
    {"Y", "P6"},
    {"U", "P7"},
};

void tca8418_setup()
{
    Serial.println(__FILE__);
    // Set IO 0 as function select key
    pinMode(0, INPUT);
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!keypad.begin(TCA8418_DEFAULT_ADDR, &Wire))
    {
        Serial.println("keypad not found, check wiring & pullups!");
        while (1)
            ;
    }

    //  configure the size of the keypad matrix.
    //  all other pins will be inputs
    keypad.matrix(ROWS, COLS);

    //  install interrupt handler
    //  going LOW is interrupt
    pinMode(IRQPIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(IRQPIN), TCA8418_irq, CHANGE);

    //  flush pending interrupts
    keypad.flush();
    //  enable interrupt mode
    keypad.enableInterrupts();
}

void tca8418_loop()
{
    if (TCA8418_event)
    {
        //  datasheet page 15 - Table 1
        int k = keypad.getEvent();

        //  try to clear the IRQ flag
        //  if there are pending events it is not cleared
        keypad.writeRegister(TCA8418_REG_INT_STAT, 1);
        int intstat = keypad.readRegister(TCA8418_REG_INT_STAT);
        if ((intstat & 0x01) == 0)
            TCA8418_event = false;
        bool is_press = (k & 0x80);
        // if (is_press)
        // {
        //     Serial.print("PRESS\tROW: ");
        // }
        // else
        // {
        //     Serial.print("RELEASE\tROW: ");
        // }
        k &= 0x7F;
        k--;
        uint8_t row = k / 10;
        uint8_t col = k % 10;
        // Serial.print(row);
        // Serial.print("\tCOL: ");
        // Serial.print(col);
        // Serial.print(" - ");
        const char *keyStr;
        if (row < DATA_ROWS)
        {
            keyStr = keymap[row][col];
        }
        else if (row == 7)
        {
            keyStr = navButtons[col];
        }
        else
        {
            keyStr = "Unknown";
        }
        Serial.print(keyStr);
        Serial.println();
        bool func_decorator = digitalRead(0) == LOW;
        // Serial.printf("IO0: func_decorator: %d\n", func_decorator);
        Serial.printf("key=%s, is_press: %d\n", keyStr, is_press);
        if (keyStr == "North")
        {
            gamepad_p1_prev.key.JOY_UP = is_press ? 1 : 0;
            Serial.printf("JOY_UP: %d\n", is_press);
        }
        else if (keyStr == "South")
        {
            gamepad_p1_prev.key.JOY_DOWN = is_press ? 1 : 0;
            Serial.printf("JOY_DOWN: %d\n", is_press);
        }
        else if (keyStr == "East")
        {
            gamepad_p1_prev.key.JOY_RIGHT = is_press ? 1 : 0;
            Serial.printf("JOY_RIGHT: %d\n", is_press);
        }
        else if (keyStr == "West")
        {
            gamepad_p1_prev.key.JOY_LEFT = is_press ? 1 : 0;
            Serial.printf("JOY_LEFT: %d\n", is_press);
        }
        else if (keyStr == "GameUp")
        {
            gamepad_p1_prev.key.JOY_START = is_press ? 1 : 0;
            Serial.printf("JOY_START: %d\n", is_press);
        }
        else if (keyStr == "GameDown")
        {
            gamepad_p1_prev.key.JOY_B = is_press ? 1 : 0;
            Serial.printf("JOY_B: %d\n", is_press);
        }
        else if (keyStr == "GameLeft")
        {
            gamepad_p1_prev.key.JOY_SELECT = is_press ? 1 : 0;
            Serial.printf("JOY_SELECT: %d\n", is_press);
        }
        else if (keyStr == "GameRight")
        {
            gamepad_p1_prev.key.JOY_A = is_press ? 1 : 0;
            Serial.printf("JOY_A: %d\n", is_press);
        }
        // 上面先处理四个方向，然后优先级更高的斜向按键在后面处理
        else if (keyStr == "NorthEast")
        {
            gamepad_p1_prev.JOY_UP_RIGHT = is_press ? 1 : 0;
            Serial.printf("JOY_UP_RIGHT: %d", is_press);
        }
        else if (keyStr == "SouthEast")
        {
            gamepad_p1_prev.JOY_RIGHT_DOWN = is_press ? 1 : 0;
            Serial.printf("JOY_RIGHT_DOWN: %d", is_press);
        }
        else if (keyStr == "SouthWest")
        {
            gamepad_p1_prev.JOY_DOWN_LEFT = is_press ? 1 : 0;
            Serial.printf("JOY_DOWN_LEFT: %d", is_press);
        }
        else if (keyStr == "NorthWest")
        {
            gamepad_p1_prev.JOY_LEFT_UP = is_press ? 1 : 0;
            Serial.printf("JOY_LEFT_UP: %d", is_press);
        }
        if (func_decorator && func_decorator_map.find(keyStr) != func_decorator_map.end())
        {
            // ruler_deck::pressed_key = func_decorator_map[keyStr];
        }
        else
        {
        }
    }
}

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

nes_pad_key_s gamepad_p1 = {0};
nes_pad_key_s gamepad_p2 = {0};

nes_pad_key_ext_s gamepad_p1_prev = {0};

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

void input_init()
{
    tca8418_setup();

#ifdef GPIO_PAD_ENABLED
    gpio_pad_init();
#endif

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
    tca8418_loop();
    // 处理斜向按键
    gamepad_p1.KEY_VALUE = gamepad_p1_prev.key.KEY_VALUE;
    if (gamepad_p1_prev.JOY_UP_RIGHT)
        gamepad_p1.KEY_VALUE |= GAMEPAD_KEY_UP | GAMEPAD_KEY_RIGHT;
    if (gamepad_p1_prev.JOY_RIGHT_DOWN)
        gamepad_p1.KEY_VALUE |= GAMEPAD_KEY_DOWN | GAMEPAD_KEY_RIGHT;
    if (gamepad_p1_prev.JOY_DOWN_LEFT)
        gamepad_p1.KEY_VALUE |= GAMEPAD_KEY_DOWN | GAMEPAD_KEY_LEFT;
    if (gamepad_p1_prev.JOY_LEFT_UP)
        gamepad_p1.KEY_VALUE |= GAMEPAD_KEY_UP | GAMEPAD_KEY_LEFT;
    gamepad_p2.KEY_VALUE = get_pad1_value();
}
