#include <bits/stdc++.h>
#include <thread>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <time.h> 
#include "json.hpp"


using namespace std;

#define BUFSIZE 10240
#define MAXCLIENTS 20

const string TEMP_FILENAME = "tmp.txt";
const string ERROR_FILENAME = "error.txt";
const string help_description = "214\n\
USER [name], Its argument is used to specify the user’s string. It is used for user authentication.\n\
PASS [password], Its argument is used to specify the user’s password. It is used for user authentication.\n\
PWD, It is used for show  current directory \n\
MKD [directory path], It is used for creating new directory by its argument.\n\
DELE -f [filename], It is used for deleting file that is given in argument.\n\
DELE -d [directory path], It is used for deleting directory that is given in argument.\n\
LS, It is used for show content of current directory.\n\
CWD [path], It is used for changing current directory. New current directory is given in argument.\n\
RENAME [from] [to] , It is used for renaming file or directory. Its arguments show old name and new name.\n\
RETR [name] , It is used for downloadind data. Its argument shows target file.\n\
HELP, Shows details of commands. \n\
QUIT, It is used for exiting. ";
string server_absolute_path;
string temp_file_path = TEMP_FILENAME;
string error_file_path = ERROR_FILENAME;
string error = "";

class UserNotFound {};
class FileNotFound {};


class User {
public:
    string username;
    string password;
    bool is_admin;
    int remaining_size;
    User(string _username, string _password, string _is_admin, string _remaining_size) {
        username = _username;
        password = _password;
        is_admin = _is_admin == "true";
        remaining_size = stoi(_remaining_size);
    }
};

class UserRepository {
public:
    inline static vector<User> users; 
    
    static void addUsers() {
        ifstream file("config.json");
        nlohmann::json data = nlohmann::json::parse(file);
        data = data["users"];
        for(int i = 0; i < data.size(); i++)
            users.push_back(User(data[i]["user"], data[i]["password"], data[i]["admin"], data[i]["size"]));
        file.close();
      }

    static User findUserByName(string username) {
        for (User user : users) {
            if (user.username == username) {
                return user; 
            }
        }
        throw UserNotFound();
    }

};



class File {
public:
    string path;
    bool for_admin;
    File(string _path, bool _for_admin){
        path = _path;
        for_admin = _for_admin;
    }
};

class FileRepository {
public:
    inline static vector<File> files;

    static void addFiles() {
        ifstream file("config.json");
        nlohmann::json data = nlohmann::json::parse(file);
        data = data["files"];
        for(int i = 0; i < data.size(); i++)
            files.push_back(File(data[i], true));
        file.close();

    }

    static File findFileByPath(string path) {
        for (File file : files) {
            for(int i = 0; i < file.path.length(); i++)
                if (file.path == path) {
                    return file;
                }
        }
        throw FileNotFound();
    }

};


class Command {
public:
	string name;
    string failed_message;
    string success_message;
	int number_of_args;
	bool use_data_channel;

	Command(string _name, string _failed_message, string _success_message, int _number_of_args, bool _use_data_channel) {
		name = _name;
        failed_message = _failed_message;
        success_message = _success_message;
		number_of_args = _number_of_args;
		use_data_channel = _use_data_channel;
	}
};

class CommandRepository {
public:
	inline static vector<Command> commands;

	static void setup() {
		addCommand("user", "430: Invalid username or password.", "331: User name okey, need password.", 1, false);
		addCommand("pass", "430: Invalid username or password.", "230: User logged in, proceed. Logged out if appropriate.", 1, false);
		addCommand("quit", "", "221: Successful Quit.", 0, false);
		//addCommand("pwd", "550: File unavailble.", "257: ", 0, false); //
		//addCommand("mkdir", "550: File unavailble.", "257: ", 1, false); //
		//addCommand("rm", "550: File unavailble.", "250: ", 1, false); //
		//addCommand("ls", "", "226: List transfer done.", 0, true);
		addCommand("cat", "550: File unavailble.", "226:Successful Download.", 1, true);
		//addCommand("cd", "501: Syntax error in parameters or arguments.", "250: Successful change.", 1, false);
		//addCommand("mv", "550: File unavailble.", "250: Successful change.", 2, false);
		addCommand("help", "", help_description, 0, false);
		addCommand("upload", "550: File unavailble.", "226:Successful Upload.", 1, false);
	}

	static void addCommand(string name, string failed_message, string success_message, int number_of_args, bool use_data_channel) {
		commands.push_back(Command(name, failed_message, success_message, number_of_args, use_data_channel));
	}

	static Command findCommandByNameInProtocol(string name) {
        for (Command command : commands) {
			if (command.name == name) {
				return command;
			}
		}
        return Command("", "", "", 0, false);
	}
};




class CustomSocket {
public:
    int fd;
    int port;
    sockaddr_in address;
    int addrlen;

    CustomSocket(int port) {
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
            perror("socket failed"); 
            exit(EXIT_FAILURE); 
        } 
        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
            perror("setsockopt"); 
            exit(EXIT_FAILURE); 
        } 
        address.sin_family = AF_INET; 
        address.sin_addr.s_addr = INADDR_ANY; 
        address.sin_port = htons(port); 
        if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) { 
            perror("bind failed"); 
            exit(EXIT_FAILURE); 
        } 
        if (listen(fd, MAXCLIENTS) < 0) { 
            perror("listen"); 
            exit(EXIT_FAILURE); 
        } 

        addrlen = sizeof(address);
    }
};




void setup_logfile() {
    time_t system_time = time(NULL);
    ofstream logfile;
    logfile.open("log.txt", ofstream::app);
    string time = ctime(&system_time);
    logfile<<time.substr(0, time.length()-1)<<" --> server run"<<endl;
    logfile.close();
}

void logg(string what, string message, int fd) {
    time_t system_time = time(NULL);
    ofstream logfile;
    logfile.open("log.txt", ofstream::app);
    string time = ctime(&system_time);
    logfile<<time.substr(0, time.length()-1)<<" --> "<<what<<'\t'<<message << '\t' << fd<<endl;    
    logfile.close();
}



void custom_send(string message, int fd) {
    send(fd, message.c_str(), message.size(), 0);
    logg("send", message, fd);
}

string custom_read(int fd) {
    char buffer[BUFSIZE] = {0};
    read(fd, buffer, BUFSIZE);
    string message(buffer);
    logg("recieve", message, fd);
    return message;
}



User login(int fd) {
    string message, command_name;
    while (true)
    {
        cout << "fd: " << fd << endl;
        message = custom_read(fd);
        cout << "message: " << message << endl;
        command_name = message.substr(0, message.find(' '));
        message = message.substr(message.find(' ')+1, message.length());
        Command command = CommandRepository::findCommandByNameInProtocol(command_name);
        if(command.name == "user"){
            try
            {
                User user = UserRepository::findUserByName(message);
                cout << message << " 255" << endl;
                custom_send(command.success_message, fd);
                while(true){
                    message = custom_read(fd);
                    command_name = message.substr(0, message.find(' '));
                    message = message.substr(message.find(' ')+1, message.length());
                    command = CommandRepository::findCommandByNameInProtocol(command_name);
                        if(command.name == "user"){
                            user = UserRepository::findUserByName(message);
                            custom_send(command.success_message, fd);
                        }
                        else if(command.name == "pass"){
                            if(message == user.password){
                                custom_send(command.success_message, fd);
                                return user;
                            }
                            custom_send(command.failed_message, fd);
                        }
                        else
                            custom_send("332: Need account for login.", fd);
                }
            }
            catch(UserNotFound)
            {
                custom_send(command.failed_message, fd);
            } 
        }
        else if(command.name == "pass")
            custom_send("503: Bad sequence of commands.", fd);
        else
            custom_send("332: Need account for login.", fd);
    }
}

string run_in_shell(string command) {
    error = "";
    string final_command = command + " > " + temp_file_path +  " 2> " + error_file_path;
    logg("shell", final_command, -1);
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

void handle_client(int command_fd, int data_fd) {
    string message, command_name, current_path = server_absolute_path;
    User user = login(command_fd);

    while (true) {
        message = custom_read(command_fd) + ' ';
        command_name = message.substr(0, message.find(' '));
        Command command = CommandRepository::findCommandByNameInProtocol(command_name);
		cout << "here 314" << endl;

        if (command.name == "quit") {
            custom_send(command.success_message, command_fd);
            close(command_fd);
            close(data_fd);
            return;
        } else if (command.name == "help") {
            custom_send(command.success_message, command_fd);
        } else if(command.name == "upload"){
                string filePath = custom_read(data_fd);
                string copyCommand = "cp -i " + filePath + " /home/mohamadkazem/Desktop/files";
                cout << copyCommand << endl;
                system(copyCommand.c_str());
                custom_send(command.success_message, command_fd);
        } 
        else {            
			cout << "here 331" << endl;

            string result = run_in_shell("cd " + current_path + " && " + message);
            string command_result = command.success_message;
            if (command.name == "cat") {
                try {
                    FileRepository::findFileByPath(message.substr(message.find(' ')+1, message.length() - message.find(' ') - 2));
                    // if(!user.is_admin){
                    //     command_result = command.failed_message;
                    //     result = "";
                    // }
                }
                catch(FileNotFound) {
                }
                int data_size = sizeof(result) * result.length();
                if(data_size <= user.remaining_size){
                    user.remaining_size -= data_size;
                } else{
                    command_result = "425: Can't open data connection.";
                    result = "";
                }

            }
           
            if(error != "")
                command_result = "501: Syntax error in parameters or arguments.";
            custom_send(command_result, command_fd);
                
            if (command.name == "cat" || command.name == "ls") {
                custom_send(result + " ", data_fd); 
            }

        }
    }
}



int main(int argc, char const *argv[]) {
    setup_logfile();
    CommandRepository::setup();
    server_absolute_path = run_in_shell("pwd"); 
    temp_file_path = server_absolute_path + "/" + TEMP_FILENAME;
    error_file_path = server_absolute_path + "/" + ERROR_FILENAME;
    

    ifstream file("config.json");
    nlohmann::json port = nlohmann::json::parse(file);
    int command_port = port["commandChannelPort"];
    int data_port = port["dataChannelPort"];
    file.close();
    UserRepository::addUsers();
    FileRepository::addFiles();

    CustomSocket command_socket(command_port);
    CustomSocket data_socket(data_port);


    vector<thread*> threads; 

    cout << "server is ready to accept clients\n";

    while (true) {
        // new client & new thread
        int new_command_fd, new_data_fd;
        if ((new_command_fd = accept(command_socket.fd, (struct sockaddr*) &command_socket.address, (socklen_t*) &command_socket.addrlen)) < 0) { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
	    } 
        if ((new_data_fd = accept(data_socket.fd, (struct sockaddr*) &data_socket.address, (socklen_t*) &data_socket.addrlen)) < 0) { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
	    } 
        threads.push_back(new thread(handle_client, new_command_fd, new_data_fd));
    }

    for (thread* th : threads) {
        th->join();
    }
	return 0; 
} 
