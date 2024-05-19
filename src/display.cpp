#include "config.h"
#include "display.h"
// #include <Arduino_GFX_Library.h>
#include "emucore/nes_palette.h"
#include <TFT_eSPI.h> //https://github.com/Bodmer/TFT_eSPI
#include "rm67162.h"

#ifdef USE_DOUBLE_BUFFER_DRAW_MJPEG
uint16_t *frame_odd_buf;
uint16_t *frame_even_buf;
#endif


#define WIDTH 536
#define HEIGHT 240

#define LBW_WIDTH 320
// WIDTH 108
#define LEFT_MARGIN (WIDTH - LBW_WIDTH) / 2


static uint16_t frameCacheStart = 0;                             // NES刷屏，从缓存数组第几位开始刷
static uint16_t screenXStart = 0;                                // NES刷屏的x轴起始点
static uint16_t linePixels = NES_SCREEN_WIDTH;                   // NES刷屏每一行的像素
static uint16_t frameHeight = NES_SCREEN_HEIGHT;                 // NES刷屏高度
static uint16_t screenYStart = 0;                                // NES刷屏的y轴起始点
static uint16_t frameCacheRowOffset = 0;                         // NES刷屏，缓存数组跳过的行
const uint32_t frameBytes = SCREEN_RES_HOR * SCREEN_RES_VER * 2; // 全屏所需要的字节
uint8_t *SCREENMEMORY = NULL;                                    // NES屏幕缓存
TaskHandle_t TASK_VID_HANDLE;

// TFT_eSPI tft = TFT_eSPI();
// TFT_eSprite spr = TFT_eSprite(&tft);

#ifdef SCR_LCOS_HX7033
// NOP
#else
// Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI);

// SCRGFX *gfx = new SCRGFX(bus, TFT_RST, TFT_ROTATION, TFT_IS_IPS);

Arduino_Canvas *gfx = new Arduino_Canvas(WIDTH - LEFT_MARGIN /* width */, HEIGHT /* height */, nullptr);
uint8_t *swappedBuffer;

void flush_screen()
{
    // printf("f\n");
    swapBytes((uint8_t *)gfx->getFramebuffer(), swappedBuffer, WIDTH * HEIGHT * 2);
    lcd_PushColors(LEFT_MARGIN, 0, WIDTH - LEFT_MARGIN, HEIGHT, (uint16_t *)swappedBuffer);
    // lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
}

void display_init()
{
    rm67162_init();
    lcd_setRotation(1);
    lcd_setBrightness(0x60);
    // spr.createSprite(WIDTH, HEIGHT);
    // spr.setSwapBytes(1);
    gfx->begin();
    gfx->setRotation(0);
    // NO USE
    gfx->fillScreen(RGB565(20, 20, 20));
    gfx->setTextColor(UI_TEXT_COLOR);
    swappedBuffer = (uint8_t *)ps_malloc(WIDTH * HEIGHT * 2);
    flush_screen();

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
#ifdef TFT_BLK_ON_LOW
    digitalWrite(TFT_BL, LOW);
#else
    digitalWrite(TFT_BL, HIGH);
#endif

#endif
}

#endif

void refresh_lcd(vid_notify_command_t cmd)
{
    xTaskNotify(TASK_VID_HANDLE, cmd, eSetValueWithOverwrite);
}

// eggfly
static void lcd_write_frame(uint32_t target)
{
    // printf("lcd_write_frame, t=%d\n", target);
#ifdef USE_DOUBLE_BUFFER_DRAW_MJPEG
    if (target == VID_DRAW_NES_FRAME)
    {
        // 刷新NES帧
        gfx->drawIndexedBitmap(screenXStart, frameCacheRowOffset, SCREENMEMORY, (uint16_t *)nes_palette_256, linePixels, frameHeight);
        // gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)SCREENMEMORY, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT);
        flush_screen();
    }
    else if (target == VID_DRAW_MJPEG_ODD_FRAME)
    {
        // 绘制MJPEG单帧
        gfx->draw16bitRGBBitmap(0, 0, frame_odd_buf, SCREEN_RES_HOR, SCREEN_RES_VER);
    }
    else if (target == VID_DRAW_MJPEG_EVEN_FRAME)
    {
        // 绘制MJPEG双帧
        gfx->draw16bitRGBBitmap(0, 0, frame_even_buf, SCREEN_RES_HOR, SCREEN_RES_VER);
    }

#endif
}

void videoTask(void *arg)
{
    // 根据各种屏幕计算必须的参数
    if (SCREEN_RES_HOR < 480)
    {
        // 分辨率 240x240 或者 320x240
        if (SCREEN_RES_HOR > NES_SCREEN_WIDTH)
        {
            // 主要是320x240的情况
            screenXStart = (SCREEN_RES_HOR - NES_SCREEN_WIDTH) / 2;
            frameCacheStart = 0;
            linePixels = NES_SCREEN_WIDTH;
        }
        else
        {
            // 主要是240x240的情况
            screenXStart = 0;
            frameCacheStart = (NES_SCREEN_WIDTH - SCREEN_RES_HOR) / 2;
            linePixels = SCREEN_RES_HOR;
        }
    }
    else
    {
        // 屏幕更大的情况，仍然显示在中间
        screenXStart = (SCREEN_RES_HOR - NES_SCREEN_WIDTH) / 2;
        frameCacheStart = 0;
        linePixels = NES_SCREEN_WIDTH;
    }
    if (SCREEN_RES_VER < NES_SCREEN_HEIGHT)
    {
        // 屏幕高度小于NES默认高度240
        frameHeight = SCREEN_RES_VER;
        screenYStart = 0;
        frameCacheRowOffset = (NES_SCREEN_HEIGHT - SCREEN_RES_VER) / 2;
    }
    else
    {
        // 屏幕高度大于等于NES默认高度，刷新在中间
        frameHeight = NES_SCREEN_HEIGHT;
        screenYStart = (SCREEN_RES_VER - NES_SCREEN_HEIGHT) / 2;
        frameCacheRowOffset = 0;
    }
    uint32_t taskNotifyCommand = 0;
    BaseType_t xResult;
    while (1)
    {
        xResult = xTaskNotifyWait(0x00, ULONG_MAX, &taskNotifyCommand, portMAX_DELAY);
        if (xResult == pdPASS)
        {
            // eggfly modify this
            lcd_write_frame(taskNotifyCommand);
        }
    }
}