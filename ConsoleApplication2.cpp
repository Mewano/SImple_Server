#include <iostream>
#include <uwebsockets/App.h>
#include <uwebsockets/WebSocketContextData.h>

using namespace std;

struct PerSocketData {
    unsigned int id;
    string name;
};

const string DELIMITER = "::";
const string PRIVATE_MESSAGE = "PRIVATE_MESSAGE"; // + DELIMITER
const string SET_NAME = "SET_NAME";
const string ONLINE = "ONLINE";
const string OFFLINE = "OFFLINE";

string online(PerSocketData* userData) {
    return ONLINE + "::" + to_string(userData->id) + "::" + userData->name;
}

string online(pair<unsigned int, string> userData) {
    return ONLINE + "::" + to_string(userData.first) + "::" + userData.second;
}

string offline(PerSocketData* userData) {
    return OFFLINE + "::" + to_string(userData->id) + "::" + userData->name;
}


string parsePrivatMessageId(string message) {
    string rest = message.substr(PRIVATE_MESSAGE.size() + 2);
    int pos = rest.find("::");
    return rest.substr(0, pos);
}

string parsePrivatMessage(string message) {
    string rest = message.substr(PRIVATE_MESSAGE.size() + 2);
    int pos = rest.find("::");
    return rest.substr(pos + 2);
}

string createPrivateMessage(string sender_id, string message, string senderName) {
    return PRIVATE_MESSAGE + "::" + sender_id + "::[" + senderName + "]: " + message;
    }

string parseUserName(string event) {
    return event.substr(SET_NAME.size() + 2);
}

bool isPrivateMessage(string event) {
    return event.find(PRIVATE_MESSAGE) == 0;
}

bool isSetName(string event) {
    return event.find(SET_NAME) == 0;
}

map<unsigned int, string> userNames;

void setUser(PerSocketData* userData) {
    userNames[userData->id] = userData->name;
}

int main()
{
    unsigned int last_user_id = 10;
    unsigned int all_users = 0;
    

    uWS::App().ws<PerSocketData>("/*", {
            /* Settings */
            .idleTimeout = 3600,
            .open = [&last_user_id, &all_users](auto* ws) {
                PerSocketData* userData = ws->getUserData();
                userData->name = "UNNAMED";
                userData->id = last_user_id++;
                all_users++;

                ws->subscribe("user" + to_string(userData->id));
                ws->subscribe("broadcast");
                
                for (auto entry : userNames) {
                    ws->send(online(entry), uWS::OpCode::TEXT);
                }

                setUser(userData);
                cout << "New user connected: " << userData->id << endl;
                cout << "Total users connected: " << all_users << endl;

            },
            .message = [](auto* ws, std::string_view eventText, uWS::OpCode opCode) {
                PerSocketData* userData = ws->getUserData();
                string eventString(eventText);

                if (isPrivateMessage(eventString)) {
                    string recieverId = parsePrivatMessageId(eventString);
                    string text = parsePrivatMessage(eventString);
                    string senderId = to_string(userData->id);

                    string eventToSend = createPrivateMessage(senderId, text, userData->name);

                    ws->publish("user" + recieverId, eventToSend);

                    cout << "User" << senderId << " sent message to " << recieverId << endl;
                }

                if (isSetName(eventString)) {
                    string name = parseUserName(eventString);
                    userData->name = name;
                    ws->publish("broadcast", online(userData));
                    setUser(userData);
                    cout << "User " << userData->id << " set their name" << endl;
                }
            },
            .close = [](auto* ws, int code, std::string_view) {
                PerSocketData* userData = ws->getUserData();
                ws->publish("broadcast", offline(userData));
                cout << "User disconnected " << userData->id << endl;;
                userNames.erase(userData->id);
            }
            }).listen(9001, [](auto* listen_socket) {
                if (listen_socket) {
                    std::cout << "Listening on port " << 9001 << std::endl;
                }
                }).run();
}

