//
// Created by spotlight on 1/14/17.
//

#ifndef VULKAN_ENGINE_VDELETER_H
#define VULKAN_ENGINE_VDELETER_H


#include <vulkan/vulkan.h>
#include <functional>

template<typename T>
class VDeleter{
  public:
    VDeleter()
            : VDeleter([](T, VkAllocationCallbacks*) {}) { }

    VDeleter(std::function<void(T,VkAllocationCallbacks*)> deletef) {
      deleter = [=](T obj) {
        deletef(obj, nullptr);
      };
    }

    VDeleter(const VDeleter<VkInstance>& instance,
    std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
      deleter = [&instance, deletef](T obj) {
        deletef(instance, obj, nullptr);
      };
    }

    VDeleter(const VDeleter<VkDevice>& device,
    std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
      deleter = [&device, deletef](T obj) {
        deletef(device, obj, nullptr);
      };
    }

    ~VDeleter() {
      cleanup();
    }

    const T* operator&() const {
      return &object;
    }

    T* replace() {
      cleanup();
      return &object;
    }

    operator T() const {
      return object;
    }

    // Assignment operator
    void operator=(T rhs) {
      if (rhs != object) {
        cleanup();
        object = rhs;
      }
    }

    template<typename V>
            bool operator==(V rhs) {
      return object == T(rhs);
    }

  private:
    std::function<void(T)> deleter;
    T object{VK_NULL_HANDLE};

    void cleanup() {
      if (object != VK_NULL_HANDLE) {
        deleter(object);
        object = VK_NULL_HANDLE;
      }
    }
};


#endif //VULKAN_ENGINE_VDELETER_H
