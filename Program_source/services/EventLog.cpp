#include "EventLog.h"
#include <iostream>
#include <sstream>

EventLog::EventLog(EventType type, const std::string& username,
                   const std::string& message, iLogger& logger)
    : type(type), username(username), message(message), logger(&logger) {
    this->logger->log(serialize());
}

std::string EventLog::serialize() const {
    return eventTypeToString(type) + "|" + username + "|" + message;
}

void EventLog::deserialize(const std::string& data) {
    std::istringstream ss(data);
    std::string typeStr;
    std::getline(ss, typeStr, '|');
    std::getline(ss, username, '|');
    std::getline(ss, message, '|');
    type = stringToEventType(typeStr);
}

void EventLog::display() const {
    std::cout << getIcon() << " " << eventTypeToString(type)
              << " | " << username << " | " << message << "\n";
}

std::string EventLog::getIcon() const {
    switch (type) {
        case EventType::LOGIN:             return "[LOGIN]";
        case EventType::LOGOUT:            return "[LOGOUT]";
        case EventType::REGISTER:          return "[REGISTER]";
        case EventType::FILE_UPLOAD:       return "[FILE_UPLOAD]";
        case EventType::FILE_DELETE:       return "[FILE_DELETE]";
        case EventType::FILE_SHARE:        return "[FILE_SHARE]";
        case EventType::PERMISSION_DENIED: return "[PERMISSION_DENIED]";
        default:                           return "[EVENT]";
    }
}

EventType EventLog::getType() const { return type; }
std::string EventLog::getUsername() const { return username; }
std::string EventLog::getMessage() const { return message; }

std::string EventLog::eventTypeToString(EventType type) const {
    switch (type) {
        case EventType::LOGIN:             return "LOGIN";
        case EventType::LOGOUT:            return "LOGOUT";
        case EventType::REGISTER:          return "REGISTER";
        case EventType::FILE_UPLOAD:       return "FILE_UPLOAD";
        case EventType::FILE_DELETE:       return "FILE_DELETE";
        case EventType::FILE_SHARE:        return "FILE_SHARE";
        case EventType::PERMISSION_DENIED: return "PERMISSION_DENIED";
        default:                           return "UNKNOWN";
    }
}

EventType EventLog::stringToEventType(const std::string& str) const {
    if (str == "LOGIN")             return EventType::LOGIN;
    if (str == "LOGOUT")            return EventType::LOGOUT;
    if (str == "REGISTER")          return EventType::REGISTER;
    if (str == "FILE_UPLOAD")       return EventType::FILE_UPLOAD;
    if (str == "FILE_DELETE")       return EventType::FILE_DELETE;
    if (str == "FILE_SHARE")        return EventType::FILE_SHARE;
    if (str == "PERMISSION_DENIED") return EventType::PERMISSION_DENIED;
    return EventType::LOGIN;
}