#include <bits/stdc++.h>
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h> 
#include "json.hpp"

using namespace std;

#define BUFSIZE 10240
const string TEMP_FILENAME = "tmp.txt";
const string ERROR_FILENAME = "error.txt";

string temp_file_path = TEMP_FILENAME;
string error_file_path = ERROR_FILENAME;
string error = "";
class CommandNotFound {};



class Command {
public:
	string name_for_user;
	string name_in_protocol;
	int number_of_args;
	bool use_data_channel;

	Command(string _name_for_user, string _name_in_protocol, int _number_of_args, bool _use_data_channel) {
		name_for_user = _name_for_user;
		name_in_protocol = _name_in_protocol;
		number_of_args = _number_of_args;
		use_data_channel = _use_data_channel;
	}
};

class CommandRepository {
public:
	inline static vector<Command> commands;

	static void setup() {
		addCommand("user", "user", 1, false);
		addCommand("pass", "pass", 1, false);
		addCommand("quit", "quit", 0, false);
		// addCommand("pwd", "pwd", 0, false);
		// addCommand("mkd", "mkdir", 1, false);
		// addCommand("dele", "rm -r", 1, false);
		// addCommand("ls", "ls", 0, true);
		addCommand("retr", "cat", 1, true);
		// addCommand("cwd", "cd", 1, false);
		// addCommand("rename", "mv", 2, false);
		addCommand("help", "help", 0, false);
		addCommand("upload", "find ~/ -type f -name ", 1, false);
	}

	static void addCommand(string name_for_user, string name_in_protocol, int number_of_args, bool use_data_channel) {
		commands.push_back(Command(name_for_user, name_in_protocol, number_of_args, use_data_channel));
	}

	static Command findCommandByNameForUser(string name_for_user) {
		for (Command command : commands) {
			if (command.name_for_user == name_for_user) {
				return command;
			}
		}
		throw CommandNotFound();
	}
};



int connect_to_server(int port) { 
    int sock_fd;
    struct sockaddr_in serv_addr; 
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		perror("Socket creation"); 
		exit(EXIT_FAILURE); 
	} 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) { 
		perror("\nInvalid address/ Address not supported \n"); 
		exit(EXIT_FAILURE); 
	} 
	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
		perror("\nConnection Failed \n"); 
		exit(EXIT_FAILURE);  
	} 
    return sock_fd;
}


void custom_send(string message, int fd) {
    send(fd, message.c_str(), message.size(), 0);
}

string custom_read(int fd) {
    char buffer[BUFSIZE] = {0};
    read(fd, buffer, BUFSIZE);
    string message(buffer);
    return message;
}

string run_in_shell(string command) {
    error = "";
    string final_command = command + " > " + temp_file_path +  " 2> " + error_file_path;
    
    system(final_command.c_str());

    ifstream tmp(temp_file_path);    
    string ret((istreambuf_iterator<char>(tmp)), istreambuf_iterator<char>());
    tmp.close();
    system(("rm " + temp_file_path).c_str());
    
    ifstream err(error_file_path);    
    string e_ret((istreambuf_iterator<char>(err)), istreambuf_iterator<char>());
    err.close();
    error = e_ret.substr(0, e_ret.size() - 1);
    return ret.substr(0, ret.size()-1);
}
inline bool is_return(const char& input)
{
    return input == '\n' || input == '\r';
}

string last_line (const string& input)
{
    if(input.length() == 1) return input;
    size_t position = input.length()-2; // last character might be a return character, we can jump over it anyway
    while((not is_return(input[position])) and position > 0) position--;
    // now we are at the \n just before the last line, or at the first character of the string
    if(is_return(input[position])) position += 1;
    // now we are at the beginning of the last line

    return input.substr(position);
}

int main(int argc, char const *argv[]) { 

	CommandRepository::setup();

    ifstream file("config.json");
    nlohmann::json port = nlohmann::json::parse(file);
    int command_port = port["commandChannelPort"];
    int data_port = port["dataChannelPort"];
    file.close();

	int command_fd = connect_to_server(command_port);
    int data_fd = connect_to_server(data_port);
	cout << "connected to server\n";

	string command_name_for_user, arg, tmp;
	while (command_name_for_user != "quit") {
		cout << "\n$ ";
		cin >> command_name_for_user;

		if (command_name_for_user == "dele") {
			cin >> tmp; // only command with 2 words
		}
		
		try {
			Command command = CommandRepository::findCommandByNameForUser(command_name_for_user);
			string command_in_protocol = command.name_in_protocol;
			cout << "here 165" << endl;
			for (int i = 0; i < command.number_of_args; i++) {
				cin >> arg;
				command_in_protocol += " " + arg;
			}
			cout << "here 170" << endl;

			if(command.name_for_user == "upload"){
				string result = run_in_shell(command_in_protocol);
				string fileToUploadPath = last_line(result);
				cout << command_in_protocol << endl;
				cout << fileToUploadPath << endl;
				custom_send(command_name_for_user, command_fd);
				custom_send(fileToUploadPath, data_fd);
				cout << custom_read(command_fd) << '\n';
				
				continue;
			}
			cout << "here 183" << endl;

			custom_send(command_in_protocol, command_fd);
			cout << "here 186" << endl;

			cout << custom_read(command_fd) << '\n';
			cout << "here 189" << endl;

			if (command.use_data_channel) {
				cout << custom_read(data_fd) << '\n';
			}
		} catch (CommandNotFound) {
			getline(cin, tmp);
			cout << "command is not defined in protocol. you can use help command.\n";
		}
	} 

	close(command_fd);
	close(data_fd);
} 
