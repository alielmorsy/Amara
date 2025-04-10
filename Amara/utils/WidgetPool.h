//
// Created by Ali Elmorsy on 3/31/2025.
//

#ifndef WIDGETPOOL_H
#define WIDGETPOOL_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <typeindex>
#include "../ui/Widget.h"
#include "../runtime/PropMap.h"
class WidgetPool {
public:
    bool finished = false;

    template<typename T>
    std::shared_ptr<T> allocate(std::unique_ptr<PropMap> propMap, std::shared_ptr<ComponentContext> component) {
        static_assert(std::is_base_of_v<Widget, T>, "T must derive from Widget");

        std::lock_guard lock(mutex_);
        auto &free_list = free_lists[typeid(T)];

        T *obj = nullptr;

        if (!free_list.empty()) {
            obj = static_cast<T *>(free_list.back());
            free_list.pop_back();
            obj->reuse(std::move(propMap), std::move(component)); // Properly typed reset
        } else {
            obj = new T(std::move(propMap), std::move(component));
        }

        auto deleter = [this](Widget *ptr) {
            if (finished) {
                delete ptr;
                return;
            }
            ptr->resetPointer(); // Generic cleanup
            std::lock_guard lock(mutex_);
            free_lists[typeid(*ptr)].push_back(ptr);
        };

        return std::shared_ptr<T>(obj, deleter);
    }

    ~WidgetPool() {
        clear();
    }

    void clear() {
        for (auto &element: free_lists) {
            for (auto p: element.second) {
                if (p) {
                    delete p;
                }
            }
        }
        free_lists.clear();
    };

private:
    std::unordered_map<std::type_index, std::vector<Widget *> > free_lists;
    std::mutex mutex_;
};

#endif //WIDGETPOOL_H
