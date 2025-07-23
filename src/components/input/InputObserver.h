#pragma once

#include "InputEvent.h"
#include <vector>
#include <algorithm>

namespace Input {

    /**
     * @brief Observer interface for input events
     */
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onInputEvent(const Event& event) = 0;
    };

    /**
     * @brief Subject class that manages observers and notifies them of events
     */
    class Subject {
    private:
        std::vector<Observer*> observers_;
        
    public:
        virtual ~Subject() = default;
        
        void addObserver(Observer* observer) {
            if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
                observers_.push_back(observer);
            }
        }
        
        void removeObserver(Observer* observer) {
            auto it = std::find(observers_.begin(), observers_.end(), observer);
            if (it != observers_.end()) {
                observers_.erase(it);
            }
        }
        
        void notifyObservers(const Event& event) {
            // Copy vector to avoid issues if observers modify the list during notification
            auto observers_copy = observers_;
            for (auto* observer : observers_copy) {
                if (observer) {
                    observer->onInputEvent(event);
                }
            }
        }
        
        size_t getObserverCount() const {
            return observers_.size();
        }
        
        void clearObservers() {
            observers_.clear();
        }
    };

    /**
     * @brief Action-based observer for specific input actions
     */
    class ActionObserver {
    public:
        virtual ~ActionObserver() = default;
        virtual void onAction(const std::string& action, const Event& event) = 0;
    };

} // namespace Input
