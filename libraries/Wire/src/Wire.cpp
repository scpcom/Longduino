#include "Arduino.h"
#include "pins_arduino.h"
#include "Wire.h"

#define VARIANT_I2C_NUM 2

const i2c_dev_t I2C_MAP[VARIANT_I2C_NUM] {
    {I2C0, RCU_I2C0, PB7,  PB6,  PB5},
    {I2C1, RCU_I2C1, PB11 ,PB10, PB12},
};

#define I2C0_SPEED              100000
#define I2C0_SLAVE_ADDRESS7     0xA0

typedef enum
{
    I2C_START = 0,
    I2C_SEND_ADDRESS,
    I2C_CLEAR_ADDRESS_FLAG,
    I2C_TRANSMIT_DATA,
    I2C_STOP,
} i2c_process_enum;

#define I2C_TIME_OUT           (uint16_t)(5000)

#define I2C_OK                 1
#define I2C_FAIL               0
#define I2C_END                1

/*!
    \brief      reset i2c bus
    \param[in]  none
    \param[out] none
    \retval     none
*/
void i2c_bus_reset(const i2c_dev_t *_dev)
{
    /* configure SDA/SCL for GPIO */
    GPIO_BC(digitalPinToPort(_dev->scl)) |= digitalPinToBitMask(_dev->scl);
    GPIO_BC(digitalPinToPort(_dev->sda)) |= digitalPinToBitMask(_dev->sda);
    gpio_init(digitalPinToPort(_dev->scl), GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, digitalPinToBitMask(_dev->scl));
    gpio_init(digitalPinToPort(_dev->sda), GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, digitalPinToBitMask(_dev->sda));
    asm ("nop");
    asm ("nop");
    asm ("nop");
    asm ("nop");
    asm ("nop");
    GPIO_BOP(digitalPinToPort(_dev->scl)) |= digitalPinToBitMask(_dev->scl);
    asm ("nop");
    asm ("nop");
    asm ("nop");
    asm ("nop");
    asm ("nop");
    GPIO_BOP(digitalPinToPort(_dev->sda)) |= digitalPinToBitMask(_dev->sda);
    /* connect pin to I2Cx_SCL */
    gpio_init(digitalPinToPort(_dev->scl), GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, digitalPinToBitMask(_dev->scl));
    /* connect pin to I2Cx_SDA */
    gpio_init(digitalPinToPort(_dev->sda), GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, digitalPinToBitMask(_dev->sda));
}

/*!
    \brief      wait for EEPROM standby state use timeout function
    \param[in]  none
    \param[out] none
    \retval     none
*/
uint8_t i2c_wait_standby_state_timeout(const i2c_dev_t *_dev, uint16_t device_address)
{
    uint8_t   state = I2C_START;
    uint16_t  timeout = 0;
    uint8_t   i2c_timeout_flag = 0;

    while(!(i2c_timeout_flag)){
    switch(state){
        case I2C_START:
            /* i2c master sends start signal only when the bus is idle */
            while(i2c_flag_get(_dev->i2c_dev, I2C_FLAG_I2CBSY)&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                i2c_start_on_bus(_dev->i2c_dev);
                timeout = 0;
                state = I2C_SEND_ADDRESS;
            }
            else{
                i2c_bus_reset(_dev);
                timeout = 0;
                state = I2C_START;
                printf("i2c bus is busy!\n");
            }
            break;
        case I2C_SEND_ADDRESS:
            /* i2c master sends START signal successfully */
            while((! i2c_flag_get(_dev->i2c_dev, I2C_FLAG_SBSEND))&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                i2c_master_addressing(_dev->i2c_dev, device_address, I2C_TRANSMITTER);
                timeout = 0;
                state = I2C_CLEAR_ADDRESS_FLAG;
            }
            else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master sends start signal timeout in EEPROM standby!\n");
            }
            break;
        case I2C_CLEAR_ADDRESS_FLAG:
            while((!((i2c_flag_get(_dev->i2c_dev, I2C_FLAG_ADDSEND))||( i2c_flag_get(_dev->i2c_dev, I2C_FLAG_AERR))))&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                if(i2c_flag_get(_dev->i2c_dev, I2C_FLAG_ADDSEND)){
                    i2c_flag_clear(_dev->i2c_dev, I2C_FLAG_ADDSEND);
                    timeout = 0;
                    /* send a stop condition to I2C bus */
                    i2c_stop_on_bus(_dev->i2c_dev);
                    i2c_timeout_flag = I2C_OK;
                    /* exit the function */
                    return I2C_END;

                }else{
                    /* clear the bit of AE */
                    i2c_flag_clear(_dev->i2c_dev,I2C_FLAG_AERR);
                    timeout = 0;
                    state = I2C_STOP;
                }
            }
            else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master clears address flag timeout in EEPROM standby!\n");
            }
            break;
        case I2C_STOP:
            /* send a stop condition to I2C bus */
            i2c_stop_on_bus(_dev->i2c_dev);
            /* i2c master sends STOP signal successfully */
            while((I2C_CTL0(_dev->i2c_dev)&0x0200)&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                timeout = 0;
                state = I2C_START;
            }else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master sends stop signal timeout in EEPROM standby!\n");
            }
            break;
        default:
            state = I2C_START;
            timeout = 0;
            printf("i2c master sends start signal end in EEPROM standby!.\n");
            break;
        }
    }
    return I2C_END;

}

uint8_t i2c_stop_write(const i2c_dev_t *_dev)
{
    uint8_t   state = I2C_START;
    uint16_t  timeout = 0;

    /* send a stop condition to I2C bus */
    i2c_stop_on_bus(_dev->i2c_dev);
    /* i2c master sends STOP signal successfully */
    while((I2C_CTL0(_dev->i2c_dev)&0x0200)&&(timeout < I2C_TIME_OUT)){
        timeout++;
    }
    if(timeout < I2C_TIME_OUT){
        timeout = 0;
        state = I2C_END;
    }
    else{
        timeout = 0;
        state = I2C_START;
        //printf("i2c master sends stop signal timeout in WRITE!\n");
    }

    return state;
}

/*!
    \brief      write buffer of data use timeout function
    \param[in]  p_buffer: pointer to the buffer containing the data to be written
    \param[in]  number_of_byte: number of bytes to write
    \param[out] none
    \retval     none
*/
uint8_t i2c_buffer_write_timeout(const i2c_dev_t *_dev, uint16_t device_address, uint8_t* p_buffer, uint16_t number_of_byte)
{
    uint8_t   state = I2C_START;
    uint16_t  timeout = 0;
    uint8_t   i2c_timeout_flag = 0;
    while(!(i2c_timeout_flag)){
        switch(state){
        case I2C_START:
            /* i2c master sends start signal only when the bus is idle */
            while(i2c_flag_get(_dev->i2c_dev, I2C_FLAG_I2CBSY)&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                i2c_start_on_bus(_dev->i2c_dev);
                timeout = 0;
                state = I2C_SEND_ADDRESS;
            }
            else{
                i2c_bus_reset(_dev);
                timeout = 0;
                state = I2C_START;
                printf("i2c bus is busy in WRITE!\n");
            }
            break;
        case I2C_SEND_ADDRESS:
            /* i2c master sends START signal successfully */
            while((! i2c_flag_get(_dev->i2c_dev, I2C_FLAG_SBSEND))&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                i2c_master_addressing(_dev->i2c_dev, device_address, I2C_TRANSMITTER);
                timeout = 0;
                state = I2C_CLEAR_ADDRESS_FLAG;
            }
            else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master sends start signal timeout in WRITE!\n");
            }
            break;
        case I2C_CLEAR_ADDRESS_FLAG:
            /* address flag set means i2c slave sends ACK */
            while((! i2c_flag_get(_dev->i2c_dev, I2C_FLAG_ADDSEND))&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                i2c_flag_clear(_dev->i2c_dev, I2C_FLAG_ADDSEND);
                timeout = 0;
                state = I2C_TRANSMIT_DATA;
            }
            else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master clears address flag timeout in WRITE!\n");
            }
            break;
        case I2C_TRANSMIT_DATA:
            /* wait until the transmit data buffer is empty */
            while((! i2c_flag_get(_dev->i2c_dev, I2C_FLAG_TBE))&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                timeout = 0;
            }
            else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master sends data timeout in WRITE!\n");
            }
            while(number_of_byte--){
                i2c_data_transmit(_dev->i2c_dev, *p_buffer);
                /* point to the next byte to be written */
                p_buffer++;
                /* wait until BTC bit is set */
                while((!i2c_flag_get(_dev->i2c_dev, I2C_FLAG_BTC))&&(timeout < I2C_TIME_OUT)){
                    timeout++;
                }
                if(timeout < I2C_TIME_OUT){
                    timeout = 0;
                }
                else{
                    timeout = 0;
                    state = I2C_START;
                    printf("i2c master sends data timeout in WRITE!\n");
                }
            }
            timeout = 0;
            state = I2C_END;
            i2c_timeout_flag = I2C_OK;
            break;
        case I2C_STOP:
            /* send a stop condition to I2C bus */
            i2c_stop_on_bus(_dev->i2c_dev);
            /* i2c master sends STOP signal successfully */
            while((I2C_CTL0(_dev->i2c_dev)&0x0200)&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                timeout = 0;
                state = I2C_END;
                i2c_timeout_flag = I2C_OK;
            }
            else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master sends stop signal timeout in WRITE!\n");
            }
            break;
        default:
            state = I2C_START;
            i2c_timeout_flag = I2C_OK;
            timeout = 0;
            printf("i2c master sends start signal in WRITE.\n");
            break;
        }
    }
    return I2C_END;
}

/*!
    \brief      read data
    \param[in]  p_buffer: pointer to the buffer that receives the data read
    \param[in]  number_of_byte: number of bytes to reads
    \param[out] none
    \retval     none
*/
uint8_t i2c_buffer_read_timeout(const i2c_dev_t *_dev, uint16_t device_address, uint8_t* p_buffer, uint16_t number_of_byte)
{
    uint8_t   state = I2C_START;
    uint16_t  timeout = 0;
    uint8_t   i2c_timeout_flag = 0;
    while(!(i2c_timeout_flag)){
        switch(state){
        case I2C_START:
                /* i2c master sends start signal only when the bus is idle */
                while(i2c_flag_get(_dev->i2c_dev, I2C_FLAG_I2CBSY)&&(timeout < I2C_TIME_OUT)){
                    timeout++;
                }
                if(timeout < I2C_TIME_OUT){
                    /* whether to send ACK or not for the next byte */
                    if(2 == number_of_byte){
                        i2c_ackpos_config(_dev->i2c_dev,I2C_ACKPOS_NEXT);
                    }
                }else{
                    i2c_bus_reset(_dev);
                    timeout = 0;
                    state = I2C_START;
                    //printf("i2c bus is busy in READ!\n");
                }
            /* send the start signal */
            i2c_start_on_bus(_dev->i2c_dev);
            timeout = 0;
            state = I2C_SEND_ADDRESS;
            break;
        case I2C_SEND_ADDRESS:
            /* i2c master sends START signal successfully */
            while((! i2c_flag_get(_dev->i2c_dev, I2C_FLAG_SBSEND))&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                    i2c_master_addressing(_dev->i2c_dev, device_address, I2C_RECEIVER);
                    if(number_of_byte < 3){
                        /* disable acknowledge */
                        i2c_ack_config(_dev->i2c_dev,I2C_ACK_DISABLE);
                    }
                    state = I2C_CLEAR_ADDRESS_FLAG;
                timeout = 0;
            }else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master sends start signal timeout in READ!\n");
            }
            break;
        case I2C_CLEAR_ADDRESS_FLAG:
            /* address flag set means i2c slave sends ACK */
            while((!i2c_flag_get(_dev->i2c_dev, I2C_FLAG_ADDSEND))&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                i2c_flag_clear(_dev->i2c_dev, I2C_FLAG_ADDSEND);
                if(1 == number_of_byte){
                    /* send a stop condition to I2C bus */
                    i2c_stop_on_bus(_dev->i2c_dev);
                }
                timeout = 0;
                state   = I2C_TRANSMIT_DATA;
            }else{
                timeout = 0;
                state   = I2C_START;
                printf("i2c master clears address flag timeout in READ!\n");
            }
            break;
        case I2C_TRANSMIT_DATA:
                while(number_of_byte){
                    timeout++;
                    if(3 == number_of_byte){
                        /* wait until BTC bit is set */
                        while(!i2c_flag_get(_dev->i2c_dev, I2C_FLAG_BTC));
                        /* disable acknowledge */
                        i2c_ack_config(_dev->i2c_dev,I2C_ACK_DISABLE);
                    }
                    if(2 == number_of_byte){
                        /* wait until BTC bit is set */
                        while(!i2c_flag_get(_dev->i2c_dev, I2C_FLAG_BTC));
                        /* send a stop condition to I2C bus */
                        i2c_stop_on_bus(_dev->i2c_dev);
                    }
                    /* wait until RBNE bit is set */
                    if(i2c_flag_get(_dev->i2c_dev, I2C_FLAG_RBNE)){
                        /* read a byte */
                        *p_buffer = i2c_data_receive(_dev->i2c_dev);

                        /* point to the next location where the byte read will be saved */
                        p_buffer++;

                        /* decrement the read bytes counter */
                        number_of_byte--;
                        timeout = 0;
                    }
                    if(timeout > I2C_TIME_OUT){
                        timeout = 0;
                        state = I2C_START;
                        printf("i2c master sends data timeout in READ!\n");
                    }
                }
            timeout = 0;
            state = I2C_STOP;
            break;
        case I2C_STOP:
            /* i2c master sends STOP signal successfully */
            while((I2C_CTL0(_dev->i2c_dev)&0x0200)&&(timeout < I2C_TIME_OUT)){
                timeout++;
            }
            if(timeout < I2C_TIME_OUT){
                timeout = 0;
                state = I2C_END;
                i2c_timeout_flag = I2C_OK;
            }else{
                timeout = 0;
                state = I2C_START;
                printf("i2c master sends stop signal timeout in READ!\n");
            }
            break;
        default:
            state = I2C_START;
            i2c_timeout_flag = I2C_OK;
            timeout = 0;
            printf("i2c master sends start signal in READ.\n");
            break;
        }
    }
    return I2C_END;
}

TwoWire::TwoWire(i2c_device_number_t i2c_device)
{
    i2c_tx_buff = 0;
    i2c_rx_buff = 0;
    _dev = NULL;
    _need_stop = false;

    if (i2c_device >= VARIANT_I2C_NUM)
      return;

    _i2c_num = i2c_device;
    _dev = &I2C_MAP[_i2c_num];
}

TwoWire::~TwoWire()
{
    //clear
}

void 
TwoWire::begin(uint8_t sda, uint8_t scl, uint32_t frequency)
{
    if (!_dev)
        return;

    /* enable GPIO clock(s) */
    rcu_periph_clock_enable(digitalPinToClkid(_dev->scl));
    rcu_periph_clock_enable(digitalPinToClkid(_dev->sda));
    
    /* connect pin to I2Cx_SCL */
    gpio_init(digitalPinToPort(_dev->scl), GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, digitalPinToBitMask(_dev->scl));
    /* connect pin to I2Cx_SDA */
    gpio_init(digitalPinToPort(_dev->sda), GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, digitalPinToBitMask(_dev->sda));

    /* enable I2C clock */
    rcu_periph_clock_enable(_dev->clk_id);
    /* configure I2C clock */
    i2c_clock_config(_dev->i2c_dev,I2C0_SPEED,I2C_DTCY_2);
    /* configure I2C address */
    i2c_mode_addr_config(_dev->i2c_dev,I2C_I2CMODE_ENABLE,I2C_ADDFORMAT_7BITS,I2C0_SLAVE_ADDRESS7);
    /* enable I2C0 */
    i2c_enable(_dev->i2c_dev);
    /* enable acknowledge */
    i2c_ack_config(_dev->i2c_dev,I2C_ACK_ENABLE);

    is_master_mode = true;
    
    delete i2c_tx_buff;
    delete i2c_rx_buff;
    i2c_tx_buff = new RingBuffer();
    i2c_rx_buff = new RingBuffer();

    setClock(frequency);
}
    
void 
TwoWire::begin(uint16_t slave_address, uint8_t sda, uint8_t scl)
{
    // todo

    is_master_mode = false;

    delete i2c_tx_buff;
    delete i2c_rx_buff;
    i2c_tx_buff = new RingBuffer();
    i2c_rx_buff = new RingBuffer();
}

void
TwoWire::setTimeOut(uint16_t timeOutMillis)
{
    _timeOutMillis = timeOutMillis;
}

uint16_t 
TwoWire::getTimeOut()
{
    return _timeOutMillis;
}

void 
TwoWire::setClock(uint32_t frequency)
{
    i2c_clk = frequency;

    // todo
}

uint32_t 
TwoWire::getClock()
{
    return i2c_clk;
}

int
TwoWire::writeTransmission(uint16_t address, uint8_t* send_buf, size_t send_buf_len, bool sendStop)
{
    i2c_buffer_write_timeout(_dev, address<<1, send_buf, send_buf_len);

    return 0;
}

int
TwoWire::readTransmission(uint16_t address, uint8_t* receive_buf, size_t receive_buf_len, bool sendStop)
{ 
    i2c_buffer_read_timeout(_dev, address<<1, receive_buf, receive_buf_len);
    
    return 0;
}

void 
TwoWire::beginTransmission(uint16_t address)
{
    if (_need_stop) {
        i2c_stop_write(_dev);
        i2c_wait_standby_state_timeout(_dev,txAddress<<1);
        _need_stop = false;
    }

    /* enable acknowledge */
    i2c_ack_config(_dev->i2c_dev,I2C_ACK_ENABLE);
    // Clear buffers when new transation/packet starts
    flush();

    transmitting = 1;
    txAddress = address;
}

uint8_t 
TwoWire::endTransmission(bool sendStop)  //结束时从rxbuff发送数据？
{
    int state = -1;
    int index = 0;
    uint8_t temp = 0;
    size_t tx_data_length = i2c_tx_buff->available();
    if(tx_data_length == 0){
        state = readTransmission(txAddress, &temp, 1, sendStop);
        return state;
    }
    uint8_t tx_data[RING_BUFFER_SIZE];
    while(i2c_tx_buff->available())
    {
        tx_data[index++] = i2c_tx_buff->read_char();
    }
    _need_stop = true;
    state = writeTransmission(txAddress, tx_data, tx_data_length,sendStop);
    return state;
}

uint8_t
TwoWire::requestFrom(uint16_t address, uint8_t size, bool sendStop)  //请求数据，存入rxbuff，供read读
{
    // Clear buffers when new transation/packet starts
    flush();

    int state,index = 0;
    uint8_t rx_data[RING_BUFFER_SIZE];
    state = readTransmission(address, rx_data, size, sendStop);
    if(0 == state){
        while(index < size)
        {
            i2c_rx_buff->store_char(rx_data[index++]); 
        }
        return size;
    }
    return 0;
}

size_t 
TwoWire::write(uint8_t data) //写到txbuff
{
    if(transmitting && !i2c_tx_buff->isFull()) {
        i2c_tx_buff->store_char(data);
        return 1;
    }
    return 0;
}

size_t 
TwoWire::write(const uint8_t *data, int quantity)
{
    for(size_t i = 0; (int)i < quantity; i++) {
        if(!write(data[i])) {
            return i;
        }
    }
    return quantity;
}

int TwoWire::available(void)   //rxbuff.available
{
    return i2c_rx_buff->available();
}

int TwoWire::read(void)    //rxbuff.read
{
    return i2c_rx_buff->read_char();
}

int TwoWire::peek(void)    
{ 
    return i2c_rx_buff->peek();
}

void TwoWire::flush(void)
{
    i2c_rx_buff->clear();
    i2c_tx_buff->clear();
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(quantity), static_cast<bool>(sendStop));
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t quantity, uint8_t sendStop)
{
    return requestFrom(address, static_cast<size_t>(quantity), static_cast<bool>(sendStop));
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(quantity), true);
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t quantity)
{
    return requestFrom(address, static_cast<size_t>(quantity), true);
}

uint8_t TwoWire::requestFrom(int address, int quantity)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(quantity), true);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop)
{
    return static_cast<uint8_t>(requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(quantity), static_cast<bool>(sendStop)));
}

void TwoWire::beginTransmission(int address)
{
    beginTransmission(static_cast<uint16_t>(address));
}

void TwoWire::beginTransmission(uint8_t address)
{
    beginTransmission(static_cast<uint16_t>(address));
}

uint8_t TwoWire::endTransmission(void)
{
    return endTransmission(true);
}

bool 
TwoWire::busy(void){ 
    return false;
}

void
TwoWire::scan(){
    uint8_t temp;
    for (int addr = 0x08; addr < 0x78; ++addr) {
        // int ret = i2c_p->writeto(self, addr, NULL, 0, true);
        // printf("find %x\n",addr);
        int ret = readTransmission(addr,&temp, 1, 1);
        // printf("ret:%x\n",ret);
        if (ret == 0) {
            Serial.print("SCAN Find device:");
            Serial.println(addr,HEX);
        }
    }
}

TwoWire Wire = TwoWire(I2C_DEVICE_0);
TwoWire Wire1 = TwoWire(I2C_DEVICE_1);
