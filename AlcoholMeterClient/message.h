#ifndef MESSAGE_H
#define MESSAGE_H

#include <iostream>
#include <array>
#include <cstdint>
#include <cstring>
#include <QByteArray>

constexpr uint8_t mHeader           = 0xa0; // Fix header
constexpr uint8_t mWrite            = 0x01; // Write request
constexpr uint8_t mRead             = 0x02; // Read request
constexpr uint8_t mCalcVal0         = 0xa0;
constexpr uint8_t mCalcVal1         = 0xa1;
constexpr uint8_t mCalcVal2         = 0xa2;
constexpr uint8_t mCalcVal3         = 0xa3;
constexpr uint8_t mAdc0             = 0xb0;
constexpr uint8_t mAdc1             = 0xb1;
constexpr uint8_t mAdc2             = 0xb2;
constexpr uint8_t mAdc3             = 0xb3;
constexpr uint8_t mR0               = 0xb4;
constexpr uint8_t mStart            = 0xc0;
constexpr uint8_t mStop             = 0xc1;
constexpr uint8_t mCalibrate        = 0xc2;

constexpr size_t MaxPayload = 1024;  // Max payload size in bytes

struct MessagePack {
    uint8_t header;
    uint8_t len;
    uint8_t rw;
    uint8_t command;
    std::array<uint8_t, MaxPayload> data;
    uint16_t checksum;
};

class Message {
public:
    Message() = default;

    bool parse(uint8_t *dataUART, uint8_t size, MessagePack *message)
    {
        static uint16_t data_index=0;
        int16_t uart_index=-1;

        if (dataUART[0]!= mHeader) return false;

        message->header = dataUART[0];
        message->len = dataUART[1];
        message->rw = dataUART[2];
        message->command = dataUART[3];

        data_index=0;
        uart_index=3;

        while(data_index<(message->len)){
            uart_index++;
            if (uart_index==size) break;

            message->data[data_index] = (dataUART[uart_index]);
            data_index++;
        }

        return true;
    }

    uint16_t calculateChecksum(const uint8_t* data, size_t length) {
        uint16_t checksum = 0;
        for (size_t i = 0; i < length; ++i) {
            checksum += data[i];
        }
        return checksum;
    }

    // Header file part
    class MessageCreator {
    public:
        static constexpr int MaxPayload = 255;  // Maximum payload size

        // Constructor if needed
        MessageCreator(uint8_t header = 0xAA) : mHeader(header) {}

        QByteArray createMessage(uint8_t command, uint8_t rw, const QByteArray& payload);

    private:
        uint8_t mHeader;

        // Fixed calculateChecksum to use uint8_t pointer
        uint16_t calculateChecksum(const uint8_t* data, int size) {
            uint16_t sum = 0;
            for (int i = 0; i < size; i++) {
                sum += data[i];
            }
            return sum;
        }
    };

    // Implementation part
    QByteArray createMessage(uint8_t command, uint8_t rw, const QByteArray& payload) {
        if (payload.size() > MaxPayload) {
            std::cout << "Payload size exceeds maximum allowed size!" << std::endl;
            return QByteArray();
        }

        QByteArray message;
        // Reserve space for header + len + rw + command + payload + checksum
        message.reserve(payload.size() + 6);

        // Add header
        message.append(static_cast<char>(mHeader));

        // Add length (payload size + 2 for checksum)
        message.append(static_cast<char>(payload.size() + 2));

        // Add RW and command
        message.append(static_cast<char>(rw));
        message.append(static_cast<char>(command));

        // Add payload
        message.append(payload);

        // Calculate and add checksum
        // Cast the const char* to const uint8_t* for checksum calculation
        uint16_t checksum = calculateChecksum(
            reinterpret_cast<const uint8_t*>(message.constData()),
            message.size()
            );

        message.append(static_cast<char>(checksum & 0xFF));
        message.append(static_cast<char>((checksum >> 8) & 0xFF));

        return message;
    }

    bool parseMessage(QByteArray *data, uint8_t &command, QByteArray &value,  uint8_t &rw)
    {
        MessagePack parsedMessage;

        uint8_t* dataToParse = reinterpret_cast<uint8_t*>(data->data());
        QByteArray returnValue;

        if(parse(dataToParse, (uint8_t)data->length(), &parsedMessage))
        {
            command = parsedMessage.command;
            rw = parsedMessage.rw;

            for(int i = 0; i< parsedMessage.len; i++)
            {
                value.append(parsedMessage.data[i]);
            }

            return true;
        }

        return false;
    }

    static float bytesToFloat(const QByteArray& bytes) {
        if (bytes.size() < static_cast<int>(sizeof(float))) {
            return 0.0f;
        }
        float floatValue;
        memcpy(&floatValue, bytes.constData(), sizeof(float));
        return floatValue;
    }

    static QByteArray floatToBytes(float value) {
        QByteArray bytes;
        bytes.resize(sizeof(float));
        memcpy(bytes.data(), &value, sizeof(float));
        return bytes;
    }
};

#endif // MESSAGE_H
