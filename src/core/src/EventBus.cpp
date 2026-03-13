/// @file EventBus.cpp
#include "core/EventBus.hpp"

#include <utility>
#include <vector>

namespace xaimassist::core {
EventBus::SubscriptionId EventBus::Subscribe(EventHandler handler) {
    if (!handler) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const SubscriptionId subscriptionId = m_nextSubscriptionId++;
    m_handlers.emplace(subscriptionId,
                       std::make_shared<EventHandler>(std::move(handler)));
    return subscriptionId;
}

void EventBus::Unsubscribe(SubscriptionId subscriptionId) {
    if (subscriptionId == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.erase(subscriptionId);
}

void EventBus::Publish(const events::CoreEvent& event) const {
    std::vector<std::shared_ptr<EventHandler>> handlers;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        handlers.reserve(m_handlers.size());
        for (const auto& [_, handler] : m_handlers) {
            handlers.push_back(handler);
        }
    }

    for (const auto& handler : handlers) {
        if (!handler || !(*handler)) {
            continue;
        }

        (*handler)(event);
    }
}
}  // namespace xaimassist::core
