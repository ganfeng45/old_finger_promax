#ifndef PUBSUBBROKER_H
#define PUBSUBBROKER_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

struct Message {
    String topicName;
    void *param;
};
class Broker {
public:
    struct Topic {
        String name;
        std::vector<std::function<void(const String&, void*)>> callbacks;
    };

    Broker();

    void subscribe(const String &topicName, std::function<void(const String&, void*)> callback);
    void unsubscribe(const String &topicName, std::function<void(const String&, void*)> callback);
    void publish(const String &topicName, void *param);

private:
    std::vector<Topic> topics;
    QueueHandle_t messageQueue;

    Topic* findTopic(const String &topicName);
    static void taskHandler(void *param);
    void processMessage(const Message &message);
};

#endif // PUBSUBBROKER_H
