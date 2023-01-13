#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"
#include <string>
#include <sstream>


class UserClient {
public:
    UserClient(int port_, std::string name_);
    void read(uint8_t* buffer, size_t len);
    void write(const uint8_t* buffer, size_t len);
    void connect_to_server();
    void get_list();
    uint16_t get_id(std::string name);
    std::string get_info(uint16_t id);
    void send_message(uint16_t id, std::string message);
    void receive_message();
    int get_fd(){
        return fd;
    };

private:
    int message_id = 0;
    int fd;
    struct sockaddr_in server_address;
    bool connected = false;
    UserID id;
    std::string name;
    uint16_t port;
};