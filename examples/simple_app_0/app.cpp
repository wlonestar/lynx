#include "lynx/app/Application.h"

int main() {
  /// Create app.
  lynx::Application App;
  /// Init app.
  App.start();
  /// Start listening.
  App.listen();
}
