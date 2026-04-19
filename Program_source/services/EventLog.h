#pragma once
#include "../interfaces/ISerializable.h"
#include "../interfaces/IDisplayable.h"
#include "../interfaces/ILogger.h"
#include <string>

enum class EventType {
    LOGIN,
    LOGOUT,
    REGISTER,
    FILE_UPLOAD,
    FILE_DELETE,
    FILE_SHARE,
    PERMISSION_DENIED
};

class EventLog : public iSerializable, public iDisplayable {
public:
    EventLog() = default;
    EventLog(EventType type, const std::string& username,
             const std::string& message, iLogger& logger);

    // iSerializable
    std::string serialize() const override;
    void deserialize(const std::string& data) override;

    // iDisplayable
    void display() const override;
    std::string getIcon() const override;

    // Getteri
    EventType getType() const;
    std::string getUsername() const;
    std::string getMessage() const;

private:
    EventType type;
    std::string username;
    std::string message;
    iLogger* logger = nullptr;

    std::string eventTypeToString(EventType type) const;
    EventType stringToEventType(const std::string& str) const;
};