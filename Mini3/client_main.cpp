#include <iostream>
#include <stdexcept>
#include <unistd.h>

#include "client.h"

using namespace std;

std::vector<std::string> split_cin_input(std::string input)
{
    std::string word;
    std::vector<std::string> splited_input;
    std::stringstream s(input);
    while(s >> word){
        splited_input.push_back(word);
    }
    return splited_input;
}

int main(int argc, char** argv) {
    // if (argc > 3)
    //     throw runtime_error("usage: [PORT; default 9000]");

    uint16_t port = 9000;
    string name;
    if (argc == 3){
        port = stoi(argv[1]);
        name = string(argv[2]);
    }
    UserClient client(port, name);
    client.connect_to_server();
    int fd = client.get_fd();
    while(true){
        fd_set read_fds;
        FD_ZERO(&read_fds);
        // FD_SET(fd,&read_fds);
        FD_SET(STDIN_FILENO,&read_fds);
        
        auto tv = timeval{3,0};
        auto ret = select(STDERR_FILENO,&read_fds,NULL,NULL,&tv);
        if(ret < 0)
            throw runtime_error("select failed");
        if(ret == 0)
            client.receive_message();
        if(FD_ISSET(STDIN_FILENO,&read_fds)){
            string input;
            getline(cin,input);
            vector<string> inputs = split_cin_input(input);
            int size = inputs.size();
            if(size == 1){
                if(inputs[0] == "exit")
                    return EXIT_SUCCESS;
                if(inputs[0] == "list")
                    client.get_list();
            }
            else{
                if(inputs[0] == "send"){
                    input.erase(0,inputs[0].length()+inputs[1].length()+2);
                    client.send_message(client.get_id(inputs[1]),input);
                }
                else
                    throw runtime_error("bad command");
            }
        }
    }
    return EXIT_SUCCESS;
}
