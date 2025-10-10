#ifndef RS485_H
#define RS485_H

#include <Arduino.h>

/**
 * @brief Class to manage a single RS-485 bus with safe send/receive
 * 
 * Handles half-duplex communication, TX/RX control, 
 * and provides a simple way to send messages and wait for responses.
 */
class RS485Bus {
public:
    /**
     * @brief Construct a new RS485Bus object
     * 
     * @param serial Reference to the HardwareSerial port to use
     * @param rwPin Pin used to switch between read and write (TX/RX)
     * @param defaultTimeout Default timeout in milliseconds for waiting a response
     */
    RS485Bus(HardwareSerial &serial, uint8_t rwPin, unsigned long defaultTimeout = 100);

    /**
     * @brief Send a message over the RS-485 bus and wait for a response
     * 
     * This function ensures exclusive access to the bus, transmits the message,
     * switches to receive mode, and reads the response until the timeout expires.
     * 
     * @param msg The message to send
     * @param timeout Optional timeout in milliseconds (overrides defaultTimeout)
     * @return String The response received from the bus (empty if timeout)
     */
    String sendMessage(String msg, unsigned long timeout = 0);

    /**
     * @brief Send a message over the RS-485 bus without waiting for a response
     * 
     * This function ensures exclusive access to the bus and transmits the message,
     * but does not switch to receive mode or read any response.
     * Useful for broadcast or fire-and-forget messages.
     * 
     * @param msg The message to send
     */
    void sendMessageNoResponse(String msg);
    
    /**
     * @brief Checks if there are at least `minBytes` available on the RS485 bus.
     *
     * If the serial buffer contains at least `minBytes` bytes, it reads all of them
     * and returns the received string. Otherwise, it returns an empty string.
     *
     * @param minBytes Minimum number of bytes required before considering the message valid (default = 3).
     * @return A string containing the received data, or "" if there are not enough bytes available.
     *
     * @note This function is non-blocking: it returns immediately even if there are not enough bytes.
     */
    String checkMessage(size_t minBytes = 3, unsigned long timeout = 0);


    /**
     * @brief Clear the receive buffer of the RS-485 bus
     * 
     * This function discards any data currently available in the serial RX buffer.
     */
    void flush();
        
    void enableBus() { busEnabled = true; }
    void disableBus() { busEnabled = false; }
    bool isBusEnabled() const { return busEnabled; }

private:
    HardwareSerial &serial;             ///< Reference to the serial port used
    uint8_t rwPin;                      ///< Pin to control TX/RX direction
    unsigned long defaultTimeout;       ///< Default response timeout in ms
    bool busBusy;                       ///< Flag to indicate if the bus is currently in use
    bool busEnabled = true;             ///< Enable or Disable the RX e TX of bus

    /**
     * @brief Transmit a message on the bus
     * 
     * Sets the TX pin high, writes the message, waits for completion, and switches back to RX.
     * 
     * @param msg Message to transmit
     */
    void transmit(String msg);

    /**
     * @brief Receive a response from the bus with timeout
     * 
     * Reads all available characters from the serial port until the timeout expires.
     * 
     * @param timeout Maximum time to wait in milliseconds
     * @return String The received message
     */
    String receive(unsigned long timeout);

};

#endif
