// Copyright (c) 2017 Timo Kröger <timokroeger93+code@gmail.com>

#include "modbus_serial.h"

#include "stm32g0xx_ll_usart.h"

void ModbusSerial::Init() {
  LL_USART_InitTypeDef usart_init{
      .PrescalerValue = LL_USART_PRESCALER_DIV1,
      .BaudRate = 19200,
      .DataWidth = LL_USART_DATAWIDTH_9B,
      .StopBits = LL_USART_STOPBITS_1,
      .Parity = LL_USART_PARITY_EVEN,
      .TransferDirection = LL_USART_DIRECTION_TX_RX,
      .HardwareFlowControl = LL_USART_HWCONTROL_NONE,
      .OverSampling = LL_USART_OVERSAMPLING_16,
  };
  LL_USART_Init(usart_, &usart_init);
  LL_USART_SetRxTimeout(usart_, 17);  // ~1.5 character times
  LL_USART_EnableRxTimeout(usart_);
  LL_USART_EnableDEMode(usart_);
}

void ModbusSerial::Enable() {
  // Enable peripheral and interrupts
  SET_BIT(usart_->CR1,
          USART_CR1_RTOIE | USART_CR1_RXNEIE_RXFNEIE | USART_CR1_UE);
}

void ModbusSerial::Disable() { CLEAR_BIT(usart_->CR1, USART_CR1_UE); }

void ModbusSerial::Send(const uint8_t* data, size_t length) {
  tx_data_ = data;
  tx_data_end_ = data + length;
  SET_BIT(usart_->CR1, USART_CR1_TXEIE_TXFNFIE);
}

bool ModbusSerial::tx_active() const { return !(usart_->ISR & USART_ISR_TC); }

void ModbusSerial::Isr() {
  uint32_t flags = usart_->ISR;

  // Receiving
  if (flags & USART_ISR_RXNE_RXFNE) {
    // Discard the 9th bit which is the received parity.
    uint8_t rxdata = static_cast<uint8_t>(usart_->RDR & USART_RDR_RDR);

    // Check and clear all possible erros: Noise, Framing and Parity
    bool data_bad =
        static_cast<bool>(flags & (USART_ISR_NE | USART_ISR_FE | USART_ISR_PE));
    SET_BIT(usart_->ICR, USART_ICR_NECF | USART_ICR_FECF | USART_ICR_PECF);

    rtu_->Notify(RxByte{rxdata, !data_bad});
  }

  // Sending
  if (flags & USART_ISR_TXE_TXFNF) {
    assert(tx_data_ < tx_data_end_);
    usart_->TDR = *tx_data_;

    tx_data_++;
    if (tx_data_ >= tx_data_end_) {
      CLEAR_BIT(usart_->CR1, USART_CR1_TXEIE_TXFNFIE);
      rtu_->Notify(TxDone{});
    }
  }

  // Idle timeout
  if (flags & USART_ISR_RTOF) {
    SET_BIT(usart_->ICR, USART_ICR_RTOCF);
    rtu_->Notify(BusIdle{});
  }
}
