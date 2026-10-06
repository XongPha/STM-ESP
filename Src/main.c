/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : VL53L0X -> OLED -> UART1 (PA9, 74880 baud) to ESP8266
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint16_t current_distance = 0;
#define VL53L0X_ADDR 0x52
/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN 0 */
uint16_t VL53L0X_ReadDistance(void) {
    uint8_t cmd_start = 0x01;
    uint8_t dist_buffer[2];
    HAL_I2C_Mem_Write(&hi2c2, VL53L0X_ADDR, 0x00, 1, &cmd_start, 1, 100);
    HAL_Delay(30);
    if (HAL_I2C_Mem_Read(&hi2c2, VL53L0X_ADDR, 0x1E, 1, dist_buffer, 2, 100) == HAL_OK) {
        return (dist_buffer[0] << 8) | dist_buffer[1];
    }
    return 8190; // Giá trị out of range / lỗi đọc
}

void OLED_Draw_Frame(void) {
    ssd1306_Fill(Black);
    ssd1306_DrawRectangle(0, 0, 127, 63, White);
    ssd1306_SetCursor(14, 6);
    ssd1306_WriteString("LASER METER", Font_11x18, White);
    ssd1306_Line(4, 24, 123, 24, White);
    ssd1306_UpdateScreen();
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  ssd1306_Init();
  HAL_Delay(100);
  OLED_Draw_Frame();
  /* USER CODE END 2 */

  char str_buf[20];
  char uart_buf[16];

  while (1)
  {
    // 1. Đọc cảm biến
    current_distance = VL53L0X_ReadDistance();

    // 2. Truyền tất cả giá trị (kể cả out of range 8190) sang ESP8266
    int len = snprintf(uart_buf, sizeof(uart_buf), "%u\n", current_distance);
    HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, len, 100);

    // 3. Hiển thị lên màn hình OLED
    for (uint8_t y = 26; y < 62; y++) {
        for (uint8_t x = 4; x < 124; x++) {
            ssd1306_DrawPixel(x, y, Black);
        }
    }

    if (current_distance >= 8190) {
        ssd1306_SetCursor(18, 34);
        ssd1306_WriteString("Out Range", Font_11x18, White);
    } else {
        snprintf(str_buf, sizeof(str_buf), "%u mm", current_distance);
        uint8_t cursor_x = (current_distance < 100) ? 30 : ((current_distance < 1000) ? 25 : 20);
        ssd1306_SetCursor(cursor_x, 34);
        ssd1306_WriteString(str_buf, Font_11x18, White);
    }
    ssd1306_UpdateScreen();

    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(200); // 200ms gửi 1 gói tin
  }
}

// Cấu hình UART1 cố định ở 74880 baud
static void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 74880;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) { Error_Handler(); }
}

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}

static void MX_I2C1_Init(void) {
  hi2c1.Instance = I2C1; hi2c1.Init.ClockSpeed = 100000; hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2; hi2c1.Init.OwnAddress1 = 0; hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT; hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; hi2c1.Init.OwnAddress2 = 0; hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) { Error_Handler(); }
}

static void MX_I2C2_Init(void) {
  hi2c2.Instance = I2C2; hi2c2.Init.ClockSpeed = 100000; hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2; hi2c2.Init.OwnAddress1 = 0; hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT; hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; hi2c2.Init.OwnAddress2 = 0; hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK) { Error_Handler(); }
}

static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Error_Handler(void) {
  __disable_irq();
  while (1) { }
}