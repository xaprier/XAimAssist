/// @file EventBus.cpp
#include "core/EventBus.hpp"

#include "core/CoreEvents.hpp"
#include <algorithm>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace xaimassist::core {
EventBus::SubscriptionId EventBus::Subscribe(EventHandler handler) {
    if (!handler) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const SubscriptionId subscriptionId = m_nextSubscriptionId++;
    m_handlers.emplace_back(subscriptionId,
                            std::make_shared<EventHandler>(std::move(handler)));
    return subscriptionId;
}

void EventBus::Unsubscribe(SubscriptionId subscriptionId) {
    if (subscriptionId == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
                                 [subscriptionId](const auto& entry) {
                                     return entry.first == subscriptionId;
                                 });
    if (it != m_handlers.end()) {
        m_handlers.erase(it);
    }
}

void EventBus::Publish(const events::CoreEvent& event) const {
    std::vector<std::shared_ptr<EventHandler>> snapshot;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        snapshot.reserve(m_handlers.size());
        for (const auto& [_, handler] : m_handlers) {
            snapshot.push_back(handler);
        }
    }

    for (const auto& handler : snapshot) {
        if (handler && *handler) {
            (*handler)(event);
        }
    }
}
}  // namespace xaimassist::core
