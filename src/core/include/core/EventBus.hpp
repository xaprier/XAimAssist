/**
 * @file EventBus.hpp
 * @brief Thread-safe publish/subscribe event dispatch.
 *
 * Central communication channel for decoupled modules.
 *
 * @warning THREADING MODEL
 * - Subscribe/Unsubscribe: Thread-safe, can be called from any thread
 * - Publish: Thread-safe but handlers execute SYNCHRONOUSLY on the
 *   publisher's thread
 * - Handlers are invoked in the order they were registered
 * - A handler should NOT call Unsubscribe on itself during execution
 *   (deferred unsubscription is safe)
 *
 * @note SINGLE-THREADED USAGE PATTERN (Current XAimAssist)
 * All Publish() calls should originate from the Qt main event loop thread.
 * This ensures handlers that are not thread-safe (like StatTracker) work
 * correctly without additional synchronization overhead.
 *
 * @note FUTURE MULTI-THREADED SUPPORT
 * If multi-threaded publishing is required:
 * 1. Document which handlers are thread-safe
 * 2. Add mutex protection in non-thread-safe handlers
 * 3. Consider std::async or thread pool for async dispatch
 */

#ifndef EVENTBUS_HPP
#define EVENTBUS_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "core/CoreEvents.hpp"

namespace xaimassist::core {

/**
 * @class EventBus
 * @brief Publish/subscribe hub for CoreEvent variants.
 *
 * Handlers are stored behind shared_ptr to allow safe iteration
 * even when a handler unsubscribes during dispatch.
 */
class EventBus {
  public:
    using SubscriptionId = std::uint64_t;
    using EventHandler = std::function<void(const events::CoreEvent& event)>;

    /// Register a handler; returns a unique subscription id.
    [[nodiscard]] SubscriptionId Subscribe(EventHandler handler);

    /// Remove a previously registered handler by its id.
    void Unsubscribe(SubscriptionId subscriptionId);

    /// Dispatch an event to all current subscribers.
    void Publish(const events::CoreEvent& event) const;

  private:
    mutable std::mutex m_mutex;
    std::vector<std::pair<SubscriptionId, std::shared_ptr<EventHandler>>> m_handlers;
    SubscriptionId m_nextSubscriptionId{1};
};
}  // namespace xaimassist::core

#endif  // EVENTBUS_HPP
