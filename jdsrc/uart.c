#include "jdsimple.h"

#define USART_IDX 1

#if USART_IDX == 1
#define USARTx USART1
#define IRQn USART1_IRQn
#define IRQHandler USART1_IRQHandler
#define LL_DMAMUX_REQ_USARTx_RX LL_DMAMUX_REQ_USART1_RX
#define LL_DMAMUX_REQ_USARTx_TX LL_DMAMUX_REQ_USART1_TX
#elif USART_IDX == 2
#define USARTx USART2
#define IRQn USART2_IRQn
#define IRQHandler USART2_IRQHandler
#define LL_DMAMUX_REQ_USARTx_RX LL_DMAMUX_REQ_USART2_RX
#define LL_DMAMUX_REQ_USARTx_TX LL_DMAMUX_REQ_USART2_TX
#else
#error "bad usart"
#endif

#if USART_IDX == 1
#define PIN_PORT GPIOB
#define PIN_PIN LL_GPIO_PIN_6
#define PIN_AF LL_GPIO_AF_0
#define PIN_MODER (1 << 6 * 2)
#elif USART_IDX == 2
#define PIN_PORT GPIOA
#define PIN_PIN LL_GPIO_PIN_2
#define PIN_AF LL_GPIO_AF_1
#define PIN_MODER (1 << 2 * 2)
#else
// generic
#error "bad usart"
#endif

#ifdef STM32G0
#define DMA_IRQn DMA1_Ch4_5_DMAMUX1_OVR_IRQn
#define DMA_Handler DMA1_Ch4_5_DMAMUX1_OVR_IRQHandler
#else
#define DMA_IRQn DMA1_Channel4_5_IRQn
#define DMA_Handler DMA1_Channel4_5_IRQHandler
#endif

// DMA Channel 4 - TX
// DMA Channel 5 - RX

// This is extracted to a function to make sure the compiler doesn't
// insert stuff between checking the input pin and setting the mode.
// This results in a collision window of around 90ns
__attribute__((noinline)) void gpio_probe_and_set(GPIO_TypeDef *gpio, uint32_t pin, uint32_t mode) {
    if (gpio->IDR & pin)
        gpio->MODER = mode;
}

static void uartOwnsPin(int doesIt) {
    if (doesIt) {
        LL_GPIO_SetPinMode(PIN_PORT, PIN_PIN, LL_GPIO_MODE_ALTERNATE);
        LL_GPIO_SetAFPin_0_7(PIN_PORT, PIN_PIN, PIN_AF);
    } else {
        LL_GPIO_SetPinMode(PIN_PORT, PIN_PIN, LL_GPIO_MODE_INPUT);
        exti_clear(PIN_PIN);
        exti_enable(PIN_PIN);
    }
}

void uart_disable() {
    LL_DMA_ClearFlag_GI5(DMA1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);

    LL_DMA_ClearFlag_GI4(DMA1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);

    LL_USART_Disable(USARTx);

    uartOwnsPin(0);
    // pulse_log_pin();
}

void DMA_Handler(void) {
    uint32_t isr = DMA1->ISR;

    // DMESG("DMA irq %x", isr);

    if (isr & (DMA_ISR_TCIF5 | DMA_ISR_TEIF5)) {
        uart_disable();
        if (isr & DMA_ISR_TCIF5) {
            // overrun?
            DMESG("USARTx RX OK, but how?!");
            jd_rx_completed(-1);
        } else {
            DMESG("USARTx RX Error");
            jd_rx_completed(-2);
        }
    }

    if (isr & (DMA_ISR_TCIF4 | DMA_ISR_TEIF4)) {
        LL_DMA_ClearFlag_GI4(DMA1);
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);

        int errCode = 0;
        if (isr & DMA_ISR_TCIF4) {
            while (!LL_USART_IsActiveFlag_TC(USARTx))
                ;
            LL_USART_RequestBreakSending(USARTx);
            // DMESG("USARTx %x", USARTx->ISR);
            while (LL_USART_IsActiveFlag_SBK(USARTx))
                ;
        } else {
            DMESG("USARTx TX Error");
            errCode = -1;
        }

        uart_disable();

        jd_tx_completed(errCode);
    }
}

static void DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();

    NVIC_SetPriority(DMA_IRQn, 0);
    NVIC_EnableIRQ(DMA_IRQn);
}

static void USART_UART_Init(void) {
    LL_USART_InitTypeDef USART_InitStruct = {0};

#if USART_IDX == 2
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
#elif USART_IDX == 1
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
#else
#error "bad usart"
#endif

    LL_GPIO_SetPinPull(PIN_PORT, PIN_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinSpeed(PIN_PORT, PIN_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(PIN_PORT, PIN_PIN, LL_GPIO_OUTPUT_PUSHPULL);
    uartOwnsPin(0);

#ifdef STM32F0
    LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART1TX_RMP_DMA1CH4 | LL_SYSCFG_USART1RX_RMP_DMA1CH5);
#endif

    /* USART_RX Init */
#ifdef STM32G0
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_5, LL_DMAMUX_REQ_USARTx_RX);
#endif
    LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_5,
                          LL_DMA_DIRECTION_PERIPH_TO_MEMORY | //
                              LL_DMA_PRIORITY_HIGH |          //
                              LL_DMA_MODE_NORMAL |            //
                              LL_DMA_PERIPH_NOINCREMENT |     //
                              LL_DMA_MEMORY_INCREMENT |       //
                              LL_DMA_PDATAALIGN_BYTE |        //
                              LL_DMA_MDATAALIGN_BYTE);

    /* USART_TX Init */
#ifdef STM32G0
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_4, LL_DMAMUX_REQ_USARTx_TX);
#endif
    LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_4,
                          LL_DMA_DIRECTION_MEMORY_TO_PERIPH | //
                              LL_DMA_PRIORITY_HIGH |          //
                              LL_DMA_MODE_NORMAL |            //
                              LL_DMA_PERIPH_NOINCREMENT |     //
                              LL_DMA_MEMORY_INCREMENT |       //
                              LL_DMA_PDATAALIGN_BYTE |        //
                              LL_DMA_MDATAALIGN_BYTE);

    /* USARTx interrupt Init */
    NVIC_SetPriority(IRQn, 0);
    NVIC_EnableIRQ(IRQn);

    /* Enable DMA transfer complete/error interrupts  */
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_4);
    LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_4);
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5);
    LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_5);

#ifdef LL_USART_PRESCALER_DIV1
    USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
#endif
    USART_InitStruct.BaudRate = 1000000;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USARTx, &USART_InitStruct);

#ifdef LL_USART_FIFOTHRESHOLD_1_8
    LL_USART_SetTXFIFOThreshold(USARTx, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetRXFIFOThreshold(USARTx, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_DisableFIFO(USARTx);
#endif
    LL_USART_ConfigHalfDuplexMode(USARTx);
    LL_USART_EnableIT_ERROR(USARTx);
    // LL_USART_EnableDMADeactOnRxErr(USARTx);

    // while (!(LL_USART_IsActiveFlag_REACK(USARTx)))
    //    ;
    exti_set_callback(PIN_PORT, PIN_PIN, jd_line_falling);
}

void uart_init() {
    DMA_Init();
    USART_UART_Init();
}

static void check_idle() {
#if 0
    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4))
        panic();
    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5))
        panic();
#endif
    if (LL_USART_IsEnabled(USARTx))
        panic();
}

void uart_wait_high() {
    while (!LL_GPIO_IsInputPinSet(PIN_PORT, PIN_PIN))
        ;
}

int uart_start_tx(const void *data, uint32_t numbytes) {
    exti_disable(PIN_PIN);
    exti_clear(PIN_PIN);
    // We assume EXTI runs at higher priority than us
    // If we got hit by EXTI, before we managed to disable it,
    // the reception routine would have enabled UART in RX mode
    if (LL_USART_IsEnabled(USARTx)) {
        // TX should never be already enabled though
        if (USARTx->CR1 & USART_CR1_TE)
            panic();
        // we don't re-enable EXTI - the RX complete will do it
        return -1;
    }
    LL_GPIO_ResetOutputPin(PIN_PORT, PIN_PIN);
    gpio_probe_and_set(PIN_PORT, PIN_PIN, PIN_MODER | PIN_PORT->MODER);
    if (!(PIN_PORT->MODER & PIN_MODER)) {
        set_log_pin5(1);
        set_log_pin5(0);
        exti_trigger(jd_line_falling);
        return -1;
    }
    wait_us(9);
    LL_GPIO_SetOutputPin(PIN_PORT, PIN_PIN);

    // from here...
    uartOwnsPin(1);
    LL_USART_DisableDirectionRx(USARTx);
    LL_USART_EnableDirectionTx(USARTx);
    USARTx->ICR = USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE; // clear error flags before we start
    LL_USART_Enable(USARTx);
    while (!(LL_USART_IsActiveFlag_TEACK(USARTx)))
        ;

    LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_4, (uint32_t)data, (uint32_t) & (USARTx->TDR),
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, numbytes);
    LL_USART_EnableDMAReq_TX(USARTx);
    // to here, it's about 1.3us

    // the USART takes a few us to start transmiting
    // this value gives 60us from the end of low pulse to start bit
    wait_us(57);

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);

    return 0;
}

void uart_start_rx(void *data, uint32_t maxbytes) {
    // DMESG("start rx");
    check_idle();

    exti_disable(PIN_PIN);
    exti_clear(PIN_PIN);

    uartOwnsPin(1);
    LL_USART_DisableDirectionTx(USARTx);
    LL_USART_EnableDirectionRx(USARTx);
    USARTx->ICR = USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE; // clear error flags before we start
    LL_USART_Enable(USARTx);
    while (!(LL_USART_IsActiveFlag_REACK(USARTx)))
        ;

    LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_5, (uint32_t) & (USARTx->RDR), (uint32_t)data,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, maxbytes);
    LL_USART_EnableDMAReq_RX(USARTx);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
}

// this is only enabled for error events
void IRQHandler(void) {
    //  pulse_log_pin();
    uint32_t dataLeft = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_5);
    uart_disable();
    jd_rx_completed(dataLeft);
}
