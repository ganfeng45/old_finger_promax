#include "PubSubBroker.h"
#define TAG "broker"

Broker::Broker()
{
    messageQueue = xQueueCreate(10, sizeof(Message));                        // 创建队列
    xTaskCreate(&Broker::taskHandler, "BrokerTask", 1024*20, this, 15, nullptr); // 创建任务
}

Broker::Topic *Broker::findTopic(const String &topicName)
{
    for (auto &topic : topics)
    {
        if (topic.name == topicName)
        {
            return &topic;
        }
    }
    return nullptr;
}

void Broker::subscribe(const String &topicName, std::function<void(const String &, void *)> callback)
{
    Topic *topic = findTopic(topicName);
    if (topic)
    {
        topic->callbacks.push_back(callback);
    }
    else
    {
        Topic newTopic;
        newTopic.name = topicName;
        newTopic.callbacks.push_back(callback);
        topics.push_back(newTopic);
    }
}

void Broker::unsubscribe(const String &topicName, std::function<void(const String &, void *)> callback)
{
    Topic *topic = findTopic(topicName);
    if (topic)
    {
        // auto it = std::find_if(topic->callbacks.begin(), topic->callbacks.end(),
        //                        [&callback](const std::function<void(const String &, void *)> &cb)
        //                        {
        //                            return cb.target<void(const String &, void *)>() == callback.target<void(const String &, void *)>();
        //                        });
        // if (it != topic->callbacks.end())
        // {
        //     topic->callbacks.erase(it);
        // }
    }
}

void Broker::publish(const String &topicName, void *param)
{
    Message message = {topicName, param};
    xQueueSend(messageQueue, &message, portMAX_DELAY); // 将消息放入队列
}

void Broker::taskHandler(void *param)
{
    Broker *broker = static_cast<Broker *>(param);
    Message message;

    while (true)
    {
        if (xQueueReceive(broker->messageQueue, &message, portMAX_DELAY) == pdPASS)
        {
            // Seria1.print("Received message: ");
            Serial.println("Received message: ");

            broker->processMessage(message);
        }
    }
}

void Broker::processMessage(const Message &message)
{
    Topic *topic = findTopic(message.topicName);
    if (topic)
    {
        for (const auto &callback : topic->callbacks)
        {
            // 复制回调函数，以防止在回调过程中发生竞争条件
            // std::function<void(const String &, void *)> safeCallback = callback;
            callback(message.topicName, message.param);
            ESP_LOGE(TAG, "******topic:%s over******", message.topicName.c_str());


        }
    }
}






