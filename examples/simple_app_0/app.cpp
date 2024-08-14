#include "lynx/app/application.h"

int main() {
  /// Create app.
  lynx::Application app;
  /// Init app.
  app.start();
  /// Start listening.
  app.listen();
}
