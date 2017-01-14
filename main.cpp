#include "engine/Vulkan/Vulkan.h"


#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>

class Application {
  public:
    void run() {
      engine.init();
      engine.mainLoop();
    }

  private:
    engine::Vulkan engine;

};

int main() {
  Application app;

  try {
    app.run();
  } catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}