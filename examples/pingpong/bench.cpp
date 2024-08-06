#include "lynx/base/timestamp.h"
#include "lynx/net/channel.h"
#include "lynx/net/event_loop.h"

#include <sys/resource.h>
#include <sys/socket.h>

std::vector<int> g_pipes;
int num_pipes;
int num_active;
int num_writes;
lynx::EventLoop *g_loop;
std::vector<std::unique_ptr<lynx::Channel>> g_channels;

int g_reads;
int g_writes;
int g_fired;

void readCallback(lynx::Timestamp time, int fd, int idx) {
  char ch;
  g_reads += static_cast<int>(::recv(fd, &ch, sizeof(ch), 0));

  if (g_writes > 0) {
    int widx = idx + 1;
    if (widx >= num_pipes) {
      widx -= num_pipes;
    }
    ::send(g_pipes[2 * widx + 1], "m", 1, 0);
    g_writes--;
    g_fired++;
  }

  if (g_fired == g_reads) {
    g_loop->quit();
  }
}

std::pair<int, int> runOnce() {
  lynx::Timestamp before_init(lynx::Timestamp::now());

  for (int i = 0; i < num_pipes; i++) {
    lynx::Channel &channel = *g_channels[i];
    channel.setReadCallback([&channel, &i](auto &&PH1) {
      readCallback(std::forward<decltype(PH1)>(PH1), channel.fd(), i);
    });
    channel.enableReading();
  }

  int space = num_pipes / num_active;
  space *= 2;
  for (int i = 0; i < num_active; i++) {
    ::send(g_pipes[i * space + 1], "m", 1, 0);
  }

  g_fired = num_active;
  g_reads = 0;
  g_writes = num_writes;

  lynx::Timestamp before_loop(lynx::Timestamp::now());

  g_loop->loop();

  lynx::Timestamp end(lynx::Timestamp::now());

  int iter_time = static_cast<int>(end.microsecsSinceEpoch() -
                                   before_init.microsecsSinceEpoch());
  int loop_time = static_cast<int>(end.microsecsSinceEpoch() -
                                   before_loop.microsecsSinceEpoch());
  return std::make_pair(iter_time, loop_time);
}

int main(int argc, char *argv[]) {
  num_pipes = 100;
  num_active = 1;
  num_writes = 100;

  int c;
  while ((c = getopt(argc, argv, "n:a:w:")) != -1) {
    switch (c) {
    case 'n':
      num_pipes = atoi(optarg);
      break;
    case 'a':
      num_active = atoi(optarg);
      break;
    case 'w':
      num_writes = atoi(optarg);
      break;
    default:
      fprintf(stderr, "Illegal argument \"%c\"\n", c);
      return 1;
    }
  }

  struct rlimit rl;
  rl.rlim_cur = rl.rlim_max = num_pipes * 2 + 50;

  if (::setrlimit(RLIMIT_NOFILE, &rl) == -1) {
    perror("setrlimit");
  }
  g_pipes.resize(2 * num_pipes);

  for (int i = 0; i < num_pipes; i++) {
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, &g_pipes[i * 2]) == -1) {
      perror("pipe");
      return 1;
    }
  }

  lynx::EventLoop loop;
  g_loop = &loop;

  for (int i = 0; i < num_pipes; i++) {
    auto *channel = new lynx::Channel(&loop, g_pipes[i * 2]);
    g_channels.emplace_back(channel);
  }

  for (int i = 0; i < 25; i++) {
    std::pair<int, int> t = runOnce();
    printf("%8d %8d\n", t.first, t.second);
  }

  for (const auto &channel : g_channels) {
    channel->disableAll();
    channel->remove();
  }
  g_channels.clear();
}
