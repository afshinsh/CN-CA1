#include "client.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <thread>
#include <tuple>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "message.h"

using namespace std;

#define MAX_PAYLOAD_LENGTH 10000

UserClient::UserClient(int port_, string name_){
    port = port_;
    name = name_;
}

void UserClient::connect_to_server() {
    Header header;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) { // checking for errors
        throw runtime_error("Error in connecting to server");
    }
    header.message_id = message_id;
    message_id++;
    header.message_type = CONNECT;
    header.length = sizeof(header)+name.length();
    write((uint8_t*)&header,sizeof(header));
    write((uint8_t*)name.c_str(), name.length());
    Header reply_header;
    read((uint8_t*)&reply_header, sizeof(reply_header));
    if(reply_header.message_type == CONNACK)
        clog << "connected" << endl;
}

string UserClient::get_info(uint16_t id) {
    Header header;
    header.message_id = message_id;
    message_id++;
    header.message_type = INFO;
    header.length = sizeof(header) + sizeof(uint16_t);
    id = htons(id);
    write((uint8_t*)&header,sizeof(header));
    write((uint8_t*)&id, sizeof(id));
    Header reply_header;
    read((uint8_t*)&reply_header, sizeof(reply_header));
    auto payload_length = reply_header.length - sizeof(reply_header);
    if (payload_length < 0)
        throw runtime_error("negative payload length");
    uint8_t payload[payload_length + 1];
    read(payload, payload_length);
    payload[payload_length] = 0;
    string user_name = string((char*)payload);
    return user_name;
}

void UserClient::get_list() {
    Header header;
    header.message_id = message_id;
    message_id++;
    header.message_type = LIST;
    header.length = sizeof(header);
    write((uint8_t*)&header,sizeof(header));
    Header reply_header;
    read((uint8_t*)&reply_header, sizeof(reply_header));
    auto payload_length = reply_header.length - sizeof(reply_header);
    if (payload_length < 0)
        throw runtime_error("negative payload length");
    uint16_t ids[payload_length/2];
    for(int i = 0; i < payload_length/2; i++){
        uint16_t id;
        read((uint8_t *)&id,2);
        ids[i] = ntohs(id);
    }
    for(int i = 0; i < payload_length/2; i++){
        cout << " -" << this->get_info(ids[i]) << endl;
    }
}

uint16_t UserClient::get_id(string name) {
    Header header;
    header.message_id = message_id;
    message_id++;
    header.message_type = LIST;
    header.length = sizeof(header);
    write((uint8_t*)&header,sizeof(header));
    Header reply_header;
    read((uint8_t*)&reply_header, sizeof(reply_header));
    auto payload_length = reply_header.length - sizeof(reply_header);
    if (payload_length < 0)
        throw runtime_error("negative payload length");
    uint16_t ids[payload_length/2];
    for(int i = 0; i < payload_length/2; i++){
        uint16_t id;
        read((uint8_t *)&id,2);
        ids[i] = ntohs(id);
    }
    for(int i = 0; i < payload_length/2; i++){
        if (this->get_info(ids[i]) == name)
            return ids[i]; 
    }
    return 0;    
}

void UserClient::send_message(uint16_t id, string message) {
    if(id == 0)
        throw runtime_error("user not found");
    Header header;
    header.message_id = message_id;
    message_id++;
    header.message_type = SEND;
    id = htons(id);
    header.length = sizeof(header) + sizeof(uint16_t) + message.length();
    write((uint8_t*)&header,sizeof(header));
    write((uint8_t*)&id, sizeof(id));
    write((uint8_t*)message.c_str(), message.length());
    Header reply_header;
    read((uint8_t*)&reply_header, sizeof(reply_header));
    auto payload_length = reply_header.length - sizeof(reply_header);
    if (payload_length < 0)
        throw runtime_error("negative payload length");
    uint8_t payload;
    read(&payload, payload_length);
    if (!unsigned(payload))
        throw runtime_error("send failed");
}

void UserClient::receive_message() {
    Header header;
    header.message_id = message_id;
    message_id++;
    header.message_type = RECEIVE;
    header.length = sizeof(header);
    write((uint8_t*)&header,sizeof(header));
    Header reply_header;
    read((uint8_t*)&reply_header, sizeof(reply_header));
    auto payload_length = reply_header.length - sizeof(reply_header);
    uint16_t sender_id;
    read((uint8_t*)&sender_id,2);
    uint8_t message[payload_length-2];
    read(message,sizeof(message));
    sender_id = ntohs(sender_id);
    if (sender_id == 0){
        return;
    }
    message[payload_length-2] = 0;
    string message_string = string((char*)message);
    cout << this->get_info(sender_id) << ": " << message_string << endl;

}

void UserClient::read(uint8_t* buffer, size_t len) {
    auto n = 0;
    while (n < len) {
        auto ret = ::read(fd, buffer + n, len - n);
        if (ret < 0)
            throw runtime_error("failed to read");
        if (ret == 0)
            throw runtime_error("usr read socket closed");

        n += ret;
    }
}


void UserClient::write(const uint8_t* buffer, size_t len) {
    auto n = 0;
    while (n < len) {
        auto ret = ::write(fd, buffer + n, len - n);
        if (ret < 0)
            throw runtime_error("failed to write");
        if (ret == 0)
            throw runtime_error("user write socket closed");

        n += ret;
    }
}